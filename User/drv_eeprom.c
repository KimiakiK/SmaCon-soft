/*
 * drv_eeprom.c
 *
 *  Created on: Jul 18, 2023
 *      Author: KimiakiK
 */


/********** Include **********/

#include "typedef.h"
#include "mcal_dio.h"
#include "mcal_spi.h"
#include "drv_eeprom.h"

/********** Define **********/

#define SPI_EEPROM			(SPI_CH3)

#define PIN_CS_OFF			(PIN_LEVEL_HIGH)
#define PIN_CS_ON			(PIN_LEVEL_LOW)

/* EEPROM(25LC080C)の容量 [byte] */
#define EEPROM_SIZE			(1024)

/* EEPROM(25LC080C)の命令 */
#define INSTRUCTION_READ	(0x03)	/* Read data from memory array beginning at selected address */
#define INSTRUCTION_WRITE	(0x02)	/* Write data to memory array beginning at selected address */
#define INSTRUCTION_WRDI	(0x04)	/* Reset the write enable latch (disable write operations) */
#define INSTRUCTION_WREN	(0x06)	/* Set the write enable latch (enable write operations) */
#define INSTRUCTION_RDSR	(0x05)	/* Read STATUS register */
#define INSTRUCTION_WRSR	(0x01)	/* Write STATUS register */

/* 通信バッファサイズ [byte] */
#define BUFFER_SIZE			(7)		/* 通常通信最大データ長 命令(1byte) + アドレス(2byte) + 書き込みデータ(4byte) */

/* EEPROM書き込みジョブキューサイズ */
#define EEPROM_WRITE_JOB_QUEUE_SIZE	(16)

/********** Enum **********/

typedef enum {
	EEPROM_STATE_IDLE = 0,
	EEPROM_STATE_WRITE_ENABLE,
	EEPROM_STATE_WRITE,
	EEPROM_STATE_READ_STATUS,
	EEPROM_STATE_WAIT_WRITING
} eeprom_state_t;

typedef enum {
	COM_STATE_IDLE = 0,
	COM_STATE_BUSY
} com_state_t;

/********** Type **********/

typedef struct {
	uint16_t address;
	uint8_t size;
} eeprom_write_job_t;

/********** Constant **********/

/********** Variable **********/

extern SPI_HandleTypeDef hspi3;

static uint8_t eeprom_buffer[EEPROM_SIZE];
static uint8_t send_buffer[BUFFER_SIZE];
static uint8_t receive_buffer[BUFFER_SIZE];

static eeprom_state_t eeprom_state;
static com_state_t com_state;

static eeprom_write_job_t eeprom_write_job_queue[EEPROM_WRITE_JOB_QUEUE_SIZE];
static uint32_t eeprom_write_job_queue_index_top;
static uint32_t eeprom_write_job_queue_index_end;

/********** Function Prototype **********/

static void waitComComplete(void);
static void callbackComComplete(void);
static void addWriteJob(uint16_t address, uint8_t size);
static void eepromWriteEnable(void);
static void callbackEepromWriteEnable(void);
static void eepromWrite(void);
static void callbackEepromWrite(void);
static void eepromReadStatus(void);
static void callbackEepromReadStatus(void);

/********** Function **********/

/*
 * Function: DRV EEPROM 初期化
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
void InitEeprom(void)
{
	/* 変数初期化 */
	eeprom_state = EEPROM_STATE_IDLE;
	com_state = COM_STATE_IDLE;
	eeprom_write_job_queue_index_top = 0;
	eeprom_write_job_queue_index_end = 0;

	/* 初回通信確立のためダミーのステータス読み出し */
	send_buffer[0] = INSTRUCTION_RDSR;
	WritePin(PIN_ID_EEPROM_CS, PIN_CS_ON);
	com_state = COM_STATE_BUSY;
	SendReceiveSpi(SPI_EEPROM, send_buffer, receive_buffer, 2, callbackComComplete);
	waitComComplete();
	WritePin(PIN_ID_EEPROM_CS, PIN_CS_OFF);

	/* EEPROMから全データを読み出す */
	send_buffer[0] = INSTRUCTION_READ;
	send_buffer[1] = 0x00;
	send_buffer[2] = 0x00;
	WritePin(PIN_ID_EEPROM_CS, PIN_CS_ON);
	com_state = COM_STATE_BUSY;
	SendSpi(SPI_EEPROM, send_buffer, 3, callbackComComplete);
	waitComComplete();
	/* 通信を継続するのでCSピンは落とさない */
	com_state = COM_STATE_BUSY;
	ReceiveSpi(SPI_EEPROM, eeprom_buffer, EEPROM_SIZE, callbackComComplete);
	waitComComplete();
	WritePin(PIN_ID_EEPROM_CS, PIN_CS_OFF);
}

/*
 * Function: DRV EEPROM 周期処理
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
void MainEeprom(void)
{
	/* 書き込み中のジョブが無く、新しい書き込みジョブがある場合は書き込み実行 */
	if ((eeprom_state == EEPROM_STATE_IDLE)
	 && (eeprom_write_job_queue_index_top != eeprom_write_job_queue_index_end)) {
		eepromWriteEnable();
	}

	/* 書き込み完了待ちの場合は再度ステータス読み出し実行 */
	if (eeprom_state == EEPROM_STATE_WAIT_WRITING) {
		eepromReadStatus();
	}
}

/*
 * Function: EEPROMデータ1byte読み出し
 * Argument: EEPROMデータID
 * Return  : 読み出しデータ
 * Note    : なし
 */
uint8_t ReadEeprom1byte(eeprom_data_id_t eeprom_data_id)
{
	return eeprom_buffer[eeprom_data_id];
}

/*
 * Function: EEPROMデータ2byte読み出し
 * Argument: EEPROMデータID
 * Return  : 読み出しデータ
 * Note    : なし
 */
uint16_t ReadEeprom2byte(eeprom_data_id_t eeprom_data_id)
{
	return (((uint16_t)eeprom_buffer[eeprom_data_id + 0] << 8) +
			((uint16_t)eeprom_buffer[eeprom_data_id + 1] << 0));
}

/*
 * Function: EEPROMデータ4byte読み出し
 * Argument: EEPROMデータID
 * Return  : 読み出しデータ
 * Note    : なし
 */
uint32_t ReadEeprom4byte(eeprom_data_id_t eeprom_data_id)
{
	return (((uint32_t)eeprom_buffer[eeprom_data_id + 0] << 24) +
			((uint32_t)eeprom_buffer[eeprom_data_id + 1] << 16) +
			((uint32_t)eeprom_buffer[eeprom_data_id + 2] <<  8) +
			((uint32_t)eeprom_buffer[eeprom_data_id + 3] <<  0));
}

/*
 * Function: EEPROMデータ1byte書き込み
 * Argument: EEPROMデータID、書き込みデータ
 * Return  : なし
 * Note    : なし
 */
void WriteEeprom1byte(eeprom_data_id_t eeprom_data_id, uint8_t write_data)
{
	eeprom_buffer[eeprom_data_id] = write_data;

	addWriteJob(eeprom_data_id, 1);
}

/*
 * Function: EEPROMデータ2byte書き込み
 * Argument: EEPROMデータID、書き込みデータ
 * Return  : なし
 * Note    : なし
 */
void WriteEeprom2byte(eeprom_data_id_t eeprom_data_id, uint16_t write_data)
{
	eeprom_buffer[eeprom_data_id + 0] = (write_data & 0xFF00) >> 8;
	eeprom_buffer[eeprom_data_id + 1] = (write_data & 0x00FF) >> 0;

	addWriteJob(eeprom_data_id, 2);
}

/*
 * Function: EEPROMデータ4byte書き込み
 * Argument: EEPROMデータID、書き込みデータ
 * Return  : なし
 * Note    : なし
 */
void WriteEeprom4byte(eeprom_data_id_t eeprom_data_id, uint32_t write_data)
{
	eeprom_buffer[eeprom_data_id + 0] = (write_data & 0xFF000000) >> 24;
	eeprom_buffer[eeprom_data_id + 1] = (write_data & 0x00FF0000) >> 16;
	eeprom_buffer[eeprom_data_id + 2] = (write_data & 0x0000FF00) >>  8;
	eeprom_buffer[eeprom_data_id + 3] = (write_data & 0x000000FF) >>  0;

	addWriteJob(eeprom_data_id, 4);
}

/*
 * Function: 通信完了待ち
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
static void waitComComplete(void)
{
	while(com_state == COM_STATE_BUSY) {
		/* 通信完了待ち */
	}
}

/*
 * Function: 通信完了コールバック
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
static void callbackComComplete(void)
{
	/* 通信完了 */
	com_state = COM_STATE_IDLE;
}

/*
 * Function: EEPROM書き込みジョブ追加
 * Argument: 書き込みアドレス、書き込みサイズ
 * Return  : なし
 * Note    : なし
 */
static void addWriteJob(uint16_t address, uint8_t size)
{
	eeprom_write_job_queue[eeprom_write_job_queue_index_top].address = address;
	eeprom_write_job_queue[eeprom_write_job_queue_index_top].size = size;

	if (eeprom_write_job_queue_index_top < EEPROM_WRITE_JOB_QUEUE_SIZE - 1) {
		eeprom_write_job_queue_index_top ++;
	} else {
		eeprom_write_job_queue_index_top = 0;
	}
}

/*
 * Function: EEPROM書き込み許可送信
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
static void eepromWriteEnable(void)
{
	eeprom_state = EEPROM_STATE_WRITE_ENABLE;

	/* 書き込み許可を送信 */
	send_buffer[0] = INSTRUCTION_WREN;
	WritePin(PIN_ID_EEPROM_CS, PIN_CS_ON);
	SendSpi(SPI_EEPROM, send_buffer, 1, callbackEepromWriteEnable);
}

/*
 * Function: EEPROM書き込み許可送信コールバック
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
static void callbackEepromWriteEnable(void)
{
	WritePin(PIN_ID_EEPROM_CS, PIN_CS_OFF);

	/* 書き込み許可が完了したので、書き込みを実行 */
	eepromWrite();
}

/*
 * Function: EEPROM書き込み送信
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
static void eepromWrite(void)
{
	eeprom_state = EEPROM_STATE_WRITE;

	/* 書き込みデータを送信 */
	send_buffer[0] = INSTRUCTION_WRITE;
	send_buffer[1] = (eeprom_write_job_queue[eeprom_write_job_queue_index_end].address & 0xFF00) >> 8;
	send_buffer[2] = (eeprom_write_job_queue[eeprom_write_job_queue_index_end].address & 0x00FF) >> 0;
	for (uint8_t index=0; index<eeprom_write_job_queue[eeprom_write_job_queue_index_end].size; index++) {
		send_buffer[3 + index] = eeprom_buffer[eeprom_write_job_queue[eeprom_write_job_queue_index_end].address + index];
	}
	WritePin(PIN_ID_EEPROM_CS, PIN_CS_ON);
	SendSpi(SPI_EEPROM, send_buffer, 3 + eeprom_write_job_queue[eeprom_write_job_queue_index_end].size, callbackEepromWrite);
}

/*
 * Function: EEPROM書き込み送信コールバック
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
static void callbackEepromWrite(void)
{
	WritePin(PIN_ID_EEPROM_CS, PIN_CS_OFF);

	/* 書き込みデータを送信したので、次の周期処理でステータスを確認 */
	eeprom_state = EEPROM_STATE_WAIT_WRITING;
}

/*
 * Function: EEPROMステータス読み出し
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
static void eepromReadStatus(void)
{
	eeprom_state = EEPROM_STATE_READ_STATUS;

	/* ステータス読み出し */
	send_buffer[0] = INSTRUCTION_READ;
	WritePin(PIN_ID_EEPROM_CS, PIN_CS_ON);
	SendReceiveSpi(SPI_EEPROM, send_buffer, receive_buffer, 2, callbackEepromReadStatus);
}

/*
 * Function: EEPROMステータス読み出しコールバック
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
static void callbackEepromReadStatus(void)
{
	WritePin(PIN_ID_EEPROM_CS, PIN_CS_OFF);

	/* Write-In-Process (WIP) bitを確認 */
	if ((receive_buffer[1] & 0x01) != 1) {
		/* 書き込み完了 */
		eeprom_state = EEPROM_STATE_IDLE;
		if (eeprom_write_job_queue_index_end < EEPROM_WRITE_JOB_QUEUE_SIZE - 1) {
			eeprom_write_job_queue_index_end ++;
		} else {
			eeprom_write_job_queue_index_end = 0;
		}
	} else {
		/* 書き込み処理中 */
		eeprom_state = EEPROM_STATE_WAIT_WRITING;
	}
}
