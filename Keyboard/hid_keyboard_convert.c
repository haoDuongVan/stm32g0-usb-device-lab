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

#include <stddef.h>
#include <stdbool.h>

/* Private variables ---------------------------------------------------------*/
static HidKeyboardReport_t sReport;

/*
 * After a key-down report is built, sNeedNullReport is set so the next
 * call clears it back to all-released (tap-style output). This avoids
 * having the host OS treat the key as physically held.
 */
static bool sNeedNullReport;

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

/* Function definitions ------------------------------------------------------*/

// Clear internal state. Call once before use
void HidKeyboardConvert_Init(void)
{
  HidKeyboardReport_Clear(&sReport);
  sNeedNullReport = false;
}

/*
 * Drain one event from the queue and convert it to a HID report.
 * Call repeatedly from the main loop.
 *
 * NOTE: this milestone only builds report bytes — it does not send them.
 * USB transport (BUSY/IDLE handshake) is added in a later milestone.
 */
void HidKeyboardConvert_Run(void)
{
  KeyEvent_t              event;
  const KeyTableEntry_t  *entry;

  if (sNeedNullReport)
  {
    HidKeyboardReport_Clear(&sReport);  // release report after the key-down above
    sNeedNullReport = false;
    return;
  }

  if (!KeyEventQueue_Pop(&event))
  {
    return;
  }

  switch (event.type)
  {
    case KEY_EVENT_ON:
    case KEY_EVENT_REPEAT:
      entry = KeyTable_Get(event.keyLoc);

      if ((entry != NULL) && (entry->kind == KEY_KIND_NORMAL))
      {
        (void)HidKeyboardConvert_BuildKeyReport(&event, &sReport);
        sNeedNullReport = true;
      }

      // TODO: KEY_KIND_MACRO needs multi-step sequencing gated on transport idle
      break;

    case KEY_EVENT_OFF:
      // Release report is already scheduled after the matching ON/REPEAT above
      break;

    case KEY_EVENT_ERROR:
      if (event.keyLoc == KEY_LOC_ERROR_ROLLOVER)
      {
        HidKeyboardReport_SetErrorRollOver(&sReport);
        sNeedNullReport = true;
      }
      break;

    default:
      break;
  }
}

// Return a read-only pointer to the current report bytes (for debug watch)
const uint8_t *HidKeyboardConvert_GetReportData(void)
{
  return HidKeyboardReport_GetData(&sReport);
}
