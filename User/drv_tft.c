/*
 * drv_tft.c
 *
 *  Created on: Jun 19, 2023
 *      Author: KimiakiK
 */


/********** Include **********/

#include "typedef.h"
#include "mcal_dio.h"
#include "mcal_spi.h"
#include "mcal_timer.h"
#include "drv_tft.h"

/********** Define **********/

#define SPI_TFT			(SPI_CH1)

#define PIN_CS_OFF		(PIN_LEVEL_HIGH)
#define PIN_CS_ON		(PIN_LEVEL_LOW)
#define PIN_DC_DATA		(PIN_LEVEL_HIGH)
#define PIN_DC_COMMAND	(PIN_LEVEL_LOW)

/* フレームバッファサイズ [byte] */
#define BUFFER_SIZE		(TFT_WIDTH * TFT_HEIGHT * COLOR_SIZE)
/* フレームバッファの数 */
#define BUFFER_NUM		(2)
/* 送信ジョブキューサイズ */
#define SEND_JOB_QUEUE_SIZE		(20)

/********** Enum **********/

typedef enum {
	SEND_MODE_DATA = 0,
	SEND_MODE_COMMAND
} send_mode_t;

typedef enum {
	SEND_STATE_IDLE = 0,
	SEND_STATE_BUSY
} send_state_t;

/********** Type **********/

typedef struct {
	uint8_t* data_address;
	uint32_t length;
	send_mode_t send_mode;
} send_job_t;

/********** Constant **********/

// const static uint8_t command_SWRESET[] = {0x01};		/* SWRESET (01h): Software Reset */
const static uint8_t command_SLPOUT[] = {0x11};			/* SLPOUT (11h): Sleep Out */
const static uint8_t command_COLMOD[] = {0x3A};			/* COLMOD (3Ah): Interface Pixel Format */
const static uint8_t data_COLMOD[] = {0x55};			/* 65K of RGB interface, 16bit/pixel */
const static uint8_t command_MADCTL[] = {0x36};			/* MADCTL (36h): Memory Data Access Control */
const static uint8_t data_MADCTL[] = {0x00};			/* Page Address Order: Top to Bottom, Column Address Order: Left to Right */
// const static uint8_t command_INVOFF[] = {0x20};		/* INVOFF (20h): Display Inversion Off (0x0000:WHITE, 0xFFFF:BLACK */
const static uint8_t command_INVON[] = {0x21};			/* INVON (21h): Display Inversion On (0x0000:BLACK, 0xFFFF:WHITE) */
const static uint8_t command_NORON[] = {0x13};			/* NORON (13h): Normal Display Mode On */
const static uint8_t command_DISPOFF[] = {0x28};		/* DISPOFF (28h): Display Off */
const static uint8_t command_DISPON[] = {0x29};			/* DISPON (29h): Display On */
const static uint8_t command_CASET[] = {0x2A};			/* CASET (2Ah): Column Address Set */
const static uint8_t data_CASET[] = {0x00, 0x00, (TFT_WIDTH & 0xFF00) >> 8, (TFT_WIDTH & 0xFF)};	/* XS [15:0], XE [15:0] */
const static uint8_t command_RASET[] = {0x2B};			/* RASET (2Bh): Row Address Set */
const static uint8_t data_RASET[] = {0x00, 0x00, (TFT_HEIGHT & 0xFF00) >> 8, (TFT_HEIGHT & 0xFF)};	/* YS [15:0], YE [15:0] */
const static uint8_t command_RAMWR[] = {0x2C};			/* RAMWR (2Ch): Memory Write */

/********** Variable **********/

static uint8_t frame_buffer[BUFFER_NUM][BUFFER_SIZE];
static uint8_t frame_buffer_index_display;
static uint8_t frame_buffer_index_draw;

static bool_t frame_buffer_swap_request;

static send_state_t sync_send_state;
static send_state_t async_send_state;

static send_job_t send_job_queue[SEND_JOB_QUEUE_SIZE];
static uint32_t send_job_queue_index_top;
static uint32_t send_job_queue_index_end;

/********** Function Prototype **********/

void changePinDC(send_mode_t send_mode);
void sendSync(uint8_t* data_address, uint32_t length, send_mode_t send_mode);
void callbackSyncSendComplete(void);
void sendAsync(uint8_t* data_address, uint32_t length, send_mode_t send_mode);
void sendJob(void);
void callbackAsyncSendComplete(void);

/********** Function **********/

/*
 * Function: DRV TFT 初期化
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
void InitTft(void)
{
	/* 変数初期化 */
	for (uint32_t index=0; index<BUFFER_SIZE; index++) {
		frame_buffer[0][index] = 0x00;
		frame_buffer[1][index] = 0xFF;
	}
	frame_buffer_index_display = 0;
	frame_buffer_index_draw = 1;
	frame_buffer_swap_request = FALSE;
	sync_send_state = SEND_STATE_IDLE;
	async_send_state = SEND_STATE_IDLE;
	send_job_queue_index_top = 0;
	send_job_queue_index_end = 0;

	/* ハードウェアリセット(RST端子)後120ms待機 */
	WaitUs(120000);

	/* チップセレクト有効化 */
	WritePin(PIN_ID_TFT_CS, PIN_CS_ON);

	/* スリープ解除後5ms待機 */
	sendSync((uint8_t*)command_SLPOUT, sizeof(command_SLPOUT), SEND_MODE_COMMAND);
	WaitUs(5000);

	/* 初期設定 */
	sendSync((uint8_t*)command_COLMOD, sizeof(command_COLMOD), SEND_MODE_COMMAND);
	sendSync((uint8_t*)data_COLMOD, sizeof(data_COLMOD), SEND_MODE_DATA);
	sendSync((uint8_t*)command_MADCTL, sizeof(command_MADCTL), SEND_MODE_COMMAND);
	sendSync((uint8_t*)data_MADCTL, sizeof(data_MADCTL), SEND_MODE_DATA);
	sendSync((uint8_t*)command_INVON, sizeof(command_INVON), SEND_MODE_COMMAND);
	sendSync((uint8_t*)command_NORON, sizeof(command_NORON), SEND_MODE_COMMAND);
}

/*
 * Function: TFT表示開始
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
void StartTft(void)
{
	sendAsync((uint8_t*)command_DISPON, sizeof(command_DISPON), SEND_MODE_COMMAND);
}

/*
 * Function: TFT表示停止
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
void StopTft(void)
{
	sendAsync((uint8_t*)command_DISPOFF, sizeof(command_DISPOFF), SEND_MODE_COMMAND);
}

/*
 * Function: TFT表示更新
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
void UpdateTft(void)
{
	if (frame_buffer_swap_request == TRUE) {
		frame_buffer_swap_request = FALSE;
		
		/* フレームバッファ入れ替え */
		uint8_t tmp_index = frame_buffer_index_draw;
		frame_buffer_index_draw = frame_buffer_index_display;
		frame_buffer_index_display = tmp_index;

		/* 表示領域設定(全画面で指示) */
		sendAsync((uint8_t*)command_CASET, sizeof(command_CASET), SEND_MODE_COMMAND);
		sendAsync((uint8_t*)data_CASET, sizeof(data_CASET), SEND_MODE_DATA);
		sendAsync((uint8_t*)command_RASET, sizeof(command_RASET), SEND_MODE_COMMAND);
		sendAsync((uint8_t*)data_RASET, sizeof(data_RASET), SEND_MODE_DATA);

		/* メモリ書き込み指示 */
		sendAsync((uint8_t*)command_RAMWR, sizeof(command_RAMWR), SEND_MODE_COMMAND);

		/* 表示データ送信 */
		sendAsync((uint8_t*)frame_buffer[frame_buffer_index_display], BUFFER_SIZE, SEND_MODE_DATA);
	}
}

/*
 * Function: フレームバッファ取得
 * Argument: なし
 * Return  : フレームバッファ先頭アドレス
 * Note    : なし
 */
uint8_t* GetFrameBuffer(void)
{
	return frame_buffer[frame_buffer_index_draw];
}

void SetSwapRequest(bool_t request)
{
	frame_buffer_swap_request = request;
}

/*
 * Function: DC端子出力変更
 * Argument: 送信モード
 * Return  : なし
 * Note    : なし
 */
void changePinDC(send_mode_t send_mode)
{
	if (send_mode == SEND_MODE_DATA) {
		WritePin(PIN_ID_TFT_DC, PIN_DC_DATA);
	} else {
		WritePin(PIN_ID_TFT_DC, PIN_DC_COMMAND);
	}
}

/*
 * Function: 同期送信
 * Argument: 送信データ先頭アドレス、送信データ長、送信モード
 * Return  : なし
 * Note    : なし
 */
void sendSync(uint8_t* data_address, uint32_t length, send_mode_t send_mode)
{
	changePinDC(send_mode);

	sync_send_state = SEND_STATE_BUSY;

	SendSpi(SPI_TFT, data_address, length, callbackSyncSendComplete);

	while (sync_send_state == SEND_STATE_BUSY) {
		/* 処理なし(送信完了待ち) */
	}
}

/*
 * Function: 同期送信完了時コールバック
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
void callbackSyncSendComplete(void)
{
	sync_send_state = SEND_STATE_IDLE;
}

/*
 * Function: 非同期送信
 * Argument: 送信データ先頭アドレス、送信データ長、送信モード
 * Return  : なし
 * Note    : なし
 */
void sendAsync(uint8_t* data_address, uint32_t length, send_mode_t send_mode)
{
	/* 送信ジョブに追加 */
	send_job_queue[send_job_queue_index_top].data_address = data_address;
	send_job_queue[send_job_queue_index_top].length = length;
	send_job_queue[send_job_queue_index_top].send_mode = send_mode;
	
	if (send_job_queue_index_top < SEND_JOB_QUEUE_SIZE - 1) {
		send_job_queue_index_top ++;
	} else {
		send_job_queue_index_top = 0;
	}

	if (async_send_state == SEND_STATE_IDLE) {
		async_send_state = SEND_STATE_BUSY;
		sendJob();
	}
}

/*
 * Function: ジョブ送信
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
void sendJob(void)
{
	uint32_t send_job_queue_index;

	if (send_job_queue_index_top != send_job_queue_index_end) {
		/* 次のジョブを送信 */
		send_job_queue_index = send_job_queue_index_end;
		
		if (send_job_queue_index_end < SEND_JOB_QUEUE_SIZE - 1) {
			send_job_queue_index_end ++;
		} else {
			send_job_queue_index_end = 0;
		}

		changePinDC(send_job_queue[send_job_queue_index].send_mode);

		SendSpi(SPI_TFT, send_job_queue[send_job_queue_index].data_address, send_job_queue[send_job_queue_index].length, callbackAsyncSendComplete);
	} else {
		/* すべてのジョブを送信済み */
		async_send_state = SEND_STATE_IDLE;
	}
}

/*
 * Function: 非同期送信完了時コールバック
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
void callbackAsyncSendComplete(void)
{
	sendJob();
}
