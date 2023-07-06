/*
 * mcal_i2c.h
 *
 *  Created on: Jun 30, 2023
 *      Author: KimiakiK
 */


#ifndef MCAL_I2C_H_
#define MCAL_I2C_H_

/********** Include **********/

#include "typedef.h"

/********** Define **********/

/********** Enum **********/

/********** Type **********/

/********** Constant **********/

/********** Variable **********/

/********** Function Prototype **********/

void InitI2c(void);
void SendI2c(uint16_t device_address, uint8_t* data, uint16_t length, callback_t callback);
void ReceiveI2c(uint16_t device_address, uint8_t* data, uint16_t length, callback_t callback);
void InterruptI2cSendComplete(void);
void InterruptI2cReceiveComplete(void);

#endif /* MCAL_I2C_H_ */
