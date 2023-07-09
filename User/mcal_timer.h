/*
 * mcal_timer.h
 *
 *  Created on: Jun 15, 2023
 *      Author: KimiakiK
 */


#ifndef MCAL_TIMER_H_
#define MCAL_TIMER_H_

/********** Include **********/

#include "typedef.h"

/********** Define **********/

/********** Enum **********/

typedef enum {
	TIMER_CH1 = 0,		/* 振動モータ用 : PWM 約150Hz duty 50% */
	TIMER_CH2,			/* 振動モータ駆動時間制御用 : 1カウント = 160MHz / (15999 + 1) = 0.1ms */
	TIMER_CH3,			/* TFTバックライト用 */
	TIMER_CH4,			/* Wait用(1us) : 160MHz / (159+1) */
	TIMER_CH5,			/* 60fps周期用 : (266666+1) * 160MHz / 1 */
	TIMER_CH6,			/* 5ms周期用 : (4999+1) * 160MHz / (159 + 1) */
	TIMER_CH_NUM
} timer_ch_t;

/********** Type **********/

/********** Constant **********/

/********** Variable **********/

/********** Function Prototype **********/

void InitTimer(void);
void InterruptTimer(timer_ch_t timer_ch);
void SetTimerCallback(timer_ch_t timer_ch, callback_t callback);
void StartTimer(timer_ch_t timer_ch);
void StopTimer(timer_ch_t timer_ch);
uint32_t GetTimerCounter(timer_ch_t timer_ch);
void SetTimerCounter(timer_ch_t timer_ch, uint32_t count);
uint32_t GetTimerPeriod(timer_ch_t timer_ch);
void SetTimerPeriod(timer_ch_t timer_ch, uint32_t period);
void WaitUs(uint32_t us);

#endif /* MCAL_TIMER_H_ */
