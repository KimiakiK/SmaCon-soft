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

/* TFT横方向画素数 */
#define TFT_WIDTH		(240)
/* TFT縦方向画素数*/
#define TFT_HEIGHT		(320)
/* TFTの色数 [byte] */
#define COLOR_SIZE		(2)

/********** Enum **********/

/********** Type **********/

/********** Constant **********/

/********** Variable **********/

/********** Function Prototype **********/

void InitTft(void);
void StartTft(void);
void StopTft(void);
void UpdateTft(void);
uint8_t* GetFrameBuffer(void);
void SetSwapRequest(bool_t request);

#endif /* DRV_TFT_H_ */
