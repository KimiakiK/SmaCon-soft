/*
 * sys_platform.c
 *
 *  Created on: Apr 22, 2023
 *      Author: KimiakiK
 */


/********** Include **********/

#include "typedef.h"
#include "drv_backlight.h"
#include "drv_controller.h"
#include "drv_draw.h"
#include "drv_eeprom.h"
#include "drv_motor.h"
#include "drv_sound.h"
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

void cyclicMainEvent(void);
void updateDisplayEvent(void);
void cyclic5msEvent(void);

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
	InitController();
	InitBacklight();
	InitTft();
	InitTouch();
	InitDraw();
	InitMotor();
	InitSound();
	InitEeprom();

	/* タイマー開始 */
	SetTimerCallback(TIMER_CH5, updateDisplayEvent);
	SetTimerCallback(TIMER_CH6, cyclic5msEvent);
	StartTimer(TIMER_CH5);
	StartTimer(TIMER_CH6);

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
			/* 表示更新直後からメイン周期イベントを実行 */
			cyclicMainEvent();
			event_update_display = FALSE;
		}
	}
}

/*
 * Function: メイン周期イベント
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
void cyclicMainEvent(void)
{

	/* 周期処理実行 */
	MainAdc();
	MainTouch();
	MainController();
	MainEeprom();
	
	// 描画確認用　↓
	StartDraw(GetFrameBuffer());
	{
		/* 表示更新確認 */
		static uint32_t y = 0;
		if (y < 59) {
			y ++;
		} else {
			y = 0;
		}
		FillRect(0, 0, TFT_WIDTH, TFT_HEIGHT, 0x00FFFFFF);
		FillRect(10, y * 5, 220, 5, 0x00FF0000);
		if (GetTouchState() == TOUCH_ON) {
			point_t point = GetTouchPoint();
			FillRect(point.x - 20.0f, point.y - 20.0f, 40, 40, 0x00FF0000);
		}
	}
	{
		/* AD値確認 */
		float h = GetAd(AD_ID_POS_H);
		float v = GetAd(AD_ID_POS_V);
		float l = GetAd(AD_ID_LEVER);
		FillRect(h*TFT_WIDTH - 20.0f, v*TFT_HEIGHT - 20.0f, 40, 40, 0x000000FF);
		FillRect(l*TFT_WIDTH - 20.0f, 10.0f, 40, 30, 0x000000FF);
	}
	{
		/* コントローラ入力確認 */
		if (GetInputState(INPUT_ID_SW_A) == INPUT_ON) {
			FillRect(210.0f, 270.0f, 20, 20, 0x0000FF00);
		}
		if (GetInputState(INPUT_ID_SW_B) == INPUT_ON) {
			FillRect(190.0f, 290.0f, 20, 20, 0x0000FF00);
		}
		if (GetInputState(INPUT_ID_SW_C) == INPUT_ON) {
			FillRect(170.0f, 270.0f, 20, 20, 0x0000FF00);
		}
		if (GetInputState(INPUT_ID_SW_D) == INPUT_ON) {
			FillRect(190.0f, 250.0f, 20, 20, 0x0000FF00);
		}
		if (GetInputState(INPUT_ID_POS_UP) == INPUT_ON) {
			FillRect(30.0f, 250.0f, 20, 20, 0x0000FF00);
		}
		if (GetInputState(INPUT_ID_POS_DOWN) == INPUT_ON) {
			FillRect(30.0f, 290.0f, 20, 20, 0x0000FF00);
		}
		if (GetInputState(INPUT_ID_POS_LEFT) == INPUT_ON) {
			FillRect(10.0f, 270.0f, 20, 20, 0x0000FF00);
		}
		if (GetInputState(INPUT_ID_POS_RIGHT) == INPUT_ON) {
			FillRect(50.0f, 270.0f, 20, 20, 0x0000FF00);
		}
		if (GetInputState(INPUT_ID_LEVER_SW) == INPUT_ON) {
			FillRect(110.0f, 270.0f, 20, 20, 0x0000FF00);
		}
		if (GetInputState(INPUT_ID_LEVER_LEFT) == INPUT_ON) {
			FillRect(90.0f, 270.0f, 20, 20, 0x0000FF00);
		}
		if (GetInputState(INPUT_ID_LEVER_RIGHT) == INPUT_ON) {
			FillRect(130.0f, 270.0f, 20, 20, 0x0000FF00);
		}
	}
	{
		static pin_level_t audio_sw;
		/* サウンド確認 */
		if (GetTouchState() == TOUCH_ON) {
			KeyOn(4, 601);
		} else if (GetTouchState() == TOUCH_OFF) {
			KeyOff();
		}
		/* サウンド出力先切り替え */
		if ((ReadPin(PIN_ID_AUDIO_SW) == PIN_LEVEL_HIGH) && (audio_sw == PIN_LEVEL_LOW)) {
			/* スピーカー出力に切り替え */
			ChangeSoundOutputDevice(SOUND_OUTPUT_SPEAKER);
		}
		if ((ReadPin(PIN_ID_AUDIO_SW) == PIN_LEVEL_LOW) && (audio_sw == PIN_LEVEL_HIGH)) {
			/* ライン出力に切り替え */
			ChangeSoundOutputDevice(SOUND_OUTPUT_LINE);
		}
		audio_sw = ReadPin(PIN_ID_AUDIO_SW);
	}
	EndDraw();
	//描画確認用　↑
	//モータ確認用　↓
	if (GetTouchState() == TOUCH_START) {
		StartMotor(50);
	}
	//モータ確認用　↑
}

/*
 * Function: 5ms周期イベント
 * Argument: なし
 * Return  : なし
 * Note    : タイマー割り込み処理
 */
void cyclic5msEvent(void)
{
	/* SW入力更新 */
	UpdateSwInput();
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
