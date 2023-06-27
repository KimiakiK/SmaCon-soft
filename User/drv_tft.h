/*
 * drv_tft.h
 *
 *  Created on: Jun 19, 2023
 *      Author: KimiakiK
 */


#ifndef DRV_TFT_H_
#define DRV_TFT_H_

/********** Include **********/

#include "typedef.h"

/********** Define **********/

#define TFT_WIDTH   (240)
#define TFT_HEIGHT  (320)

/********** Enum **********/

/********** Type **********/

/********** Constant **********/

/********** Variable **********/

/********** Function Prototype **********/

void InitTft(void);
void StartTft(void);
void StopTft(void);
void UpdateTft(void);

#endif /* DRV_TFT_H_ */
