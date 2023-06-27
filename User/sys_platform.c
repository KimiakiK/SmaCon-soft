/*
 * sys_platform.c
 *
 *  Created on: Apr 22, 2023
 *      Author: KimiakiK
 */


/********** Include **********/

#include "typedef.h"
#include "drv_backlight.h"
#include "drv_tft.h"
#include "mcal_dio.h"
#include "mcal_spi.h"
#include "mcal_timer.h"
#include "sys_platform.h"

/********** Define **********/

/********** Enum **********/

/********** Type **********/

/********** Constant **********/

/********** Variable **********/

static bool_t event_update_display;

/********** Function Prototype **********/

void updateDisplayEvent(void);

/********** Function **********/

/*
 * Function: 初期化
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
void InitPlatform(void)
{
	/* 変数初期化 */
	event_update_display = FALSE;

	/* MCAL初期化 */
	InitDio();
	InitSpi();
	InitTimer();

	/* ドライバ初期化 */
	InitBacklight();
	InitTft();

	/* 表示更新用タイマー開始 */
	SetTimerCallback(TIMER_CH5, updateDisplayEvent);
	StartTimer(TIMER_CH5);

	/* TFT表示開始(暫定処理) */
	StartTft();
}

/*
 * Function: メインループ
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
void MainPlatform(void)
{
	while (TRUE) {
		if (event_update_display == TRUE) {
			/* 周期処理実行 */

			event_update_display = FALSE;
		}
	}
}

/*
 * Function: 表示更新イベント
 * Argument: なし
 * Return  : なし
 * Note    : タイマー割り込み処理
 */
void updateDisplayEvent(void)
{
	/* 表示更新実行 */
	UpdateTft();
	/* 表示更新イベント発生 */
	event_update_display = TRUE;
}
