/*
 * drv_draw.c
 *
 *  Created on: 2023/06/29
 *      Author: KimiakiK
 */


/********** Include **********/

#include "typedef.h"
#include "drv_tft.h"
#include "mcal_dma2d.h"
#include "drv_draw.h"

/********** Define **********/

/********** Enum **********/

/********** Type **********/

/********** Constant **********/

/********** Variable **********/

static uint32_t buffer_address;

/********** Function Prototype **********/

static void callbackDrawComplete(void);

/********** Function **********/

/*
 * Function: DRV DRAW 初期化
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
void InitDraw(void)
{
	/* 処理なし */
}

/*
 * Function: 描画指示開始
 * Argument: フレームバッファ先頭アドレス
 * Return  : なし
 * Note    : なし
 */
void StartDraw(uint8_t* frame_buffer)
{
	/* 描画対象のバッファを保持 */
	buffer_address = (uint32_t)frame_buffer;
}

/*
 * Function: 描画指示終了
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
void EndDraw(void)
{
	/* 描画完了時を処理するためのコールバック関数を設定 */
	SetDma2dCallbackJob(callbackDrawComplete);
}

/*
 * Function: 四角形塗りつぶし描画
 * Argument: 横方向開始座標、縦方向開始座標、横幅、縦幅、カラー(RGB888)
 * Return  : なし
 * Note    : なし
 */
void FillRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color_ARGB8888)
{
	uint32_t address;
	uint32_t width;
	uint32_t height;
	uint32_t output_offset;

	/* 座標の範囲チェック */
	if ((x < TFT_WIDTH) && (y < TFT_HEIGHT) && (w <= TFT_WIDTH) && (h <= TFT_HEIGHT)) {
		/* 開始座標に合わせて描画開始アドレスをずらす */
		address = buffer_address + (x * COLOR_SIZE) + (y * TFT_WIDTH * COLOR_SIZE);
		/* TFTの横幅を超えないように四角形の横幅を設定 */
		if ((x + w) <= TFT_WIDTH) {
			width = w;
		} else {
			width = (x + w) - TFT_WIDTH;
		}
		/* TFTの縦幅を超えないように四角形の縦幅を設定 */
		if ((y + h) <= TFT_HEIGHT) {
			height = h;
		} else {
			height = (y + h) - TFT_HEIGHT;
		}
		/* 横方向のオフセットを設定 */
		output_offset = TFT_WIDTH - width;
		/* 描画ジョブ発行 */
		SetRegisterToMemoryTransferJob(address, width, height, output_offset, color_ARGB8888);
	}
}

/*
 * Function: 描画完了コールバック
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
static void callbackDrawComplete(void)
{
	/* 描画を完了したのでバッファ切り替えを要求 */
	SetSwapRequest(TRUE);
}