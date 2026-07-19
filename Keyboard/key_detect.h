/*
 * key_detect.h
 *
 *  Created on: Jul 2, 2026
 *      Author: haodu
 */

#ifndef INC_KEY_DETECT_H_
#define INC_KEY_DETECT_H_

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Function prototypes -------------------------------------------------------*/

void      KeyDetect_Init(void);
void      KeyDetect_Run(void);
uint16_t  KeyDetect_GetKeyStatus(void);

#endif /* INC_KEY_DETECT_H_ */
