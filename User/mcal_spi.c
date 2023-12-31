/*
 * mcal_spi.c
 *
 *  Created on: May 10, 2023
 *      Author: KimiakiK
 */


/********** Include **********/

#include "typedef.h"
#include "mcal_spi.h"

/********** Define **********/

/* 一回で送信できる最大長 [byte] */
#define SEND_LENGHT_MAX		(0xFFFF)

/********** Enum **********/

typedef enum {
	SPI_STATE_IDLE = 0,
	SPI_STATE_BUSY
} spi_state_t;

typedef enum {
	SPI_MODE_TX = 0,
	SPI_MODE_RX,
	SPI_MODE_TXRX
} spi_mode_t;

/********** Type **********/

/********** Constant **********/

extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi2;
extern SPI_HandleTypeDef hspi3;

static SPI_HandleTypeDef* const hspi[SPI_CH_NUM] =
{
	&hspi1,		/* SPI_CH1 */
	&hspi2,		/* SPI_CH2 */
	&hspi3		/* SPI_CH3 */
};

/********** Variable **********/

static spi_state_t spi_state[SPI_CH_NUM];
static spi_mode_t spi_mode[SPI_CH_NUM];
static uint8_t* send_data[SPI_CH_NUM];
static uint32_t send_length[SPI_CH_NUM];
static callback_t send_callback[SPI_CH_NUM];


/********** Function Prototype **********/

static void sendSpiData(spi_ch_t spi_ch);

/********** Function **********/

/*
 * Function: MCAL SPI 初期化
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
void InitSpi(void)
{
	uint32_t channel;

	for (channel=0; channel<SPI_CH_NUM; channel++) {
		spi_state[channel] = SPI_STATE_IDLE;
		spi_mode[channel] = SPI_MODE_TX;
		send_data[channel] = NULL;
		send_length[channel] = 0;
		send_callback[channel] = NULL;
	}
}

/*
 * Function: SPIデータ送信
 * Argument: 送信チャネル、送信データ先頭アドレス、送信データ長、送信完了時コールバック
 * Return  : 送信開始成功/失敗
 * Note    : 非同期送信
 */
result_t SendSpi(spi_ch_t spi_ch, uint8_t* data, uint32_t length, callback_t callback)
{
	result_t result;

	if ((spi_ch < SPI_CH_NUM) && (spi_state[spi_ch] == SPI_STATE_IDLE)) {
		result = RESULT_OK;
		spi_mode[spi_ch] = SPI_MODE_TX;
		send_data[spi_ch] = data;
		send_length[spi_ch] = length;
		send_callback[spi_ch] = callback;
		sendSpiData(spi_ch);
	} else {
		result = RESULT_NG;
	}

	return result;
}

/*
 * Function: SPIデータ受信
 * Argument: 受信チャネル、受信バッファ先頭アドレス、受信データ長、受信完了時コールバック
 * Return  : 受信開始成功/失敗
 * Note    : 非同期受信
 */
result_t ReceiveSpi(spi_ch_t spi_ch, uint8_t* receive_buffer, uint16_t length, callback_t callback)
{
	result_t result;

	if ((spi_ch < SPI_CH_NUM) && (spi_state[spi_ch] == SPI_STATE_IDLE)) {
		result = RESULT_OK;
		spi_mode[spi_ch] = SPI_MODE_RX;
		send_length[spi_ch] = 0;
		send_callback[spi_ch] = callback;
		HAL_SPI_Receive_DMA((SPI_HandleTypeDef*)hspi[spi_ch], receive_buffer, length);
	} else {
		result = RESULT_NG;
	}

	return result;
}

/*
 * Function: SPIデータ送受信
 * Argument: 送受信チャネル、送信データ先頭アドレス、受信バッファ戦闘アドレス、送受信データ長、送受信完了時コールバック
 * Return  : 送受信開始成功/失敗
 * Note    : 非同期送受信
 */
result_t SendReceiveSpi(spi_ch_t spi_ch, uint8_t* send_data, uint8_t* receive_buffer, uint16_t length, callback_t callback)
{
	result_t result;

	if ((spi_ch < SPI_CH_NUM) && (spi_state[spi_ch] == SPI_STATE_IDLE)) {
		result = RESULT_OK;
		spi_mode[spi_ch] = SPI_MODE_TXRX;
		send_length[spi_ch] = 0;
		send_callback[spi_ch] = callback;
		HAL_SPI_TransmitReceive_DMA((SPI_HandleTypeDef*)hspi[spi_ch], send_data, receive_buffer, length);
	} else {
		result = RESULT_NG;
	}

	return result;
}

/*
 * Function: SPI DMA転送完了処理
 * Argument: 送信チャネル
 * Return  : なし
 * Note    : 割り込み処理
 */
void InterruptSpiComplete(spi_ch_t spi_ch)
{
	if (send_length[spi_ch] == 0) {
		spi_state[spi_ch] = SPI_STATE_IDLE;
		if (send_callback[spi_ch] != NULL) {
			send_callback[spi_ch]();
		}
	} else {
		switch(spi_mode[spi_ch]) {
		case SPI_MODE_TX:
			sendSpiData(spi_ch);
			break;
		case SPI_MODE_RX:
			/* 処理なし */
			break;
		case SPI_MODE_TXRX:
			/* 処理なし */
			break;
		default:
			/* 処理なし */
			break;
		}
	}
}

/*
 * Function: SPI送信実行
 * Argument: 送信チャネル
 * Return  : なし
 * Note    : なし
 */
static void sendSpiData(spi_ch_t spi_ch)
{
	uint8_t* data;
	uint32_t length;

	data = send_data[spi_ch];
	if (send_length[spi_ch] > SEND_LENGHT_MAX) {
		length = SEND_LENGHT_MAX;
	} else {
		length = send_length[spi_ch];
	}

	send_data[spi_ch] += length;
	send_length[spi_ch] -= length;

	HAL_SPI_Transmit_DMA((SPI_HandleTypeDef*)hspi[spi_ch], data, length);
}
