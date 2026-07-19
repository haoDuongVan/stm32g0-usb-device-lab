/*
 * usb_cdc_log.h
 *
 *  Created on: Jul 14, 2026
 *      Author: haodu
 */

#ifndef INC_USB_CDC_LOG_H_
#define INC_USB_CDC_LOG_H_

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported defines ----------------------------------------------------------*/

/* Ring buffer capacity in bytes. Must be a power of two */
#define CDC_LOG_BUF_SIZE            512U

/* Function prototypes -------------------------------------------------------*/

void  CdcLog_Init(void);
void  CdcLog_Run(void);
void  CdcLog_Write(const char *buf, uint16_t len);
void  CdcLog_Printf(const char *fmt, ...);

#endif /* INC_USB_CDC_LOG_H_ */
