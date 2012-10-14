/***********************************************************************
* Copyright (C) 2001, ZTE Corporation.
* 
* File Name:    hw_ver.h
* Description:  hw version macro
* Author:       liuzhongzhi
* Date:         2011-07-11
* 
**********************************************************************/
#ifndef __HW_VER_H
#define __HW_VER_H

/*
 * HW_VERSION() - compute hw version
 * @board   : the board num
 * @gpio    : the gpio value
 *
 * This macro compute hw_version from board and project num
 */
#define HW_VERSION(board, gpio)  ((board) << 8 | (gpio))

/*
 * BOARD_NUM() - get board num
 * @hw_ver  : the hw version
 *
 * This macro deduce board num from hw version
 */
#define BOARD_NUM(hw_ver)  ((hw_ver) >> 8)


/*
 * GPIO_VALUE() - get project num
 * @hw_ver  : the hw version
 *
 * This macro deduce project num from hw version
 */
#define GPIO_VALUE(hw_ver)  ((hw_ver) & 0xFF)

/*******************************************************
*               Define HW Board  Info                  *
*******************************************************/
#define BOARD_NUM_V71A  0
#define BOARD_NUM_V9X   1
#define BOARD_NUM_V55   2
#define BOARD_NUM_V66   3
#define BOARD_NUM_V68   4
#define BOARD_NUM_V11A  5
#define BOARD_NUM_V11   6

#define BOARD_STR_V71A  "V71A"
#define BOARD_STR_V9X   "V9X"
#define BOARD_STR_V55   "V55"
#define BOARD_STR_V66   "V66"
#define BOARD_STR_V68   "V68"
#define BOARD_STR_V11A  "V11A"
#define BOARD_STR_V11   "V11"

/*******************************************************
*               Define Project Info                    *
*******************************************************/
/* V71A add here */
#define GPIO_VALUE_V71A_A   0
#define GPIO_VALUE_V71A_B   1
#define GPIO_VALUE_V71A_C   2
#define GPIO_VALUE_V71A_D   18/** ZTE_MODIFY hezhibin add for V9S */

#define HW_VERSION_V71A_A   HW_VERSION(BOARD_NUM_V71A, GPIO_VALUE_V71A_A)
#define HW_VERSION_V71A_B   HW_VERSION(BOARD_NUM_V71A, GPIO_VALUE_V71A_B)
#define HW_VERSION_V71A_C   HW_VERSION(BOARD_NUM_V71A, GPIO_VALUE_V71A_C)
#define HW_VERSION_V71A_D   HW_VERSION(BOARD_NUM_V71A, GPIO_VALUE_V71A_D)/** ZTE_MODIFY hezhibin add for V9S */

#define VERSION_STR_V71A_A  "HW_V71A_A"
#define VERSION_STR_V71A_B  "HW_V71A_B"
#define VERSION_STR_V71A_C  "HW_V71A_C"
#define VERSION_STR_V71A_D  "HW_V71A_D"/** ZTE_MODIFY hezhibin add for V9S */


/* V9X add here */			/* V9X temp use the V71A board */
#define GPIO_VALUE_V9X_A    0	//3
#define GPIO_VALUE_V9X_B    1	//4
#define GPIO_VALUE_V9X_C    2	//5

#define HW_VERSION_V9X_A    HW_VERSION(BOARD_NUM_V9X, GPIO_VALUE_V9X_A)
#define HW_VERSION_V9X_B    HW_VERSION(BOARD_NUM_V9X, GPIO_VALUE_V9X_B)
#define HW_VERSION_V9X_C    HW_VERSION(BOARD_NUM_V9X, GPIO_VALUE_V9X_C)

#define VERSION_STR_V9X_A   "HW_V9X_A"
#define VERSION_STR_V9X_B   "HW_V9X_B"
#define VERSION_STR_V9X_C   "HW_V9X_C"

/* V55 add here */
#define GPIO_VALUE_V55_A    6
#define GPIO_VALUE_V55_B    7
#define GPIO_VALUE_V55_C    8

#define HW_VERSION_V55_A    HW_VERSION(BOARD_NUM_V55, GPIO_VALUE_V55_A)
#define HW_VERSION_V55_B    HW_VERSION(BOARD_NUM_V55, GPIO_VALUE_V55_B)
#define HW_VERSION_V55_C    HW_VERSION(BOARD_NUM_V55, GPIO_VALUE_V55_C)

#define VERSION_STR_V55_A   "HW_V55_A"
#define VERSION_STR_V55_B   "HW_V55_B"
#define VERSION_STR_V55_C   "HW_V55_C"

/* V66 add here */
#define GPIO_VALUE_V66_A    9
#define GPIO_VALUE_V66_B    10
#define GPIO_VALUE_V66_C    11
#define GPIO_VALUE_V66_D    27
#define GPIO_VALUE_V66_E    26
#define GPIO_VALUE_V66_F    25
#define GPIO_VALUE_V66_G    0
#define GPIO_VALUE_V66_H    1
#define GPIO_VALUE_V66_I    2

#define HW_VERSION_V66_A    HW_VERSION(BOARD_NUM_V66, GPIO_VALUE_V66_A)
#define HW_VERSION_V66_B    HW_VERSION(BOARD_NUM_V66, GPIO_VALUE_V66_B)
#define HW_VERSION_V66_C    HW_VERSION(BOARD_NUM_V66, GPIO_VALUE_V66_C)
#define HW_VERSION_V66_D    HW_VERSION(BOARD_NUM_V66, GPIO_VALUE_V66_D)
#define HW_VERSION_V66_E    HW_VERSION(BOARD_NUM_V66, GPIO_VALUE_V66_E)
#define HW_VERSION_V66_F    HW_VERSION(BOARD_NUM_V66, GPIO_VALUE_V66_F)
#define HW_VERSION_V66_G    HW_VERSION(BOARD_NUM_V66, GPIO_VALUE_V66_G)
#define HW_VERSION_V66_H    HW_VERSION(BOARD_NUM_V66, GPIO_VALUE_V66_H)
#define HW_VERSION_V66_I    HW_VERSION(BOARD_NUM_V66, GPIO_VALUE_V66_I)

#define VERSION_STR_V66_A   "HW_V1.1"
#define VERSION_STR_V66_B   "HW_V1.2"
#define VERSION_STR_V66_C   "HW_V1.4"
#define VERSION_STR_V66_D   "HW_V1.4"
#define VERSION_STR_V66_E   "HW_V1.5"
#define VERSION_STR_V66_F   "HW_V1.6"
#define VERSION_STR_V66_G   "HW_V1.7"
#define VERSION_STR_V66_H   "HW_V1.8"
#define VERSION_STR_V66_I   "HW_V1.9"

/* V68 add here */
#define GPIO_VALUE_V68_A    12
#define GPIO_VALUE_V68_B    13
#define GPIO_VALUE_V68_C    14
#define GPIO_VALUE_V68_D    0
#define GPIO_VALUE_V68_E    1
#define GPIO_VALUE_V68_F    2
#define GPIO_VALUE_V68_G    3
#define GPIO_VALUE_V68_H    4
#define GPIO_VALUE_V68_I    5

#define HW_VERSION_V68_A    HW_VERSION(BOARD_NUM_V68, GPIO_VALUE_V68_A)
#define HW_VERSION_V68_B    HW_VERSION(BOARD_NUM_V68, GPIO_VALUE_V68_B)
#define HW_VERSION_V68_C    HW_VERSION(BOARD_NUM_V68, GPIO_VALUE_V68_C)
#define HW_VERSION_V68_D    HW_VERSION(BOARD_NUM_V68, GPIO_VALUE_V68_D)
#define HW_VERSION_V68_E    HW_VERSION(BOARD_NUM_V68, GPIO_VALUE_V68_E)
#define HW_VERSION_V68_F    HW_VERSION(BOARD_NUM_V68, GPIO_VALUE_V68_F)
#define HW_VERSION_V68_G    HW_VERSION(BOARD_NUM_V68, GPIO_VALUE_V68_G)
#define HW_VERSION_V68_H    HW_VERSION(BOARD_NUM_V68, GPIO_VALUE_V68_H)
#define HW_VERSION_V68_I    HW_VERSION(BOARD_NUM_V68, GPIO_VALUE_V68_I)

#define VERSION_STR_V68_A   "HW_V1.0"
#define VERSION_STR_V68_B   "HW_V2.0"
#define VERSION_STR_V68_C   "HW_V2.1"
#define VERSION_STR_V68_D   "HW_V2.2"
#define VERSION_STR_V68_E   "HW_V2.3"
#define VERSION_STR_V68_F   "HW_V2.4"
#define VERSION_STR_V68_G   "HW_V2.5"
#define VERSION_STR_V68_H   "HW_V2.6"
#define VERSION_STR_V68_I   "HW_V2.7"

/* V11A add here */
#define GPIO_VALUE_V11A_A   23
#define GPIO_VALUE_V11A_B   15
#define GPIO_VALUE_V11A_C   16

#define HW_VERSION_V11A_A   HW_VERSION(BOARD_NUM_V11A, GPIO_VALUE_V11A_A)
#define HW_VERSION_V11A_B   HW_VERSION(BOARD_NUM_V11A, GPIO_VALUE_V11A_B)
#define HW_VERSION_V11A_C   HW_VERSION(BOARD_NUM_V11A, GPIO_VALUE_V11A_C)

#define VERSION_STR_V11A_A  "HW_V11A_A"
#define VERSION_STR_V11A_B  "HW_V11A_B"
#define VERSION_STR_V11A_C  "HW_V11A_C"

/* V11 add here */
#define GPIO_VALUE_V11_A    0xff
#define GPIO_VALUE_V11_B    0xfe

#define HW_VERSION_V11_A    HW_VERSION(BOARD_NUM_V11, GPIO_VALUE_V11_A)
#define HW_VERSION_V11_B    HW_VERSION(BOARD_NUM_V11, GPIO_VALUE_V11_B)

#define VERSION_STR_V11_A   "HW_V11_A"
#define VERSION_STR_V11_B   "HW_V11_B"

/*******************************************************
*               Global Variable                        *
*******************************************************/
extern int hw_ver;
#endif
