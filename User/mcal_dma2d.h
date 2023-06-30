/*
 * mcal_dma2d.h
 *
 *  Created on: Jun 27, 2023
 *      Author: KimiakiK
 */


#ifndef MCAL_DMA2D_H_
#define MCAL_DMA2D_H_

/********** Include **********/

#include "typedef.h"

/********** Define **********/

/********** Enum **********/

/********** Type **********/

/********** Constant **********/

/********** Variable **********/

/********** Function Prototype **********/

void InitDma2d(void);
void SetRegisterToMemoryTransferJob(uint32_t buffer_address, uint32_t width, uint32_t height, uint32_t output_offset, uint32_t color_RGB888);
void SetDma2dCallbackJob(callback_t callback);
void InterruptDma2dTransferComplete(void);

#endif /* MCAL_DMA2D_H_ */
