/*
 * usb_hid_keyboard.c
 *
 *  Created on: Jul 2, 2026
 *      Author: haodu
 *
 * Wraps the ST HID class (Middlewares/.../usbd_hid.c) so the Keyboard layer
 * never touches USBD_* types directly. This project currently uses the
 * plain (non-composite) HID class, so BUSY/IDLE state lives inside ST's own
 * USBD_HID_HandleTypeDef, not in a state variable owned by this file.
 *
 * USBD_HID_SendReport() silently no-ops (still returns USBD_OK) when the
 * class is not idle or the device is not configured, so UsbHidKeyboard_IsIdle()
 * must be checked by the caller before sending - the return value alone
 * cannot tell whether a report was actually queued.
 */

/* Includes ------------------------------------------------------------------*/
#include "usb_hid_keyboard.h"

#include "usbd_hid.h"

#include <stddef.h>

/* External variables ---------------------------------------------------------*/
extern USBD_HandleTypeDef hUsbDeviceFS;

/* Function definitions ------------------------------------------------------*/

// No local state to reset - HID class state lives inside the ST middleware handle
void UsbHidKeyboard_Init(void)
{
}

// Return true when the HID IN endpoint is free to accept a new report
bool UsbHidKeyboard_IsIdle(void)
{
  USBD_HID_HandleTypeDef *hhid = (USBD_HID_HandleTypeDef *)hUsbDeviceFS.pClassDataCmsit[hUsbDeviceFS.classId];

  return (hhid != NULL) && (hhid->state == USBD_HID_IDLE);
}

// Send one 8-byte HID keyboard report. Returns false if not idle or report is NULL
bool UsbHidKeyboard_SendReport(const uint8_t report[USB_HID_KEYBOARD_REPORT_SIZE])
{
  if (report == NULL)
  {
    return false;
  }

  if (!UsbHidKeyboard_IsIdle())
  {
    return false;
  }

  (void)USBD_HID_SendReport(&hUsbDeviceFS, (uint8_t *)report, USB_HID_KEYBOARD_REPORT_SIZE);

  return true;
}
