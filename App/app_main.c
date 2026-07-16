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
  Keypad_Init();
  ScanScheduler_Init();
  HAL_TIM_Base_Start_IT(&htim6);
}

// Main loop handler
void HID_Keyboard_App(void)
{
  while (ScanScheduler_TakeRequest() != 0U)
  {
    // TODO: Read keypad matrix and detect pressed/released keys
  }

  // TODO: Do other application tasks here
}
