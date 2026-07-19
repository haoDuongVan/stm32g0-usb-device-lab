/*
 * app_main.c
 *
 *  Created on: Jul 2, 2026
 *      Author: haodu
 */

/* Includes ------------------------------------------------------------------*/
#include "app_main.h"
#include "keypad.h"
#include "scan_scheduler.h"
#include "key_event_queue.h"
#include "key_detect.h"
#include "hid_keyboard_convert.h"
#include "usb_hid_keyboard.h"
#include "usb_cdc_log.h"
#include "usb_vendor_cmd.h"

#include "stm32g0xx_hal.h"

/* Private variables ---------------------------------------------------------*/

/* External variables --------------------------------------------------------*/
extern TIM_HandleTypeDef  htim6;

/* Function definitions ------------------------------------------------------*/

// Forward TIM6 period-elapsed ticks to the scan scheduler
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM6)
  {
    ScanScheduler_OnTimerTick();
  }
}

// Initialise all keyboard subsystems
void HID_Keyboard_Init(void)
{
  KeyEventQueue_Init();
  Keypad_Init();
  KeyDetect_Init();
  ScanScheduler_Init();
  UsbHidKeyboard_Init();
  HidKeyboardConvert_Init();
  CdcLog_Init();
  VendorCmd_Init();
  CdcLog_Printf("[BOOT] stm32g0-usb-device-lab started\r\n");
  HAL_TIM_Base_Start_IT(&htim6);
}

// Main loop handler
void HID_Keyboard_App(void)
{
  while (ScanScheduler_TakeRequest() != 0U)
  {
    KeyDetect_Run();
  }

  HidKeyboardConvert_Run();
  CdcLog_Run();
  VendorCmd_FlushPendingLog();
  VendorCmd_UpdateLed();
}
