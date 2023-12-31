/*
 * drv_draw.h
 *
 *  Created on: 2023/06/29
 *      Author: KimiakiK
 */


#ifndef DRV_DRAW_H_
#define DRV_DRAW_H_

/********** Include **********/

#include "typedef.h"

/********** Define **********/

/********** Enum **********/

/********** Type **********/

/********** Constant **********/

/********** Variable **********/

/********** Function Prototype **********/

void InitDraw(void);
void StartDraw(uint8_t* frame_buffer);
void EndDraw(void);
void FillRect(float x, float y, uint32_t w, uint32_t h, uint32_t color_ARGB8888);

#endif /* DRV_DRAW_H_ */
