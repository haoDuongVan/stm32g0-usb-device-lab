/*
 * usb_vendor_cmd.c
 *
 *  Created on: Jul 14, 2026
 *      Author: haodu
 *
 * EP0 vendor request handler.
 * All commands use control IN (bmRequestType = 0xC0) and always return a
 * response struct so the host has an explicit success/failure signal.
 *
 * CDC log safety:
 *   VendorCmd_HandleSetup runs in USB stack context (called from
 *   Composite_Setup). CdcLog_Printf is not ISR-safe, so the handler only
 *   sets a pending log event flag. VendorCmd_FlushPendingLog(), called from
 *   the main loop via HID_Keyboard_App, reads the flag and does the actual log.
 */

/* Includes ------------------------------------------------------------------*/
#include "usb_vendor_cmd.h"
#include "usbd_composite.h"
#include "usb_vendor_bulk.h"
#include "usb_cdc_log.h"
#include "main.h"
#include "usbd_ctlreq.h"
#include <string.h>

/* Private defines -----------------------------------------------------------*/
#define FW_VERSION_MAJOR            0U
#define FW_VERSION_MINOR            1U

/* LED blink cadences */
#define LED_BLINK_SLOW_MS           500U
#define LED_BLINK_FAST_MS           125U

/* Private types -------------------------------------------------------------*/

/*
 * Pending log event set by VendorCmd_HandleSetup (USB context),
 * consumed and cleared by VendorCmd_FlushPendingLog (main loop).
 */
typedef enum
{
  VLOG_NONE            = 0,
  VLOG_GET_INFO_OK,
  VLOG_SET_REPEAT_ON,
  VLOG_SET_REPEAT_OFF,
  VLOG_SET_LED_OK,
  VLOG_SET_LED_ERR,
  VLOG_DUMP_OK,
  VLOG_DUMP_BUSY,
} VendorLogEvent_t;

/* Private variables ---------------------------------------------------------*/
static bool            sRepeatEnable;
static VendorLedMode_t sLedMode;

/*
 * Pending log state written by the EP0 handler (USB context),
 * read and cleared by VendorCmd_FlushPendingLog (main loop).
 */
static volatile VendorLogEvent_t sPendingLog;
static volatile uint32_t         sPendingLogVal; /* carries wValue for LED/repeat */

/*
 * Static response buffers - must outlive the EP0 IN transfer, so they cannot
 * be stack-allocated. Each command writes its response here before calling
 * USBD_CtlSendData.
 */
static FirmwareInfo_t       sFwInfo;
static VendorResponse_t     sVendorRsp;
static VendorDumpResponse_t sDumpRsp;

/* Function definitions ------------------------------------------------------*/

void VendorCmd_Init(void)
{
  sRepeatEnable  = true;
  sLedMode       = LED_MODE_OFF;
  sPendingLog    = VLOG_NONE;
  sPendingLogVal = 0U;

  (void)memset(&sFwInfo, 0, sizeof(sFwInfo));
  sFwInfo.magic               = FW_INFO_MAGIC;
  sFwInfo.versionMajor        = FW_VERSION_MAJOR;
  sFwInfo.versionMinor        = FW_VERSION_MINOR;
  sFwInfo.featureFlags        = FW_FEATURE_HID_KEYBOARD
                              | FW_FEATURE_CDC_LOG
                              | FW_FEATURE_VENDOR_BULK
                              | FW_FEATURE_REPEAT_CONTROL;
  sFwInfo.hidInterface        = 0U;
  sFwInfo.cdcControlInterface = 1U;
  sFwInfo.cdcDataInterface    = 2U;
  sFwInfo.hidInEp             = COMP_HID_EPIN_ADDR;
  sFwInfo.cdcLogInEp          = COMP_CDC_DATA_IN_EP_ADDR;

  HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);

  CdcLog_Printf("[BOOT] Vendor command interface ready\r\n");
}

bool VendorCmd_GetRepeatEnable(void)
{
  return sRepeatEnable;
}

VendorLedMode_t VendorCmd_GetLedMode(void)
{
  return sLedMode;
}

/*
 * Drive LED_GREEN to match sLedMode.
 * BLINK_SLOW toggles every 500 ms, BLINK_FAST every 125 ms.
 * OFF/ON write the pin immediately. Idempotent on repeated calls.
 */
void VendorCmd_UpdateLed(void)
{
  static uint32_t sLastToggleTick = 0U;
  uint32_t period = 0U;

  switch (sLedMode)
  {
    case LED_MODE_OFF:
      HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
      return;

    case LED_MODE_ON:
      HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);
      return;

    case LED_MODE_BLINK_SLOW:
      period = LED_BLINK_SLOW_MS;
      break;

    case LED_MODE_BLINK_FAST:
      period = LED_BLINK_FAST_MS;
      break;

    default:
      return;
  }

  uint32_t now = HAL_GetTick();
  if ((now - sLastToggleTick) >= period)
  {
    HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
    sLastToggleTick = now;
  }
}

/*
 * Flush any pending CDC log event written by VendorCmd_HandleSetup.
 * Must be called from the main loop - not ISR-safe.
 * Reads sPendingLog once, clears it, then formats the log string.
 */
void VendorCmd_FlushPendingLog(void)
{
  VendorLogEvent_t evt = sPendingLog;
  if (evt == VLOG_NONE) return;

  uint32_t val = sPendingLogVal;
  sPendingLog  = VLOG_NONE;   /* clear before log - log may take a while */

  switch (evt)
  {
    case VLOG_GET_INFO_OK:
      CdcLog_Printf("[VREQ] GET_FIRMWARE_INFO SUCCESS\r\n");
      break;
    case VLOG_SET_REPEAT_ON:
    case VLOG_SET_REPEAT_OFF:
      CdcLog_Printf("[VREQ] SET_REPEAT_ENABLE value=%u SUCCESS\r\n",
                    (unsigned int)val);
      break;
    case VLOG_SET_LED_OK:
      CdcLog_Printf("[VREQ] SET_LED_MODE mode=%u SUCCESS\r\n",
                    (unsigned int)val);
      break;
    case VLOG_SET_LED_ERR:
      CdcLog_Printf("[VREQ] SET_LED_MODE mode=%u FAILURE\r\n",
                    (unsigned int)val);
      break;
    case VLOG_DUMP_OK:
      CdcLog_Printf("[VREQ] START_RAM_DUMP accepted len=%u SUCCESS\r\n",
                    (unsigned int)val);
      break;
    case VLOG_DUMP_BUSY:
      CdcLog_Printf("[VREQ] START_RAM_DUMP FAILURE (busy)\r\n");
      break;
    default:
      break;
  }
}

/*
 * Dispatch one EP0 vendor Setup request.
 * Runs in USB stack context - must not call CdcLog_Printf directly.
 * Sets sPendingLog for VendorCmd_FlushPendingLog (main loop) to consume.
 * Returns USBD_OK, or USBD_FAIL after USBD_CtlError for unknown requests.
 */
uint8_t VendorCmd_HandleSetup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  uint16_t len;

  switch (req->bRequest)
  {
    /* ------------------------------------------------------------------ */
    case VENDOR_REQ_GET_FIRMWARE_INFO:
      len = (uint16_t)MIN(sizeof(sFwInfo), req->wLength);
      (void)USBD_CtlSendData(pdev, (uint8_t *)&sFwInfo, len);
      sPendingLog = VLOG_GET_INFO_OK;
      break;

    /* ------------------------------------------------------------------ */
    case VENDOR_REQ_SET_REPEAT_ENABLE:
      sRepeatEnable          = (req->wValue != 0U);
      sVendorRsp.status      = VENDOR_STATUS_OK;
      sVendorRsp.request     = VENDOR_REQ_SET_REPEAT_ENABLE;
      len = (uint16_t)MIN(sizeof(sVendorRsp), req->wLength);
      (void)USBD_CtlSendData(pdev, (uint8_t *)&sVendorRsp, len);
      sPendingLogVal = (uint32_t)sRepeatEnable;
      sPendingLog    = sRepeatEnable ? VLOG_SET_REPEAT_ON : VLOG_SET_REPEAT_OFF;
      break;

    /* ------------------------------------------------------------------ */
    case VENDOR_REQ_SET_LED_MODE:
      if (req->wValue > (uint16_t)LED_MODE_BLINK_FAST)
      {
        sVendorRsp.status  = VENDOR_STATUS_ERROR;
        sVendorRsp.request = VENDOR_REQ_SET_LED_MODE;
        len = (uint16_t)MIN(sizeof(sVendorRsp), req->wLength);
        (void)USBD_CtlSendData(pdev, (uint8_t *)&sVendorRsp, len);
        sPendingLogVal = (uint32_t)req->wValue;
        sPendingLog    = VLOG_SET_LED_ERR;
        break;
      }
      sLedMode               = (VendorLedMode_t)req->wValue;
      sVendorRsp.status      = VENDOR_STATUS_OK;
      sVendorRsp.request     = VENDOR_REQ_SET_LED_MODE;
      len = (uint16_t)MIN(sizeof(sVendorRsp), req->wLength);
      (void)USBD_CtlSendData(pdev, (uint8_t *)&sVendorRsp, len);
      sPendingLogVal = (uint32_t)sLedMode;
      sPendingLog    = VLOG_SET_LED_OK;
      break;

    /* ------------------------------------------------------------------ */
    case VENDOR_REQ_START_RAM_DUMP:
    {
      uint32_t acceptedLength = 0U;

      if (VendorDump_Start(&acceptedLength))
      {
        sDumpRsp.status         = VENDOR_STATUS_OK;
        sDumpRsp.acceptedLength = acceptedLength;
        sPendingLogVal          = acceptedLength;
        sPendingLog             = VLOG_DUMP_OK;
      }
      else
      {
        sDumpRsp.status         = VENDOR_STATUS_ERROR;
        sDumpRsp.acceptedLength = 0U;
        sPendingLog             = VLOG_DUMP_BUSY;
      }

      sDumpRsp.request = VENDOR_REQ_START_RAM_DUMP;
      len = (uint16_t)MIN(sizeof(sDumpRsp), req->wLength);
      (void)USBD_CtlSendData(pdev, (uint8_t *)&sDumpRsp, len);
      break;
    }

    /* ------------------------------------------------------------------ */
    default:
      USBD_CtlError(pdev, req);
      return (uint8_t)USBD_FAIL;
  }

  return (uint8_t)USBD_OK;
}
