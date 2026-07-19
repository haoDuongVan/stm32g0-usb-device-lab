/*
 * usbd_composite.c
 *
 *  Created on: Jul 14, 2026
 *      Author: haodu
 *
 * Custom USB class, replacing usbd_hid.c as the single registered class.
 * This step (lab-09a, part 1) implements only HID Keyboard (Interface 0),
 * to confirm the hand-written class mechanism (Init/DeInit/Setup/DataIn/
 * GetCfgDesc) works end-to-end before adding CDC ACM in the next step.
 *
 * Endpoint map (this step):
 *   EP1 IN  0x81  HID Keyboard Interrupt IN  8 bytes  10 ms
 */

/* Includes ------------------------------------------------------------------*/
#include "usbd_composite.h"
#include "usbd_ctlreq.h"

#include <stddef.h>
#include <string.h>

/* Private defines -----------------------------------------------------------*/
#define HID_DESCRIPTOR_TYPE         0x21U
#define HID_REPORT_DESC_TYPE        0x22U
#define HID_KEYBOARD_REPORT_DESC_SIZE 45U

#define HID_REQ_SET_PROTOCOL        0x0BU
#define HID_REQ_GET_PROTOCOL        0x03U
#define HID_REQ_SET_IDLE            0x0AU
#define HID_REQ_GET_IDLE            0x02U

/* Interface numbers */
#define COMP_IF_HID                 0U

/* Configuration descriptor total length: cfg(9) + if(9) + hid(9) + ep(7) */
#define COMP_CFG_DESC_SIZE          34U

/* Private variables ---------------------------------------------------------*/

/*
 * HID Boot Keyboard report descriptor - same 45-byte descriptor already
 * validated in lab-08 (modifier + reserved + 6 keycodes, no LED report).
 */
__ALIGN_BEGIN static const uint8_t sHidReportDesc[HID_KEYBOARD_REPORT_DESC_SIZE] __ALIGN_END =
{
  0x05, 0x01,        /* Usage Page (Generic Desktop) */
  0x09, 0x06,        /* Usage (Keyboard) */
  0xA1, 0x01,        /* Collection (Application) */

  0x05, 0x07,        /* Usage Page (Key Codes) */
  0x19, 0xE0,        /* Usage Minimum (224) */
  0x29, 0xE7,        /* Usage Maximum (231) */
  0x15, 0x00,        /* Logical Minimum (0) */
  0x25, 0x01,        /* Logical Maximum (1) */
  0x75, 0x01,        /* Report Size (1) */
  0x95, 0x08,        /* Report Count (8) */
  0x81, 0x02,        /* Input (Data, Variable, Absolute) - modifier byte */

  0x95, 0x01,        /* Report Count (1) */
  0x75, 0x08,        /* Report Size (8) */
  0x81, 0x03,        /* Input (Constant) - reserved byte */

  0x95, 0x06,        /* Report Count (6) */
  0x75, 0x08,        /* Report Size (8) */
  0x15, 0x00,        /* Logical Minimum (0) */
  0x25, 0x65,        /* Logical Maximum (101) */
  0x05, 0x07,        /* Usage Page (Key Codes) */
  0x19, 0x00,        /* Usage Minimum (0) */
  0x29, 0x65,        /* Usage Maximum (101) */
  0x81, 0x00,        /* Input (Data, Array) - key usage array */

  0xC0               /* End Collection */
};

__ALIGN_BEGIN static uint8_t sCompositeCfgDesc[COMP_CFG_DESC_SIZE] __ALIGN_END =
{
  /* ---------- Configuration Descriptor ---------- */
  0x09,                          /* bLength */
  USB_DESC_TYPE_CONFIGURATION,   /* bDescriptorType */
  COMP_CFG_DESC_SIZE, 0x00,      /* wTotalLength */
  0x01,                          /* bNumInterfaces: HID(0) only */
  0x01,                          /* bConfigurationValue */
  0x00,                          /* iConfiguration */
#if (USBD_SELF_POWERED == 1U)
  0xC0,                          /* bmAttributes: Self-powered */
#else
  0x80,                          /* bmAttributes: Bus-powered */
#endif
  0x32,                          /* MaxPower: 100 mA */

  /* ---------- Interface 0: HID Keyboard ---------- */
  0x09,                          /* bLength */
  USB_DESC_TYPE_INTERFACE,       /* bDescriptorType */
  COMP_IF_HID,                   /* bInterfaceNumber */
  0x00,                          /* bAlternateSetting */
  0x01,                          /* bNumEndpoints */
  0x03,                          /* bInterfaceClass: HID */
  0x01,                          /* bInterfaceSubClass: Boot */
  0x01,                          /* bInterfaceProtocol: Keyboard */
  0x00,                          /* iInterface */

  /* HID Class Descriptor */
  0x09,                          /* bLength */
  HID_DESCRIPTOR_TYPE,           /* bDescriptorType: HID */
  0x11, 0x01,                    /* bcdHID: 1.11 */
  0x0F,                          /* bCountryCode: Japan */
  0x01,                          /* bNumDescriptors */
  HID_REPORT_DESC_TYPE,          /* bDescriptorType: Report */
  HID_KEYBOARD_REPORT_DESC_SIZE, /* wItemLength low */
  0x00,                          /* wItemLength high */

  /* EP1 IN: HID Keyboard Interrupt IN */
  0x07,                          /* bLength */
  USB_DESC_TYPE_ENDPOINT,        /* bDescriptorType */
  COMP_HID_EPIN_ADDR,            /* bEndpointAddress: IN EP1 */
  0x03,                          /* bmAttributes: Interrupt */
  COMP_HID_EPIN_SIZE, 0x00,      /* wMaxPacketSize: 8 bytes */
  COMP_HID_FS_BINTERVAL,         /* bInterval: 10 ms */
};

/* Device qualifier - not used in FS-only devices but required by the class interface */
__ALIGN_BEGIN static uint8_t sDeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END =
{
  USB_LEN_DEV_QUALIFIER_DESC,     /* bLength */
  USB_DESC_TYPE_DEVICE_QUALIFIER, /* bDescriptorType */
  0x00, 0x02,                     /* bcdUSB */
  0x00,                           /* bDeviceClass */
  0x00,                           /* bDeviceSubClass */
  0x00,                           /* bDeviceProtocol */
  0x40,                           /* bMaxPacketSize0 */
  0x01,                           /* bNumConfigurations */
  0x00,                           /* bReserved */
};

/* Private function prototypes -----------------------------------------------*/
static uint8_t  Composite_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t  Composite_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t  Composite_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t  Composite_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t  Composite_SOF(USBD_HandleTypeDef *pdev);
static uint8_t *Composite_GetCfgDesc(uint16_t *length);
static uint8_t *Composite_GetDeviceQualifierDesc(uint16_t *length);

/* Exported class handle -----------------------------------------------------*/
USBD_ClassTypeDef USBD_COMPOSITE =
{
  Composite_Init,
  Composite_DeInit,
  Composite_Setup,
  NULL,                     /* EP0_TxSent */
  NULL,                     /* EP0_RxReady */
  Composite_DataIn,
  NULL,                     /* DataOut */
  Composite_SOF,
  NULL,                     /* IsoINIncomplete */
  NULL,                     /* IsoOUTIncomplete */
  Composite_GetCfgDesc,
  Composite_GetCfgDesc,     /* HS config = FS config */
  Composite_GetCfgDesc,     /* Other speed */
  Composite_GetDeviceQualifierDesc,
};

/* Private functions ---------------------------------------------------------*/

static uint8_t Composite_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  USBD_Composite_HandleTypeDef *hcomp;
  UNUSED(cfgidx);

  hcomp = (USBD_Composite_HandleTypeDef *)USBD_malloc(sizeof(USBD_Composite_HandleTypeDef));

  if (hcomp == NULL)
  {
    pdev->pClassDataCmsit[pdev->classId] = NULL;
    return (uint8_t)USBD_EMEM;
  }

  (void)memset(hcomp, 0, sizeof(USBD_Composite_HandleTypeDef));
  pdev->pClassDataCmsit[pdev->classId] = hcomp;

  hcomp->hidTxBusy = false;

  /* Open HID IN endpoint */
  (void)USBD_LL_OpenEP(pdev, COMP_HID_EPIN_ADDR, USBD_EP_TYPE_INTR, COMP_HID_EPIN_SIZE);
  pdev->ep_in[COMP_HID_EPIN_ADDR & 0x0FU].is_used = 1U;

  return (uint8_t)USBD_OK;
}

static uint8_t Composite_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  UNUSED(cfgidx);

  (void)USBD_LL_CloseEP(pdev, COMP_HID_EPIN_ADDR);
  pdev->ep_in[COMP_HID_EPIN_ADDR & 0x0FU].is_used = 0U;

  if (pdev->pClassDataCmsit[pdev->classId] != NULL)
  {
    USBD_free(pdev->pClassDataCmsit[pdev->classId]);
    pdev->pClassDataCmsit[pdev->classId] = NULL;
  }

  return (uint8_t)USBD_OK;
}

static uint8_t Composite_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  USBD_Composite_HandleTypeDef *hcomp = (USBD_Composite_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId];
  uint8_t        ifNum = (uint8_t)(req->wIndex & 0xFFU);
  uint16_t       len   = 0U;
  const uint8_t *pbuf  = NULL;

  if (hcomp == NULL)
  {
    return (uint8_t)USBD_FAIL;
  }

  /* Only Interface 0 (HID) exists in this step - CDC interfaces added next */
  if (ifNum != COMP_IF_HID)
  {
    USBD_CtlError(pdev, req);
    return (uint8_t)USBD_FAIL;
  }

  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
    case USB_REQ_TYPE_CLASS:
      switch (req->bRequest)
      {
        case HID_REQ_SET_PROTOCOL:
          hcomp->hidProtocol = (uint32_t)(req->wValue);
          break;

        case HID_REQ_GET_PROTOCOL:
          (void)USBD_CtlSendData(pdev, (uint8_t *)&hcomp->hidProtocol, 1U);
          break;

        case HID_REQ_SET_IDLE:
          hcomp->hidIdleState = (uint32_t)(req->wValue >> 8U);
          break;

        case HID_REQ_GET_IDLE:
          (void)USBD_CtlSendData(pdev, (uint8_t *)&hcomp->hidIdleState, 1U);
          break;

        default:
          USBD_CtlError(pdev, req);
          return (uint8_t)USBD_FAIL;
      }
      break;

    case USB_REQ_TYPE_STANDARD:
      switch (req->bRequest)
      {
        case USB_REQ_GET_DESCRIPTOR:
          if ((req->wValue >> 8U) == HID_REPORT_DESC_TYPE)
          {
            pbuf = sHidReportDesc;
            len  = (uint16_t)MIN(HID_KEYBOARD_REPORT_DESC_SIZE, req->wLength);
          }
          else if ((req->wValue >> 8U) == HID_DESCRIPTOR_TYPE)
          {
            /* HID descriptor sits at byte offset 18 in the config descriptor */
            pbuf = &sCompositeCfgDesc[18U];
            len  = (uint16_t)MIN(9U, req->wLength);
          }
          else
          {
            USBD_CtlError(pdev, req);
            return (uint8_t)USBD_FAIL;
          }

          (void)USBD_CtlSendData(pdev, (uint8_t *)pbuf, len);
          break;

        case USB_REQ_GET_INTERFACE:
          (void)USBD_CtlSendData(pdev, (uint8_t *)&hcomp->hidProtocol, 1U);
          break;

        case USB_REQ_SET_INTERFACE:
          break;

        default:
          USBD_CtlError(pdev, req);
          return (uint8_t)USBD_FAIL;
      }
      break;

    default:
      USBD_CtlError(pdev, req);
      return (uint8_t)USBD_FAIL;
  }

  return (uint8_t)USBD_OK;
}

static uint8_t Composite_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  USBD_Composite_HandleTypeDef *hcomp = (USBD_Composite_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId];

  if (hcomp == NULL)
  {
    return (uint8_t)USBD_FAIL;
  }

  if (epnum == (COMP_HID_EPIN_ADDR & 0x0FU))
  {
    hcomp->hidTxBusy = false;
  }

  return (uint8_t)USBD_OK;
}

/*
 * SOF fires every 1 ms from the host. Used here to clear hidTxBusy when the
 * HID IN endpoint has become idle after a USB reset or re-enumeration, so the
 * transport layer does not get permanently stuck in BUSY state.
 */
static uint8_t Composite_SOF(USBD_HandleTypeDef *pdev)
{
  USBD_Composite_HandleTypeDef *hcomp = (USBD_Composite_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId];

  if (hcomp == NULL)
  {
    return (uint8_t)USBD_OK;
  }

  if ((hcomp->hidTxBusy) && (pdev->ep_in[COMP_HID_EPIN_ADDR & 0x0FU].is_used == 0U))
  {
    hcomp->hidTxBusy = false;
  }

  return (uint8_t)USBD_OK;
}

static uint8_t *Composite_GetCfgDesc(uint16_t *length)
{
  *length = (uint16_t)sizeof(sCompositeCfgDesc);
  return sCompositeCfgDesc;
}

static uint8_t *Composite_GetDeviceQualifierDesc(uint16_t *length)
{
  *length = (uint16_t)sizeof(sDeviceQualifierDesc);
  return sDeviceQualifierDesc;
}

/* Exported functions --------------------------------------------------------*/

uint8_t USBD_COMPOSITE_HID_SendReport(USBD_HandleTypeDef *pdev, uint8_t *report, uint16_t len)
{
  USBD_Composite_HandleTypeDef *hcomp = (USBD_Composite_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId];

  if (hcomp == NULL)
  {
    return (uint8_t)USBD_FAIL;
  }

  if (pdev->dev_state != USBD_STATE_CONFIGURED)
  {
    return (uint8_t)USBD_FAIL;
  }

  if (hcomp->hidTxBusy)
  {
    return (uint8_t)USBD_BUSY;
  }

  hcomp->hidTxBusy = true;

  uint8_t ret = USBD_LL_Transmit(pdev, COMP_HID_EPIN_ADDR, report, len);
  if (ret != (uint8_t)USBD_OK)
  {
    hcomp->hidTxBusy = false;
    return ret;
  }

  return (uint8_t)USBD_OK;
}
