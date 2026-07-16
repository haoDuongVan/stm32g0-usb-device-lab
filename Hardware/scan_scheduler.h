/*
 * scan_scheduler.h
 *
 *  Created on: Jul 2, 2026
 *      Author: haodu
 */

#ifndef INC_SCAN_SCHEDULER_H_
#define INC_SCAN_SCHEDULER_H_

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Function prototypes -------------------------------------------------------*/

void      ScanScheduler_Init(void);
void      ScanScheduler_OnTimerTick(void);
uint8_t   ScanScheduler_TakeRequest(void);

#endif /* INC_SCAN_SCHEDULER_H_ */
