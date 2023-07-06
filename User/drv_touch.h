/*
 * drv_touch.h
 *
 *  Created on: Jun 30, 2023
 *      Author: KimiakiK
 */


#ifndef DRV_TOUCH_H_
#define DRV_TOUCH_H_

/********** Include **********/

#include "typedef.h"

/********** Define **********/

/********** Enum **********/

typedef enum {
	TOUCH_OFF = 0,
	TOUCH_START,
	TOUCH_ON,
	TOUCH_END
} touch_state_t;

/********** Type **********/

/********** Constant **********/

/********** Variable **********/

/********** Function Prototype **********/

void InitTouch(void);
void MainTouch(void);
touch_state_t GetTouchState(void);
point_t GetTouchPoint(void);

#endif /* DRV_TOUCH_H_ */
