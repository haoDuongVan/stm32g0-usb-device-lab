/*
 * scan_scheduler.c
 *
 *  Created on: Jul 2, 2026
 *      Author: haodu
 */

/* Includes ------------------------------------------------------------------*/
#include "scan_scheduler.h"
#include "stm32g0xx_hal.h"

/* Private defines -----------------------------------------------------------*/

/*
 * Maximum number of unserviced scan requests that can accumulate.
 * If the main loop is blocked for longer than 10 × 5 ms = 50 ms,
 * further timer ticks are silently dropped rather than letting the
 * counter grow without bound.
 */
#define SCAN_REQUEST_MAX_COUNT      10U

/* Private variables ---------------------------------------------------------*/
static volatile uint8_t  sScanRequestCount = 0U;

// Free-running tick total, never capped or consumed. For evidence/debug watch only
static volatile uint32_t sScanTickTotal = 0U;

/* Function definitions ------------------------------------------------------*/

// Reset the request counter to zero. Call once before starting the timer
void ScanScheduler_Init(void)
{
  sScanRequestCount = 0U;
  sScanTickTotal    = 0U;
}

// Increment the pending scan request counter. Called from TIM6 ISR every 5 ms
void ScanScheduler_OnTimerTick(void)
{
  sScanTickTotal++;

  if (sScanRequestCount < SCAN_REQUEST_MAX_COUNT)
  {
    sScanRequestCount++;
  }
}

// Atomically consume one request. Returns 1 if a request was pending, else 0
uint8_t ScanScheduler_TakeRequest(void)
{
  uint8_t  hasRequest = 0U;
  uint32_t primask    = __get_PRIMASK();

  __disable_irq();

  if (sScanRequestCount > 0U)
  {
    sScanRequestCount--;
    hasRequest = 1U;
  }

  if (primask == 0U) { __enable_irq(); }

  return hasRequest;
}
