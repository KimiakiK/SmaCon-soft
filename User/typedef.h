/*
 * typedef.h
 *
 *  Created on: Apr 22, 2023
 *      Author: KimiakiK
 */


#ifndef TYPEDEF_H_
#define TYPEDEF_H_

/********** Include **********/

#include "main.h"

/********** Define **********/

/********** Enum **********/

/* Bool型 */
typedef enum {
	FALSE = 0,
	TRUE
} bool_t;

/* 結果型 */
typedef enum {
	RESULT_OK = 0,
	RESULT_NG
} result_t;

/********** Type **********/

/* コールバック関数用 関数ポインタ型 */
typedef void(* callback_t)(void);

/********** Constant **********/

/********** Variable **********/

/********** Function Prototype **********/

#endif /* TYPEDEF_H_ */
