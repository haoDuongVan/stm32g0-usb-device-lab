/*
 * usb_cdc_log.c
 *
 *  Created on: Jul 14, 2026
 *      Author: haodu
 */

/* Includes ------------------------------------------------------------------*/
#include "usb_cdc_log.h"
#include "usbd_composite.h"
#include "stm32g0xx_hal.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* Private defines -----------------------------------------------------------*/
#define CDC_LOG_BUF_MASK            (CDC_LOG_BUF_SIZE - 1U)
#define CDC_LOG_PRINTF_MAX          128U

/*
 * DTR is asserted by the OS driver as soon as the port is opened, which can
 * be a few tens of ms before the terminal application actually starts
 * reading from it. Delaying the greeting avoids it arriving before the
 * terminal is ready to display it.
 */
#define CDC_LOG_CONNECT_GREETING_DELAY_MS   100U

/* Private variables ---------------------------------------------------------*/
static uint8_t  sRingBuf[CDC_LOG_BUF_SIZE];
static uint16_t sWriteIdx;
static uint16_t sReadIdx;
static bool     sWasHostConnected;
static bool     sGreetingPending;
static uint32_t sConnectedAtMs;

/*
 * Scratch buffer used by CdcLog_Run to hold one USB packet before handing
 * it to USBD_COMPOSITE_CDC_Transmit. Kept static so the buffer remains
 * valid while the USB transfer is in progress after the function returns.
 */
static uint8_t sTxScratch[COMP_CDC_DATA_EP_SIZE];

/* External variables --------------------------------------------------------*/
extern USBD_HandleTypeDef hUsbDeviceFS;

/* Private functions ---------------------------------------------------------*/

// Return the number of bytes currently stored in the ring buffer
static uint16_t RingBuf_Used(void)
{
  return (sWriteIdx - sReadIdx) & CDC_LOG_BUF_MASK;
}

// Return the number of free bytes. One slot is always kept empty to distinguish full from empty
static uint16_t RingBuf_Free(void)
{
  return (CDC_LOG_BUF_SIZE - 1U) - RingBuf_Used();
}

// Push one byte. Caller must verify space is available first
static void RingBuf_Push(uint8_t byte)
{
  sRingBuf[sWriteIdx] = byte;
  sWriteIdx = (sWriteIdx + 1U) & CDC_LOG_BUF_MASK;
}

// Pop up to maxLen bytes into dst. Returns the number of bytes actually popped
static uint16_t RingBuf_Pop(uint8_t *dst, uint16_t maxLen)
{
  uint16_t avail = RingBuf_Used();
  uint16_t count = (avail < maxLen) ? avail : maxLen;

  for (uint16_t i = 0U; i < count; i++)
  {
    dst[i]    = sRingBuf[sReadIdx];
    sReadIdx  = (sReadIdx + 1U) & CDC_LOG_BUF_MASK;
  }

  return count;
}

/*
 * Track the host connection and queue a greeting CDC_LOG_CONNECT_GREETING_DELAY_MS
 * after it rises. DTR going high (port opened) doesn't mean the terminal has
 * started reading yet, so the greeting is delayed instead of sent immediately.
 */
static void CdcLog_UpdateConnection(bool hostConnected)
{
  if (hostConnected && !sWasHostConnected)
  {
    sGreetingPending = true;
    sConnectedAtMs   = HAL_GetTick();
  }

  sWasHostConnected = hostConnected;

  if (!hostConnected)
  {
    sGreetingPending = false;
    return;
  }

  if (sGreetingPending && ((HAL_GetTick() - sConnectedAtMs) >= CDC_LOG_CONNECT_GREETING_DELAY_MS))
  {
    CdcLog_Printf("[CDC] log channel connected\r\n");
    sGreetingPending = false;
  }
}

/* Function definitions ------------------------------------------------------*/

// Reset the ring buffer to empty. Call once before starting the main loop
void CdcLog_Init(void)
{
  sWriteIdx         = 0U;
  sReadIdx          = 0U;
  sWasHostConnected = false;
  sGreetingPending  = false;
  sConnectedAtMs    = 0U;
  (void)memset(sRingBuf, 0, sizeof(sRingBuf));
}

/*
 * Send one USB packet worth of buffered data to the CDC IN endpoint.
 * Returns immediately if the host has not opened the virtual COM port,
 * if a previous transfer is still pending, or if the buffer is empty.
 */
void CdcLog_Run(void)
{
  bool hostConnected = USBD_COMPOSITE_CDC_IsHostConnected(&hUsbDeviceFS);

  CdcLog_UpdateConnection(hostConnected);

  if (!hostConnected)
  {
    return;
  }

  if (!USBD_COMPOSITE_CDC_IsTxIdle(&hUsbDeviceFS))
  {
    return;
  }

  if (RingBuf_Used() == 0U)
  {
    return;
  }

  uint16_t count = RingBuf_Pop(sTxScratch, COMP_CDC_DATA_EP_SIZE);
  (void)USBD_COMPOSITE_CDC_Transmit(&hUsbDeviceFS, sTxScratch, count);
}

/*
 * Copy up to len bytes from buf into the ring buffer.
 * Bytes that exceed the available space are silently dropped.
 */
void CdcLog_Write(const char *buf, uint16_t len)
{
  uint16_t space = RingBuf_Free();
  uint16_t count = (len < space) ? len : space;

  for (uint16_t i = 0U; i < count; i++)
  {
    RingBuf_Push((uint8_t)buf[i]);
  }
}

/*
 * Format a string with printf-style arguments and write it to the ring buffer.
 * Uses a 128-byte stack scratch buffer. Output longer than 128 bytes is truncated.
 */
void CdcLog_Printf(const char *fmt, ...)
{
  char     scratch[CDC_LOG_PRINTF_MAX];
  uint16_t outLen = 0U;
  va_list  args;

  va_start(args, fmt);
  int len = vsnprintf(scratch, sizeof(scratch), fmt, args);
  va_end(args);

  if (len > 0)
  {
    /*
     * vsnprintf() returns the number of bytes that *would* have been written,
     * which can exceed sizeof(scratch) when the formatted string is truncated.
     * Clamp to the actual bytes present in scratch to avoid reading past the buffer.
     */
    outLen = ((uint16_t)len < (uint16_t)sizeof(scratch))
              ? (uint16_t)len
              : (uint16_t)(sizeof(scratch) - 1U);
    CdcLog_Write(scratch, outLen);
  }
}
