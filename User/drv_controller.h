/*
 * drv_controller.h
 *
 *  Created on: Jul 8, 2023
 *      Author: KimiakiK
 */


#ifndef DRV_CONTROLLER_H_
#define DRV_CONTROLLER_H_

/********** Include **********/

#include "typedef.h"

/********** Define **********/

/********** Enum **********/

typedef enum {
	INPUT_ID_SW_A = 0,
	INPUT_ID_SW_B,
	INPUT_ID_SW_C,
	INPUT_ID_SW_D,
	INPUT_ID_POS_UP,
	INPUT_ID_POS_DOWN,
	INPUT_ID_POS_LEFT,
	INPUT_ID_POS_RIGHT,
	INPUT_ID_LEVER_SW,
	INPUT_ID_LEVER_LEFT,
	INPUT_ID_LEVER_RIGHT,
	INPUT_ID_NUM
} input_id_t;

typedef enum {
	INPUT_OFF = 0,
	INPUT_PUSH,
	INPUT_ON,
	INPUT_RELEASE
} input_state_t;

/********** Type **********/

/********** Constant **********/

/********** Variable **********/

/********** Function Prototype **********/

void InitController(void);
void MainController(void);
void UpdateSwInput(void);
input_state_t GetInputState(input_id_t input_id);

#endif /* DRV_CONTROLLER_H_ */
