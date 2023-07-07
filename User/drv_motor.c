/*
 * drv_motor.c
 *
 *  Created on: 2023/07/08
 *      Author: KimiakiK
 */


/********** Include **********/

#include "typedef.h"
#include "mcal_timer.h"
#include "drv_motor.h"

/********** Define **********/

/********** Enum **********/

/********** Type **********/

/********** Constant **********/

/********** Variable **********/

/********** Function Prototype **********/

/********** Function **********/

/*
 * Function: DRV MOTOR 初期化
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
void InitMotor(void)
{
	/* モータ停止用コールバック設定 */
	SetTimerCallback(TIMER_CH2, StopMotor);
}

/*
 * Function: モータ動作開始
 * Argument: モータ駆動時間 [ms] (0:連続駆動)
 * Return  : なし
 * Note    : なし
 */
void StartMotor(uint32_t time)
{
	/* モータ動作開始 */
	StartTimer(TIMER_CH1);

	if (time != 0) {
		/* 時間設定 */
		SetTimerPeriod(TIMER_CH2, time * 10);
		/* タイマースタート */
		StartTimer(TIMER_CH2);
	}
}

/*
 * Function: モータ動作停止
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
void StopMotor(void)
{
	StopTimer(TIMER_CH1);
}
