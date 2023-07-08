/*
 * sys_platform.c
 *
 *  Created on: Apr 22, 2023
 *      Author: KimiakiK
 */


/********** Include **********/

#include "typedef.h"
#include "drv_backlight.h"
#include "drv_draw.h"
#include "drv_motor.h"
#include "drv_tft.h"
#include "drv_touch.h"
#include "mcal_adc.h"
#include "mcal_dio.h"
#include "mcal_dma2d.h"
#include "mcal_i2c.h"
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
	InitAdc();
	InitDma2d();
	InitI2c();
	InitSpi();
	InitTimer();

	/* ドライバ初期化 */
	InitBacklight();
	InitTft();
	InitTouch();
	InitDraw();
	InitMotor();

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
	uint32_t y = 0; // 描画確認用

	while (TRUE) {
		if (event_update_display == TRUE) {
			/* 周期処理実行 */
			MainAdc();
			MainTouch();
			
			// 描画確認用　↓
			if (y < 59) {
				y ++;
			} else {
				y = 0;
			}
			StartDraw(GetFrameBuffer());
			FillRect(0, 0, TFT_WIDTH, TFT_HEIGHT, 0x00FFFFFF);
			FillRect(10, y * 5, 220, 5, 0x00FF0000);
			if (GetTouchState() == TOUCH_ON) {
				point_t point = GetTouchPoint();
				FillRect(point.x - 20.0f, point.y - 20.0f, 40, 40, 0x00FF0000);
			}
			{
				float h = GetAd(AD_ID_POS_H);
				float v = GetAd(AD_ID_POS_V);
				float l = GetAd(AD_ID_LEVER);
				FillRect(h*TFT_WIDTH - 20.0f, v*TFT_HEIGHT - 20.0f, 40, 40, 0x000000FF);
				FillRect(l*TFT_WIDTH - 20.0f, 10.0f, 40, 30, 0x000000FF);
			}
			EndDraw();
			//描画確認用　↑

			//モータ確認用　↓
			if (GetTouchState() == TOUCH_START) {
				StartMotor(50);
			}
			//モータ確認用　↑

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
