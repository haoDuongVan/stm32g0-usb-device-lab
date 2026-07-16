/*
 * keypad.h
 *
 *  Created on: Jul 2, 2026
 *      Author: haodu
 */

#ifndef INC_KEYPAD_H_
#define INC_KEYPAD_H_

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported defines ----------------------------------------------------------*/
#define KEYPAD_ROW_NUM              4U
#define KEYPAD_COL_NUM              4U
#define KEYPAD_KEY_NUM              16U

/*
 * Bitmask covering all 16 key positions in the raw state word.
 * Bit N corresponds to keyLoc N (row = N/4, col = N%4).
 */
#define KEYPAD_KEY_MASK             0xFFFFU

/* Function prototypes -------------------------------------------------------*/

void      Keypad_Init(void);
uint16_t  Keypad_ReadRaw(void);
uint8_t   Keypad_CountPressed(uint16_t rawState);

#endif /* INC_KEYPAD_H_ */
