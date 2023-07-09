/*
 * mcal_dio.h
 *
 *  Created on: 2023/06/19
 *      Author: KimiakiK
 */


#ifndef MCAL_DIO_H_
#define MCAL_DIO_H_

/********** Include **********/

#include "typedef.h"

/********** Define **********/

/********** Enum **********/

typedef enum {
	PIN_ID_TFT_CS = 0,
	PIN_ID_TFT_DC,
	PIN_ID_SD_CS,
	PIN_ID_SW_A,
	PIN_ID_SW_B,
	PIN_ID_SW_C,
	PIN_ID_SW_D,
	PIN_ID_NUM
} pin_id_t;

typedef enum {
	PIN_LEVEL_LOW = 0,
	PIN_LEVEL_HIGH
} pin_level_t;

/********** Type **********/

/********** Constant **********/

/********** Variable **********/

/********** Function Prototype **********/

void InitDio(void);
pin_level_t ReadPin(pin_id_t pin_id);
void WritePin(pin_id_t pin_id, pin_level_t pin_level);

#endif /* MCAL_DIO_H_ */
