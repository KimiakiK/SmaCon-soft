/*
 * mcal_dio.c
 *
 *  Created on: 2023/06/19
 *      Author: KimiakiK
 */


/********** Include **********/

#include "typedef.h"
#include "mcal_dio.h"

/********** Define **********/

/********** Enum **********/

/********** Type **********/

typedef struct {
	GPIO_TypeDef* gpio;
	uint16_t pin;
} gpio_t;

/********** Constant **********/

const gpio_t gpio_table[PIN_ID_NUM] = {
	{TFT_CS_GPIO_Port, TFT_CS_Pin},		/* PIN_ID_TFT_CS */
	{TFT_DC_GPIO_Port, TFT_DC_Pin},		/* PIN_ID_TFT_DC */
	{SD_CS_GPIO_Port, SD_CS_Pin},		/* PIN_ID_SD_CS */
};

/********** Variable **********/

/********** Function Prototype **********/

/********** Function **********/

/*
 * Function: MCAL DIO 初期化
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
void InitDio(void)
{
	/* 処理なし */
}

/*
 * Function: 端子読み出し
 * Argument: 端子ID
 * Return  : 端子レベル
 * Note    : なし
 */
pin_level_t ReadPin(pin_id_t pin_id)
{
	pin_level_t pin_level = PIN_LEVEL_LOW;
	GPIO_PinState pin_state;

	if (pin_id < PIN_ID_NUM) {
		pin_state = HAL_GPIO_ReadPin(gpio_table[pin_id].gpio, gpio_table[pin_id].pin);
		if (pin_state == GPIO_PIN_SET) {
			pin_level = PIN_LEVEL_HIGH;
		}
	}

	return pin_level;
}

/*
 * Function: 端子書き込み
 * Argument: 端子ID、端子レベル
 * Return  : なし
 * Note    : なし
 */
void WritePin(pin_id_t pin_id, pin_level_t pin_level)
{
	GPIO_PinState pin_state;

	if (pin_id < PIN_ID_NUM) {
		if (pin_level == PIN_LEVEL_LOW) {
			pin_state = GPIO_PIN_RESET;
		} else {
			pin_state = GPIO_PIN_SET;
		}
		HAL_GPIO_WritePin(gpio_table[pin_id].gpio, gpio_table[pin_id].pin, pin_state);
	}
}
