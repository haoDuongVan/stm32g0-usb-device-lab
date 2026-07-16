/*
 * keypad.c
 *
 *  Created on: Jul 2, 2026
 *      Author: haodu
 */

/* Includes ------------------------------------------------------------------*/
#include "keypad.h"
#include "main.h"

/* Private defines -----------------------------------------------------------*/

// Row pin array - driven LOW one at a time during scan
static const uint16_t sRowPins[KEYPAD_ROW_NUM] = {
  KEYPAD_ROW0_Pin,
  KEYPAD_ROW1_Pin,
  KEYPAD_ROW2_Pin,
  KEYPAD_ROW3_Pin,
};

// Column pin array - read while the corresponding row is driven LOW
static const uint16_t sColPins[KEYPAD_COL_NUM] = {
  KEYPAD_COL0_Pin,
  KEYPAD_COL1_Pin,
  KEYPAD_COL2_Pin,
  KEYPAD_COL3_Pin,
};

/*
 * All keypad pins are on GPIOB.
 *
 * BSRR write layout (32-bit):
 *   bits [15:0]  - set the corresponding ODR bit (drive HIGH)
 *   bits [31:16] - reset the corresponding ODR bit (drive LOW)
 *
 * IDR is read as a 32-bit value, so each bit matches the GPIO pin number.
 */
#define KEYPAD_GPIO                 GPIOB

/* Precomputed mask of all four row pins - used in Init and Reset */
#define KEYPAD_ALL_ROWS_MASK \
  ((uint32_t)(KEYPAD_ROW0_Pin | KEYPAD_ROW1_Pin | \
              KEYPAD_ROW2_Pin | KEYPAD_ROW3_Pin))

/* Function definitions ------------------------------------------------------*/

// Drive all row outputs HIGH (inactive). Only call once after GPIO init
void Keypad_Init(void)
{
  // Set all row pins HIGH via BSRR bits [15:0]
  KEYPAD_GPIO->BSRR = KEYPAD_ALL_ROWS_MASK;
}

/*
 * Scan all rows and return a 16-bit raw state word.
 * Bit N is set when the key at keyLoc N is currently pressed
 * (the column input reads LOW while the row is driven LOW).
 */
uint16_t Keypad_ReadRaw(void)
{
  uint16_t state = 0U;
  uint8_t  row;
  uint8_t  col;

  for (row = 0U; row < KEYPAD_ROW_NUM; row++)
  {
    // Drive this row LOW: write pin mask to BSRR bits [31:16] (reset field)
    KEYPAD_GPIO->BSRR = (uint32_t)sRowPins[row] << 16U;

    // Short settle delay so the input line stabilises before reading
    __NOP();
    __NOP();
    __NOP();
    __NOP();

    for (col = 0U; col < KEYPAD_COL_NUM; col++)
    {
      // Column reads LOW when key is pressed (pull-up + active-low row)
      if ((KEYPAD_GPIO->IDR & (uint32_t)sColPins[col]) == 0U)
      {
        state |= (uint16_t)(1U << ((row * KEYPAD_COL_NUM) + col));
      }
    }

    // Restore row HIGH: write pin mask to BSRR bits [15:0] (set field)
    KEYPAD_GPIO->BSRR = (uint32_t)sRowPins[row];
  }

  return state;
}

// Return the number of bits set in rawState (number of keys currently pressed)
uint8_t Keypad_CountPressed(uint16_t rawState)
{
  uint8_t  count = 0U;
  uint16_t mask  = rawState;

  while (mask != 0U)
  {
    // Kernighan bit-count: clears the lowest set bit each iteration
    mask  &= (uint16_t)(mask - 1U);
    count++;
  }

  return count;
}
