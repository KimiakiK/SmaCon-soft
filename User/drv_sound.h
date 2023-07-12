/*
 * drv_sound.h
 *
 *  Created on: Jul 9, 2023
 *      Author: KimiakiK
 */


#ifndef DRV_SOUND_H_
#define DRV_SOUND_H_

/********** Include **********/

#include "typedef.h"

/********** Define **********/

/********** Enum **********/

typedef enum {
	SOUND_OUTPUT_SPEAKER = 0,
	SOUND_OUTPUT_LINE
} sound_output_device_t;

/********** Type **********/

/********** Constant **********/

/********** Variable **********/

/********** Function Prototype **********/

void InitSound(void);
void KeyOn(uint8_t block, uint16_t fnum);
void KeyOff(void);
void ChangeSoundOutputDevice(sound_output_device_t output_device);

#endif /* DRV_SOUND_H_ */
