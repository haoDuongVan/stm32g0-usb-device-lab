/*
 * hid_keyboard_convert.h
 *
 *  Created on: Jul 2, 2026
 *      Author: haodu
 */

#ifndef INC_HID_KEYBOARD_CONVERT_H_
#define INC_HID_KEYBOARD_CONVERT_H_

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Function prototypes -------------------------------------------------------*/

void            HidKeyboardConvert_Init(void);
void            HidKeyboardConvert_Run(void);
const uint8_t  *HidKeyboardConvert_GetReportData(void);

#endif /* INC_HID_KEYBOARD_CONVERT_H_ */
