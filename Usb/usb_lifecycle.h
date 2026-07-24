/*
 * usb_lifecycle.h
 *
 *  Created on: Jul 14, 2026
 *      Author: haodu
 *
 * USB bus reset/suspend/resume handling - keeps internal buffers/flags
 * (vendor RAM dump state, hidTxBusy/cdcTxBusy/vendorTxBusy) from wedging
 * across a bus event, and reports each event over CDC log for evidence.
 *
 * UsbLifecycle_OnReset is called from Composite_Init (usbd_composite.c),
 * since that runs on every fresh SetConfiguration including the one that
 * follows a bus reset. UsbLifecycle_OnSuspend/OnResume are called from
 * HAL_PCD_SuspendCallback/HAL_PCD_ResumeCallback (usbd_conf.c).
 */

#ifndef INC_USB_LIFECYCLE_H_
#define INC_USB_LIFECYCLE_H_

/* Includes ------------------------------------------------------------------*/
#include "usbd_ioreq.h"

/* Function prototypes -------------------------------------------------------*/

void  UsbLifecycle_OnReset(void);
void  UsbLifecycle_OnSuspend(USBD_HandleTypeDef *pdev);
void  UsbLifecycle_OnResume(USBD_HandleTypeDef *pdev);
void  UsbLifecycle_FlushPendingLog(void);

#endif /* INC_USB_LIFECYCLE_H_ */
