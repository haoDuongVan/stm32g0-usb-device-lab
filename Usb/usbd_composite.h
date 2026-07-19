/*
 * usbd_composite.h
 *
 *  Created on: Jul 14, 2026
 *      Author: haodu
 *
 * Custom USB class, replacing the CubeMX-generated usbd_hid.c entirely.
 * This step (lab-09a, part 1) contains only HID Keyboard (Interface 0).
 * CDC ACM (Interfaces 1-2) is added in the next step once this skeleton
 * is confirmed working end-to-end.
 *
 * Endpoint map (this step):
 *   EP1 IN  0x81  HID Keyboard Interrupt IN   8 bytes  10 ms
 */

#ifndef INC_USBD_COMPOSITE_H_
#define INC_USBD_COMPOSITE_H_

/* Includes ------------------------------------------------------------------*/
#include "usbd_ioreq.h"
#include <stdbool.h>

/* Exported defines ----------------------------------------------------------*/

/* HID sub-class */
#define COMP_HID_EPIN_ADDR          0x81U
#define COMP_HID_EPIN_SIZE          0x08U
#define COMP_HID_FS_BINTERVAL       0x0AU   /* 10 ms */

/* Exported types ------------------------------------------------------------*/

typedef struct
{
  uint32_t      hidProtocol;
  uint32_t      hidIdleState;
  volatile bool hidTxBusy;
} USBD_Composite_HandleTypeDef;

/* Exported variables --------------------------------------------------------*/
extern USBD_ClassTypeDef USBD_COMPOSITE;

/* Exported functions --------------------------------------------------------*/

/*
 * Send one HID keyboard report (8 bytes).
 * Returns USBD_OK if the transfer was accepted, USBD_BUSY if a previous
 * transfer is still in progress.
 */
uint8_t USBD_COMPOSITE_HID_SendReport(USBD_HandleTypeDef *pdev, uint8_t *report, uint16_t len);

#endif /* INC_USBD_COMPOSITE_H_ */
