/*
 * mcal_adc.h
 *
 *  Created on: Jul 8, 2023
 *      Author: KimiakiK
 */


#ifndef MCAL_ADC_H_
#define MCAL_ADC_H_

/********** Include **********/

#include "typedef.h"

/********** Define **********/

/********** Enum **********/

typedef enum {
	AD_ID_POS_H = 0,
	AD_ID_POS_V,
	AD_ID_LEVER,
	AD_ID_NUM
} ad_id_t;

/********** Type **********/

/********** Constant **********/

/********** Variable **********/

/********** Function Prototype **********/

void InitAdc(void);
void MainAdc(void);
float GetAd(ad_id_t ad_id);
void InterruptAdcComplete(void);

#endif /* MCAL_ADC_H_ */
