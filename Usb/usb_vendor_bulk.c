/*
 * usb_vendor_bulk.c
 *
 *  Created on: Jul 14, 2026
 *      Author: haodu
 *
 * RAM dump state machine. VendorDump_Start arms a dump (called by the EP0
 * VENDOR_REQ_START_RAM_DUMP handler in usb_vendor_cmd.c).
 * VendorDump_Run kicks the first chunk from the main loop once armed;
 * VendorDump_OnTxCplt (called from Composite_DataIn) queues every
 * subsequent chunk as each transfer completes. VendorDump_Abort is called
 * on USB reset (Composite_Init) and resume (USBD_COMPOSITE_OnResume).
 */

/* Includes ------------------------------------------------------------------*/
#include "usb_vendor_bulk.h"
#include "usbd_composite.h"
#include "usb_cdc_log.h"

/* External variables --------------------------------------------------------*/
extern USBD_HandleTypeDef hUsbDeviceFS;

/* Private defines -----------------------------------------------------------*/
/* SRAM1 base and fixed dump size (144 KB) - matches STM32G0B1RETX_FLASH.ld */
#define RAM_DUMP_BASE               0x20000000UL
#define RAM_DUMP_MAX_SIZE           (144UL * 1024UL)

/* Private variables ---------------------------------------------------------*/
static bool           sDumpActive;
static uint32_t       sDumpAddr;
static uint32_t       sDumpTotal;
static uint32_t       sDumpOffset;
static volatile bool  sDumpDone;

/* Private functions ---------------------------------------------------------*/

/* Queue one 64-byte chunk. Called from ISR (OnTxCplt) or main loop (Run) */
static void VendorDump_SendNextChunk(USBD_HandleTypeDef *pdev)
{
  uint32_t remaining = sDumpTotal - sDumpOffset;
  uint16_t chunkLen   = (remaining > COMP_VENDOR_DATA_EP_SIZE)
                      ? (uint16_t)COMP_VENDOR_DATA_EP_SIZE
                      : (uint16_t)remaining;

  const uint8_t *src = (const uint8_t *)(uintptr_t)(sDumpAddr + sDumpOffset);

  if (USBD_COMPOSITE_VENDOR_Transmit(pdev, src, chunkLen) == (uint8_t)USBD_OK)
  {
    sDumpOffset += chunkLen;

    if (sDumpOffset >= sDumpTotal)
    {
      sDumpActive = false;
      sDumpDone   = true;   /* signal main loop - CDC log not safe from ISR */
    }
  }
}

/* Function definitions ------------------------------------------------------*/

/*
 * Arm a RAM dump of the fixed SRAM1 region. Returns false (leaving
 * *acceptedLength untouched) if a dump is already in progress.
 * Does not send the first chunk - VendorDump_Run does that on the next
 * main loop iteration.
 */
bool VendorDump_Start(uint32_t *acceptedLength)
{
  if (sDumpActive)
  {
    return false;
  }

  sDumpAddr    = RAM_DUMP_BASE;
  sDumpTotal   = RAM_DUMP_MAX_SIZE;
  sDumpOffset  = 0U;
  sDumpActive  = true;

  *acceptedLength = RAM_DUMP_MAX_SIZE;

  return true;
}

/*
 * Clear the dump state machine unconditionally. Called on USB reset
 * (Composite_Init) and resume (USBD_COMPOSITE_OnResume) so a dump left
 * mid-flight across a bus reset or suspend cannot wedge sDumpActive in a
 * state no new START_RAM_DUMP request can ever clear. Safe to call whether
 * or not a dump is active. ISR-safe - only touches plain state, no
 * CdcLog_Printf.
 * Returns true if a dump was actually active (a real abort happened),
 * false if this call was a no-op - lets the caller log which case it was.
 */
bool VendorDump_Abort(void)
{
  bool wasActive = sDumpActive;

  sDumpActive = false;
  sDumpAddr   = 0U;
  sDumpTotal  = 0U;
  sDumpOffset = 0U;
  sDumpDone   = false;

  return wasActive;
}

/* Called from Composite_DataIn (ISR) when EP4 IN transfer completes */
void VendorDump_OnTxCplt(USBD_HandleTypeDef *pdev)
{
  if (sDumpActive)
  {
    VendorDump_SendNextChunk(pdev);
  }
}

/*
 * Called every main loop iteration via HID_Keyboard_App.
 * Kicks the first chunk when a dump is freshly armed (sDumpOffset == 0),
 * and logs completion once the ISR signals sDumpDone.
 */
void VendorDump_Run(void)
{
  if (sDumpDone)
  {
    sDumpDone = false;
    CdcLog_Printf("[BULK] RAM dump complete len=%lu\r\n",
                  (unsigned long)sDumpTotal);
    return;
  }

  if (sDumpActive && (sDumpOffset == 0U) && USBD_COMPOSITE_VENDOR_IsTxIdle(&hUsbDeviceFS))
  {
    CdcLog_Printf("[BULK] RAM dump started len=%lu\r\n",
                  (unsigned long)sDumpTotal);
    VendorDump_SendNextChunk(&hUsbDeviceFS);
  }
}
