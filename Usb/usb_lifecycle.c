/*
 * usb_lifecycle.c
 *
 *  Created on: Jul 14, 2026
 *      Author: haodu
 *
 * Reset: per USB spec, a bus reset invalidates the previous session, so
 * UsbLifecycle_OnReset aborts any in-flight vendor dump. Composite_Init
 * already resets hidTxBusy/cdcTxBusy/vendorTxBusy itself (memset + explicit
 * assignment on the freshly allocated class handle), so this file does not
 * touch those.
 *
 * Suspend: per USB spec, suspend must preserve device state - this only
 * counts the event, it does not touch any busy flag or the vendor dump.
 *
 * Resume: a transfer in flight when the bus suspended may never get its
 * DataIn completion, leaving hidTxBusy/cdcTxBusy/vendorTxBusy stuck true
 * forever. UsbLifecycle_OnResume force-clears them and aborts any in-flight
 * vendor dump, since resuming a bulk transfer exactly where suspend left
 * off is not worth the complexity for a debug feature - the host just
 * issues START_RAM_DUMP again.
 */

/* Includes ------------------------------------------------------------------*/
#include "usb_lifecycle.h"
#include "usbd_composite.h"
#include "usb_vendor_bulk.h"
#include "usb_cdc_log.h"

/* Private types -------------------------------------------------------------*/

/*
 * Pending log event set by UsbLifecycle_OnReset/OnSuspend/OnResume (USB
 * stack / PCD callback context), consumed by UsbLifecycle_FlushPendingLog
 * (main loop).
 */
typedef enum
{
  ULOG_NONE = 0,
  ULOG_RESET,
  ULOG_SUSPEND,
  ULOG_RESUME,
} UsbLifecycleLogEvent_t;

/* Private variables ---------------------------------------------------------*/

/*
 * Reset/suspend/resume counters and the pending log event they feed.
 * sPendingLogDumpAborted only applies to ULOG_RESET/ULOG_RESUME.
 */
static uint32_t                        sResetCount;
static uint32_t                        sSuspendCount;
static uint32_t                        sResumeCount;
static volatile UsbLifecycleLogEvent_t sPendingLog;
static volatile uint32_t               sPendingLogCount;
static volatile bool                   sPendingLogDumpAborted;

/* Function definitions ------------------------------------------------------*/

void UsbLifecycle_OnReset(void)
{
  sResetCount++;
  sPendingLogDumpAborted = VendorDump_Abort();
  sPendingLogCount       = sResetCount;
  sPendingLog            = ULOG_RESET;
}

void UsbLifecycle_OnSuspend(USBD_HandleTypeDef *pdev)
{
  UNUSED(pdev);

  sSuspendCount++;
  sPendingLogCount = sSuspendCount;
  sPendingLog      = ULOG_SUSPEND;
}

void UsbLifecycle_OnResume(USBD_HandleTypeDef *pdev)
{
  USBD_Composite_HandleTypeDef *hcomp = (USBD_Composite_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId];

  if (hcomp != NULL)
  {
    hcomp->hidTxBusy    = false;
    hcomp->cdcTxBusy    = false;
    hcomp->vendorTxBusy = false;
  }

  sResumeCount++;
  sPendingLogDumpAborted = VendorDump_Abort();
  sPendingLogCount       = sResumeCount;
  sPendingLog            = ULOG_RESUME;
}

/*
 * Flush any pending reset/suspend/resume log event.
 * Must be called from the main loop - not ISR-safe.
 */
void UsbLifecycle_FlushPendingLog(void)
{
  UsbLifecycleLogEvent_t evt = sPendingLog;
  if (evt == ULOG_NONE) return;

  uint32_t count       = sPendingLogCount;
  bool     dumpAborted = sPendingLogDumpAborted;
  sPendingLog          = ULOG_NONE;   /* clear before log - log may take a while */

  switch (evt)
  {
    case ULOG_RESET:
      CdcLog_Printf("[USB] RESET #%lu dumpAborted=%u\r\n",
                    (unsigned long)count, (unsigned int)dumpAborted);
      break;
    case ULOG_SUSPEND:
      CdcLog_Printf("[USB] SUSPEND #%lu\r\n", (unsigned long)count);
      break;
    case ULOG_RESUME:
      CdcLog_Printf("[USB] RESUME #%lu dumpAborted=%u\r\n",
                    (unsigned long)count, (unsigned int)dumpAborted);
      break;
    default:
      break;
  }
}
