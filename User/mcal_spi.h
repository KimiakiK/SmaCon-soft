/*
 * mcal_spi.h
 *
 *  Created on: May 10, 2023
 *      Author: KimiakiK
 */


#ifndef MCAL_SPI_H_
#define MCAL_SPI_H_

/********** Include **********/

#include "typedef.h"

/********** Define **********/

/********** Enum **********/

/* SPIチャネル定義 */
typedef enum {
	SPI_CH1 = 0,		/* TFT */
	SPI_CH2,			/* SOUND */
	SPI_CH3,			/* EEPROM */
	SPI_CH_NUM
} spi_ch_t;

/********** Type **********/


/********** Constant **********/

/********** Variable **********/

/********** Function Prototype **********/

void InitSpi(void);
result_t SendSpi(spi_ch_t spi_ch, uint8_t* data, uint32_t length, callback_t callback);
result_t ReceiveSpi(spi_ch_t spi_ch, uint8_t* receive_buffer, uint16_t length, callback_t callback);
result_t SendReceiveSpi(spi_ch_t spi_ch, uint8_t* send_data, uint8_t* receive_buffer, uint16_t length, callback_t callback);
void InterruptSpiComplete(spi_ch_t spi_ch);


#endif /* MCAL_SPI_H_ */
