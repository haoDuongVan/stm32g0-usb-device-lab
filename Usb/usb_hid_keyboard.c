/*
 * usb_hid_keyboard.c
 *
 *  Created on: Jul 2, 2026
 *      Author: haodu
 *
 * Wraps the custom composite class (usbd_composite.c) so the Keyboard layer
 * never touches USBD_* types directly. BUSY/IDLE state lives in
 * USBD_Composite_HandleTypeDef.hidTxBusy, cleared by Composite_DataIn when
 * the HID IN transfer completes.
 */

/* Includes ------------------------------------------------------------------*/
#include "usb_hid_keyboard.h"

#include "usbd_composite.h"

#include <stddef.h>

/* External variables --------------------------------------------------------*/
extern USBD_HandleTypeDef hUsbDeviceFS;

/* Function definitions ------------------------------------------------------*/

// No local state to reset - HID class state lives inside USBD_Composite_HandleTypeDef
void UsbHidKeyboard_Init(void)
{
}

// Return true when the HID IN endpoint is free to accept a new report
bool UsbHidKeyboard_IsIdle(void)
{
  USBD_Composite_HandleTypeDef *hcomp = (USBD_Composite_HandleTypeDef *)hUsbDeviceFS.pClassDataCmsit[hUsbDeviceFS.classId];

  return (hcomp != NULL) && !hcomp->hidTxBusy;
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

  (void)USBD_COMPOSITE_HID_SendReport(&hUsbDeviceFS, (uint8_t *)report, USB_HID_KEYBOARD_REPORT_SIZE);

  return true;
}
