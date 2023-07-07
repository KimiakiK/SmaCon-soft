/*
 * mcal_timer.c
 *
 *  Created on: Jun 15, 2023
 *      Author: KimiakiK
 */


/********** Include **********/

#include "typedef.h"
#include "mcal_timer.h"

/********** Define **********/

/********** Enum **********/

typedef enum {
	TIMER_MODE_SIMPLE = 0,
	TIMER_MODE_INTERRUPT,
	TIMER_MODE_PWM
} timer_mode_t;

/********** Type **********/

/********** Constant **********/

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim5;

static TIM_HandleTypeDef* const htim[TIMER_CH_NUM] = {
	&htim1,		/* TIMER_CH1 */
	&htim2,		/* TIMER_CH2 */
	&htim3,		/* TIMER_CH3 */
	&htim4,		/* TIMER_CH4 */
	&htim5,		/* TIMER_CH5 */
};

static const timer_mode_t timer_mode[TIMER_CH_NUM] = {
	TIMER_MODE_PWM,			/* TIMER_CH1 */
	TIMER_MODE_INTERRUPT,	/* TIMER_CH2 */
	TIMER_MODE_PWM,			/* TIMER_CH3 */
	TIMER_MODE_SIMPLE,		/* TIMER_CH4 */
	TIMER_MODE_INTERRUPT,	/* TIMER_CH5 */
};

static const uint32_t pwm_ch[TIMER_CH_NUM] = {
	TIM_CHANNEL_3,			/* TIMER_CH1 */
	0,						/* TIMER_CH2 */
	TIM_CHANNEL_3,			/* TIMER_CH3 */
	0,						/* TIMER_CH4 */
	0,						/* TIMER_CH5 */
};

/********** Variable **********/

static callback_t timer_callback[TIMER_CH_NUM];

/********** Function Prototype **********/

/********** Function **********/

/*
 * Function: MCAL Timer 初期化
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
void InitTimer(void)
{
	uint32_t index;

	for (index=0; index<TIMER_CH_NUM; index++) {
		timer_callback[index] = NULL;
	}
}

/*
 * Function: タイマー割り込み処理
 * Argument: タイマーチャネル
 * Return  : なし
 * Note    : なし
 */
void InterruptTimer(timer_ch_t timer_ch)
{
	if (timer_ch < TIMER_CH_NUM) {
		if (timer_callback[timer_ch] != NULL) {
			timer_callback[timer_ch]();
		}
	}
}


/*
 * Function: タイマーコールバック関数設定
 * Argument: タイマーチャネル、コールバック関数
 * Return  : なし
 * Note    : なし
 */
void SetTimerCallback(timer_ch_t timer_ch, callback_t callback)
{
	if (timer_ch < TIMER_CH_NUM) {
		timer_callback[timer_ch] = callback;
	}
}

/*
 * Function: タイマー開始
 * Argument: タイマーチャネル
 * Return  : なし
 * Note    : なし
 */
void StartTimer(timer_ch_t timer_ch)
{
	if (timer_ch < TIMER_CH_NUM) {
		switch (timer_mode[timer_ch]) {
		case TIMER_MODE_SIMPLE:
			HAL_TIM_Base_Start(htim[timer_ch]);
			break;
		case TIMER_MODE_INTERRUPT:
			HAL_TIM_Base_Start_IT(htim[timer_ch]);
			break;
		case TIMER_MODE_PWM:
			HAL_TIM_PWM_Start(htim[timer_ch],pwm_ch[timer_ch]);
			HAL_TIMEx_PWMN_Start(htim[timer_ch],pwm_ch[timer_ch]);
			break;
		default:
			/* 処理なし */
			break;
		}
	}
}

/*
 * Function: タイマー停止
 * Argument: タイマーチャネル
 * Return  : なし
 * Note    : なし
 */
void StopTimer(timer_ch_t timer_ch)
{
	if (timer_ch < TIMER_CH_NUM) {
		switch (timer_mode[timer_ch]) {
		case TIMER_MODE_SIMPLE:
			HAL_TIM_Base_Stop(htim[timer_ch]);
			break;
		case TIMER_MODE_INTERRUPT:
			HAL_TIM_Base_Stop_IT(htim[timer_ch]);
			break;
		case TIMER_MODE_PWM:
			HAL_TIM_PWM_Stop(htim[timer_ch],pwm_ch[timer_ch]);
			HAL_TIMEx_PWMN_Stop(htim[timer_ch],pwm_ch[timer_ch]);
			break;
		default:
			/* 処理なし */
			break;
		}
	}
}

/*
 * Function: タイマーカウンタ取得
 * Argument: タイマーチャネル
 * Return  : カウンタ
 * Note    : なし
 */
uint32_t GetTimerCounter(timer_ch_t timer_ch)
{
	uint32_t count = 0;

	if (timer_ch < TIMER_CH_NUM) {
		count =  htim[timer_ch]->Instance->CNT;
	}

	return count;
}

/*
 * Function: タイマーカウンタ設定
 * Argument: タイマーチャネル、カウンタ
 * Return  : なし
 * Note    : なし
 */
void SetTimerCounter(timer_ch_t timer_ch, uint32_t count)
{
	if (timer_ch < TIMER_CH_NUM) {
		htim[timer_ch]->Instance->CNT = count;
	}
}

/*
 * Function: タイマー周期取得
 * Argument: タイマーチャネル
 * Return  : 周期
 * Note    : なし
 */
uint32_t GetTimerPeriod(timer_ch_t timer_ch)
{
	uint32_t period = 0;

	if (timer_ch < TIMER_CH_NUM) {
		period =  htim[timer_ch]->Instance->ARR;
	}

	return period;
}

/*
 * Function: タイマー周期設定
 * Argument: タイマーチャネル、周期
 * Return  : なし
 * Note    : なし
 */
void SetTimerPeriod(timer_ch_t timer_ch, uint32_t period)
{
	if (timer_ch < TIMER_CH_NUM) {
		htim[timer_ch]->Instance->ARR = period;
	}
}

/*
 * Function: Wait
 * Argument: Wait時間 [us]
 * Return  : なし
 * Note    : なし
 */
void WaitUs(uint32_t us)
{
	SetTimerCounter(TIMER_CH4, 0);
	StartTimer(TIMER_CH4);
	while (GetTimerCounter(TIMER_CH4) < us) {
		/* 処理なし(時間経過待ち) */
	}
	StopTimer(TIMER_CH4);
}
