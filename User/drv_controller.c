/*
 * drv_controller.c
 *
 *  Created on: Jul 8, 2023
 *      Author: KimiakiK
 */


/********** Include **********/

#include "typedef.h"
#include "mcal_adc.h"
#include "mcal_dio.h"
#include "drv_controller.h"

/********** Define **********/

/* チャタリング除去対象のSW数 */
#define SW_NUM					(4)
/* チャタリング除去用のバッファ数 */
#define SW_INPUT_BUFFER_NUM		(3)

/* ジョイスティック入力の閾値(上側) */
#define POS_THRESHOLD_UPPER		(0.8f)
/* ジョイスティック入力の閾値(下側) */
#define POS_THRESHOLD_LOWER		(0.2f)
/* ジョイススティック入力ニュートラルの閾値(上側) */
#define POS_THRESHOLD_NEUTRAL_UPPER	(0.65f)
/* ジョイススティック入力ニュートラルの閾値(下側) */
#define POS_THRESHOLD_NEUTRAL_LOWER	(0.35f)

/********** Enum **********/

/********** Type **********/

typedef struct {
	pin_id_t pin_id;
	input_id_t input_id;
	pin_level_t active_level;
} sw_info_t;

/********** Constant **********/

static const sw_info_t sw_info_table[SW_NUM] =
{
	{PIN_ID_SW_A,	INPUT_ID_SW_A,	PIN_LEVEL_LOW},
	{PIN_ID_SW_B,	INPUT_ID_SW_B,	PIN_LEVEL_LOW},
	{PIN_ID_SW_C,	INPUT_ID_SW_C,	PIN_LEVEL_LOW},
	{PIN_ID_SW_D,	INPUT_ID_SW_D,	PIN_LEVEL_LOW},
};

/********** Variable **********/

static input_state_t input_state[INPUT_ID_NUM];
static pin_level_t sw_input_buffer[SW_NUM][SW_INPUT_BUFFER_NUM];
static uint32_t sw_input_buffer_index;

/********** Function Prototype **********/

static void judgeSwInputState(input_state_t current_input_state[]);
static void judgePosInputState(input_state_t current_input_state[]);
static void judgeLeverInputState(input_state_t current_input_state[]);
static void updateInputState(input_state_t current_input_state[]);

/********** Function **********/

/*
 * Function: DRV CONTROLLER 初期化
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
void InitController(void)
{
	for (input_id_t input_id=0; input_id<INPUT_ID_NUM; input_id++) {
		input_state[input_id] = INPUT_OFF;
	}

	for (uint32_t sw_id=0; sw_id<SW_NUM; sw_id++) {
		for (uint32_t buffer_index=0; buffer_index<SW_INPUT_BUFFER_NUM; buffer_index++) {
			/* SW入力の初期値はSW非アクティブ */
			sw_input_buffer[sw_id][buffer_index] = ~sw_info_table[sw_id].active_level;
		}
	}

	sw_input_buffer_index = 0;
}

/*
 * Function: DRV CONTROLLER 周期処理
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
void MainController(void)
{
	input_state_t current_input_state[INPUT_ID_NUM];

	/* 各種入力状態判定 */
	judgeSwInputState(current_input_state);
	judgePosInputState(current_input_state);
	judgeLeverInputState(current_input_state);

	/* 入力状態更新 */
	updateInputState(current_input_state);
}

/*
 * Function: SW入力更新
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
void UpdateSwInput(void)
{
	/* SW端子の状態を取得*/
	for (uint32_t sw_id=0; sw_id<SW_NUM; sw_id++) {
		sw_input_buffer[sw_id][sw_input_buffer_index] = ReadPin(sw_info_table[sw_id].pin_id);
	}

	if (sw_input_buffer_index < SW_INPUT_BUFFER_NUM - 1) {
		sw_input_buffer_index ++;
	} else {
		sw_input_buffer_index = 0;
	}
}

/*
 * Function: 入力状態取得
 * Argument: 入力ID
 * Return  : 入力状態
 * Note    : なし
 */
input_state_t GetInputState(input_id_t input_id)
{
	return input_state[input_id];
}

/*
 * Function: SW入力状態判定
 * Argument: 現在入力状態
 * Return  : なし
 * Note    : なし
 */
static void judgeSwInputState(input_state_t current_input_state[])
{
	for (uint32_t sw_id=0; sw_id<SW_NUM; sw_id++) {
		uint32_t count = 0;
		for (uint32_t buffer_index=1; buffer_index<SW_INPUT_BUFFER_NUM; buffer_index++) {
			if (sw_input_buffer[sw_id][buffer_index - 1] == sw_input_buffer[sw_id][buffer_index]) {
				count ++;
			}
		}

		if (count == (SW_INPUT_BUFFER_NUM - 1)) {
			/* バッファ内の値がすべて一致しているので、入力確定 */
			if (sw_input_buffer[sw_id][0] == sw_info_table[sw_id].active_level) {
				current_input_state[sw_info_table[sw_id].input_id] = INPUT_ON;
			} else {
				current_input_state[sw_info_table[sw_id].input_id] = INPUT_OFF;
			}
		} else {
			/* バッファ内の値に変化があるので、前周期の値を採用 */
			current_input_state[sw_info_table[sw_id].input_id] = input_state[sw_info_table[sw_id].input_id];
		}
	}
}

/*
 * Function: ジョイスティック入力状態判定
 * Argument: 現在入力状態
 * Return  : なし
 * Note    : なし
 */
static void judgePosInputState(input_state_t current_input_state[])
{
	float ad_value;

	/* ジョイスティック横方向判定 */
	ad_value = GetAd(AD_ID_POS_H);
	if (ad_value > POS_THRESHOLD_UPPER) {
		current_input_state[INPUT_ID_POS_LEFT] = INPUT_OFF;
		current_input_state[INPUT_ID_POS_RIGHT] = INPUT_ON;
	} else if (ad_value < POS_THRESHOLD_LOWER) {
		current_input_state[INPUT_ID_POS_LEFT] = INPUT_ON;
		current_input_state[INPUT_ID_POS_RIGHT] = INPUT_OFF;
	} else if ((ad_value < POS_THRESHOLD_NEUTRAL_UPPER) && (ad_value > POS_THRESHOLD_NEUTRAL_LOWER)) {
		current_input_state[INPUT_ID_POS_LEFT] = INPUT_OFF;
		current_input_state[INPUT_ID_POS_RIGHT] = INPUT_OFF;
	} else {
		/* 途中位置は前回値保持 */
		current_input_state[INPUT_ID_POS_LEFT] = input_state[INPUT_ID_POS_LEFT];
		current_input_state[INPUT_ID_POS_RIGHT] = input_state[INPUT_ID_POS_RIGHT];
	}
	
	/* ジョイスティック縦方向判定 */
	ad_value = GetAd(AD_ID_POS_V);
	if (ad_value > POS_THRESHOLD_UPPER) {
		current_input_state[INPUT_ID_POS_UP] = INPUT_OFF;
		current_input_state[INPUT_ID_POS_DOWN] = INPUT_ON;
	} else if (ad_value < POS_THRESHOLD_LOWER) {
		current_input_state[INPUT_ID_POS_UP] = INPUT_ON;
		current_input_state[INPUT_ID_POS_DOWN] = INPUT_OFF;
	} else if ((ad_value < POS_THRESHOLD_NEUTRAL_UPPER) && (ad_value > POS_THRESHOLD_NEUTRAL_LOWER)) {
		current_input_state[INPUT_ID_POS_UP] = INPUT_OFF;
		current_input_state[INPUT_ID_POS_DOWN] = INPUT_OFF;
	} else {
		/* 途中位置は前回値保持 */
		current_input_state[INPUT_ID_POS_UP] = input_state[INPUT_ID_POS_UP];
		current_input_state[INPUT_ID_POS_DOWN] = input_state[INPUT_ID_POS_DOWN];
	}
}

/*
 * Function: レバー入力状態判定
 * Argument: 現在入力状態
 * Return  : なし
 * Note    : なし
 */
static void judgeLeverInputState(input_state_t current_input_state[])
{
	float ad_value = GetAd(AD_ID_LEVER);

	/* レバー位置判定 */
	if (ad_value < 0.15f) {
		current_input_state[INPUT_ID_LEVER_SW] = INPUT_ON;
		current_input_state[INPUT_ID_LEVER_LEFT] = INPUT_OFF;
		current_input_state[INPUT_ID_LEVER_RIGHT] = INPUT_OFF;
	} else if ((ad_value > 0.2f) && (ad_value < 0.5f)) {
		current_input_state[INPUT_ID_LEVER_SW] = INPUT_OFF;
		current_input_state[INPUT_ID_LEVER_LEFT] = INPUT_ON;
		current_input_state[INPUT_ID_LEVER_RIGHT] = INPUT_OFF;
	} else if ((ad_value > 0.5f) && (ad_value < 0.7f)) {
		current_input_state[INPUT_ID_LEVER_SW] = INPUT_OFF;
		current_input_state[INPUT_ID_LEVER_LEFT] = INPUT_OFF;
		current_input_state[INPUT_ID_LEVER_RIGHT] = INPUT_ON;
	} else {
		current_input_state[INPUT_ID_LEVER_SW] = INPUT_OFF;
		current_input_state[INPUT_ID_LEVER_LEFT] = INPUT_OFF;
		current_input_state[INPUT_ID_LEVER_RIGHT] = INPUT_OFF;
	}
}

/*
 * Function: 入力状態更新
 * Argument: 現在入力状態
 * Return  : なし
 * Note    : なし
 */
static void updateInputState(input_state_t current_input_state[])
{
	for (input_id_t input_id=0; input_id<INPUT_ID_NUM; input_id++) {
		switch (input_state[input_id]) {
		case INPUT_OFF:				/* INPUT_OFFとINPUT_RELEASEは同じ処理 */
		case INPUT_RELEASE:
			if (current_input_state[input_id] == INPUT_ON) {
				input_state[input_id] = INPUT_PUSH;
			} else {
				input_state[input_id] = INPUT_OFF;
			}
			break;
		case INPUT_ON:				/* INPUT_ONとINPUT_PUSHは同じ処理 */
		case INPUT_PUSH:
			if (current_input_state[input_id] == INPUT_OFF) {
				input_state[input_id] = INPUT_RELEASE;
			} else {
				input_state[input_id] = INPUT_ON;
			}
			break;
		default:
			input_state[input_id] = INPUT_OFF;
			break;
		}
	}
}
