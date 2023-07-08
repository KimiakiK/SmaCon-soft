/*
 * mcal_adc.c
 *
 *  Created on: Jul 8, 2023
 *      Author: KimiakiK
 */


/********** Include **********/

#include "typedef.h"
#include "mcal_adc.h"

/********** Define **********/

#define AD_VALUE_MAX		(0x03FFF)

/********** Enum **********/

/********** Type **********/

/********** Constant **********/

const uint32_t injection_rank_table[AD_ID_NUM] =
{
	ADC_INJECTED_RANK_1,
	ADC_INJECTED_RANK_2,
	ADC_INJECTED_RANK_3
};

/********** Variable **********/

extern ADC_HandleTypeDef hadc1;

static float ad_value[AD_ID_NUM];

/********** Function Prototype **********/

/********** Function **********/

/*
 * Function: MCAL ADC 初期化
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
void InitAdc(void)
{
	/* 各デバイスの初期値を設定 */
	ad_value[AD_ID_POS_H] = 0.5f;
	ad_value[AD_ID_POS_V] = 0.5f;
	ad_value[AD_ID_LEVER] = 1.0f;
}

/*
 * Function: MCAL ADC 周期処理
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
void MainAdc(void)
{
	/* AD変換実行 */
	HAL_ADCEx_InjectedStart_IT(&hadc1);
}

/*
 * Function: AD値取得
 * Argument: AD値ID
 * Return  : AD値(0.0～1.0)
 * Note    : なし
 */
float GetAd(ad_id_t ad_id)
{
	return ad_value[ad_id];
}

/*
 * Function: AD変換完了割り込み処理
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
void InterruptAdcComplete(void)
{
	uint32_t ad_id;
	uint32_t value;

	for (ad_id=0; ad_id<AD_ID_NUM; ad_id++) {
		/* AD変換結果取得 */
		value = HAL_ADCEx_InjectedGetValue(&hadc1, injection_rank_table[ad_id]);
		ad_value[ad_id] = value / (float)AD_VALUE_MAX;
	}
}
