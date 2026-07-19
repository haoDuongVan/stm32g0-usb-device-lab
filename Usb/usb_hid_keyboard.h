/*
 * usb_hid_keyboard.h
 *
 *  Created on: Jul 2, 2026
 *      Author: haodu
 */

#ifndef INC_USB_HID_KEYBOARD_H_
#define INC_USB_HID_KEYBOARD_H_

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/* Exported defines ----------------------------------------------------------*/
#define USB_HID_KEYBOARD_REPORT_SIZE  8U

/* Function prototypes -------------------------------------------------------*/

void  UsbHidKeyboard_Init(void);
bool  UsbHidKeyboard_IsIdle(void);
bool  UsbHidKeyboard_SendReport(const uint8_t report[USB_HID_KEYBOARD_REPORT_SIZE]);

#endif /* INC_USB_HID_KEYBOARD_H_ */
