/*
 * drv_eeprom.h
 *
 *  Created on: Jul 18, 2023
 *      Author: KimiakiK
 */


#ifndef DRV_EEPROM_H_
#define DRV_EEPROM_H_

/********** Include **********/

#include "typedef.h"
#include "drv_eeprom_id.h"

/********** Define **********/

/********** Enum **********/

/********** Type **********/

/********** Constant **********/

/********** Variable **********/

/********** Function Prototype **********/

void InitEeprom(void);
void MainEeprom(void);
uint8_t ReadEeprom1byte(eeprom_data_id_t eeprom_data_id);
uint16_t ReadEeprom2byte(eeprom_data_id_t eeprom_data_id);
uint32_t ReadEeprom4byte(eeprom_data_id_t eeprom_data_id);
void WriteEeprom1byte(eeprom_data_id_t eeprom_data_id, uint8_t write_data);
void WriteEeprom2byte(eeprom_data_id_t eeprom_data_id, uint16_t write_data);
void WriteEeprom4byte(eeprom_data_id_t eeprom_data_id, uint32_t write_data);

#endif /* DRV_EEPROM_H_ */
