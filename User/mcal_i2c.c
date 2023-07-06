/*
 * mcal_i2c.c
 *
 *  Created on: Jun 30, 2023
 *      Author: KimiakiK
 */


/********** Include **********/

#include "typedef.h"
#include "mcal_i2c.h"

/********** Define **********/

/********** Enum **********/

/********** Type **********/

/********** Constant **********/

/********** Variable **********/

extern I2C_HandleTypeDef hi2c1;

static callback_t send_callback;
static callback_t receive_callback;

/********** Function Prototype **********/

/********** Function **********/

/*
 * Function: MCAL I2C 初期化
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
void InitI2c(void)
{
	send_callback = NULL;
	receive_callback = NULL;
}

/*
 * Function: I2Cデータ送信
 * Argument: デバイスアドレス(7bit)、送信データ先頭アドレス、送信データ長、送信完了時コールバック
 * Return  : なし
 * Note    : なし
 */
void SendI2c(uint16_t device_address, uint8_t* data, uint16_t length, callback_t callback)
{
	send_callback = callback;
	HAL_I2C_Master_Transmit_DMA(&hi2c1, device_address << 1, data, length);
}

/*
 * Function: I2Cデータ受信
 * Argument: デバイスアドレス(7bit)、受信データ格納アドレス、受信データ長、受信完了時コールバック
 * Return  : なし
 * Note    : なし
 */
void ReceiveI2c(uint16_t device_address, uint8_t* data, uint16_t length, callback_t callback)
{
	receive_callback = callback;
	HAL_I2C_Master_Receive_DMA(&hi2c1, device_address << 1, data, length);
}

/*
 * Function: I2C送信完了割り込み処理
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
void InterruptI2cSendComplete(void)
{
	if (send_callback != NULL) {
		send_callback();
	}
}

/*
 * Function: I2C受信完了割り込み処理
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
void InterruptI2cReceiveComplete(void)
{
	if (receive_callback != NULL) {
		receive_callback();
	}
}
