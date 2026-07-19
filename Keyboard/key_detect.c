/*
 * key_detect.c
 *
 *  Created on: Jul 2, 2026
 *      Author: haodu
 */

/* Includes ------------------------------------------------------------------*/
#include "key_detect.h"

#include "keypad.h"
#include "key_event_queue.h"
#include "key_table.h"
#include <stdbool.h>
#include <stddef.h>

/* Private defines -----------------------------------------------------------*/
#define KEY_DETECT_SCAN_BUFFER_NUM  4U

/*
 * Repeat interval expressed in scan ticks.
 * With a 5 ms scan period, 40 ticks = 200 ms.
 */
#define KEY_DETECT_SCAN_PERIOD_MS   5U
#define KEY_REPEAT_INTERVAL_MS      200U
#define KEY_REPEAT_INTERVAL_TICKS   (KEY_REPEAT_INTERVAL_MS / KEY_DETECT_SCAN_PERIOD_MS)

/* Private variables ---------------------------------------------------------*/
static uint16_t sScanBuffer[KEY_DETECT_SCAN_BUFFER_NUM];
static uint16_t sKeyStatus;
static uint16_t sRepeatTick[KEYPAD_KEY_NUM];

/*
 * Set when two or more keys are pressed simultaneously.
 * While active, all normal key detection is suppressed until every
 * physical key has been released (no-diode ghost-key safety policy).
 */
static bool sSimultaneousErrorActive;

/* Private functions ---------------------------------------------------------*/

// Shift the raw scan into the debounce history. Oldest sample drops off
static void KeyDetect_UpdateScanBuffer(uint16_t rawState)
{
  sScanBuffer[3] = sScanBuffer[2];                // shift history back one slot
  sScanBuffer[2] = sScanBuffer[1];
  sScanBuffer[1] = sScanBuffer[0];
  sScanBuffer[0] = (rawState & KEYPAD_KEY_MASK);  // newest sample goes in front
}

// Build and push one key event. Returns false if the queue is full
static bool KeyDetect_PushEvent(KeyEventType_t type, uint8_t keyLoc)
{
  KeyEvent_t event;

  event.type   = type;
  event.keyLoc = keyLoc;

  return KeyEventQueue_Push(&event);
}

// Push a KEY_EVENT_ERROR with the reserved rollover keyLoc
static void KeyDetect_PushErrorRollOver(void)
{
  (void)KeyDetect_PushEvent(KEY_EVENT_ERROR, KEY_LOC_ERROR_ROLLOVER);
}

// Clear the repeat tick counter for one key
static void KeyDetect_ResetRepeatState(uint8_t keyLoc)
{
  if (keyLoc < KEYPAD_KEY_NUM)
  {
    sRepeatTick[keyLoc] = 0U;
  }
}

// Clear the repeat tick counter for every key
static void KeyDetect_ResetAllRepeatStates(void)
{
  uint8_t keyLoc;

  for (keyLoc = 0U; keyLoc < KEYPAD_KEY_NUM; keyLoc++)
  {
    sRepeatTick[keyLoc] = 0U;
  }
}

// Clear the debounce history back to all-released
static void KeyDetect_ResetScanBuffer(void)
{
  uint8_t i;

  for (i = 0U; i < KEY_DETECT_SCAN_BUFFER_NUM; i++)
  {
    sScanBuffer[i] = 0U;
  }
}

// Force every currently-held key to release and push KEY_EVENT_OFF for each
static void KeyDetect_ReleaseAllKeys(void)
{
  uint8_t keyLoc;

  for (keyLoc = 0U; keyLoc < KEYPAD_KEY_NUM; keyLoc++)
  {
    uint16_t bit = (uint16_t)(1U << keyLoc);

    if ((sKeyStatus & bit) != 0U)
    {
      sKeyStatus &= (uint16_t)(~bit);
      KeyDetect_ResetRepeatState(keyLoc);
      (void)KeyDetect_PushEvent(KEY_EVENT_OFF, keyLoc);
    }
  }
}

// Push KEY_EVENT_OFF for every held key that is now stable off
static void KeyDetect_CheckKeyOff(uint16_t stableOffMask)
{
  uint16_t offMask = (stableOffMask & sKeyStatus);  // only currently-held keys can go off
  uint8_t  keyLoc;

  for (keyLoc = 0U; keyLoc < KEYPAD_KEY_NUM; keyLoc++)
  {
    uint16_t bit = (uint16_t)(1U << keyLoc);

    if ((offMask & bit) != 0U)
    {
      sKeyStatus &= (uint16_t)(~bit);
      KeyDetect_ResetRepeatState(keyLoc);
      (void)KeyDetect_PushEvent(KEY_EVENT_OFF, keyLoc);
    }
  }
}

/*
 * Generate KEY_EVENT_REPEAT for each key that:
 *   - is already in pressed state (sKeyStatus bit set)
 *   - is still stable ON in the current debounce result
 *   - has repeatEnable = 1 and kind = KEY_KIND_NORMAL in the key table
 *
 * Called between CheckKeyOff and CheckKeyOn so a newly pressed key
 * cannot trigger a repeat in the same scan cycle it was detected.
 */
static void KeyDetect_CheckRepeat(uint16_t stableOnMask)
{
  const KeyTableEntry_t *entry;
  uint8_t                keyLoc;

  for (keyLoc = 0U; keyLoc < KEYPAD_KEY_NUM; keyLoc++)
  {
    uint16_t bit = (uint16_t)(1U << keyLoc);

    if (((sKeyStatus & bit) == 0U) || ((stableOnMask & bit) == 0U))
    {
      continue;
    }

    entry = KeyTable_Get(keyLoc);

    if ((entry == NULL) || (entry->repeatEnable == 0U) || (entry->kind != KEY_KIND_NORMAL))
    {
      KeyDetect_ResetRepeatState(keyLoc);
      continue;
    }

    if (sRepeatTick[keyLoc] < KEY_REPEAT_INTERVAL_TICKS)
    {
      sRepeatTick[keyLoc]++;  // accumulate ticks toward the repeat threshold
    }

    if (sRepeatTick[keyLoc] >= KEY_REPEAT_INTERVAL_TICKS)
    {
      if (KeyDetect_PushEvent(KEY_EVENT_REPEAT, keyLoc))
      {
        sRepeatTick[keyLoc] = 0U;  // restart the interval after firing
      }
    }
  }
}

// Push KEY_EVENT_ON for every newly stable-on key
static void KeyDetect_CheckKeyOn(uint16_t stableOnMask)
{
  uint16_t onMask = (stableOnMask & (uint16_t)(~sKeyStatus));  // exclude already-held keys
  uint8_t  keyLoc;

  for (keyLoc = 0U; keyLoc < KEYPAD_KEY_NUM; keyLoc++)
  {
    uint16_t bit = (uint16_t)(1U << keyLoc);

    if ((onMask & bit) != 0U)
    {
      sKeyStatus |= bit;
      KeyDetect_ResetRepeatState(keyLoc);
      (void)KeyDetect_PushEvent(KEY_EVENT_ON, keyLoc);
    }
  }
}

/* Function definitions ------------------------------------------------------*/

// Clear all state. Call once after Keypad_Init
void KeyDetect_Init(void)
{
  uint8_t i;

  for (i = 0U; i < KEY_DETECT_SCAN_BUFFER_NUM; i++)
  {
    sScanBuffer[i] = 0U;
  }

  sKeyStatus               = 0U;
  sSimultaneousErrorActive = false;

  KeyDetect_ResetAllRepeatStates();
}

// Debounce one scan cycle and push press/release/repeat/error events. Call at the scan tick rate (5 ms)
void KeyDetect_Run(void)
{
  uint16_t rawState;
  uint16_t stableOnMask;
  uint16_t stableOffMask;
  uint8_t  pressedCount;

  rawState     = Keypad_ReadRaw();               // bit N set = keyLoc N currently pressed
  pressedCount = Keypad_CountPressed(rawState);

  /*
   * Strict simultaneous-error latch (no-diode safety policy):
   *
   * Once a simultaneous error is active, ALL key input is ignored until
   * every physical key is released. A single key remaining after releasing
   * one finger does NOT re-trigger normal detection — the user must lift
   * everything before the firmware accepts new input.
   */
  if (sSimultaneousErrorActive)
  {
    if (pressedCount == 0U)
    {
      // all keys released: clear the latch and start clean next cycle
      KeyDetect_ResetScanBuffer();
      KeyDetect_ResetAllRepeatStates();
      sKeyStatus               = 0U;
      sSimultaneousErrorActive = false;
    }

    return;
  }

  /*
   * Enter simultaneous error state.
   * Cancel all currently held keys immediately so no phantom key lingers
   * while waiting for full release.
   */
  if (pressedCount >= 2U)
  {
    KeyDetect_PushErrorRollOver();          // report the rollover once
    KeyDetect_ReleaseAllKeys();             // force-release before latching
    KeyDetect_ResetScanBuffer();
    KeyDetect_ResetAllRepeatStates();
    sKeyStatus               = 0U;
    sSimultaneousErrorActive = true;

    return;
  }

  KeyDetect_UpdateScanBuffer(rawState);

  /*
   * Debounce policy:
   * - ON  is accepted when the key appears in the 2 newest scan buffers.
   * - OFF is accepted when the key is absent from the 2 newest scan buffers.
   */
  stableOnMask  = (uint16_t)( sScanBuffer[0] &  sScanBuffer[1]);
  stableOffMask = (uint16_t)(~(sScanBuffer[0] | sScanBuffer[1]) & KEYPAD_KEY_MASK);

  /*
   * Detection order matters:
   * 1. Key off first  — free held state before new keys can fire.
   * 2. Repeat for already-held keys — must come before key-on so a new
   *    key cannot repeat in the same cycle it is first detected.
   * 3. Key on last.
   */
  KeyDetect_CheckKeyOff(stableOffMask);
  KeyDetect_CheckRepeat(stableOnMask);
  KeyDetect_CheckKeyOn(stableOnMask);
}

// Return the current debounced key status bitmask (bit N = keyLoc N is held)
uint16_t KeyDetect_GetKeyStatus(void)
{
  return sKeyStatus;
}
