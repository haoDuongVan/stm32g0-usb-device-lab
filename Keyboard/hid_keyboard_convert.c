/*
 * hid_keyboard_convert.c
 *
 *  Created on: Jul 2, 2026
 *      Author: haodu
 */

/* Includes ------------------------------------------------------------------*/
#include "hid_keyboard_convert.h"

#include "hid_keyboard_report.h"
#include "key_event_queue.h"
#include "key_table.h"
#include "usb_hid_keyboard.h"
#include "usb_cdc_log.h"

#include <stddef.h>
#include <stdbool.h>

/* Private variables ---------------------------------------------------------*/
static HidKeyboardReport_t sReport;

/*
 * After a key-down report is sent, sNeedNullReport is set so the next
 * USB-idle cycle sends a null report (tap-style output). This avoids
 * having the host OS treat the key as physically held.
 */
static bool sNeedNullReport;

/*
 * Macro sequence state. When a KEY_KIND_MACRO event is consumed, sMacroActive
 * is set and HidKeyboardConvert_RunMacroSequence() advances through the
 * steps on each USB-idle call until the final null report is sent.
 */
static bool      sMacroActive;
static MacroId_t sMacroId;
static uint8_t   sMacroStep;

/* Private functions ---------------------------------------------------------*/

// Translate a key ON/REPEAT event into a HID keyboard key-down report
static bool HidKeyboardConvert_BuildKeyReport(const KeyEvent_t *event, HidKeyboardReport_t *report)
{
  const KeyTableEntry_t *entry;

  if ((event == NULL) || (report == NULL))
  {
    return false;
  }

  entry = KeyTable_Get(event->keyLoc);

  if ((entry == NULL) || (entry->kind != KEY_KIND_NORMAL))
  {
    return false;
  }

  HidKeyboardReport_SetKey(report, entry->modifier, entry->usage);

  return true;
}

/*
 * Build one step of a macro HID sequence.
 * Returns false when the step index is out of range for the given macroId,
 * which signals the caller to abort the sequence.
 * Sets *isLastStep = true on the final step so the caller knows to stop.
 *
 * Alt+Tab sequence (3 steps):
 *   0: Alt only  (04 00 00 ...)   - host sees Alt pressed
 *   1: Alt+Tab   (04 00 2B ...)
 *   2: null      (00 00 00 ...)   - release
 *
 * Ctrl+C/V/S sequences (2 steps):
 *   0: Ctrl+key
 *   1: null
 */
static bool HidKeyboardConvert_BuildMacroReport(MacroId_t macroId,
                                                 uint8_t step,
                                                 HidKeyboardReport_t *report,
                                                 bool *isLastStep)
{
  if ((report == NULL) || (isLastStep == NULL))
  {
    return false;
  }

  *isLastStep = false;

  switch (macroId)
  {
    case MACRO_CTRL_C:
      if (step == 0U) { HidKeyboardReport_SetKey(report, HID_MOD_LEFT_CTRL, HID_USAGE_C); return true; }
      if (step == 1U) { HidKeyboardReport_Clear(report); *isLastStep = true;               return true; }
      break;

    case MACRO_CTRL_V:
      if (step == 0U) { HidKeyboardReport_SetKey(report, HID_MOD_LEFT_CTRL, HID_USAGE_V); return true; }
      if (step == 1U) { HidKeyboardReport_Clear(report); *isLastStep = true;               return true; }
      break;

    case MACRO_CTRL_S:
      if (step == 0U) { HidKeyboardReport_SetKey(report, HID_MOD_LEFT_CTRL, HID_USAGE_S); return true; }
      if (step == 1U) { HidKeyboardReport_Clear(report); *isLastStep = true;               return true; }
      break;

    case MACRO_ALT_TAB:
      if (step == 0U) { HidKeyboardReport_SetKey(report, HID_MOD_LEFT_ALT, 0x00U);         return true; }
      if (step == 1U) { HidKeyboardReport_SetKey(report, HID_MOD_LEFT_ALT, HID_USAGE_TAB); return true; }
      if (step == 2U) { HidKeyboardReport_Clear(report); *isLastStep = true;                return true; }
      break;

    default:
      break;
  }

  return false;
}

/*
 * Advance the macro sequence by one step.
 * Called when sMacroActive is true and the USB endpoint is idle.
 * Clears sMacroActive when the last step is sent successfully.
 */
static void HidKeyboardConvert_RunMacroSequence(void)
{
  bool isLastStep = false;

  if (!sMacroActive)
  {
    return;
  }

  if (!HidKeyboardConvert_BuildMacroReport(sMacroId, sMacroStep, &sReport, &isLastStep))
  {
    // Step out of range - abort and schedule a null report to release any held keys
    sMacroActive    = false;
    sMacroId        = MACRO_NONE;
    sMacroStep      = 0U;
    sNeedNullReport = true;
    return;
  }

  if (UsbHidKeyboard_SendReport(HidKeyboardReport_GetData(&sReport)))
  {
    if (isLastStep)
    {
      sMacroActive = false;
      sMacroId     = MACRO_NONE;
      sMacroStep   = 0U;
    }
    else
    {
      sMacroStep++;
    }
  }
}

/* Function definitions ------------------------------------------------------*/

// Clear internal state. Call once after transport and queue are ready
void HidKeyboardConvert_Init(void)
{
  HidKeyboardReport_Clear(&sReport);
  sNeedNullReport = false;
  sMacroActive    = false;
  sMacroId        = MACRO_NONE;
  sMacroStep      = 0U;
}

/*
 * Priority order each call:
 *   1. Wait for USB endpoint to be idle.
 *   2. Send pending null report (tap-style key release).
 *   3. Advance active macro sequence.
 *   4. Pop next event from queue and dispatch.
 */
void HidKeyboardConvert_Run(void)
{
  KeyEvent_t              event;
  const KeyTableEntry_t  *entry;

  if (!UsbHidKeyboard_IsIdle())
  {
    return;
  }

  if (sNeedNullReport)
  {
    HidKeyboardReport_Clear(&sReport);

    if (UsbHidKeyboard_SendReport(HidKeyboardReport_GetData(&sReport)))
    {
      sNeedNullReport = false;
    }

    return;
  }

  if (sMacroActive)
  {
    HidKeyboardConvert_RunMacroSequence();
    return;
  }

  if (!KeyEventQueue_Peek(&event))
  {
    return;
  }

  switch (event.type)
  {
    case KEY_EVENT_ON:
    case KEY_EVENT_REPEAT:
      entry = KeyTable_Get(event.keyLoc);
      CdcLog_Printf("[HID] keyLoc=%u event=%d usage=0x%02X\r\n",
                    event.keyLoc, event.type,
                    (entry != NULL) ? entry->usage : 0U);

      if (entry == NULL)
      {
        (void)KeyEventQueue_Pop(NULL);
        return;
      }

      if (entry->kind == KEY_KIND_NORMAL)
      {
        (void)HidKeyboardConvert_BuildKeyReport(&event, &sReport);

        if (UsbHidKeyboard_SendReport(HidKeyboardReport_GetData(&sReport)))
        {
          (void)KeyEventQueue_Pop(NULL);
          sNeedNullReport = true;
        }

        return;
      }

      if (entry->kind == KEY_KIND_MACRO)
      {
        sMacroActive = true;
        sMacroId     = entry->macroId;
        sMacroStep   = 0U;

        HidKeyboardConvert_RunMacroSequence();

        /*
         * Pop only after the first macro step is accepted.
         * If SendReport returned false (USB busy), sMacroStep is still 0
         * and sMacroActive is still true; the event stays in the queue and
         * will be re-peeked next call, where the macro branch will be skipped
         * because sMacroActive is already set.
         * Once sMacroStep > 0 the sequence owns the state, so pop now.
         */
        if ((sMacroStep > 0U) || !sMacroActive)
        {
          (void)KeyEventQueue_Pop(NULL);
        }

        return;
      }

      // Unsupported KEY_KIND_SPECIAL - drop
      (void)KeyEventQueue_Pop(NULL);
      break;

    case KEY_EVENT_OFF:
      // Release report is already scheduled after the matching ON/REPEAT above
      (void)KeyEventQueue_Pop(NULL);
      break;

    case KEY_EVENT_ERROR:
      if (event.keyLoc != KEY_LOC_ERROR_ROLLOVER)
      {
        (void)KeyEventQueue_Pop(NULL);
        break;
      }

      CdcLog_Printf("[ERR] simultaneous key detected - ErrorRollOver\r\n");
      HidKeyboardReport_SetErrorRollOver(&sReport);

      if (UsbHidKeyboard_SendReport(HidKeyboardReport_GetData(&sReport)))
      {
        (void)KeyEventQueue_Pop(NULL);
        sNeedNullReport = true;
      }
      break;

    default:
      (void)KeyEventQueue_Pop(NULL);
      break;
  }
}

// Return a read-only pointer to the current report bytes (for evidence/debug watch)
const uint8_t *HidKeyboardConvert_GetReportData(void)
{
  return HidKeyboardReport_GetData(&sReport);
}
