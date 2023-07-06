/*
 * drv_touch.c
 *
 *  Created on: Jun 30, 2023
 *      Author: KimiakiK
 */


/********** Include **********/

#include "typedef.h"
#include "drv_tft.h"
#include "mcal_i2c.h"
#include "drv_touch.h"

/********** Define **********/

/* 通信データバッファサイズ [byte] */
#define DATA_BUFFER_SIZE		(64)

/* 読み出しリトライ試行回数 */
#define READ_RETRY_MAX			(10)

/* タッチ同時検出最大数 */
#define TOUCH_POINT_NUM_MAX		(5)

/********** Enum **********/

typedef enum {
	TOUCH_COM_STATE_INIT = 0,
	TOUCH_COM_STATE_COM,
	TOUCH_COM_STATE_WAIT,
	TOUCH_COM_STATE_ERROR,
	TOUCH_COM_STATE_TIMEOUT
} touch_com_state_t;

/********** Type **********/

typedef struct {
	uint8_t* send_data;
	uint16_t send_length;
	callback_t send_callback;
	uint8_t* receive_buffer;
	uint16_t receive_length;
	callback_t receive_callback;
} touch_com_job_t;

typedef struct {
	uint8_t track_id;
	uint16_t x;
	uint16_t y;
	uint16_t size;
} touch_point_t;

/********** Variable **********/

static touch_com_state_t touch_com_state;
static touch_com_job_t* current_job;
static uint32_t job_retry_count;
static uint8_t data_buffer[DATA_BUFFER_SIZE];

static uint8_t touch_point_num;
static uint8_t touch_point_num_old;
static touch_point_t touch_point[TOUCH_POINT_NUM_MAX];

static touch_state_t touch_input_state;
static touch_point_t touch_input_point;

/********** Function Prototype **********/

static void startJob(const touch_com_job_t* job);
static void callbackSendCompleteOnlySend(void);
static void callbackSendCompleteStartRead(void);
static void callbackReceiveCompleteCheckInfo(void);
static void callbackReceiveCompleteCheckCoordinate(void);
static void parseReadData(void);
static void transitionTouchState(void);
static result_t updateTouchInputPoint(void);

/********** Constant **********/

/* GT911 I2C device addresses */
static const uint8_t gt911_i2c_device_address = 0x5D;

/* GT911デバイス情報レジスタアドレス */
static const uint8_t gt911_device_info_register[] = {0x81, 0x40};

/* GT911タッチ座標情報レジスタアドレス */
static const uint8_t gt911_touch_coordinate_register[] = {0x81, 0x4E};

/* GT911 Buffer statusクリア用送信データ(レジスタ0x814Eに0x00を書き込む) */
static const uint8_t gt911_buffer_status_clear_data[] = {0x81, 0x4E, 0x00};

/* GT911デバイス情報読み出し用ジョブ */
static const touch_com_job_t job_read_info = {
	(uint8_t*)gt911_device_info_register,
	sizeof(gt911_device_info_register),
	callbackSendCompleteStartRead,
	data_buffer,
	11,
	callbackReceiveCompleteCheckInfo
};

/* GT911タッチ座標情報読み出し用ジョブ */
static const touch_com_job_t job_read_coordinate = {
	(uint8_t*)gt911_touch_coordinate_register,
	sizeof(gt911_touch_coordinate_register),
	callbackSendCompleteStartRead,
	data_buffer,
	50,
	callbackReceiveCompleteCheckCoordinate
};

/* GT911 Buffer statusクリア用ジョブ */
static const touch_com_job_t job_clear_buffer_status = {
	(uint8_t*)gt911_buffer_status_clear_data,
	sizeof(gt911_buffer_status_clear_data),
	callbackSendCompleteOnlySend,
	NULL,
	0,
	NULL
};

/********** Function **********/

/*
 * Function: DRV TOUCH 初期化
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
void InitTouch(void)
{
	/* 変数初期化 */
	touch_com_state = TOUCH_COM_STATE_INIT;
	current_job = NULL;
	job_retry_count = 0;
	touch_point_num = 0;
	touch_point_num_old = 0;
	touch_input_state = TOUCH_OFF;

	/* GT911の設定確認 */
	startJob(&job_read_info);
}

/*
 * Function: DRV TOUCH 周期処理
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
void MainTouch(void)
{
	if (touch_com_state == TOUCH_COM_STATE_WAIT) {
		/* 受信データからタッチ情報を作成 */
		parseReadData();
	} else {
		/* 処理なし(タッチ情報は前回値を使用する) */
	}

	/* タッチ情報をもとにタッチ状態を遷移 */
	transitionTouchState();

	/* GT911タッチ座標情報読み出し(エラー時や通信中は行わない) */
	if ((touch_com_state == TOUCH_COM_STATE_WAIT)
	 || (touch_com_state == TOUCH_COM_STATE_TIMEOUT)) {
		startJob(&job_read_coordinate);
	}

	/* 前回値更新 */
	touch_point_num_old = touch_point_num;
}

/*
 * Function: タッチ入力状態取得
 * Argument: なし
 * Return  : タッチ入力状態
 * Note    : なし
 */
touch_state_t GetTouchState(void)
{
	return touch_input_state;
}

/*
 * Function: タッチ入力座標取得
 * Argument: なし
 * Return  : タッチ入力座標
 * Note    : なし
 */
point_t GetTouchPoint(void)
{
	point_t point;

	point.x = touch_input_point.x;
	point.y = touch_input_point.y;

	return point;
}

/*
 * Function: ジョブ処理開始
 * Argument: ジョブポインタ
 * Return  : なし
 * Note    : なし
 */
static void startJob(const touch_com_job_t* job)
{
	/* 通信中状態へ遷移 */
	touch_com_state = TOUCH_COM_STATE_COM;
	/* 送信開始 */
	current_job = (touch_com_job_t*)job;
	SendI2c(gt911_i2c_device_address, current_job->send_data, current_job->send_length, current_job->send_callback);
}

/*
 * Function: 送信完了コールバック(送信のみ)
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
static void callbackSendCompleteOnlySend(void)
{
	/* 通信が完了したので、待機状態へ遷移 */
	touch_com_state = TOUCH_COM_STATE_WAIT;
}

/*
 * Function: 送信完了コールバック(受信開始)
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
static void callbackSendCompleteStartRead(void)
{
	/* 受信開始 */
	ReceiveI2c(gt911_i2c_device_address, current_job->receive_buffer, current_job->receive_length, current_job->receive_callback);
}

/*
 * Function: 受信完了コールバック(GT911デバイス情報読み出し完了時)
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
static void callbackReceiveCompleteCheckInfo(void)
{
	/* GT911のタッチ検出解像度判定 */
	if ((data_buffer[6] == (uint8_t)(TFT_WIDTH >> 0))		/* x coordinate resolution ( low byte ) */
	 && (data_buffer[7] == (uint8_t)(TFT_WIDTH >> 8))		/* x coordinate resolution ( high byte )*/
	 && (data_buffer[8] == (uint8_t)(TFT_HEIGHT >> 0))		/* y coordinate resolution ( low byte ) */
	 && (data_buffer[9] == (uint8_t)(TFT_HEIGHT >> 8))) {	/* y coordinate resolution ( high byte ) */
		/* GT911のタッチ検出解像度設定が正しいので座標読み出し実行 */
		startJob(&job_read_coordinate);
	} else {
		/* GT911のタッチ検出解像度設定が不正なのでエラー状態へ遷移 */
		touch_com_state = TOUCH_COM_STATE_ERROR;
	}
}

/*
 * Function: 受信完了コールバック(GT911タッチ座標情報読み出し完了時)
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
static void callbackReceiveCompleteCheckCoordinate(void)
{
	/* Buffer status確認 */
	if ((data_buffer[0] & 0x80) == 0x80) {
		/* タッチ座標情報読み出し成功、Buffer statusクリア */
		startJob(&job_clear_buffer_status);
	} else {
		/* タッチ座標情報読み出し失敗、読み出しリトライ */
		if (job_retry_count < READ_RETRY_MAX) {
			/* 座標読み出し実行 */
			job_retry_count ++;
			startJob(&job_read_coordinate);
		} else {
			/* リトライ試行上限なのでタイムアウト状態へ遷移 */
			touch_com_state = TOUCH_COM_STATE_TIMEOUT;
		}
	}
}

/*
 * Function: 受信データ解析
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
static void parseReadData(void)
{
	uint8_t index;

	touch_point_num = data_buffer[0] & 0x07;

	for (index=0; index<TOUCH_POINT_NUM_MAX; index++) {
		touch_point[index].track_id = data_buffer[index * 8 + 1];
		touch_point[index].x = data_buffer[index * 8 + 2];
		touch_point[index].x += data_buffer[index * 8 + 3] << 8;
		touch_point[index].y = data_buffer[index * 8 + 4];
		touch_point[index].y += data_buffer[index * 8 + 5] << 8;
		touch_point[index].size = data_buffer[index * 8 + 6];
		touch_point[index].size += data_buffer[index * 8 + 7] << 8;
	}
}

/*
 * Function: タッチ状態遷移
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
static void transitionTouchState(void)
{
	result_t result;
	switch (touch_input_state) {
	case TOUCH_OFF:				/* TOUCH_OFFとTOUCH_ENDは同じ処理 */
	case TOUCH_END:
		if ((touch_point_num_old == 0) && (touch_point_num > 0)) {
			/* 一度タッチ検出がなくなった後に1つ以上のタッチを検出したら、先頭のタッチ情報をタッチ入力として扱う */
			touch_input_point = touch_point[0];
			touch_input_state = TOUCH_START;
		} else {
			/* タッチ無し */
			touch_input_state = TOUCH_OFF;
		}
		break;
	case TOUCH_ON:				/* TOUCH_ONとTOUCH_STARTは同じ処理 */
	case TOUCH_START:
		if (touch_point_num == 0) {
			/* タッチがなければタッチ入力終了 */
			touch_input_state = TOUCH_END;
		} else {
			/* タッチ情報を更新 */
			result = updateTouchInputPoint();
			if (result == RESULT_NG) {
				/* 最初に検出したタッチ入力がなくなったらタッチ入力終了 */
				touch_input_state = TOUCH_END;
			} else {
				/* タッチ継続 */
				touch_input_state = TOUCH_ON;
			}
		}
		break;
	default:
		touch_input_state = TOUCH_OFF;
		break;
	}
}

/*
 * Function: タッチ入力更新
 * Argument: なし
 * Return  : タッチ入力更新成功/失敗
 * Note    : なし
 */
static result_t updateTouchInputPoint(void)
{
	result_t result = RESULT_NG;
	uint8_t index;

	for (index=0; index<touch_point_num; index++) {
		if (touch_input_point.track_id == touch_point[index].track_id) {
			/* 同じtrack idが検出出来たら、タッチ入力を更新*/
			touch_input_point = touch_point[index];
			result = RESULT_OK;
			break;
		}
	}

	return result;
}
