/*
 * mcal_dma2d.c
 *
 *  Created on: Jun 27, 2023
 *      Author: KimiakiK
 */


/********** Include **********/

#include "typedef.h"
#include "drv_tft.h"
#include "mcal_dma2d.h"

/********** Define **********/

#define TRANSFER_JOB_QUEUE_SIZE	(32)

/********** Enum **********/

typedef enum {
	TRANSFER_MODE_R2M = 0,
	TRANSFER_MODE_CALLBACK
} transfer_mdoe_t;

typedef enum {
	TRANSFER_STATE_IDLE = 0,
	TRANSFER_STATE_BUSY
} transfer_state_t;

/********** Type **********/

typedef struct {
	transfer_mdoe_t mode;
	uint32_t output_offset;
	uint32_t pdata;
	uint32_t destination_address;
	uint32_t width;
	uint32_t height;
	callback_t callback;
} transfer_job_t;

/********** Constant **********/

/********** Variable **********/

extern DMA2D_HandleTypeDef hdma2d;

static transfer_job_t transfer_job_queue[TRANSFER_JOB_QUEUE_SIZE];
static uint32_t transfer_job_queue_index_top;
static uint32_t transfer_job_queue_index_end;

static transfer_state_t transfer_state;

/********** Function Prototype **********/

void transferAsync(transfer_job_t* job);
void transferJob(void);

/********** Function **********/


/*
 * Function: MCAL DMA2D 初期化
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
void InitDma2d(void)
{
	transfer_job_queue_index_top = 0;
	transfer_job_queue_index_end = 0;
	transfer_state = TRANSFER_STATE_IDLE;
}

/*
 * Function: レジスタ→メモリ転送ジョブ設定
 * Argument: バッファアドレス、幅、高さ、出力オフセット、カラー(RGB888)
 * Return  : なし
 * Note    : なし
 */
void SetRegisterToMemoryTransferJob(uint32_t buffer_address, uint32_t width, uint32_t height, uint32_t output_offset, uint32_t color_RGB888)
{
	transfer_job_t job;

	job.mode = TRANSFER_MODE_R2M;
	job.destination_address = buffer_address;
	job.width = width;
	job.height = height;
	job.output_offset = output_offset;
	job.pdata = color_RGB888;

	transferAsync(&job);
}

/*
 * Function: コールバックジョブ設定
 * Argument: コールバック関数ポインタ
 * Return  : なし
 * Note    : なし
 */
void SetDma2dCallbackJob(callback_t callback)
{
	transfer_job_t job;

	job.mode = TRANSFER_MODE_CALLBACK;
	job.callback = callback;

	transferAsync(&job);
}

/*
 * Function: 非同期転送
 * Argument: 転送ジョブ
 * Return  : なし
 * Note    : なし
 */
void transferAsync(transfer_job_t* job)
{
	/* 転送ジョブに追加 */
	transfer_job_queue[transfer_job_queue_index_top] = *job;

	if (transfer_job_queue_index_top < TRANSFER_JOB_QUEUE_SIZE - 1) {
		transfer_job_queue_index_top ++;
	} else {
		transfer_job_queue_index_top = 0;
	}

	if (transfer_state == TRANSFER_STATE_IDLE) {
		transfer_state = TRANSFER_STATE_BUSY;
		transferJob();
	}
}

/*
 * Function: ジョブ転送
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
void transferJob(void)
{
	uint32_t transfer_job_queue_index;
	transfer_job_t* job;

	if (transfer_job_queue_index_top != transfer_job_queue_index_end) {
		/* 次のジョブを転送 */
		transfer_job_queue_index = transfer_job_queue_index_end;

		if (transfer_job_queue_index_end < TRANSFER_JOB_QUEUE_SIZE - 1) {
			transfer_job_queue_index_end ++;
		} else {
			transfer_job_queue_index_end = 0;
		}

		job = &transfer_job_queue[transfer_job_queue_index];

		switch (job->mode) {
		case TRANSFER_MODE_R2M:
			/* レジスタ→メモリ転送実行 */
				hdma2d.Init.Mode = DMA2D_R2M;
				hdma2d.Init.ColorMode = DMA2D_OUTPUT_RGB565;
				hdma2d.Init.OutputOffset = job->output_offset;
				hdma2d.Init.RedBlueSwap = DMA2D_RB_REGULAR;
				hdma2d.Init.BytesSwap = DMA2D_BYTES_REGULAR;		/* BytesSwapを有効にするとpixel per line (PL)に奇数が許容されないため、有効にできない */
				hdma2d.Init.LineOffsetMode = DMA2D_LOM_PIXELS;

				HAL_DMA2D_Init(&hdma2d);
				HAL_DMA2D_Start_IT(&hdma2d, job->pdata, job->destination_address, job->width, job->height);
			break;
		case TRANSFER_MODE_CALLBACK:
			/* コールバック関数呼び出し */
			if (job->callback != NULL) {
				job->callback();
			}
			/* 次のジョブも実行 */
			transferJob();
			break;
		default:
			/* 処理なし */
			break;
		}
	} else {
		/* すべてのジョブを転送済み */
		transfer_state = TRANSFER_STATE_IDLE;
	}
}

/*
 * Function: DMA2D転送完了割り込み処理
 * Argument: なし
 * Return  : なし
 * Note    : なし
 */
void InterruptDma2dTransferComplete(void)
{
	transferJob();
}
