/*
 * drv_sound.c
 *
 *  Created on: Jul 9, 2023
 *      Author: KimiakiK
 */


/********** Include **********/

#include "typedef.h"
#include "mcal_dio.h"
#include "mcal_spi.h"
#include "mcal_timer.h"
#include "drv_sound.h"

/********** Define **********/

/* 送信データバッファサイズ */
#define SEND_BUFFER_SIZE		(256)
/* 非同期送信ジョブキューサイズ */
#define SEND_JOB_QUEUE_SIZE		(20)

#define SPI_YMF825				(SPI_CH2)

#define PIN_CS_OFF				(PIN_LEVEL_HIGH)
#define PIN_CS_ON				(PIN_LEVEL_LOW)

#define YMF825_REG_CLKE			(0x00)		/* #0  Clock Enable */
#define YMF825_REG_ALRST		(0x01)		/* #1  Reset */
#define YMF825_REG_AP			(0x02)		/* #2  Analog Block Power-down control, AP0: VREF, IREF, AP1: SPAMP, SPOUT1, AP2: SPAMP, SPOUT2, AP3: DAC */
#define YMF825_REG_GAIN			(0x03)		/* #3  Speaker Amplifier Gain Setting */
#define YMF825_REG_HW_ID		(0x04)		/* #4  Hardware ID */
#define YMF825_REG_CONTENTS		(0x07)		/* #7  Contents Data Write Port */
#define YMF825_REG_SEQUENCER	(0x08)		/* #8  Sequencer Setting */
#define YMF825_REG_SEQ_VOL		(0x09)		/* #9  Sequencer volume */
#define YMF825_REG_SEQ_SIZE		(0x0A)		/* #10 Size of sequence data in bytes */
#define YMF825_REG_CRGD_VNO		(0x0B)		/* #11 Voice number */
#define YMF825_REG_VOVOL		(0x0C)		/* #12 Volume each voice number */
#define YMF825_REG_BLOCK		(0x0D)		/* #13 Specifies an octave */
#define YMF825_REG_FNUM			(0x0E)		/* #14 Frequency information for one octave */
#define YMF825_REG_KEYON		(0x0F)		/* #15 KeyOn, Mute, EG_RST, ToneNum */
#define YMF825_REG_CHVOL		(0x10)		/* #16 Volume for each voice, interpolation of the SEQ_Vol and ChVol */
#define YMF825_REG_XVB			(0x11)		/* #17 Vibrato modulation */
#define YMF825_REG_INT			(0x12)		/* #18 Integer part */
#define YMF825_REG_FRA			(0x13)		/* #19 Fraction part */
#define YMF825_REG_DIR_MT		(0x14)		/* #20 Control the interpolation in a mute state */
#define YMF825_REG_MS_S_U		(0x17)		/* #23 Sequencer Time unit Setting MS_S13 - MS_S7 */
#define YMF825_REG_MS_S_L		(0x18)		/* #24 Sequencer Time unit Setting MS_S6 - MS_S0 */
#define YMF825_REG_MASTER_VOL	(0x19)		/* #25 Master Volume */
#define YMF825_REG_SFTRST		(0x1A)		/* #26 Soft Reset */
#define YMF825_REG_MUTE_ITIME	(0x1B)		/* #27 Sequencer Delay, Recovery Function Setting, Volume Interpolation Setting */
#define YMF825_REG_LFO_RST		(0x1C)		/* #28 LFO Reset */
#define YMF825_REG_DRV_SEL		(0x1D)		/* #29 Power Rail Selection */
#define YMF825_REG_W_CEQ0		(0x20)		/* #32 Band 0 coefficients */
#define YMF825_REG_W_CEQ1		(0x21)		/* #33 Band 1 coefficients */
#define YMF825_REG_W_CEQ2		(0x22)		/* #34 Band 2 coefficients */

/********** Enum **********/

typedef enum {
	SEND_MODE_SYNC = 0,
	SEND_MODE_ASYNC
} send_mode_t;

typedef enum {
	SEND_STATE_IDLE = 0,
	SEND_STATE_BUSY
} send_state_t;

/********** Type **********/

typedef struct {
	uint16_t buffer_index;
	uint16_t length;
} send_job_t;

/********** Constant **********/

static const uint8_t tone_data[] ={
	0x81,//header
	//T_ADR 0
	0x01,0x85,
	0x00,0x7F,0xF4,0xBB,0x00,0x10,0x40,
	0x00,0xAF,0xA0,0x0E,0x03,0x10,0x40,
	0x00,0x2F,0xF3,0x9B,0x00,0x20,0x41,
	0x00,0xAF,0xA0,0x0E,0x01,0x10,0x40,
	0x80,0x03,0x81,0x80,
};

/********** Variable **********/

static uint8_t send_buffer[SEND_BUFFER_SIZE];
static uint16_t send_buffer_index_top;
static send_state_t sync_send_state;
static send_state_t async_send_state;

static send_job_t send_job_queue[SEND_JOB_QUEUE_SIZE];
static uint32_t send_job_queue_index_top;
static uint32_t send_job_queue_index_end;

/********** Function Prototype **********/

static void sendSingleWrite(uint8_t command, uint8_t data, send_mode_t send_mode);
static void sendBurstWrite(uint8_t command, uint8_t* data_address, uint16_t length, send_mode_t send_mode);
static void addSendBufferIndex(uint16_t* buffer_index, uint16_t add_value);
static void sendSync(uint16_t buffer_index, uint16_t length);
static void callbackSyncSendComplete(void);
static void sendAsync(uint16_t buffer_index, uint16_t length);
static void sendJob(void);
static void callbackAsyncSendComplete(void);

/********** Function **********/

/*
 * Function: 
 * Argument: 
 * Return  : 
 * Note    : 
 */
void InitSound(void)
{
	send_buffer_index_top = 0;
	sync_send_state = SEND_STATE_IDLE;
	async_send_state = SEND_STATE_IDLE;
	send_job_queue_index_top = 0;
	send_job_queue_index_end = 0;

	sendSingleWrite(YMF825_REG_DRV_SEL, 0x01, SEND_MODE_SYNC);		/* YMF825複数電源設定(5V, 3.3V) */
	sendSingleWrite(YMF825_REG_AP, 0x0E, SEND_MODE_SYNC);			/* AP0(VREF, IREF)有効化 */
	WaitUs(1000);													/* 発振器安定待ち */
	sendSingleWrite(YMF825_REG_CLKE, 0x01, SEND_MODE_SYNC);			/* クロック有効化 */
	sendSingleWrite(YMF825_REG_ALRST, 0x00, SEND_MODE_SYNC);		/* 内部リセット解除 */
	sendSingleWrite(YMF825_REG_SFTRST, 0xA3, SEND_MODE_SYNC);		/* Synthesizer blockリセット */
	WaitUs(1000);													/* リセット待ち */
	sendSingleWrite(YMF825_REG_SFTRST, 0x00, SEND_MODE_SYNC);		/* Synthesizer blockリセット解除 */
	WaitUs(30000);													/* VREF安定、リセット完了待ち*/
	sendSingleWrite(YMF825_REG_AP, 0x04, SEND_MODE_SYNC);			/* AP1(SPAMP, SPOUT1)、AP3(DAC)有効化 */
	WaitUs(10);														/* ポップノイズ抑制 */
	sendSingleWrite(YMF825_REG_AP, 0x00, SEND_MODE_SYNC);			/* AP2(SPAMP, SPOUT2)有効化 */

	sendSingleWrite(YMF825_REG_GAIN, 0x01, SEND_MODE_SYNC);			/* Analog Gain */
	sendSingleWrite(YMF825_REG_MASTER_VOL, 0x60, SEND_MODE_SYNC);	/* Master volume level */
	sendSingleWrite(YMF825_REG_MUTE_ITIME, 0x3F, SEND_MODE_SYNC);	/* Interpolation(補間)有効化 */
	sendSingleWrite(YMF825_REG_DIR_MT, 0x00, SEND_MODE_SYNC);		/* Interpolation(補間)有効化 */

	sendSingleWrite(YMF825_REG_SEQUENCER, 0xF6, SEND_MODE_SYNC);	/* Sequencerリセット */
	WaitUs(6);														/* リセット待ち */
	sendSingleWrite(YMF825_REG_SEQUENCER, 0x00, SEND_MODE_SYNC);	/* Sequencerリセット解除 */
	sendSingleWrite(YMF825_REG_SEQ_VOL, 0xF9, SEND_MODE_SYNC);		/* sequencer volume、sequence data size */
	sendSingleWrite(YMF825_REG_SEQ_SIZE, 0x00, SEND_MODE_SYNC);		/* sequence data size */

	sendSingleWrite(YMF825_REG_MS_S_U, 0x40, SEND_MODE_SYNC);		/* Sequencer Time unit Setting */
	sendSingleWrite(YMF825_REG_MS_S_L, 0x00, SEND_MODE_SYNC);		/* Sequencer Time unit Setting */

	/* トーンデータ設定 */
	sendBurstWrite(YMF825_REG_CONTENTS, (uint8_t*)tone_data, sizeof(tone_data), SEND_MODE_SYNC);

	sendSingleWrite(YMF825_REG_KEYON, 0x30, SEND_MODE_SYNC);		/* KeyOff, Mute */
	sendSingleWrite(YMF825_REG_CHVOL, 0x71, SEND_MODE_SYNC);		/* Volume for each voice */
	sendSingleWrite(YMF825_REG_XVB, 0x00, SEND_MODE_SYNC);			/* Vibrato modulation */
	sendSingleWrite(YMF825_REG_INT, 0x08, SEND_MODE_SYNC);			/* Integer part */
	sendSingleWrite(YMF825_REG_FRA, 0x00, SEND_MODE_SYNC);			/* Fraction part */
}

/*
 * Function: サウンド生成開始
 * Argument: BLOCK: Specifies an octave、FNUM: Sets the frequency information for one octave.
 * Return  : なし
 * Note    : なし
 */
void KeyOn(uint8_t block, uint16_t fnum)
{
	sendSingleWrite(YMF825_REG_CRGD_VNO, 0x00, SEND_MODE_ASYNC);	/* Voice number */
	sendSingleWrite(YMF825_REG_VOVOL, 0x54, SEND_MODE_ASYNC);		/* Volume each voice number */
	sendSingleWrite(YMF825_REG_BLOCK, ((fnum & 0x0380) >> 4) + (block & 0x07), SEND_MODE_ASYNC);	/* Specifies an octave */
	sendSingleWrite(YMF825_REG_FNUM, (fnum & 0x3F), SEND_MODE_ASYNC);	/* Frequency information for one octave */
	sendSingleWrite(YMF825_REG_KEYON, 0x40, SEND_MODE_ASYNC);		/* KeyOn */
}

/*
 * Function: サウンド生成停止
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
void KeyOff(void)
{
	sendSingleWrite(YMF825_REG_KEYON, 0x00, SEND_MODE_ASYNC);		/* KeyOff */
}

/*
 * Function: サウンド出力先デバイス変更
 * Argument: サウンド出力先デバイス
 * Return  : なし
 * Note    : なし
 */
void ChangeSoundOutputDevice(sound_output_device_t output_device)
{
	if (output_device == SOUND_OUTPUT_SPEAKER) {
		sendSingleWrite(YMF825_REG_AP, 0x00, SEND_MODE_SYNC);			/* AP2(SPAMP, SPOUT2)有効化 */
	} else if (output_device == SOUND_OUTPUT_LINE) {
		sendSingleWrite(YMF825_REG_AP, 0x04, SEND_MODE_SYNC);			/* AP2(SPAMP, SPOUT2)無効化 */
	} else {
		/* 処理なし */
	}
}

/*
 * Function: 単一データ送信
 * Argument: コマンド(YMF825 REG)、送信データ、送信モード(同期/非同期)
 * Return  : なし
 * Note    : なし
 */
static void sendSingleWrite(uint8_t command, uint8_t data, send_mode_t send_mode)
{
	uint16_t send_buffer_index;

	/* 送信データをバッファに格納 */
	if ((send_buffer_index_top + 2) >= SEND_BUFFER_SIZE) {
		send_buffer_index_top = 0;
	}
	send_buffer_index = send_buffer_index_top;
	send_buffer[send_buffer_index_top] = command;
	send_buffer[send_buffer_index_top + 1] = data;
	addSendBufferIndex(&send_buffer_index_top, 2);

	if (send_mode == SEND_MODE_SYNC) {
		sendSync(send_buffer_index, 2);
	} else {
		sendAsync(send_buffer_index, 2);
	}
}

/*
 * Function: 複数データ送信
 * Argument: コマンド(YMF825 REG)、送信データ格納アドレス、送信データ長、送信モード(同期/非同期)
 * Return  : なし
 * Note    : なし
 */
static void sendBurstWrite(uint8_t command, uint8_t* data_address, uint16_t length, send_mode_t send_mode)
{
	uint16_t send_buffer_index;

	/* 送信データをバッファに格納 */
	if ((send_buffer_index_top + 1 + length) >= SEND_BUFFER_SIZE) {
		send_buffer_index_top = 0;
	}
	send_buffer_index = send_buffer_index_top;
	send_buffer[send_buffer_index_top] = command;
	for (uint16_t index=0; index<length; index++) {
		send_buffer[send_buffer_index_top + 1 + index] = data_address[index];
	}
	addSendBufferIndex(&send_buffer_index_top, 1 + length);

	if (send_mode == SEND_MODE_SYNC) {
		sendSync(send_buffer_index, length);
	} else {
		sendAsync(send_buffer_index, length);
	}
}

/*
 * Function: 送信バッファインデックス加算
 * Argument: 送信バッファインデックスアドレス、加算値
 * Return  : なし
 * Note    : なし
 */
static void addSendBufferIndex(uint16_t* buffer_index, uint16_t add_value)
{
	if ((*buffer_index + add_value) < SEND_BUFFER_SIZE) {
		*buffer_index += add_value;
	} else {
		*buffer_index = (*buffer_index + add_value) - SEND_BUFFER_SIZE;
	}
}

/*
 * Function: 同期送信
 * Argument: 送信バッファインデックス、送信データ長
 * Return  : なし
 * Note    : なし
 */
static void sendSync(uint16_t buffer_index, uint16_t length)
{
	sync_send_state = SEND_STATE_BUSY;

	WritePin(PIN_ID_SOUND_CS, PIN_CS_ON);
	SendSpi(SPI_YMF825, &send_buffer[buffer_index], length, callbackSyncSendComplete);

	while (sync_send_state == SEND_STATE_BUSY) {
		/* 処理なし(送信完了待ち) */
	}

	WritePin(PIN_ID_SOUND_CS, PIN_CS_OFF);
}

/*
 * Function: 同期送信完了時コールバック
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
static void callbackSyncSendComplete(void)
{
	sync_send_state = SEND_STATE_IDLE;
}

/*
 * Function: 非同期送信
 * Argument: 送信バッファインデックス、送信データ長
 * Return  : なし
 * Note    : なし
 */
static void sendAsync(uint16_t buffer_index, uint16_t length)
{
	send_job_queue[send_job_queue_index_top].buffer_index = buffer_index;
	send_job_queue[send_job_queue_index_top].length = length;

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
static void sendJob(void)
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

		WritePin(PIN_ID_SOUND_CS, PIN_CS_ON);

		SendSpi(SPI_YMF825, &send_buffer[send_job_queue[send_job_queue_index].buffer_index], send_job_queue[send_job_queue_index].length, callbackAsyncSendComplete);
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
static void callbackAsyncSendComplete(void)
{
	WritePin(PIN_ID_SOUND_CS, PIN_CS_OFF);
	sendJob();
}
