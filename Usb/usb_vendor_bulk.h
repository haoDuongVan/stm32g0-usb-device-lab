/*
 * usb_vendor_bulk.h
 *
 *  Created on: Jul 14, 2026
 *      Author: haodu
 *
 * RAM dump engine, streamed over the vendor bulk IN endpoint (Interface 3,
 * EP4 IN 0x84) added in usbd_composite.c/h. Dumps a fixed region (SRAM1,
 * 0x20000000, 144 KB) in 64-byte chunks.
 *
 * VendorDump_Start is armed by the EP0 VENDOR_REQ_START_RAM_DUMP handler
 * in usb_vendor_cmd.c. VendorDump_Abort is called on USB reset/resume
 * (Composite_Init, USBD_COMPOSITE_OnResume) so a dump left in flight across
 * a bus reset or suspend cannot wedge the state machine.
 */

#ifndef INC_USB_VENDOR_BULK_H_
#define INC_USB_VENDOR_BULK_H_

/* Includes ------------------------------------------------------------------*/
#include "usbd_ioreq.h"
#include <stdbool.h>

/* Function prototypes -------------------------------------------------------*/

bool  VendorDump_Start(uint32_t *acceptedLength);
bool  VendorDump_Abort(void);
void  VendorDump_OnTxCplt(USBD_HandleTypeDef *pdev);
void  VendorDump_Run(void);

#endif /* INC_USB_VENDOR_BULK_H_ */
