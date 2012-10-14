/******************************************************************************
  @file    mt9d115.c
  @brief   The camera mt9d115 driver
  @author  liyibo 2011/03/22

  DESCRIPTION
  The camera mt9d115 driver, include probe func, setting many kinds of modes
  (preview/snapshot) and effect setting.

  ---------------------------------------------------------------------------
  Copyright (c) 2011 ZTE Incorporated. All Rights Reserved. 
  ZTE Proprietary and Confidential.
  ---------------------------------------------------------------------------
******************************************************************************/

/*===========================================================================

                        EDIT HISTORY FOR V11

when              comment tag        who                  what, where, why                           
----------    ------------     -----------      --------------------------      
2011/04/14    liyibo0002          liyibo            modify for mt9d115 driver preview dead
2011/04/15    liyibo0003          liyibo            added sensor angle and modify from BACK_CAMERA_2D TO FRONT_CAMERA_2D
2011/05/31	  liyibo0004          liyibo            added sensor mirror
2011/06/09	  liyibo0005          liyibo            added sensor vflip
===========================================================================*/

#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <media/msm_camera.h>
#include <mach/gpio.h>
#include <mach/camera.h>
#include "mt9d115.h"

//#define CONFIG_MT9D115_DEBUG
//#define CONFIG_MT9D115_INFO
#ifdef CONFIG_MT9D115_DEBUG
#define log_debug(fmt, args...) printk(fmt, ##args)
#else
#define log_debug(fmt, args...) do { } while (0)
#endif
#ifdef CONFIG_MT9D115_INFO
#define logs_info(fmt, args...) printk(fmt, ##args)
#else
#define logs_info(fmt, args...) do { } while (0)
#endif
#define logs_err(fmt, args...) printk(fmt, ##args)
/*=============================================================
    SENSOR REGISTER DEFINES
==============================================================*/
#define Q8    0x00000100

/* Omnivision8810 product ID register address */
#define REG_mt9d115_MODEL_ID_MSB                       0x0000

#define mt9d115_MODEL_ID                       0x2580
/* Omnivision8810 product ID */

/* Time in milisecs for waiting for the sensor to reset */
#define mt9d115_RESET_DELAY_MSECS    66
#if 1
#define mt9d115_DEFAULT_CLOCK_RATE   24000000
#else
#define mt9d115_DEFAULT_CLOCK_RATE   12000000
#endif
/* Registers*/

/* Color bar pattern selection */
#define mt9d115_COLOR_BAR_PATTERN_SEL_REG     0x82
/* Color bar enabling control */
#define mt9d115_COLOR_BAR_ENABLE_REG           0x601
/* Time in milisecs for waiting for the sensor to reset*/
#define mt9d115_RESET_DELAY_MSECS    66
/*============================================================================
                            DATA DECLARATIONS
============================================================================*/
/*  96MHz PCLK @ 24MHz MCLK */

struct reg_addr_val_pair_struct camera_contrast_neg_2[] = { 
    {0x098C, 0xAB3C},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
    {0x0990, 0x0000},     // MCU_DATA_0
    {0x098C, 0xAB3D},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
    {0x0990, 0x0018},     // MCU_DATA_0
    {0x098C, 0xAB3E},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
    {0x0990, 0x0041},     // MCU_DATA_0
    {0x098C, 0xAB3F},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
    {0x0990, 0x0064},     // MCU_DATA_0
    {0x098C, 0xAB40},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
    {0x0990, 0x0083},     // MCU_DATA_0
    {0x098C, 0xAB41},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
    {0x0990, 0x0096},     // MCU_DATA_0
    {0x098C, 0xAB42},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
    {0x0990, 0x00A5},     // MCU_DATA_0
    {0x098C, 0xAB43},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
    {0x0990, 0x00B1},     // MCU_DATA_0
    {0x098C, 0xAB44},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
    {0x0990, 0x00BC},     // MCU_DATA_0
    {0x098C, 0xAB45},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
    {0x0990, 0x00C5},     // MCU_DATA_0
    {0x098C, 0xAB46},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
    {0x0990, 0x00CE},     // MCU_DATA_0
    {0x098C, 0xAB47},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
    {0x0990, 0x00D6},     // MCU_DATA_0
    {0x098C, 0xAB48},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
    {0x0990, 0x00DD},     // MCU_DATA_0
    {0x098C, 0xAB49},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
    {0x0990, 0x00E3},     // MCU_DATA_0
    {0x098C, 0xAB4A},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
    {0x0990, 0x00E9},     // MCU_DATA_0
    {0x098C, 0xAB4B},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
    {0x0990, 0x00EF},     // MCU_DATA_0
    {0x098C, 0xAB4C},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
    {0x0990, 0x00F5},     // MCU_DATA_0
    {0x098C, 0xAB4D},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
    {0x0990, 0x00FA},     // MCU_DATA_0
    {0x098C, 0xAB4E},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
    {0x0990, 0x00FF},     // MCU_DATA_0
    {0x098C, 0xAB4F},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_0]
    {0x0990, 0x0000},     // MCU_DATA_0
    {0x098C, 0xAB50},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_1]
    {0x0990, 0x0018},     // MCU_DATA_0
    {0x098C, 0xAB51},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_2]
    {0x0990, 0x0041},     // MCU_DATA_0
    {0x098C, 0xAB52},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_3]
    {0x0990, 0x0064},     // MCU_DATA_0
    {0x098C, 0xAB53},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_4]
    {0x0990, 0x0083},     // MCU_DATA_0
    {0x098C, 0xAB54},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_5]
    {0x0990, 0x0096},     // MCU_DATA_0
    {0x098C, 0xAB55},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_6]
    {0x0990, 0x00A5},     // MCU_DATA_0
    {0x098C, 0xAB56},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_7]
    {0x0990, 0x00B1},     // MCU_DATA_0
    {0x098C, 0xAB57},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_8]
    {0x0990, 0x00BC},     // MCU_DATA_0
    {0x098C, 0xAB58},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_9]
    {0x0990, 0x00C5},     // MCU_DATA_0
    {0x098C, 0xAB59},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_10]
    {0x0990, 0x00CE},     // MCU_DATA_0
    {0x098C, 0xAB5A},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_11]
    {0x0990, 0x00D6},     // MCU_DATA_0
    {0x098C, 0xAB5B},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_12]
    {0x0990, 0x00DD},     // MCU_DATA_0
    {0x098C, 0xAB5C},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_13]
    {0x0990, 0x00E3},     // MCU_DATA_0
    {0x098C, 0xAB5D},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_14]
    {0x0990, 0x00E9},     // MCU_DATA_0
    {0x098C, 0xAB5E},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_15]
    {0x0990, 0x00EF},     // MCU_DATA_0
    {0x098C, 0xAB5F},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_16]
    {0x0990, 0x00F5},     // MCU_DATA_0
    {0x098C, 0xAB60},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_17]
    {0x0990, 0x00FA},     // MCU_DATA_0
    {0x098C, 0xAB61},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_18]
    {0x0990, 0x00FF},     // MCU_DATA_0
};
struct reg_addr_val_pair_struct camera_contrast_neg_1[] = { 
    {0x098C, 0xAB3C},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
    {0x0990, 0x0000},     // MCU_DATA_0
    {0x098C, 0xAB3D},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
    {0x0990, 0x0010},     // MCU_DATA_0
    {0x098C, 0xAB3E},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
    {0x0990, 0x002E},     // MCU_DATA_0
    {0x098C, 0xAB3F},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
    {0x0990, 0x004F},     // MCU_DATA_0
    {0x098C, 0xAB40},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
    {0x0990, 0x0072},     // MCU_DATA_0
    {0x098C, 0xAB41},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
    {0x0990, 0x008C},     // MCU_DATA_0
    {0x098C, 0xAB42},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
    {0x0990, 0x009F},     // MCU_DATA_0
    {0x098C, 0xAB43},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
    {0x0990, 0x00AD},     // MCU_DATA_0
    {0x098C, 0xAB44},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
    {0x0990, 0x00BA},     // MCU_DATA_0
    {0x098C, 0xAB45},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
    {0x0990, 0x00C4},     // MCU_DATA_0
    {0x098C, 0xAB46},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
    {0x0990, 0x00CE},     // MCU_DATA_0
    {0x098C, 0xAB47},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
    {0x0990, 0x00D6},     // MCU_DATA_0
    {0x098C, 0xAB48},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
    {0x0990, 0x00DD},     // MCU_DATA_0
    {0x098C, 0xAB49},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
    {0x0990, 0x00E4},     // MCU_DATA_0
    {0x098C, 0xAB4A},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
    {0x0990, 0x00EA},     // MCU_DATA_0
    {0x098C, 0xAB4B},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
    {0x0990, 0x00F0},     // MCU_DATA_0
    {0x098C, 0xAB4C},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
    {0x0990, 0x00F5},     // MCU_DATA_0
    {0x098C, 0xAB4D},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
    {0x0990, 0x00FA},     // MCU_DATA_0
    {0x098C, 0xAB4E},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
    {0x0990, 0x00FF},     // MCU_DATA_0
    {0x098C, 0xAB4F},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_0]
    {0x0990, 0x0000},     // MCU_DATA_0
    {0x098C, 0xAB50},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_1]
    {0x0990, 0x0010},     // MCU_DATA_0
    {0x098C, 0xAB51},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_2]
    {0x0990, 0x002E},     // MCU_DATA_0
    {0x098C, 0xAB52},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_3]
    {0x0990, 0x004F},     // MCU_DATA_0
    {0x098C, 0xAB53},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_4]
    {0x0990, 0x0072},     // MCU_DATA_0
    {0x098C, 0xAB54},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_5]
    {0x0990, 0x008C},     // MCU_DATA_0
    {0x098C, 0xAB55},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_6]
    {0x0990, 0x009F},     // MCU_DATA_0
    {0x098C, 0xAB56},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_7]
    {0x0990, 0x00AD},     // MCU_DATA_0
    {0x098C, 0xAB57},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_8]
    {0x0990, 0x00BA},     // MCU_DATA_0
    {0x098C, 0xAB58},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_9]
    {0x0990, 0x00C4},     // MCU_DATA_0
    {0x098C, 0xAB59},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_10]
    {0x0990, 0x00CE},     // MCU_DATA_0
    {0x098C, 0xAB5A},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_11]
    {0x0990, 0x00D6},     // MCU_DATA_0
    {0x098C, 0xAB5B},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_12]
    {0x0990, 0x00DD},     // MCU_DATA_0
    {0x098C, 0xAB5C},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_13]
    {0x0990, 0x00E4},     // MCU_DATA_0
    {0x098C, 0xAB5D},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_14]
    {0x0990, 0x00EA},     // MCU_DATA_0
    {0x098C, 0xAB5E},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_15]
    {0x0990, 0x00F0},     // MCU_DATA_0
    {0x098C, 0xAB5F},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_16]
    {0x0990, 0x00F5},     // MCU_DATA_0
    {0x098C, 0xAB60},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_17]
    {0x0990, 0x00FA},     // MCU_DATA_0
    {0x098C, 0xAB61},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_18]
    {0x0990, 0x00FF},     // MCU_DATA_0
};
struct reg_addr_val_pair_struct camera_contrast_zero[] = { 

    {0x098C, 0xAB4F},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_0]
    {0x0990, 0x0000},     // MCU_DATA_0
    {0x098C, 0xAB50},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_1]
    {0x0990, 0x000C},     // MCU_DATA_0
    {0x098C, 0xAB51},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_2]
    {0x0990, 0x0022},     // MCU_DATA_0
    {0x098C, 0xAB52},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_3]
    {0x0990, 0x003F},     // MCU_DATA_0
    {0x098C, 0xAB53},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_4]
    {0x0990, 0x0062},     // MCU_DATA_0
    {0x098C, 0xAB54},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_5]
    {0x0990, 0x007D},     // MCU_DATA_0
    {0x098C, 0xAB55},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_6]
    {0x0990, 0x0093},     // MCU_DATA_0
    {0x098C, 0xAB56},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_7]
    {0x0990, 0x00A5},     // MCU_DATA_0
    {0x098C, 0xAB57},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_8]
    {0x0990, 0x00B3},     // MCU_DATA_0
    {0x098C, 0xAB58},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_9]
    {0x0990, 0x00BF},     // MCU_DATA_0
    {0x098C, 0xAB59},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_10]
    {0x0990, 0x00C9},     // MCU_DATA_0
    {0x098C, 0xAB5A},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_11]
    {0x0990, 0x00D3},     // MCU_DATA_0
    {0x098C, 0xAB5B},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_12]
    {0x0990, 0x00DB},     // MCU_DATA_0
    {0x098C, 0xAB5C},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_13]
    {0x0990, 0x00E2},     // MCU_DATA_0
    {0x098C, 0xAB5D},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_14]
    {0x0990, 0x00E9},     // MCU_DATA_0
    {0x098C, 0xAB5E},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_15]
    {0x0990, 0x00EF},     // MCU_DATA_0
    {0x098C, 0xAB5F},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_16]
    {0x0990, 0x00F5},     // MCU_DATA_0
    {0x098C, 0xAB60},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_17]
    {0x0990, 0x00FA},     // MCU_DATA_0
    {0x098C, 0xAB61},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_18]
    {0x0990, 0x00FF},     // MCU_DATA_0
    {0x098C, 0xAB3C},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
    {0x0990, 0x0000},     // MCU_DATA_0
    {0x098C, 0xAB3D},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
    {0x0990, 0x000C},     // MCU_DATA_0
    {0x098C, 0xAB3E},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
    {0x0990, 0x0022},     // MCU_DATA_0
    {0x098C, 0xAB3F},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
    {0x0990, 0x003F},     // MCU_DATA_0
    {0x098C, 0xAB40},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
    {0x0990, 0x0062},     // MCU_DATA_0
    {0x098C, 0xAB41},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
    {0x0990, 0x007D},     // MCU_DATA_0
    {0x098C, 0xAB42},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
    {0x0990, 0x0093},     // MCU_DATA_0
    {0x098C, 0xAB43},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
    {0x0990, 0x00A5},     // MCU_DATA_0
    {0x098C, 0xAB44},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
    {0x0990, 0x00B3},     // MCU_DATA_0
    {0x098C, 0xAB45},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
    {0x0990, 0x00BF},     // MCU_DATA_0
    {0x098C, 0xAB46},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
    {0x0990, 0x00C9},     // MCU_DATA_0
    {0x098C, 0xAB47},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
    {0x0990, 0x00D3},     // MCU_DATA_0
    {0x098C, 0xAB48},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
    {0x0990, 0x00DB},     // MCU_DATA_0
    {0x098C, 0xAB49},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
    {0x0990, 0x00E2},     // MCU_DATA_0
    {0x098C, 0xAB4A},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
    {0x0990, 0x00E9},     // MCU_DATA_0
    {0x098C, 0xAB4B},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
    {0x0990, 0x00EF},     // MCU_DATA_0
    {0x098C, 0xAB4C},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
    {0x0990, 0x00F5},     // MCU_DATA_0
    {0x098C, 0xAB4D},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
    {0x0990, 0x00FA},     // MCU_DATA_0
    {0x098C, 0xAB4E},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
    {0x0990, 0x00FF},     // MCU_DATA_0
};
struct reg_addr_val_pair_struct camera_contrast_posi_1[] = { 
    {0x098C, 0xAB3C},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
    {0x0990, 0x0000},     // MCU_DATA_0
    {0x098C, 0xAB3D},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
    {0x0990, 0x0005},     // MCU_DATA_0
    {0x098C, 0xAB3E},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
    {0x0990, 0x0010},     // MCU_DATA_0
    {0x098C, 0xAB3F},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
    {0x0990, 0x0029},     // MCU_DATA_0
    {0x098C, 0xAB40},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
    {0x0990, 0x0049},     // MCU_DATA_0
    {0x098C, 0xAB41},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
    {0x0990, 0x0062},     // MCU_DATA_0
    {0x098C, 0xAB42},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
    {0x0990, 0x0078},     // MCU_DATA_0
    {0x098C, 0xAB43},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
    {0x0990, 0x008D},     // MCU_DATA_0
    {0x098C, 0xAB44},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
    {0x0990, 0x009E},     // MCU_DATA_0
    {0x098C, 0xAB45},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
    {0x0990, 0x00AD},     // MCU_DATA_0
    {0x098C, 0xAB46},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
    {0x0990, 0x00BA},     // MCU_DATA_0
    {0x098C, 0xAB47},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
    {0x0990, 0x00C6},     // MCU_DATA_0
    {0x098C, 0xAB48},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
    {0x0990, 0x00D0},     // MCU_DATA_0
    {0x098C, 0xAB49},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
    {0x0990, 0x00DA},     // MCU_DATA_0
    {0x098C, 0xAB4A},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
    {0x0990, 0x00E2},     // MCU_DATA_0
    {0x098C, 0xAB4B},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
    {0x0990, 0x00EA},     // MCU_DATA_0
    {0x098C, 0xAB4C},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
    {0x0990, 0x00F2},     // MCU_DATA_0
    {0x098C, 0xAB4D},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
    {0x0990, 0x00F9},     // MCU_DATA_0
    {0x098C, 0xAB4E},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
    {0x0990, 0x00FF},     // MCU_DATA_0
    {0x098C, 0xAB4F},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_0]
    {0x0990, 0x0000},     // MCU_DATA_0
    {0x098C, 0xAB50},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_1]
    {0x0990, 0x0005},     // MCU_DATA_0
    {0x098C, 0xAB51},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_2]
    {0x0990, 0x0010},     // MCU_DATA_0
    {0x098C, 0xAB52},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_3]
    {0x0990, 0x0029},     // MCU_DATA_0
    {0x098C, 0xAB53},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_4]
    {0x0990, 0x0049},     // MCU_DATA_0
    {0x098C, 0xAB54},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_5]
    {0x0990, 0x0062},     // MCU_DATA_0
    {0x098C, 0xAB55},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_6]
    {0x0990, 0x0078},     // MCU_DATA_0
    {0x098C, 0xAB56},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_7]
    {0x0990, 0x008D},     // MCU_DATA_0
    {0x098C, 0xAB57},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_8]
    {0x0990, 0x009E},     // MCU_DATA_0
    {0x098C, 0xAB58},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_9]
    {0x0990, 0x00AD},     // MCU_DATA_0
    {0x098C, 0xAB59},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_10]
    {0x0990, 0x00BA},     // MCU_DATA_0
    {0x098C, 0xAB5A},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_11]
    {0x0990, 0x00C6},     // MCU_DATA_0
    {0x098C, 0xAB5B},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_12]
    {0x0990, 0x00D0},     // MCU_DATA_0
    {0x098C, 0xAB5C},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_13]
    {0x0990, 0x00DA},     // MCU_DATA_0
    {0x098C, 0xAB5D},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_14]
    {0x0990, 0x00E2},     // MCU_DATA_0
    {0x098C, 0xAB5E},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_15]
    {0x0990, 0x00EA},     // MCU_DATA_0
    {0x098C, 0xAB5F},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_16]
    {0x0990, 0x00F2},     // MCU_DATA_0
    {0x098C, 0xAB60},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_17]
    {0x0990, 0x00F9},     // MCU_DATA_0
    {0x098C, 0xAB61},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_18]
    {0x0990, 0x00FF},     // MCU_DATA_0
};
struct reg_addr_val_pair_struct camera_contrast_posi_2[] = {
    {0x098C, 0xAB3C},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_0]
    {0x0990, 0x0000},     // MCU_DATA_0
    {0x098C, 0xAB3D},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_1]
    {0x0990, 0x0003},     // MCU_DATA_0
    {0x098C, 0xAB3E},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_2]
    {0x0990, 0x000A},     // MCU_DATA_0
    {0x098C, 0xAB3F},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_3]
    {0x0990, 0x001C},     // MCU_DATA_0
    {0x098C, 0xAB40},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_4]
    {0x0990, 0x0036},     // MCU_DATA_0
    {0x098C, 0xAB41},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_5]
    {0x0990, 0x004D},     // MCU_DATA_0
    {0x098C, 0xAB42},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_6]
    {0x0990, 0x0063},     // MCU_DATA_0
    {0x098C, 0xAB43},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_7]
    {0x0990, 0x0078},     // MCU_DATA_0
    {0x098C, 0xAB44},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_8]
    {0x0990, 0x008D},     // MCU_DATA_0
    {0x098C, 0xAB45},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_9]
    {0x0990, 0x009E},     // MCU_DATA_0
    {0x098C, 0xAB46},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_10]
    {0x0990, 0x00AE},     // MCU_DATA_0
    {0x098C, 0xAB47},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_11]
    {0x0990, 0x00BC},     // MCU_DATA_0
    {0x098C, 0xAB48},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_12]
    {0x0990, 0x00C8},     // MCU_DATA_0
    {0x098C, 0xAB49},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_13]
    {0x0990, 0x00D3},     // MCU_DATA_0
    {0x098C, 0xAB4A},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_14]
    {0x0990, 0x00DD},     // MCU_DATA_0
    {0x098C, 0xAB4B},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_15]
    {0x0990, 0x00E7},     // MCU_DATA_0
    {0x098C, 0xAB4C},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_16]
    {0x0990, 0x00EF},     // MCU_DATA_0
    {0x098C, 0xAB4D},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_17]
    {0x0990, 0x00F7},     // MCU_DATA_0
    {0x098C, 0xAB4E},     // MCU_ADDRESS [HG_GAMMA_TABLE_A_18]
    {0x0990, 0x00FF},     // MCU_DATA_0
    {0x098C, 0xAB4F},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_0]
    {0x0990, 0x0000},     // MCU_DATA_0
    {0x098C, 0xAB50},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_1]
    {0x0990, 0x0003},     // MCU_DATA_0
    {0x098C, 0xAB51},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_2]
    {0x0990, 0x000A},     // MCU_DATA_0
    {0x098C, 0xAB52},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_3]
    {0x0990, 0x001C},     // MCU_DATA_0
    {0x098C, 0xAB53},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_4]
    {0x0990, 0x0036},     // MCU_DATA_0
    {0x098C, 0xAB54},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_5]
    {0x0990, 0x004D},     // MCU_DATA_0
    {0x098C, 0xAB55},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_6]
    {0x0990, 0x0063},     // MCU_DATA_0
    {0x098C, 0xAB56},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_7]
    {0x0990, 0x0078},     // MCU_DATA_0
    {0x098C, 0xAB57},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_8]
    {0x0990, 0x008D},     // MCU_DATA_0
    {0x098C, 0xAB58},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_9]
    {0x0990, 0x009E},     // MCU_DATA_0
    {0x098C, 0xAB59},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_10]
    {0x0990, 0x00AE},     // MCU_DATA_0
    {0x098C, 0xAB5A},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_11]
    {0x0990, 0x00BC},     // MCU_DATA_0
    {0x098C, 0xAB5B},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_12]
    {0x0990, 0x00C8},     // MCU_DATA_0
    {0x098C, 0xAB5C},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_13]
    {0x0990, 0x00D3},     // MCU_DATA_0
    {0x098C, 0xAB5D},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_14]
    {0x0990, 0x00DD},     // MCU_DATA_0
    {0x098C, 0xAB5E},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_15]
    {0x0990, 0x00E7},     // MCU_DATA_0
    {0x098C, 0xAB5F},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_16]
    {0x0990, 0x00EF},     // MCU_DATA_0
    {0x098C, 0xAB60},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_17]
    {0x0990, 0x00F7},     // MCU_DATA_0
    {0x098C, 0xAB61},     // MCU_ADDRESS [HG_GAMMA_TABLE_B_18]
    {0x0990, 0x00FF},     // MCU_DATA_0
};
#define    MT9D115_MAX_BRIGHT     7
#define    MT9D115_BRIGHT_REG_NUM    2
struct reg_addr_val_pair_struct mt9d115_camera_bright_list[MT9D115_MAX_BRIGHT][MT9D115_BRIGHT_REG_NUM] = {
    {{0x098C, 0xA24F},{0x0990, 0x0016}},            //bright -3
    {{0x098C, 0xA24F},{0x0990, 0x0026}},            //bright -2
    {{0x098C, 0xA24F},{0x0990, 0x0036}},            //bright -1
    {{0x098C, 0xA24F},{0x0990, 0x0046}},            //bright 0
    {{0x098C, 0xA24F},{0x0990, 0x0056}},            //bright 1
    {{0x098C, 0xA24F},{0x0990, 0x0066}},            //bright 2
    {{0x098C, 0xA24F},{0x0990, 0x006F}}            //bright 3
};

#define    MT9D115_MAX_SATURATION     5
#define    MT9D115_SATURATION_REG_NUM    2
struct reg_addr_val_pair_struct mt9d115_camera_saturation_list[MT9D115_MAX_SATURATION][MT9D115_SATURATION_REG_NUM] = {
    {{0x098C, 0xAB20},{0x0990, 0x003e}},              // saturation -2
    {{0x098C, 0xAB20},{0x0990, 0x004e}},        // saturation -1
    {{0x098C, 0xAB20},{0x0990, 0x005e}},        // saturation 0
    {{0x098C, 0xAB20},{0x0990, 0x006e}},        // saturation +1
    {{0x098C, 0xAB20},{0x0990, 0x007e}}         // saturation +2
};

#define    MT9D115_MAX_SHARPNESS     5
#define    MT9D115_SHARPNESS_REG_NUM    2
struct reg_addr_val_pair_struct mt9d115_camera_sharpness_list[MT9D115_MAX_SHARPNESS][MT9D115_SHARPNESS_REG_NUM] = {
    {{0x098C, 0xAB22},{0x0990, 0x0007}},            //sharpness 2
    {{0x098C, 0xAB22},{0x0990, 0x0006}},             //sharpness 1
    {{0x098C, 0xAB22},{0x0990, 0x0005}},             //sharpness 0         
    {{0x098C, 0xAB22},{0x0990, 0x0003}},            //sharpness -1
    {{0x098C, 0xAB22},{0x0990, 0x0001}}        //sharpness -2
};

struct reg_addr_val_pair_struct camera_balance_cloudy[] = {
    {0x098C, 0xA34A},      // MCU_ADDRESS [AWB_GAIN_MIN]
    {0x0990, 0x00D0},      // MCU_DATA_0
    {0x098C, 0xA34B},      // MCU_ADDRESS [AWB_GAIN_MAX]
    {0x0990, 0x00D0},      // MCU_DATA_0
    {0x098C, 0xA34C},      // MCU_ADDRESS [AWB_GAINMIN_B]
    {0x0990, 0x0056},      // MCU_DATA_0
    {0x098C, 0xA34D},      // MCU_ADDRESS [AWB_GAINMAX_B]
    {0x0990, 0x0056},      // MCU_DATA_0
    {0x098C, 0xA351},      // MCU_ADDRESS [AWB_CCM_POSITION_MIN]
    {0x0990, 0x007f},      // MCU_DATA_0
    {0x098C, 0xA352},      // MCU_ADDRESS [AWB_CCM_POSITION_MAX]
    {0x0990, 0x007F},      // MCU_DATA_0
    {0x098C, 0xA103},      // MCU_ADDRESS [SEQ_CMD]
    {0x0990, 0x0005},      // MCU_DATA_0
};

struct reg_addr_val_pair_struct camera_balance_daylight[] = {
    {0x098C, 0xA34A},      // MCU_ADDRESS [AWB_GAIN_MIN]
    {0x0990, 0x00C2},             // MCU_DATA_0
    {0x098C, 0xA34B},      // MCU_ADDRESS [AWB_GAIN_MAX]
    {0x0990, 0x00c4},      // MCU_DATA_0
    {0x098C, 0xA34C},      // MCU_ADDRESS [AWB_GAINMIN_B]
    {0x0990, 0x005d},      // MCU_DATA_0
    {0x098C, 0xA34D},      // MCU_ADDRESS [AWB_GAINMAX_B]
    {0x0990, 0x005f},      // MCU_DATA_0
    {0x098C, 0xA351},      // MCU_ADDRESS [AWB_CCM_POSITION_MIN]
    {0x0990, 0x007f},      // MCU_DATA_0
    {0x098C, 0xA352},      // MCU_ADDRESS [AWB_CCM_POSITION_MAX]
    {0x0990, 0x007F},      // MCU_DATA_0
    {0x098C, 0xA103},      // MCU_ADDRESS [SEQ_CMD]
    {0x0990, 0x0005},      // MCU_DATA_0
};
struct reg_addr_val_pair_struct camera_balance_flourescant[] = {
    {0x098C, 0xA34A},      // MCU_ADDRESS [AWB_GAIN_MIN]
    {0x0990, 0x0080},      // MCU_DATA_0
    {0x098C, 0xA34B},      // MCU_ADDRESS [AWB_GAIN_MAX]
    {0x0990, 0x0080},      // MCU_DATA_0
    {0x098C, 0xA34C},      // MCU_ADDRESS [AWB_GAINMIN_B]
    {0x0990, 0x00c4},      // MCU_DATA_0
    {0x098C, 0xA34D},      // MCU_ADDRESS [AWB_GAINMAX_B]
    {0x0990, 0x00c4},      // MCU_DATA_0
    {0x098C, 0xA351},      // MCU_ADDRESS [AWB_CCM_POSITION_MIN]
    {0x0990, 0x0000},      // MCU_DATA_0
    {0x098C, 0xA352},      // MCU_ADDRESS [AWB_CCM_POSITION_MAX]
    {0x0990, 0x0000},      // MCU_DATA_0
    {0x098C, 0xA103},      // MCU_ADDRESS [SEQ_CMD]
    {0x0990, 0x0005},      // MCU_DATA_0
};
struct reg_addr_val_pair_struct camera_balance_U30[] = {                                                                                                                                                                                     
    {0x098C, 0xA34A},     // MCU_ADDRESS [AWB_GAIN_MIN]          
    {0x0990, 0x0099},     // MCU_DATA_0                          
    {0x098C, 0xA34B},     // MCU_ADDRESS [AWB_GAIN_MAX]          
    {0x0990, 0x0099},     // MCU_DATA_0                          
    {0x098C, 0xA34C},     // MCU_ADDRESS [AWB_GAINMIN_B]         
    {0x0990, 0x0080},     // MCU_DATA_0                          
    {0x098C, 0xA34D},     // MCU_ADDRESS [AWB_GAINMAX_B]         
    {0x0990, 0x0080},     // MCU_DATA_0                          
    {0x098C, 0xA351},     // MCU_ADDRESS [AWB_CCM_POSITION_MIN]  
    {0x0990, 0x0048},     // MCU_DATA_0                          
    {0x098C, 0xA352},     // MCU_ADDRESS [AWB_CCM_POSITION_MAX]  
    {0x0990, 0x0048},     // MCU_DATA_0                                                                                        
    {0x098C, 0xA103},     // MCU_ADDRESS [SEQ_CMD]               
    {0x0990, 0x0005},     // MCU_DATA_0                                                                                       
}; 
struct reg_addr_val_pair_struct camera_balance_CWF[] = {                                                                                                                                                                                       
    {0x098C, 0xA34A},     // MCU_ADDRESS [AWB_GAIN_MIN]          
    {0x0990, 0x00AE},     // MCU_DATA_0                          
    {0x098C, 0xA34B},     // MCU_ADDRESS [AWB_GAIN_MAX]          
    {0x0990, 0x00AE},     // MCU_DATA_0                          
    {0x098C, 0xA34C},     // MCU_ADDRESS [AWB_GAINMIN_B]         
    {0x0990, 0x007F},     // MCU_DATA_0                          
    {0x098C, 0xA34D},     // MCU_ADDRESS [AWB_GAINMAX_B]         
    {0x0990, 0x007F},     // MCU_DATA_0                          
    {0x098C, 0xA351},     // MCU_ADDRESS [AWB_CCM_POSITION_MIN]  
    {0x0990, 0x0038},     // MCU_DATA_0                          
    {0x098C, 0xA352},     // MCU_ADDRESS [AWB_CCM_POSITION_MAX]  
    {0x0990, 0x0038},     // MCU_DATA_0                                                                                            
    {0x098C, 0xA103},     // MCU_ADDRESS [SEQ_CMD]               
    {0x0990, 0x0005},     // MCU_DATA_0                          
};
struct reg_addr_val_pair_struct camera_balance_incandescent[] = {   
    {0x098C, 0xA34A},      // MCU_ADDRESS [AWB_GAIN_MIN]
    {0x0990, 0x0086},      // MCU_DATA_0
    {0x098C, 0xA34B},      // MCU_ADDRESS [AWB_GAIN_MAX]
    {0x0990, 0x0088},      // MCU_DATA_0
    {0x098C, 0xA34C},      // MCU_ADDRESS [AWB_GAINMIN_B]
    {0x0990, 0x0092},      // MCU_DATA_0
    {0x098C, 0xA34D},      // MCU_ADDRESS [AWB_GAINMAX_B]
    {0x0990, 0x0094},      // MCU_DATA_0
    {0x098C, 0xA351},      // MCU_ADDRESS [AWB_CCM_POSITION_MIN]
    {0x0990, 0x0000},      // MCU_DATA_0
    {0x098C, 0xA352},      // MCU_ADDRESS [AWB_CCM_POSITION_MAX]
    {0x0990, 0x0000},      // MCU_DATA_0
    {0x098C, 0xA103},      // MCU_ADDRESS [SEQ_CMD]
    {0x0990, 0x0005},      // MCU_DATA_0
};
struct reg_addr_val_pair_struct camera_balance_auto[] = { 
    {0x098C, 0xA34A},      // MCU_ADDRESS [AWB_GAIN_MIN]
    {0x0990, 0x0059},      // MCU_DATA_0
    {0x098C, 0xA34B},      // MCU_ADDRESS [AWB_GAIN_MAX]
    {0x0990, 0x00C8},      // MCU_DATA_0
    {0x098C, 0xA34C},      // MCU_ADDRESS [AWB_GAINMIN_B]
    {0x0990, 0x0059},      // MCU_DATA_0
    {0x098C, 0xA34D},      // MCU_ADDRESS [AWB_GAINMAX_B]
    {0x0990, 0x00A6},      // MCU_DATA_0
    {0x098C, 0xA351},      // MCU_ADDRESS [AWB_CCM_POSITION_MIN]
    {0x0990, 0x0000},      // MCU_DATA_0
    {0x098C, 0xA352},      // MCU_ADDRESS [AWB_CCM_POSITION_MAX]
    {0x0990, 0x007F},      // MCU_DATA_0
    {0x098C, 0xA103},      // MCU_ADDRESS [SEQ_CMD]
    {0x0990, 0x0005},      // MCU_DATA_0
};

struct reg_addr_val_pair_struct camera_effect_off_capture[] = {
    {0x098C, 0x275B},         
    {0x0990, 0x6440},             
    {0x098C, 0x275B},             
    {0x0990, 0x6440},             
    {0x098C, 0xA103},             
    {0x0990, 0x0005},
};

struct reg_addr_val_pair_struct camera_effect_mono_capture[] = {
    {0x098C, 0x275B},             
    {0x0990, 0x6441},            
    {0x098C, 0x275B},            
    {0x0990, 0x6441},            
    {0x098C, 0xA103},           
    {0x0990, 0x0005},
};

struct reg_addr_val_pair_struct camera_effect_negative_capture[] = {
    {0x098C, 0x275B},           
    {0x0990, 0x6443},          
    {0x098C, 0x275B},           
    {0x0990, 0x6443},           
    {0x098C, 0xA103},           
    {0x0990, 0x0005},
};
  
struct reg_addr_val_pair_struct camera_effect_solarize_capture[] = {
    {0x098C, 0x275B},          
    {0x0990, 0x6444},   //加深 0X6445       
    {0x098C, 0x275B},           
    {0x0990, 0x6444},   //加深 0X6445        
    {0x098C, 0xA103},          
    {0x0990, 0x0005},
};

struct reg_addr_val_pair_struct camera_effect_sepia_capture[] = {
    {0x098C, 0x2763},     // MCU_ADDRESS [MODE_COMMONMODESETTINGS_FX_SEPIA_SETTINGS]
    {0x0990, 0xB023},     // MCU_DATA_0 
      
    {0x098C, 0x275B},           
    {0x0990, 0x6442},            
    {0x098C, 0x275B},           
    {0x0990, 0x6442},            
    {0x098C, 0xA103},            
    {0x0990, 0x0005},
};

struct reg_addr_val_pair_struct camera_effect_Green_capture[] = {
    { 0x098C, 0x2763},     // MCU_ADDRESS [MODE_COMMONMODESETTINGS_FX_SEPIA_SETTINGS]
    { 0x0990, 0xDBDB},     // MCU_DATA_0

    { 0x098C, 0x275B},     // MCU_ADDRESS [MODE_SPEC_EFFECTS_A]
    { 0x0990, 0x6442},     // MCU_DATA_0
    { 0x098C, 0x275B},     // MCU_ADDRESS [MODE_SPEC_EFFECTS_B]
    { 0x0990, 0x6442},     // MCU_DATA_0
    { 0x098C, 0xA103},     // MCU_ADDRESS [SEQ_CMD]
    { 0x0990, 0x0005},     // MCU_DATA_0
};

struct reg_addr_val_pair_struct camera_effect_off[] = {
    {0x098C, 0x2759},         
    {0x0990, 0x6440},             
    {0x098C, 0x275B},             
    {0x0990, 0x6440},             
    {0x098C, 0xA103},             
    {0x0990, 0x0005},
};

struct reg_addr_val_pair_struct camera_effect_mono[] = {
    {0x098C, 0x2759},             
    {0x0990, 0x6441},            
    {0x098C, 0x275B},            
    {0x0990, 0x6441},            
    {0x098C, 0xA103},           
    {0x0990, 0x0005},
};

struct reg_addr_val_pair_struct camera_effect_negative[] = {
    {0x098C, 0x2759},           
    {0x0990, 0x6443},          
    {0x098C, 0x275B},           
    {0x0990, 0x6443},           
    {0x098C, 0xA103},           
    {0x0990, 0x0005},
};
  
struct reg_addr_val_pair_struct camera_effect_solarize[] = {
    {0x098C, 0x2759},          
    {0x0990, 0x6444},   //加深 0X6445       
    {0x098C, 0x275B},           
    {0x0990, 0x6444},   //加深 0X6445        
    {0x098C, 0xA103},          
    {0x0990, 0x0005},
};

struct reg_addr_val_pair_struct camera_effect_sepia[] = {
    {0x098C, 0x2763},     // MCU_ADDRESS [MODE_COMMONMODESETTINGS_FX_SEPIA_SETTINGS]
    {0x0990, 0xB023},     // MCU_DATA_0 
      
    {0x098C, 0x2759},           
    {0x0990, 0x6442},            
    {0x098C, 0x275B},           
    {0x0990, 0x6442},            
    {0x098C, 0xA103},            
    {0x0990, 0x0005},
};

struct reg_addr_val_pair_struct camera_effect_Green[] = {
    { 0x098C, 0x2763},     // MCU_ADDRESS [MODE_COMMONMODESETTINGS_FX_SEPIA_SETTINGS]
    { 0x0990, 0xDBDB},     // MCU_DATA_0

    { 0x098C, 0x2759},     // MCU_ADDRESS [MODE_SPEC_EFFECTS_A]
    { 0x0990, 0x6442},     // MCU_DATA_0
    { 0x098C, 0x275B},     // MCU_ADDRESS [MODE_SPEC_EFFECTS_B]
    { 0x0990, 0x6442},     // MCU_DATA_0
    { 0x098C, 0xA103},     // MCU_ADDRESS [SEQ_CMD]
    { 0x0990, 0x0005},     // MCU_DATA_0
};

struct reg_addr_val_pair_struct capture_mode_setting[] = {
    {0x098C, 0xA115},     // MCU_ADDRESS [SEQ_CAP_MODE]
    {0x0990, 0x0002},     // MCU_DATA_0
    {0x098C, 0xA103},     // MCU_ADDRESS [SEQ_CMD]
    {0x0990, 0x0002},     // MCU_DATA_0
};
struct reg_addr_val_pair_struct preview_mode_setting[] = {
    {0x098C, 0xA115},     // MCU_ADDRESS [SEQ_CAP_MODE]
    {0x0990, 0x0000},     // MCU_DATA_0
    {0x098C, 0xA103},     // MCU_ADDRESS [SEQ_CMD]
    {0x0990, 0x0001},     // MCU_DATA_0
};
uint16_t reset_sensor_setting[] = {
    0x0051,     // RESET_AND_MISC_CONTROL
    0x0050,     // RESET_AND_MISC_CONTROL
    0x0058,     // RESET_AND_MISC_CONTROL
};

struct reg_addr_val_pair_struct mt9d115_pll_settings[] = {
    {0x0014, 0x21F9}, 	// PLL_CONTROL
    {0x0010, 0x0115}, 	// PLL_DIVIDERS
    {0x0012, 0x00F5}, 	// PLL_P_DIVIDERS
    {0x0014, 0x2545}, 	// PLL_CONTROL
    {0x0014, 0x2547}, 	// PLL_CONTROL
    {0x0014, 0x2447}, 	// PLL_CONTROL
};
struct reg_addr_val_pair_struct mt9d115_settings[] = {
    {0x0014, 0x2047}, 	// PLL_CONTROL
    {0x0014, 0x2046}, 	// PLL_CONTROL
    {0x0018, 0x402D}, 	// STANDBY_CONTROL
    {0x0018, 0x402C}, 	// STANDBY_CONTROL
};

struct reg_addr_val_pair_struct mt9d115_init_preview_array[] = {
//800x600
    {0x098C, 0x2703}, 	// MCU_ADDRESS [MODE_OUTPUT_WIDTH_A]
    {0x0990, 0x0320}, 	// MCU_DATA_0
    {0x098C, 0x2705}, 	// MCU_ADDRESS [MODE_OUTPUT_HEIGHT_A]
    {0x0990, 0x0258}, 	// MCU_DATA_0

//1024x768
//    {0x098C, 0x2703}, 	// MCU_ADDRESS [MODE_OUTPUT_WIDTH_A]
//    {0x0990, 0x0400}, 	// MCU_DATA_0
//    {0x098C, 0x2705}, 	// MCU_ADDRESS [MODE_OUTPUT_HEIGHT_A]
//    {0x0990, 0x0300}, 	// MCU_DATA_0

//640x480
//    {0x098C, 0x2703}, 	// MCU_ADDRESS [MODE_OUTPUT_WIDTH_A]
//    {0x0990, 0x0280}, 	// MCU_DATA_0
//    {0x098C, 0x2705}, 	// MCU_ADDRESS [MODE_OUTPUT_HEIGHT_A]
//    {0x0990, 0x01E0}, 	// MCU_DATA_0

    {0x098C, 0x2707}, 	// MCU_ADDRESS [MODE_OUTPUT_WIDTH_B]
    {0x0990, 0x0640}, 	// MCU_DATA_0
    {0x098C, 0x2709}, 	// MCU_ADDRESS [MODE_OUTPUT_HEIGHT_B]
    {0x0990, 0x04B0}, 	// MCU_DATA_0
    {0x098C, 0x270D}, 	// MCU_ADDRESS [MODE_SENSOR_ROW_START_A]
    {0x0990, 0x0000}, 	// MCU_DATA_0
    {0x098C, 0x270F}, 	// MCU_ADDRESS [MODE_SENSOR_COL_START_A]
    {0x0990, 0x0000}, 	// MCU_DATA_0
    {0x098C, 0x2711}, 	// MCU_ADDRESS [MODE_SENSOR_ROW_END_A]
    {0x0990, 0x04BD}, 	// MCU_DATA_0
    {0x098C, 0x2713}, 	// MCU_ADDRESS [MODE_SENSOR_COL_END_A]
    {0x0990, 0x064D}, 	// MCU_DATA_0
    {0x098C, 0x2715}, 	// MCU_ADDRESS [MODE_SENSOR_ROW_SPEED_A]
    {0x0990, 0x0111}, 	// MCU_DATA_0
    {0x098C, 0x2717}, 	// MCU_ADDRESS [MODE_SENSOR_READ_MODE_A]
    {0x0990, 0x046C}, 	// MCU_DATA_0
    {0x098C, 0x2719}, 	// MCU_ADDRESS [MODE_SENSOR_FINE_CORRECTION_A]
    {0x0990, 0x005A}, 	// MCU_DATA_0
    {0x098C, 0x271B}, 	// MCU_ADDRESS [MODE_SENSOR_FINE_IT_MIN_A]
    {0x0990, 0x01BE}, 	// MCU_DATA_0
    {0x098C, 0x271D}, 	// MCU_ADDRESS [MODE_SENSOR_FINE_IT_MAX_MARGIN_A]
    {0x0990, 0x0131}, 	// MCU_DATA_0
    {0x098C, 0x271F}, 	// MCU_ADDRESS [MODE_SENSOR_FRAME_LENGTH_A]
    {0x0990, 0x02BB}, 	// MCU_DATA_0
    {0x098C, 0x2721}, 	// MCU_ADDRESS [MODE_SENSOR_LINE_LENGTH_PCK_A]
    {0x0990, 0x0888}, 	// MCU_DATA_0
    {0x098C, 0x2723}, 	// MCU_ADDRESS [MODE_SENSOR_ROW_START_B]
    {0x0990, 0x0004}, 	// MCU_DATA_0
    {0x098C, 0x2725}, 	// MCU_ADDRESS [MODE_SENSOR_COL_START_B]
    {0x0990, 0x0004}, 	// MCU_DATA_0
    {0x098C, 0x2727}, 	// MCU_ADDRESS [MODE_SENSOR_ROW_END_B]
    {0x0990, 0x04BB}, 	// MCU_DATA_0
    {0x098C, 0x2729}, 	// MCU_ADDRESS [MODE_SENSOR_COL_END_B]
    {0x0990, 0x064B}, 	// MCU_DATA_0
    {0x098C, 0x272B}, 	// MCU_ADDRESS [MODE_SENSOR_ROW_SPEED_B]
    {0x0990, 0x0111}, 	// MCU_DATA_0
    {0x098C, 0x272D}, 	// MCU_ADDRESS [MODE_SENSOR_READ_MODE_B]
    {0x0990, 0x0024}, 	// MCU_DATA_0
    {0x098C, 0x272F}, 	// MCU_ADDRESS [MODE_SENSOR_FINE_CORRECTION_B]
    {0x0990, 0x003A}, 	// MCU_DATA_0
    {0x098C, 0x2731}, 	// MCU_ADDRESS [MODE_SENSOR_FINE_IT_MIN_B]
    {0x0990, 0x00F6}, 	// MCU_DATA_0
    {0x098C, 0x2733}, 	// MCU_ADDRESS [MODE_SENSOR_FINE_IT_MAX_MARGIN_B]
    {0x0990, 0x008B}, 	// MCU_DATA_0
    {0x098C, 0x2735}, 	// MCU_ADDRESS [MODE_SENSOR_FRAME_LENGTH_B]
    {0x0990, 0x0521}, 	// MCU_DATA_0
    {0x098C, 0x2737}, 	// MCU_ADDRESS [MODE_SENSOR_LINE_LENGTH_PCK_B]
    {0x0990, 0x0888}, 	// MCU_DATA_0
    {0x098C, 0x2739}, 	// MCU_ADDRESS [MODE_CROP_X0_A]
    {0x0990, 0x0000}, 	// MCU_DATA_0
    {0x098C, 0x273B}, 	// MCU_ADDRESS [MODE_CROP_X1_A]
    {0x0990, 0x031F}, 	// MCU_DATA_0
    {0x098C, 0x273D}, 	// MCU_ADDRESS [MODE_CROP_Y0_A]
    {0x0990, 0x0000}, 	// MCU_DATA_0
    {0x098C, 0x273F}, 	// MCU_ADDRESS [MODE_CROP_Y1_A]
    {0x0990, 0x0257}, 	// MCU_DATA_0
    {0x098C, 0x2747}, 	// MCU_ADDRESS [MODE_CROP_X0_B]
    {0x0990, 0x0000}, 	// MCU_DATA_0
    {0x098C, 0x2749}, 	// MCU_ADDRESS [MODE_CROP_X1_B]
    {0x0990, 0x063F}, 	// MCU_DATA_0
    {0x098C, 0x274B}, 	// MCU_ADDRESS [MODE_CROP_Y0_B]
    {0x0990, 0x0000}, 	// MCU_DATA_0
    {0x098C, 0x274D}, 	// MCU_ADDRESS [MODE_CROP_Y1_B]
    {0x0990, 0x04AF}, 	// MCU_DATA_0
    {0x098C, 0x2222}, 	// MCU_ADDRESS [AE_R9]
    {0x0990, 0x00A0}, 	// MCU_DATA_0
    {0x098C, 0xA408}, 	// MCU_ADDRESS [FD_SEARCH_F1_50]
    {0x0990, 0x0026}, 	// MCU_DATA_0
    {0x098C, 0xA409}, 	// MCU_ADDRESS [FD_SEARCH_F2_50]
    {0x0990, 0x0029}, 	// MCU_DATA_0
    {0x098C, 0xA40A}, 	// MCU_ADDRESS [FD_SEARCH_F1_60]
    {0x0990, 0x002E}, 	// MCU_DATA_0
    {0x098C, 0xA40B}, 	// MCU_ADDRESS [FD_SEARCH_F2_60]
    {0x0990, 0x0031}, 	// MCU_DATA_0
    {0x098C, 0x2411}, 	// MCU_ADDRESS [FD_R9_STEP_F60_A]
    {0x0990, 0x00A0}, 	// MCU_DATA_0
    {0x098C, 0x2413}, 	// MCU_ADDRESS [FD_R9_STEP_F50_A]
    {0x0990, 0x00C0}, 	// MCU_DATA_0
    {0x098C, 0x2415}, 	// MCU_ADDRESS [FD_R9_STEP_F60_B]
    {0x0990, 0x00A0}, 	// MCU_DATA_0
    {0x098C, 0x2417}, 	// MCU_ADDRESS [FD_R9_STEP_F50_B]
    {0x0990, 0x00C0}, 	// MCU_DATA_0
    {0x098C, 0xA404}, 	// MCU_ADDRESS [FD_MODE]
    {0x0990, 0x0010}, 	// MCU_DATA_0
    {0x098C, 0xA40D}, 	// MCU_ADDRESS [FD_STAT_MIN]
    {0x0990, 0x0002}, 	// MCU_DATA_0
    {0x098C, 0xA40E}, 	// MCU_ADDRESS [FD_STAT_MAX]
    {0x0990, 0x0003}, 	// MCU_DATA_0
    {0x098C, 0xA410}, 	// MCU_ADDRESS [FD_MIN_AMPLITUDE]
    {0x0990, 0x000A}, 	// MCU_DATA_0
    {0x098C, 0xA117}, 	// MCU_ADDRESS [SEQ_PREVIEW_0_AE]
    {0x0990, 0x0002}, 	// MCU_DATA_0
    {0x098C, 0xA11D}, 	// MCU_ADDRESS [SEQ_PREVIEW_1_AE]
    {0x0990, 0x0002}, 	// MCU_DATA_0
    {0x098C, 0xA129}, 	// MCU_ADDRESS [SEQ_PREVIEW_3_AE]
    {0x0990, 0x0002}, 	// MCU_DATA_0
    {0x098C, 0xA24F}, 	// MCU_ADDRESS [AE_BASETARGET]
    {0x0990, 0x0032}, 	// MCU_DATA_0
    {0x098C, 0xA20C}, 	// MCU_ADDRESS [AE_MAX_INDEX]
    {0x0990, 0x0010}, 	// MCU_DATA_0
    {0x098C, 0xA216}, 	// MCU_ADDRESS
    {0x0990, 0x0091}, 	// MCU_DATA_0
    {0x098C, 0xA20E}, 	// MCU_ADDRESS [AE_MAX_VIRTGAIN]
    {0x0990, 0x0091}, 	// MCU_DATA_0
    {0x098C, 0x2212}, 	// MCU_ADDRESS [AE_MAX_DGAIN_AE1]
    {0x0990, 0x00A4}, 	// MCU_DATA_0
    {0x3210, 0x01B8}, 	// COLOR_PIPELINE_CONTROL
    {0x098C, 0xAB36}, 	// MCU_ADDRESS [HG_CLUSTERDC_TH]
    {0x0990, 0x0014}, 	// MCU_DATA_0
    {0x098C, 0x2B66}, 	// MCU_ADDRESS [HG_CLUSTER_DC_BM]
    {0x0990, 0x2AF8}, 	// MCU_DATA_0
    {0x098C, 0xAB20}, 	// MCU_ADDRESS [HG_LL_SAT1]
    {0x0990, 0x0080}, 	// MCU_DATA_0
    {0x098C, 0xAB24}, 	// MCU_ADDRESS [HG_LL_SAT2]
    {0x0990, 0x0000}, 	// MCU_DATA_0
    {0x098C, 0xAB21}, 	// MCU_ADDRESS [HG_LL_INTERPTHRESH1]
    {0x0990, 0x000A}, 	// MCU_DATA_0
    {0x098C, 0xAB25}, 	// MCU_ADDRESS [HG_LL_INTERPTHRESH2]
    {0x0990, 0x002A}, 	// MCU_DATA_0
    {0x098C, 0xAB22}, 	// MCU_ADDRESS [HG_LL_APCORR1]
    {0x0990, 0x0007}, 	// MCU_DATA_0
    {0x098C, 0xAB26}, 	// MCU_ADDRESS [HG_LL_APCORR2]
    {0x0990, 0x0001}, 	// MCU_DATA_0
    {0x098C, 0xAB23}, 	// MCU_ADDRESS [HG_LL_APTHRESH1]
    {0x0990, 0x0004}, 	// MCU_DATA_0
    {0x098C, 0xAB27}, 	// MCU_ADDRESS [HG_LL_APTHRESH2]
    {0x0990, 0x0009}, 	// MCU_DATA_0
    {0x098C, 0x2B28}, 	// MCU_ADDRESS [HG_LL_BRIGHTNESSSTART]
    {0x0990, 0x0BB8}, 	// MCU_DATA_0
    {0x098C, 0x2B2A}, 	// MCU_ADDRESS [HG_LL_BRIGHTNESSSTOP]
    {0x0990, 0x2968}, 	// MCU_DATA_0
    {0x098C, 0xAB2C}, 	// MCU_ADDRESS [HG_NR_START_R]
    {0x0990, 0x00FF}, 	// MCU_DATA_0
    {0x098C, 0xAB30}, 	// MCU_ADDRESS [HG_NR_STOP_R]
    {0x0990, 0x00FF}, 	// MCU_DATA_0
    {0x098C, 0xAB2D}, 	// MCU_ADDRESS [HG_NR_START_G]
    {0x0990, 0x00FF}, 	// MCU_DATA_0
    {0x098C, 0xAB31}, 	// MCU_ADDRESS [HG_NR_STOP_G]
    {0x0990, 0x00FF}, 	// MCU_DATA_0
    {0x098C, 0xAB2E}, 	// MCU_ADDRESS [HG_NR_START_B]
    {0x0990, 0x00FF}, 	// MCU_DATA_0
    {0x098C, 0xAB32}, 	// MCU_ADDRESS [HG_NR_STOP_B]
    {0x0990, 0x00FF}, 	// MCU_DATA_0
    {0x098C, 0xAB2F}, 	// MCU_ADDRESS [HG_NR_START_OL]
    {0x0990, 0x000A}, 	// MCU_DATA_0
    {0x098C, 0xAB33}, 	// MCU_ADDRESS [HG_NR_STOP_OL]
    {0x0990, 0x0006}, 	// MCU_DATA_0
    {0x098C, 0xAB34}, 	// MCU_ADDRESS [HG_NR_GAINSTART]
    {0x0990, 0x0020}, 	// MCU_DATA_0
    {0x098C, 0xAB35}, 	// MCU_ADDRESS [HG_NR_GAINSTOP]
    {0x0990, 0x0091}, 	// MCU_DATA_0
    {0x098C, 0xA765}, 	// MCU_ADDRESS [MODE_COMMONMODESETTINGS_FILTER_MODE]
    {0x0990, 0x0006}, 	// MCU_DATA_0
    {0x098C, 0xAB37}, 	// MCU_ADDRESS [HG_GAMMA_MORPH_CTRL]
    {0x0990, 0x0003}, 	// MCU_DATA_0
    {0x098C, 0x2B38}, 	// MCU_ADDRESS [HG_GAMMASTARTMORPH]
    {0x0990, 0x2968}, 	// MCU_DATA_0
    {0x098C, 0x2B3A}, 	// MCU_ADDRESS [HG_GAMMASTOPMORPH]
    {0x0990, 0x2D50}, 	// MCU_DATA_0
    {0x098C, 0x2B62}, 	// MCU_ADDRESS [HG_FTB_START_BM]
    {0x0990, 0xFFFE}, 	// MCU_DATA_0
    {0x098C, 0x2B64}, 	// MCU_ADDRESS [HG_FTB_STOP_BM]
    {0x0990, 0xFFFF}, 	// MCU_DATA_0
    {0x098C, 0xAB4F}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_0]
    {0x0990, 0x0000}, 	// MCU_DATA_0
    {0x098C, 0xAB50}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_1]
    {0x0990, 0x0013}, 	// MCU_DATA_0
    {0x098C, 0xAB51}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_2]
    {0x0990, 0x0027}, 	// MCU_DATA_0
    {0x098C, 0xAB52}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_3]
    {0x0990, 0x0043}, 	// MCU_DATA_0
    {0x098C, 0xAB53}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_4]
    {0x0990, 0x0068}, 	// MCU_DATA_0
    {0x098C, 0xAB54}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_5]
    {0x0990, 0x0081}, 	// MCU_DATA_0
    {0x098C, 0xAB55}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_6]
    {0x0990, 0x0093}, 	// MCU_DATA_0
    {0x098C, 0xAB56}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_7]
    {0x0990, 0x00A3}, 	// MCU_DATA_0
    {0x098C, 0xAB57}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_8]
    {0x0990, 0x00B0}, 	// MCU_DATA_0
    {0x098C, 0xAB58}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_9]
    {0x0990, 0x00BC}, 	// MCU_DATA_0
    {0x098C, 0xAB59}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_10]
    {0x0990, 0x00C7}, 	// MCU_DATA_0
    {0x098C, 0xAB5A}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_11]
    {0x0990, 0x00D1}, 	// MCU_DATA_0
    {0x098C, 0xAB5B}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_12]
    {0x0990, 0x00DA}, 	// MCU_DATA_0
    {0x098C, 0xAB5C}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_13]
    {0x0990, 0x00E2}, 	// MCU_DATA_0
    {0x098C, 0xAB5D}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_14]
    {0x0990, 0x00E9}, 	// MCU_DATA_0
    {0x098C, 0xAB5E}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_15]
    {0x0990, 0x00EF}, 	// MCU_DATA_0
    {0x098C, 0xAB5F}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_16]
    {0x0990, 0x00F4}, 	// MCU_DATA_0
    {0x098C, 0xAB60}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_17]
    {0x0990, 0x00FA}, 	// MCU_DATA_0
    {0x098C, 0xAB61}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_18]
    {0x0990, 0x00FF}, 	// MCU_DATA_0
    {0x098C, 0x2306}, 	// MCU_ADDRESS [AWB_CCM_L_0]
    {0x0990, 0x01D6}, 	// MCU_DATA_0
    {0x098C, 0x2308}, 	// MCU_ADDRESS [AWB_CCM_L_1]
    {0x0990, 0xFF89}, 	// MCU_DATA_0
    {0x098C, 0x230A}, 	// MCU_ADDRESS [AWB_CCM_L_2]
    {0x0990, 0xFFA1}, 	// MCU_DATA_0
    {0x098C, 0x230C}, 	// MCU_ADDRESS [AWB_CCM_L_3]
    {0x0990, 0xFF73}, 	// MCU_DATA_0
    {0x098C, 0x230E}, 	// MCU_ADDRESS [AWB_CCM_L_4]
    {0x0990, 0x019C}, 	// MCU_DATA_0
    {0x098C, 0x2310}, 	// MCU_ADDRESS [AWB_CCM_L_5]
    {0x0990, 0xFFF1}, 	// MCU_DATA_0
    {0x098C, 0x2312}, 	// MCU_ADDRESS [AWB_CCM_L_6]
    {0x0990, 0xFFB0}, 	// MCU_DATA_0
    {0x098C, 0x2314}, 	// MCU_ADDRESS [AWB_CCM_L_7]
    {0x0990, 0xFF2D}, 	// MCU_DATA_0
    {0x098C, 0x2316}, 	// MCU_ADDRESS [AWB_CCM_L_8]
    {0x0990, 0x0223}, 	// MCU_DATA_0
    {0x098C, 0x2318}, 	// MCU_ADDRESS [AWB_CCM_L_9]
    {0x0990, 0x001C}, 	// MCU_DATA_0
    {0x098C, 0x231A}, 	// MCU_ADDRESS [AWB_CCM_L_10]
    {0x0990, 0x0048}, 	// MCU_DATA_0
    {0x098C, 0x2318}, 	// MCU_ADDRESS [AWB_CCM_L_9]
    {0x0990, 0x001C}, 	// MCU_DATA_0
    {0x098C, 0x231A}, 	// MCU_ADDRESS [AWB_CCM_L_10]
    {0x0990, 0x0038}, 	// MCU_DATA_0
    {0x098C, 0x2318}, 	// MCU_ADDRESS [AWB_CCM_L_9]
    {0x0990, 0x001E}, 	// MCU_DATA_0
    {0x098C, 0x231A}, 	// MCU_ADDRESS [AWB_CCM_L_10]
    {0x0990, 0x0038}, 	// MCU_DATA_0
    {0x098C, 0x2318}, 	// MCU_ADDRESS [AWB_CCM_L_9]
    {0x0990, 0x0022}, 	// MCU_DATA_0
    {0x098C, 0x231A}, 	// MCU_ADDRESS [AWB_CCM_L_10]
    {0x0990, 0x0038}, 	// MCU_DATA_0
    {0x098C, 0x2318}, 	// MCU_ADDRESS [AWB_CCM_L_9]
    {0x0990, 0x002C}, 	// MCU_DATA_0
    {0x098C, 0x231A}, 	// MCU_ADDRESS [AWB_CCM_L_10]
    {0x0990, 0x0038}, 	// MCU_DATA_0
    {0x098C, 0x2318}, 	// MCU_ADDRESS [AWB_CCM_L_9]
    {0x0990, 0x0024}, 	// MCU_DATA_0
    {0x098C, 0x231A}, 	// MCU_ADDRESS [AWB_CCM_L_10]
    {0x0990, 0x0038}, 	// MCU_DATA_0
    {0x098C, 0x231C}, 	// MCU_ADDRESS [AWB_CCM_RL_0]
    {0x0990, 0xFFCD}, 	// MCU_DATA_0
    {0x098C, 0x231E}, 	// MCU_ADDRESS [AWB_CCM_RL_1]
    {0x0990, 0x0023}, 	// MCU_DATA_0
    {0x098C, 0x2320}, 	// MCU_ADDRESS [AWB_CCM_RL_2]
    {0x0990, 0x0010}, 	// MCU_DATA_0
    {0x098C, 0x2322}, 	// MCU_ADDRESS [AWB_CCM_RL_3]
    {0x0990, 0x0026}, 	// MCU_DATA_0
    {0x098C, 0x2324}, 	// MCU_ADDRESS [AWB_CCM_RL_4]
    {0x0990, 0xFFE9}, 	// MCU_DATA_0
    {0x098C, 0x2326}, 	// MCU_ADDRESS [AWB_CCM_RL_5]
    {0x0990, 0xFFF1}, 	// MCU_DATA_0
    {0x098C, 0x2328}, 	// MCU_ADDRESS [AWB_CCM_RL_6]
    {0x0990, 0x003A}, 	// MCU_DATA_0
    {0x098C, 0x232A}, 	// MCU_ADDRESS [AWB_CCM_RL_7]
    {0x0990, 0x005D}, 	// MCU_DATA_0
    {0x098C, 0x232C}, 	// MCU_ADDRESS [AWB_CCM_RL_8]
    {0x0990, 0xFF69}, 	// MCU_DATA_0
    {0x098C, 0x232E}, 	// MCU_ADDRESS [AWB_CCM_RL_9]
    {0x0990, 0x000C}, 	// MCU_DATA_0
    {0x098C, 0x2330}, 	// MCU_ADDRESS [AWB_CCM_RL_10]
    {0x0990, 0xFFE4}, 	// MCU_DATA_0
    {0x098C, 0x232E}, 	// MCU_ADDRESS [AWB_CCM_RL_9]
    {0x0990, 0x000C}, 	// MCU_DATA_0
    {0x098C, 0x2330}, 	// MCU_ADDRESS [AWB_CCM_RL_10]
    {0x0990, 0xFFF4}, 	// MCU_DATA_0
    {0x098C, 0x232E}, 	// MCU_ADDRESS [AWB_CCM_RL_9]
    {0x0990, 0x000A}, 	// MCU_DATA_0
    {0x098C, 0x2330}, 	// MCU_ADDRESS [AWB_CCM_RL_10]
    {0x0990, 0xFFF4}, 	// MCU_DATA_0
    {0x098C, 0x232E}, 	// MCU_ADDRESS [AWB_CCM_RL_9]
    {0x0990, 0x0006}, 	// MCU_DATA_0
    {0x098C, 0x2330}, 	// MCU_ADDRESS [AWB_CCM_RL_10]
    {0x0990, 0xFFF4}, 	// MCU_DATA_0
    {0x098C, 0x232E}, 	// MCU_ADDRESS [AWB_CCM_RL_9]
    {0x0990, 0xFFFC}, 	// MCU_DATA_0
    {0x098C, 0x2330}, 	// MCU_ADDRESS [AWB_CCM_RL_10]
    {0x0990, 0xFFF4}, 	// MCU_DATA_0
    {0x098C, 0x232E}, 	// MCU_ADDRESS [AWB_CCM_RL_9]
    {0x0990, 0x0004}, 	// MCU_DATA_0
    {0x098C, 0x2330}, 	// MCU_ADDRESS [AWB_CCM_RL_10]
    {0x0990, 0xFFF4}, 	// MCU_DATA_0
    {0x098C, 0x0415}, 	// MCU_ADDRESS
    {0x0990, 0xF601},
    {0x0992, 0x42C1},
    {0x0994, 0x0326},
    {0x0996, 0x11F6},
    {0x0998, 0x0143},
    {0x099A, 0xC104},
    {0x099C, 0x260A},
    {0x099E, 0xCC04},
    {0x098C, 0x0425},     // MCU_ADDRESS
    {0x0990, 0x33BD},
    {0x0992, 0xA362},
    {0x0994, 0xBD04},
    {0x0996, 0x3339},
    {0x0998, 0xC6FF},
    {0x099A, 0xF701},
    {0x099C, 0x6439},
    {0x099E, 0xFE01},
    {0x098C, 0x0435},     // MCU_ADDRESS
    {0x0990, 0x6918},
    {0x0992, 0xCE03},
    {0x0994, 0x25CC},
    {0x0996, 0x0013},
    {0x0998, 0xBDC2},
    {0x099A, 0xB8CC},
    {0x099C, 0x0489},
    {0x099E, 0xFD03},
    {0x098C, 0x0445},     // MCU_ADDRESS
    {0x0990, 0x27CC},
    {0x0992, 0x0325},
    {0x0994, 0xFD01},
    {0x0996, 0x69FE},
    {0x0998, 0x02BD},
    {0x099A, 0x18CE},
    {0x099C, 0x0339},
    {0x099E, 0xCC00},
    {0x098C, 0x0455},     // MCU_ADDRESS
    {0x0990, 0x11BD},
    {0x0992, 0xC2B8},
    {0x0994, 0xCC04},
    {0x0996, 0xC8FD},
    {0x0998, 0x0347},
    {0x099A, 0xCC03},
    {0x099C, 0x39FD},
    {0x099E, 0x02BD},
    {0x098C, 0x0465},     // MCU_ADDRESS
    {0x0990, 0xDE00},
    {0x0992, 0x18CE},
    {0x0994, 0x00C2},
    {0x0996, 0xCC00},
    {0x0998, 0x37BD},
    {0x099A, 0xC2B8},
    {0x099C, 0xCC04},
    {0x099E, 0xEFDD},
    {0x098C, 0x0475},     // MCU_ADDRESS
    {0x0990, 0xE6CC},
    {0x0992, 0x00C2},
    {0x0994, 0xDD00},
    {0x0996, 0xC601},
    {0x0998, 0xF701},
    {0x099A, 0x64C6},
    {0x099C, 0x03F7},
    {0x099E, 0x0165},
    {0x098C, 0x0485},     // MCU_ADDRESS
    {0x0990, 0x7F01},
    {0x0992, 0x6639},
    {0x0994, 0x3C3C},
    {0x0996, 0x3C34},
    {0x0998, 0xCC32},
    {0x099A, 0x3EBD},
    {0x099C, 0xA558},
    {0x099E, 0x30ED},
    {0x098C, 0x0495},     // MCU_ADDRESS
    {0x0990, 0x04BD},
    {0x0992, 0xB2D7},
    {0x0994, 0x30E7},
    {0x0996, 0x06CC},
    {0x0998, 0x323E},
    {0x099A, 0xED00},
    {0x099C, 0xEC04},
    {0x099E, 0xBDA5},
    {0x098C, 0x04A5},     // MCU_ADDRESS
    {0x0990, 0x44CC},
    {0x0992, 0x3244},
    {0x0994, 0xBDA5},
    {0x0996, 0x585F},
    {0x0998, 0x30ED},
    {0x099A, 0x02CC},
    {0x099C, 0x3244},
    {0x099E, 0xED00},
    {0x098C, 0x04B5},     // MCU_ADDRESS
    {0x0990, 0xF601},
    {0x0992, 0xD54F},
    {0x0994, 0xEA03},
    {0x0996, 0xAA02},
    {0x0998, 0xBDA5},
    {0x099A, 0x4430},
    {0x099C, 0xE606},
    {0x099E, 0x3838},
    {0x098C, 0x04C5},     // MCU_ADDRESS
    {0x0990, 0x3831},
    {0x0992, 0x39BD},
    {0x0994, 0xD661},
    {0x0996, 0xF602},
    {0x0998, 0xF4C1},
    {0x099A, 0x0126},
    {0x099C, 0x0BFE},
    {0x099E, 0x02BD},
    {0x098C, 0x04D5},     // MCU_ADDRESS
    {0x0990, 0xEE10},
    {0x0992, 0xFC02},
    {0x0994, 0xF5AD},
    {0x0996, 0x0039},
    {0x0998, 0xF602},
    {0x099A, 0xF4C1},
    {0x099C, 0x0226},
    {0x099E, 0x0AFE},
    {0x098C, 0x04E5},     // MCU_ADDRESS
    {0x0990, 0x02BD},
    {0x0992, 0xEE10},
    {0x0994, 0xFC02},
    {0x0996, 0xF7AD},
    {0x0998, 0x0039},
    {0x099A, 0x3CBD},
    {0x099C, 0xB059},
    {0x099E, 0xCC00},
    {0x098C, 0x04F5},     // MCU_ADDRESS
    {0x0990, 0x28BD},
    {0x0992, 0xA558},
    {0x0994, 0x8300},
    {0x0996, 0x0027},
    {0x0998, 0x0BCC},
    {0x099A, 0x0026},
    {0x099C, 0x30ED},
    {0x099E, 0x00C6},
    {0x098C, 0x0505},     // MCU_ADDRESS
    {0x0990, 0x03BD},
    {0x0992, 0xA544},
    {0x0994, 0x3839},
    {0x098C, 0x2006},     // MCU_ADDRESS [MON_ARG1]
    {0x0990, 0x0415},     // MCU_DATA_0
    {0x098C, 0xA005},     // MCU_ADDRESS [MON_CMD]
    {0x0990, 0x0001},     // MCU_DATA_0    
    
    {0x098C, 0x2755},     // MCU_ADDRESS [MODE_OUTPUT_FORMAT_A]
    {0x0990, 0x0002},     // MCU_DATA_0
    {0x098C, 0x2757},     // MCU_ADDRESS [MODE_OUTPUT_FORMAT_B]
    {0x0990, 0x0002},     // MCU_DATA_0
//liyibo0005 : added by liyibo for flip at 2011-06-9 
    #ifdef CONFIG_CAMERA_MT9D115_MIRROR_ZTE
    {0x098C, 0x2717},
    {0x0990, 0x046D},
    {0x098C, 0x272D},
    {0x0990, 0x0025},
    {0x098C, 0xA103},
    {0x0990, 0x0006},
    #endif 
    #ifdef CONFIG_CAMERA_MT9D115_FLIP_ZTE 
    {0x098C, 0x2717},
    {0x0990, 0x046E},
    {0x098C, 0x272D},
    {0x0990, 0x0026},
    {0x098C, 0xA103},
    {0x0990, 0x0006},
    #endif
    #ifdef CONFIG_CAMERA_MT9D115_FLIP_MIRROR_ZTE
    {0x098C, 0x2717},
    {0x0990, 0x046F},
    {0x098C, 0x272D},
    {0x0990, 0x0027},
    {0x098C, 0xA103},
    {0x0990, 0x0006},
    #endif
// added by liyibo end at 2011-06-9 
                    
};

//#define _LEN_CORRECTION_85_
//#define _LEN_CORRECTION_90_
#define _LEN_CORRECTION_100_
struct reg_addr_val_pair_struct mt9d115_init_fine_tune[] = {
    {0x098C, 0x2306}, 	// MCU_ADDRESS [AWB_CCM_L_0]
    {0x0990, 0x0163}, 	// MCU_DATA_0
    {0x098C, 0x231C}, 	// MCU_ADDRESS [AWB_CCM_RL_0]
    {0x0990, 0x0040}, 	// MCU_DATA_0
    {0x098C, 0x2308}, 	// MCU_ADDRESS [AWB_CCM_L_1]
    {0x0990, 0xFFD8}, 	// MCU_DATA_0
    {0x098C, 0x231E}, 	// MCU_ADDRESS [AWB_CCM_RL_1]
    {0x0990, 0xFFD4}, 	// MCU_DATA_0
    {0x098C, 0x230A}, 	// MCU_ADDRESS [AWB_CCM_L_2]
    {0x0990, 0xFFC5}, 	// MCU_DATA_0
    {0x098C, 0x2320}, 	// MCU_ADDRESS [AWB_CCM_RL_2]
    {0x0990, 0xFFEC}, 	// MCU_DATA_0
    {0x098C, 0x230C}, 	// MCU_ADDRESS [AWB_CCM_L_3]
    {0x0990, 0xFFB2}, 	// MCU_DATA_0
    {0x098C, 0x2322}, 	// MCU_ADDRESS [AWB_CCM_RL_3]
    {0x0990, 0xFFE7}, 	// MCU_DATA_0
    {0x098C, 0x230E}, 	// MCU_ADDRESS [AWB_CCM_L_4]
    {0x0990, 0x0150}, 	// MCU_DATA_0
    {0x098C, 0x2324}, 	// MCU_ADDRESS [AWB_CCM_RL_4]
    {0x0990, 0x0035}, 	// MCU_DATA_0
    {0x098C, 0x2310}, 	// MCU_ADDRESS [AWB_CCM_L_5]
    {0x0990, 0xFFFE}, 	// MCU_DATA_0
    {0x098C, 0x2326}, 	// MCU_ADDRESS [AWB_CCM_RL_5]
    {0x0990, 0xFFE4}, 	// MCU_DATA_0
    {0x098C, 0x2312}, 	// MCU_ADDRESS [AWB_CCM_L_6]
    {0x0990, 0xFFDD}, 	// MCU_DATA_0
    {0x098C, 0x2328}, 	// MCU_ADDRESS [AWB_CCM_RL_6]
    {0x0990, 0x000D}, 	// MCU_DATA_0
    {0x098C, 0x2314}, 	// MCU_ADDRESS [AWB_CCM_L_7]
    {0x0990, 0xFF95}, 	// MCU_DATA_0
    {0x098C, 0x232A}, 	// MCU_ADDRESS [AWB_CCM_RL_7]
    {0x0990, 0xFFF5}, 	// MCU_DATA_0
    {0x098C, 0x2316}, 	// MCU_ADDRESS [AWB_CCM_L_8]
    {0x0990, 0x018C}, 	// MCU_DATA_0
    {0x098C, 0x232C}, 	// MCU_ADDRESS [AWB_CCM_RL_8]
    {0x0990, 0x0000}, 	// MCU_DATA_0

    {0x098C, 0xA366}, 	// MCU_ADDRESS [AWB_KR_L]
    {0x0990, 0x0080}, 	// MCU_DATA_0
    {0x098C, 0xA368}, 	// MCU_ADDRESS [AWB_KB_L]
    {0x0990, 0x008B}, 	// MCU_DATA_0

//[Day light]
    {0x098C, 0xA369}, 	// MCU_ADDRESS [AWB_KR_R]
    {0x0990, 0x007A}, 	// MCU_DATA_0
    {0x098C, 0xAB20}, 	// MCU_ADDRESS [HG_LL_SAT1]
    {0x0990, 0x00B0}, 	// MCU_DATA_0

// for CWF
    {0x098C, 0xA36E}, 	// MCU_ADDRESS [AWB_EDGETH_MAX]
    {0x0990, 0x0028}, 	// MCU_DATA_0
    {0x098C, 0xA363}, 	// MCU_ADDRESS [AWB_TG_MIN0]
    {0x0990, 0x00CF}, 	// MCU_DATA_0
// End finetune. 2011-7-7

		
//[Lens Correction 85% 07/08/11 21:50:50]
#ifdef _LEN_CORRECTION_85_
    {0x3210, 0x01B0}, 	// COLOR_PIPELINE_CONTROL
    {0x364E, 0x0390}, 	// P_GR_P0Q0
    {0x3650, 0x04ED}, 	// P_GR_P0Q1
    {0x3652, 0x1871}, 	// P_GR_P0Q2
    {0x3654, 0x0D0D}, 	// P_GR_P0Q3
    {0x3656, 0x90B2}, 	// P_GR_P0Q4
    {0x3658, 0x7EEF}, 	// P_RD_P0Q0
    {0x365A, 0x3B6C}, 	// P_RD_P0Q1
    {0x365C, 0x1411}, 	// P_RD_P0Q2
    {0x365E, 0x474E}, 	// P_RD_P0Q3
    {0x3660, 0xB891}, 	// P_RD_P0Q4
    {0x3662, 0x7F0F}, 	// P_BL_P0Q0
    {0x3664, 0x05CD}, 	// P_BL_P0Q1
    {0x3666, 0x0151}, 	// P_BL_P0Q2
    {0x3668, 0x2ECE}, 	// P_BL_P0Q3
    {0x366A, 0xFB91}, 	// P_BL_P0Q4
    {0x366C, 0x7E8F}, 	// P_GB_P0Q0
    {0x366E, 0x138C}, 	// P_GB_P0Q1
    {0x3670, 0x1DD1}, 	// P_GB_P0Q2
    {0x3672, 0x1CEE}, 	// P_GB_P0Q3
    {0x3674, 0x9F72}, 	// P_GB_P0Q4
    {0x3676, 0x190C}, 	// P_GR_P1Q0
    {0x3678, 0xCE8E}, 	// P_GR_P1Q1
    {0x367A, 0xE7CE}, 	// P_GR_P1Q2
    {0x367C, 0x0091}, 	// P_GR_P1Q3
    {0x367E, 0x586A}, 	// P_GR_P1Q4
    {0x3680, 0x78AB}, 	// P_RD_P1Q0
    {0x3682, 0x35AE}, 	// P_RD_P1Q1
    {0x3684, 0x58CF}, 	// P_RD_P1Q2
    {0x3686, 0xCA50}, 	// P_RD_P1Q3
    {0x3688, 0x9E71}, 	// P_RD_P1Q4
    {0x368A, 0xFD8C}, 	// P_BL_P1Q0
    {0x368C, 0xEAAD}, 	// P_BL_P1Q1
    {0x368E, 0xB86E}, 	// P_BL_P1Q2
    {0x3690, 0x4270}, 	// P_BL_P1Q3
    {0x3692, 0x9CEC}, 	// P_BL_P1Q4
    {0x3694, 0x99CD}, 	// P_GB_P1Q0
    {0x3696, 0x07EF}, 	// P_GB_P1Q1
    {0x3698, 0x542F}, 	// P_GB_P1Q2
    {0x369A, 0xBEF0}, 	// P_GB_P1Q3
    {0x369C, 0xD770}, 	// P_GB_P1Q4
    {0x369E, 0x0CB2}, 	// P_GR_P2Q0
    {0x36A0, 0x462E}, 	// P_GR_P2Q1
    {0x36A2, 0x4512}, 	// P_GR_P2Q2
    {0x36A4, 0x1393}, 	// P_GR_P2Q3
    {0x36A6, 0x8297}, 	// P_GR_P2Q4
    {0x36A8, 0x7CF1}, 	// P_RD_P2Q0
    {0x36AA, 0x3CB0}, 	// P_RD_P2Q1
    {0x36AC, 0x6C92}, 	// P_RD_P2Q2
    {0x36AE, 0x136F}, 	// P_RD_P2Q3
    {0x36B0, 0xE1B6}, 	// P_RD_P2Q4
    {0x36B2, 0x7811}, 	// P_BL_P2Q0
    {0x36B4, 0x09D0}, 	// P_BL_P2Q1
    {0x36B6, 0xC8B0}, 	// P_BL_P2Q2
    {0x36B8, 0x7F11}, 	// P_BL_P2Q3
    {0x36BA, 0x8876}, 	// P_BL_P2Q4
    {0x36BC, 0x10F2}, 	// P_GB_P2Q0
    {0x36BE, 0x03D0}, 	// P_GB_P2Q1
    {0x36C0, 0x04F2}, 	// P_GB_P2Q2
    {0x36C2, 0x5692}, 	// P_GB_P2Q3
    {0x36C4, 0xFE96}, 	// P_GB_P2Q4
    {0x36C6, 0xB9AE}, 	// P_GR_P3Q0
    {0x36C8, 0x2CD0}, 	// P_GR_P3Q1
    {0x36CA, 0x704F}, 	// P_GR_P3Q2
    {0x36CC, 0x9713}, 	// P_GR_P3Q3
    {0x36CE, 0x1554}, 	// P_GR_P3Q4
    {0x36D0, 0x176E}, 	// P_RD_P3Q0
    {0x36D2, 0xB8AF}, 	// P_RD_P3Q1
    {0x36D4, 0xB411}, 	// P_RD_P3Q2
    {0x36D6, 0x4AF2}, 	// P_RD_P3Q3
    {0x36D8, 0x9A12}, 	// P_RD_P3Q4
    {0x36DA, 0x44EF}, 	// P_BL_P3Q0
    {0x36DC, 0x2691}, 	// P_BL_P3Q1
    {0x36DE, 0x9F73}, 	// P_BL_P3Q2
    {0x36E0, 0xC433}, 	// P_BL_P3Q3
    {0x36E2, 0x0A56}, 	// P_BL_P3Q4
    {0x36E4, 0x58EE}, 	// P_GB_P3Q0
    {0x36E6, 0xF790}, 	// P_GB_P3Q1
    {0x36E8, 0xE8B1}, 	// P_GB_P3Q2
    {0x36EA, 0xE1CC}, 	// P_GB_P3Q3
    {0x36EC, 0x3B14}, 	// P_GB_P3Q4
    {0x36EE, 0x94D3}, 	// P_GR_P4Q0
    {0x36F0, 0x1B53}, 	// P_GR_P4Q1
    {0x36F2, 0xE217}, 	// P_GR_P4Q2
    {0x36F4, 0x83B7}, 	// P_GR_P4Q3
    {0x36F6, 0x3B9A}, 	// P_GR_P4Q4
    {0x36F8, 0x8B53}, 	// P_RD_P4Q0
    {0x36FA, 0xA592}, 	// P_RD_P4Q1
    {0x36FC, 0xA437}, 	// P_RD_P4Q2
    {0x36FE, 0x2734}, 	// P_RD_P4Q3
    {0x3700, 0x39F9}, 	// P_RD_P4Q4
    {0x3702, 0xD1B3}, 	// P_BL_P4Q0
    {0x3704, 0x8F32}, 	// P_BL_P4Q1
    {0x3706, 0xD796}, 	// P_BL_P4Q2
    {0x3708, 0xB1F6}, 	// P_BL_P4Q3
    {0x370A, 0x67B9}, 	// P_BL_P4Q4
    {0x370C, 0xA013}, 	// P_GB_P4Q0
    {0x370E, 0x6192}, 	// P_GB_P4Q1
    {0x3710, 0xD8B7}, 	// P_GB_P4Q2
    {0x3712, 0xD7D6}, 	// P_GB_P4Q3
    {0x3714, 0x3ABA}, 	// P_GB_P4Q4
    {0x3644, 0x02BC}, 	// POLY_ORIGIN_C
    {0x3642, 0x024C}, 	// POLY_ORIGIN_R
    {0x3210, 0x01B8}, 	// COLOR_PIPELINE_CONTROL
#endif


//[Lens Correction 90% 07/08/11 21:50:57]
#ifdef _LEN_CORRECTION_90_
    {0x3210, 0x01B0}, 	// COLOR_PIPELINE_CONTROL
    {0x364E, 0x0370}, 	// P_GR_P0Q0
    {0x3650, 0x462C}, 	// P_GR_P0Q1
    {0x3652, 0x2FD1}, 	// P_GR_P0Q2
    {0x3654, 0x3C6B}, 	// P_GR_P0Q3
    {0x3656, 0xE931}, 	// P_GR_P0Q4
    {0x3658, 0x7F8F}, 	// P_RD_P0Q0
    {0x365A, 0x75EB}, 	// P_RD_P0Q1
    {0x365C, 0x2AB1}, 	// P_RD_P0Q2
    {0x365E, 0x19CE}, 	// P_RD_P0Q3
    {0x3660, 0xF7B0}, 	// P_RD_P0Q4
    {0x3662, 0x7F2F}, 	// P_BL_P0Q0
    {0x3664, 0x454C}, 	// P_BL_P0Q1
    {0x3666, 0x17B1}, 	// P_BL_P0Q2
    {0x3668, 0x17EE}, 	// P_BL_P0Q3
    {0x366A, 0xC971}, 	// P_BL_P0Q4
    {0x366C, 0x7E6F}, 	// P_GB_P0Q0
    {0x366E, 0x21CB}, 	// P_GB_P0Q1
    {0x3670, 0x34D1}, 	// P_GB_P0Q2
    {0x3672, 0x4C6D}, 	// P_GB_P0Q3
    {0x3674, 0x83F2}, 	// P_GB_P0Q4
    {0x3676, 0x124C}, 	// P_GR_P1Q0
    {0x3678, 0xD04E}, 	// P_GR_P1Q1
    {0x367A, 0xDFAE}, 	// P_GR_P1Q2
    {0x367C, 0x7B50}, 	// P_GR_P1Q3
    {0x367E, 0x98CE}, 	// P_GR_P1Q4
    {0x3680, 0x674B}, 	// P_RD_P1Q0
    {0x3682, 0x304E}, 	// P_RD_P1Q1
    {0x3684, 0x4A6F}, 	// P_RD_P1Q2
    {0x3686, 0xB710}, 	// P_RD_P1Q3
    {0x3688, 0xF770}, 	// P_RD_P1Q4
    {0x368A, 0x862D}, 	// P_BL_P1Q0
    {0x368C, 0xDA6D}, 	// P_BL_P1Q1
    {0x368E, 0xC70E}, 	// P_BL_P1Q2
    {0x3690, 0x31B0}, 	// P_BL_P1Q3
    {0x3692, 0xC2AE}, 	// P_BL_P1Q4
    {0x3694, 0xA18D}, 	// P_GB_P1Q0
    {0x3696, 0x09EF}, 	// P_GB_P1Q1
    {0x3698, 0x6E6F}, 	// P_GB_P1Q2
    {0x369A, 0xA6F0}, 	// P_GB_P1Q3
    {0x369C, 0xC111}, 	// P_GB_P1Q4
    {0x369E, 0x1892}, 	// P_GR_P2Q0
    {0x36A0, 0x4D6D}, 	// P_GR_P2Q1
    {0x36A2, 0x1C13}, 	// P_GR_P2Q2
    {0x36A4, 0x1533}, 	// P_GR_P2Q3
    {0x36A6, 0x8A17}, 	// P_GR_P2Q4
    {0x36A8, 0x0912}, 	// P_RD_P2Q0
    {0x36AA, 0x18B0}, 	// P_RD_P2Q1
    {0x36AC, 0x2AD3}, 	// P_RD_P2Q2
    {0x36AE, 0x6F70}, 	// P_RD_P2Q3
    {0x36B0, 0xEAF6}, 	// P_RD_P2Q4
    {0x36B2, 0x0752}, 	// P_BL_P2Q0
    {0x36B4, 0x0170}, 	// P_BL_P2Q1
    {0x36B6, 0x1810}, 	// P_BL_P2Q2
    {0x36B8, 0x6171}, 	// P_BL_P2Q3
    {0x36BA, 0x8C96}, 	// P_BL_P2Q4
    {0x36BC, 0x1C12}, 	// P_GB_P2Q0
    {0x36BE, 0x65AF}, 	// P_GB_P2Q1
    {0x36C0, 0x70F2}, 	// P_GB_P2Q2
    {0x36C2, 0x6172}, 	// P_GB_P2Q3
    {0x36C4, 0x8657}, 	// P_GB_P2Q4
    {0x36C6, 0xC5CE}, 	// P_GR_P3Q0
    {0x36C8, 0x1890}, 	// P_GR_P3Q1
    {0x36CA, 0xB5CE}, 	// P_GR_P3Q2
    {0x36CC, 0x8473}, 	// P_GR_P3Q3
    {0x36CE, 0x6B34}, 	// P_GR_P3Q4
    {0x36D0, 0x146E}, 	// P_RD_P3Q0
    {0x36D2, 0xD1AE}, 	// P_RD_P3Q1
    {0x36D4, 0x9391}, 	// P_RD_P3Q2
    {0x36D6, 0x37F1}, 	// P_RD_P3Q3
    {0x36D8, 0xD952}, 	// P_RD_P3Q4
    {0x36DA, 0x3F2F}, 	// P_BL_P3Q0
    {0x36DC, 0x1C31}, 	// P_BL_P3Q1
    {0x36DE, 0xB5F3}, 	// P_BL_P3Q2
    {0x36E0, 0xA2D3}, 	// P_BL_P3Q3
    {0x36E2, 0x1BF6}, 	// P_BL_P3Q4
    {0x36E4, 0x452E}, 	// P_GB_P3Q0
    {0x36E6, 0xE450}, 	// P_GB_P3Q1
    {0x36E8, 0xCD72}, 	// P_GB_P3Q2
    {0x36EA, 0x9D31}, 	// P_GB_P3Q3
    {0x36EC, 0x3815}, 	// P_GB_P3Q4
    {0x36EE, 0xFA32}, 	// P_GR_P4Q0
    {0x36F0, 0x2793}, 	// P_GR_P4Q1
    {0x36F2, 0xF0D7}, 	// P_GR_P4Q2
    {0x36F4, 0xF056}, 	// P_GR_P4Q3
    {0x36F6, 0x3F7A}, 	// P_GR_P4Q4
    {0x36F8, 0xE252}, 	// P_RD_P4Q0
    {0x36FA, 0xD271}, 	// P_RD_P4Q1
    {0x36FC, 0xAAF7}, 	// P_RD_P4Q2
    {0x36FE, 0x6313}, 	// P_RD_P4Q3
    {0x3700, 0x3299}, 	// P_RD_P4Q4
    {0x3702, 0xBEF3}, 	// P_BL_P4Q0
    {0x3704, 0x9452}, 	// P_BL_P4Q1
    {0x3706, 0xDF76}, 	// P_BL_P4Q2
    {0x3708, 0x9E76}, 	// P_BL_P4Q3
    {0x370A, 0x5B19}, 	// P_BL_P4Q4
    {0x370C, 0x8753}, 	// P_GB_P4Q0
    {0x370E, 0x5492}, 	// P_GB_P4Q1
    {0x3710, 0xDFF7}, 	// P_GB_P4Q2
    {0x3712, 0xC1D6}, 	// P_GB_P4Q3
    {0x3714, 0x3A1A}, 	// P_GB_P4Q4
    {0x3644, 0x02BC}, 	// POLY_ORIGIN_C
    {0x3642, 0x024C}, 	// POLY_ORIGIN_R
    {0x3210, 0x01B8}, 	// COLOR_PIPELINE_CONTROL
#endif



//[Lens Correction 100% 07/08/11 21:51:09]
#ifdef _LEN_CORRECTION_100_
    {0x3210, 0x01B0}, 	// COLOR_PIPELINE_CONTROL
    {0x364E, 0x03B0}, 	// P_GR_P0Q0
    {0x3650, 0x74EA}, 	// P_GR_P0Q1
    {0x3652, 0x5C91}, 	// P_GR_P0Q2
    {0x3654, 0x9E0D}, 	// P_GR_P0Q3
    {0x3656, 0xD4F0}, 	// P_GR_P0Q4
    {0x3658, 0x7FAF}, 	// P_RD_P0Q0
    {0x365A, 0x99E8}, 	// P_RD_P0Q1
    {0x365C, 0x5611}, 	// P_RD_P0Q2
    {0x365E, 0x454C}, 	// P_RD_P0Q3
    {0x3660, 0x066D}, 	// P_RD_P0Q4
    {0x3662, 0x7FCF}, 	// P_BL_P0Q0
    {0x3664, 0x028B}, 	// P_BL_P0Q1
    {0x3666, 0x42D1}, 	// P_BL_P0Q2
    {0x3668, 0x120D}, 	// P_BL_P0Q3
    {0x366A, 0xABB0}, 	// P_BL_P0Q4
    {0x366C, 0x7EAF}, 	// P_GB_P0Q0
    {0x366E, 0xCF8A}, 	// P_GB_P0Q1
    {0x3670, 0x6151}, 	// P_GB_P0Q2
    {0x3672, 0x9189}, 	// P_GB_P0Q3
    {0x3674, 0x9251}, 	// P_GB_P0Q4
    {0x3676, 0x7E4B}, 	// P_GR_P1Q0
    {0x3678, 0xCE8E}, 	// P_GR_P1Q1
    {0x367A, 0xBE8E}, 	// P_GR_P1Q2
    {0x367C, 0x6490}, 	// P_GR_P1Q3
    {0x367E, 0xD86F}, 	// P_GR_P1Q4
    {0x3680, 0x456B}, 	// P_RD_P1Q0
    {0x3682, 0x3B2E}, 	// P_RD_P1Q1
    {0x3684, 0x52AF}, 	// P_RD_P1Q2
    {0x3686, 0xCA50}, 	// P_RD_P1Q3
    {0x3688, 0xFDD0}, 	// P_RD_P1Q4
    {0x368A, 0x8D8D}, 	// P_BL_P1Q0
    {0x368C, 0xDB0D}, 	// P_BL_P1Q1
    {0x368E, 0xF5CE}, 	// P_BL_P1Q2
    {0x3690, 0x3D90}, 	// P_BL_P1Q3
    {0x3692, 0xE0AF}, 	// P_BL_P1Q4
    {0x3694, 0xA64D}, 	// P_GB_P1Q0
    {0x3696, 0x0D6F}, 	// P_GB_P1Q1
    {0x3698, 0x1B6F}, 	// P_GB_P1Q2
    {0x369A, 0x8E10}, 	// P_GB_P1Q3
    {0x369C, 0xD1B0}, 	// P_GB_P1Q4
    {0x369E, 0x2DF2}, 	// P_GR_P2Q0
    {0x36A0, 0xB2CD}, 	// P_GR_P2Q1
    {0x36A2, 0x1454}, 	// P_GR_P2Q2
    {0x36A4, 0x4753}, 	// P_GR_P2Q3
    {0x36A6, 0xA777}, 	// P_GR_P2Q4
    {0x36A8, 0x1EF2}, 	// P_RD_P2Q0
    {0x36AA, 0x6DCF}, 	// P_RD_P2Q1
    {0x36AC, 0x0A14}, 	// P_RD_P2Q2
    {0x36AE, 0x37B1}, 	// P_RD_P2Q3
    {0x36B0, 0xFAD6}, 	// P_RD_P2Q4
    {0x36B2, 0x1D92}, 	// P_BL_P2Q0
    {0x36B4, 0x366F}, 	// P_BL_P2Q1
    {0x36B6, 0x7192}, 	// P_BL_P2Q2
    {0x36B8, 0x4D52}, 	// P_BL_P2Q3
    {0x36BA, 0xB096}, 	// P_BL_P2Q4
    {0x36BC, 0x31B2}, 	// P_GB_P2Q0
    {0x36BE, 0x5E6E}, 	// P_GB_P2Q1
    {0x36C0, 0x7533}, 	// P_GB_P2Q2
    {0x36C2, 0x12D3}, 	// P_GB_P2Q3
    {0x36C4, 0x9A37}, 	// P_GB_P2Q4
    {0x36C6, 0xBF6E}, 	// P_GR_P3Q0
    {0x36C8, 0x694F}, 	// P_GR_P3Q1
    {0x36CA, 0x9071}, 	// P_GR_P3Q2
    {0x36CC, 0xDB32}, 	// P_GR_P3Q3
    {0x36CE, 0x2CD5}, 	// P_GR_P3Q4
    {0x36D0, 0x1D4E}, 	// P_RD_P3Q0
    {0x36D2, 0x8FAF}, 	// P_RD_P3Q1
    {0x36D4, 0x8251}, 	// P_RD_P3Q2
    {0x36D6, 0x4D72}, 	// P_RD_P3Q3
    {0x36D8, 0xA2D2}, 	// P_RD_P3Q4
    {0x36DA, 0x170F}, 	// P_BL_P3Q0
    {0x36DC, 0x1C11}, 	// P_BL_P3Q1
    {0x36DE, 0xC2F3}, 	// P_BL_P3Q2
    {0x36E0, 0x89F3}, 	// P_BL_P3Q3
    {0x36E2, 0x27B6}, 	// P_BL_P3Q4
    {0x36E4, 0x1F0D}, 	// P_GB_P3Q0
    {0x36E6, 0xC270}, 	// P_GB_P3Q1
    {0x36E8, 0xFDD0}, 	// P_GB_P3Q2
    {0x36EA, 0x9E71}, 	// P_GB_P3Q3
    {0x36EC, 0x7B53}, 	// P_GB_P3Q4
    {0x36EE, 0x8632}, 	// P_GR_P4Q0
    {0x36F0, 0x47F3}, 	// P_GR_P4Q1
    {0x36F2, 0x9058}, 	// P_GR_P4Q2
    {0x36F4, 0x9997}, 	// P_GR_P4Q3
    {0x36F6, 0x64BA}, 	// P_GR_P4Q4
    {0x36F8, 0x8D92}, 	// P_RD_P4Q0
    {0x36FA, 0x8732}, 	// P_RD_P4Q1
    {0x36FC, 0xB457}, 	// P_RD_P4Q2
    {0x36FE, 0x7E14}, 	// P_RD_P4Q3
    {0x3700, 0x1C99}, 	// P_RD_P4Q4
    {0x3702, 0x97D3}, 	// P_BL_P4Q0
    {0x3704, 0xDDB1}, 	// P_BL_P4Q1
    {0x3706, 0x8B97}, 	// P_BL_P4Q2
    {0x3708, 0xE0F6}, 	// P_BL_P4Q3
    {0x370A, 0x059A}, 	// P_BL_P4Q4
    {0x370C, 0xA5F2}, 	// P_GB_P4Q0
    {0x370E, 0x1373}, 	// P_GB_P4Q1
    {0x3710, 0x80F8}, 	// P_GB_P4Q2
    {0x3712, 0xDA36}, 	// P_GB_P4Q3
    {0x3714, 0x4D3A}, 	// P_GB_P4Q4
    {0x3644, 0x02BC}, 	// POLY_ORIGIN_C
    {0x3642, 0x024C}, 	// POLY_ORIGIN_R
    {0x3210, 0x01B8}, 	// COLOR_PIPELINE_CONTROL
#endif

};
/* 816x612, 24MHz MCLK 96MHz PCLK */
uint32_t mt9d115_FULL_SIZE_WIDTH        = 640;
uint32_t mt9d115_FULL_SIZE_HEIGHT       = 480;

uint32_t mt9d115_QTR_SIZE_WIDTH         = 640;
uint32_t mt9d115_QTR_SIZE_HEIGHT        = 480;

uint32_t mt9d115_HRZ_FULL_BLK_PIXELS    = 16;
uint32_t mt9d115_VER_FULL_BLK_LINES     = 12;
uint32_t mt9d115_HRZ_QTR_BLK_PIXELS     = 16;
uint32_t mt9d115_VER_QTR_BLK_LINES      = 12;

struct mt9d115_work_t {
    struct work_struct work;
};
static struct  mt9d115_work_t *mt9d115_sensorw;
static struct  i2c_client *mt9d115_client;
struct mt9d115_ctrl_t {
    const struct  msm_camera_sensor_info *sensordata;
    uint32_t sensormode;
    uint32_t fps_divider;        /* init to 1 * 0x00000400 */
    uint32_t pict_fps_divider;    /* init to 1 * 0x00000400 */
    uint32_t fps;
    int32_t  curr_lens_pos;
    uint32_t curr_step_pos;
    uint32_t my_reg_gain;
    uint32_t my_reg_line_count;
    uint32_t total_lines_per_frame;
    enum mt9d115_resolution_t prev_res;
    enum mt9d115_resolution_t pict_res;
    enum mt9d115_resolution_t curr_res;
    enum mt9d115_test_mode_t  set_test;
    unsigned short imgaddr;
};
static struct mt9d115_ctrl_t *mt9d115_ctrl;
static DECLARE_WAIT_QUEUE_HEAD(mt9d115_wait_queue);
DEFINE_MUTEX(mt9d115_mut);
static struct wake_lock mt9d115_wlock;//used to fix vfe31 error when switch camera in sleep mode
/*=============================================================*/

static int mt9d115_i2c_rxdata(unsigned short saddr,
    unsigned char *rxdata, int length)
{
    struct i2c_msg msgs[] = {
    {
        .addr   = saddr,
        .flags = 0,
        .len   = 2,
        .buf   = rxdata,
    },
    {
        .addr   = saddr,
        .flags = I2C_M_RD,
        .len   = length,
        .buf   = rxdata,
    },
    };
    log_debug("mt9d115_i2c_rxdata :: rxdata = 0x%x ================\n" ,*(u16 *) rxdata);
    
    if (i2c_transfer(mt9d115_client->adapter, msgs, 2) < 0) {
        logs_err("mt9d115_i2c_rxdata failed!\n");
        return -EIO;
    }
    
    return 0;
}

static int32_t mt9d115_i2c_read(unsigned short raddr, uint16_t *rdata)
{
    int32_t rc = 0;
    unsigned char buf[4];
    log_debug("mt9d115_i2c_read :: ================\n");
    if (!rdata)
        return -EIO;
    
    memset(buf, 0, sizeof(buf));
    
    buf[0] = (raddr & 0xFF00)>>8;
    buf[1] = (raddr & 0x00FF);
    
    rc = mt9d115_i2c_rxdata(0x3D, buf, 2);
    if (rc < 0)
        return rc;
    
//    *rdata = buf[0];
    *rdata = buf[0] << 8 | buf[1];


    if (rc < 0)
        logs_err("mt9d115_i2c_read 0x%x failed!\n",raddr);
    
    return rc;
}
static int32_t mt9d115_i2c_txdata(unsigned short saddr,
    unsigned char *txdata, int length)
{
    struct i2c_msg msg[] = {
            {
                .addr = saddr,
                .flags = 0,
                .len = length,
                .buf = txdata,
            },
      };
#if 0
    if (length == 2)
        log_debug("msm_io_i2c_w: 0x%04x 0x%04x\n",
            *(u16 *) txdata, *(u16 *) (txdata + 2));
    else if (length == 4)
        log_debug("msm_io_i2c_w: 0x%04x\n", *(u16 *) txdata);
    else
        log_debug("msm_io_i2c_w: length = %d\n", length);
#endif
    if (i2c_transfer(mt9d115_client->adapter, msg, 1) < 0) {
        logs_err("mt9d115_i2c_txdata failed\n");
        return -EIO;
    }
    
    return 0;
}

static int32_t mt9d115_i2c_write(unsigned short waddr, unsigned short wdata)
{
    int32_t rc = -EIO;
    unsigned char buf[4];
//    log_debug("mt9d115_i2c_write :: waddr = 0x%x wdata = 0x%x================\n" , waddr , wdata);
    memset(buf, 0, sizeof(buf));
        
    buf[0] = (waddr & 0xFF00)>>8;
    buf[1] = (waddr & 0x00FF);
    buf[2] = (wdata & 0xFF00) >> 8;
    buf[3] = (wdata & 0x00FF);
    
    
    rc = mt9d115_i2c_txdata(0x3d, buf, 4);
    
    if (rc < 0)
        logs_err("i2c_write failed, addr = 0x%x, val = 0x%x!\n",waddr, wdata);
    
    return rc;
}


static int32_t mt9d115_i2c_write_table(struct reg_addr_val_pair_struct const *array, int array_length)
{
    int i ;
    int32_t  rc = -EIO;

    for (i = 0; i < array_length; i++) 
    {
        rc = mt9d115_i2c_write(array[i].reg_addr, array[i].reg_val);
        if (rc < 0)
        {
            logs_err("write reg 0x%x value 0x%x failed!", array[i].reg_addr, array[i].reg_val);
            break;
        }
    }
    return rc;
}
/* liyibo0001 : modify for wb option at 2011-07-20 */
static long mt9d115_init_register(void)
{
    int32_t array_length,pll_length,len;
    int32_t i;
    long rc = 0;
//    uint16_t model_id = 0;
//modify by liyibo 2012-3-29 for Gtalk open camera I2C can not connect begin
    for(i = 0; i < 3; i++)
    {  
    	 log_debug("reset_sensor_setting[%d] = 0x%x\n" , i , reset_sensor_setting[i]);
       rc = mt9d115_i2c_write(0x001A, reset_sensor_setting[i]);
       if (rc < 0)	
       {
           printk("write 0x%x to 0x001A failed!" , reset_sensor_setting[i]);
           return rc;  
	   }
	   else 
           msleep(2);
    }
//modify by liyibo 2012-3-29 for Gtalk open camera I2C can not connect end
    msleep(10);
    pll_length = sizeof(mt9d115_pll_settings) /sizeof(mt9d115_pll_settings[0]);
    for (i = 0; i < pll_length; i++) {
        rc = mt9d115_i2c_write(mt9d115_pll_settings[i].reg_addr, mt9d115_pll_settings[i].reg_val);
        if (rc < 0)  return rc;
    }          
    msleep(10);
                
    len = sizeof(mt9d115_settings) /sizeof(mt9d115_settings[0]);
    for (i = 0; i < len; i++) {
        rc = mt9d115_i2c_write(mt9d115_settings[i].reg_addr, mt9d115_settings[i].reg_val);
        if (rc < 0)  return rc;
    }  
    msleep(50);
    
    array_length = sizeof(mt9d115_init_preview_array) /sizeof(mt9d115_init_preview_array[0]);
    for (i = 0; i < array_length; i++) {
        rc = mt9d115_i2c_write(mt9d115_init_preview_array[i].reg_addr, mt9d115_init_preview_array[i].reg_val);
        if (rc < 0) return rc;
    }
    
    msleep(10);
    array_length = sizeof(mt9d115_init_fine_tune) /sizeof(mt9d115_init_fine_tune[0]);
    for (i = 0; i < array_length; i++) {
        rc = mt9d115_i2c_write(mt9d115_init_fine_tune[i].reg_addr, mt9d115_init_fine_tune[i].reg_val);
        if (rc < 0) return rc;
    }
    msleep(50);

//    rc =  mt9d115_i2c_write(0x098C, 0xA702);
//    if (rc < 0) return rc;  
//    rc = mt9d115_i2c_read(0x0990, &model_id);
//    if (rc < 0) return rc; 
//        
//    if(model_id != 0x0000)
//    {
//        log_debug("liyibo :: sorry , now it  is preview mode,model_id = 0x%x can not preview!\n" ,model_id);
//        rc = -EINVAL;
//    }
//    msleep(10);
	
    rc = mt9d115_i2c_write(0x0018, 0x0028);
    if (rc < 0) return rc;       
    msleep(10);    
        
    rc = mt9d115_i2c_write(0x098C, 0xA103);
    if (rc < 0)	return rc;           
    rc = mt9d115_i2c_write(0x0990, 0x0006);
    if (rc < 0)	return rc;  
//    
//    rc = mt9d115_i2c_write(0x0012, 0x00F5);
//    if (rc < 0)	return rc;       
//    
//    rc = mt9d115_i2c_write(0x0018, 0x0028);
//    if (rc < 0)		return rc;       
    msleep(10);

    return rc; 
}

static int csi_config = 0;
static int32_t mt9d115_sensor_setting(int update_type, int rt)
{
    int32_t array_length; //,pll_length,len;
    int32_t i;
    long rc = 0;
    uint16_t model_id = 0;
    struct msm_camera_csi_params mt9d115_csi_params;
    
    switch (update_type) {
    case REG_INIT:
        break;
    case UPDATE_PERIODIC:
    {
        if(0 == csi_config) {
            mt9d115_csi_params.lane_cnt = 1;
            mt9d115_csi_params.data_format = CSI_8BIT;
            mt9d115_csi_params.lane_assign = 0xe4;
            mt9d115_csi_params.dpcm_scheme = 0;
            mt9d115_csi_params.settle_cnt = 0x014;
            rc = msm_camio_csi_config(&mt9d115_csi_params);
            msleep(10);
            mt9d115_init_register();
            csi_config = 1;
        }
        else
        {
            if(rt == RES_PREVIEW)
            {
                array_length = sizeof(preview_mode_setting) /sizeof(preview_mode_setting[0]);
                for (i = 0; i < array_length; i++) {
                rc = mt9d115_i2c_write(preview_mode_setting[i].reg_addr, preview_mode_setting[i].reg_val);
                    if (rc < 0)
                    {
                        logs_err("write reg 0x%x value 0x%x failed!", capture_mode_setting[i].reg_addr, capture_mode_setting[i].reg_val);
                        return rc;
                    }
                }
                msleep(50);
                rc = mt9d115_i2c_write(0x098C, 0xA702);
                if (rc < 0)
                {
                     logs_err("write reg 0x098C value 0xA702 failed!");
                     return rc;
                }    
                rc = mt9d115_i2c_read(0x0990, &model_id);
                if (rc < 0)
                {
                     logs_err("read reg 0x0990 failed!");
                     return rc;
                } 
                
                log_debug("liyibo: ########## set preview mt9d115 model_id = 0x%x\n",model_id);            
            }
            else if(rt == RES_CAPTURE)
 	          {
 	            array_length = sizeof(capture_mode_setting) /sizeof(capture_mode_setting[0]);
                for (i = 0; i < array_length; i++) {
                    rc = mt9d115_i2c_write(capture_mode_setting[i].reg_addr, capture_mode_setting[i].reg_val);
                    if (rc < 0)
                    {
                        logs_err("write reg 0x%x value 0x%x failed!", capture_mode_setting[i].reg_addr, capture_mode_setting[i].reg_val);
                        return rc;
                    }
                }
                msleep(50);
                
               rc = mt9d115_i2c_write(0x098C, 0xA702);
               if (rc < 0)
               {
                    logs_err("write reg 0x098C value 0xA702 failed!");
                    return rc;
               }    
               rc = mt9d115_i2c_read(0x0990, &model_id);
               if (rc < 0)
               {
                    logs_err("read reg 0x0990 failed!");
                    return rc;
               } 
               log_debug("liyibo: ########## set capture mt9d115 model_id = 0x%x\n",model_id);
 		        }
 		    }
    }
    break;
    default:
        rc = -EINVAL; 
        break;
    }
        
    return rc;
}       

static int32_t mt9d115_video_config(int mode)
{
    
    int32_t rc = 0;
    int rt;
    /* change sensor resolution if needed */
    if(mode == SENSOR_PREVIEW_MODE) {        
        rt = RES_PREVIEW;    
    }    
    else if(mode == SENSOR_SNAPSHOT_MODE) {        
        rt = RES_CAPTURE;    
    }
    
    if (mt9d115_sensor_setting(UPDATE_PERIODIC, rt) < 0)
        return rc;
    mt9d115_ctrl->curr_res = mt9d115_ctrl->prev_res;
    mt9d115_ctrl->sensormode = mode;
    return rc;
}

static int32_t mt9d115_set_sensor_mode(int mode,
    int res)
{
//    int  array_length,i;
    int32_t rc = 0;
//    uint16_t model_id = 0;
    switch (mode) {
    case SENSOR_SNAPSHOT_MODE:
//        array_length = sizeof(capture_mode_setting) /sizeof(capture_mode_setting[0]);
//            for (i = 0; i < array_length; i++) {
//                rc = mt9d115_i2c_write(capture_mode_setting[i].reg_addr, capture_mode_setting[i].reg_val);
//                if (rc < 0)
//                {
//                    log_debug("write reg 0x%x value 0x%x failed!", capture_mode_setting[i].reg_addr, capture_mode_setting[i].reg_val);
//                    return rc;
//                }
//            }
//            msleep(50);
//           rc = mt9d115_i2c_write(0x098C, 0xA702);
//           if (rc < 0)
//           {
//                log_debug("write reg 0x098C value 0xA702 failed!");
//                return rc;
//           }    
//           rc = mt9d115_i2c_read(0x0990, &model_id);
//           if (rc < 0)
//           {
//                log_debug("read reg 0x0990 failed!");
//                return rc;
//           }    
////           if(model_id != 0x0001)
////           {
////               log_debug("liyibo :: sorry , now it  is capture mode, model_id = 0x%x can not capture picture!!\n " , model_id);
////               rc = -EINVAL;
////           }
//
//    break;
    case SENSOR_PREVIEW_MODE:
        rc = mt9d115_video_config(mode);
        break;
    case SENSOR_RAW_SNAPSHOT_MODE:
        break;
    default:
        rc = -EINVAL;
        break;
    }
    return rc;
}
/* liyibo0001 : modify end at 2011-07-20 */
static int32_t mt9d115_power_down(void)
{
    return 0;
}

static int mt9d115_probe_init_done(const struct msm_camera_sensor_info *data)
{
    gpio_free(data->sensor_pwd);
    gpio_free(data->sensor_reset);
    return 0;
}



static int mt9d115_probe_init_sensor(const struct msm_camera_sensor_info *data)
{
    uint16_t model_id_msb = 0;
    uint16_t model_id;
    int32_t rc = 0;
    log_debug("liyibo: mt9d115 sensor init\n");
    rc = gpio_request(data->sensor_pwd, "mt9d115");
    if (!rc) {
        logs_err("sensor_pwd = %d\n", rc);
        gpio_direction_output(data->sensor_pwd, 0);
        msleep(20);
        //gpio_set_value(data->sensor_pwd, 1);
    } else {
        logs_err("gpio pwd fail");
        goto init_probe_done;
    }
         //gpio_free(data->sensor_pwd);
    
    rc = gpio_request(data->sensor_reset, "mt9d115");
    if (!rc) {
        logs_err("sensor_reset = %d  gpio = %d\n", rc, data->sensor_reset);
        gpio_direction_output(data->sensor_reset, 0);
        msleep(20);
        gpio_set_value(data->sensor_reset, 1);
        msleep(20);
    } else {
        logs_err("gpio reset fail");
        goto init_probe_done;
    }

    /* 3. Read sensor Model ID: */
    if (mt9d115_i2c_read(REG_mt9d115_MODEL_ID_MSB, &model_id_msb) < 0) {
        log_debug("liyibo: mt9d115 read wrong\n");
    //goto init_probe_fail;
    }
    model_id = model_id_msb;
    logs_err("liyibo: mt9d115 model_id = 0x%x\n",model_id);
//    msleep(500);
//}

    /* 4. Compare sensor ID to mt9d115 ID: */
    if (model_id != mt9d115_MODEL_ID) {
        rc = -ENODEV;
        goto init_probe_fail;
    }
    goto init_probe_done;
init_probe_fail:
    logs_err(KERN_INFO " mt9d115_probe_init_sensor fails_kalyani\n");
    gpio_set_value_cansleep(data->sensor_reset, 0);
init_probe_done:
    logs_err(KERN_INFO " mt9d115_probe_init_sensor finishes\n");
    return rc;
}

int mt9d115_sensor_open_init(const struct msm_camera_sensor_info *data)
{
    int32_t rc = 0;
    
    log_debug("%s: %d\n", __func__, __LINE__);
    log_debug("Calling mt9d115_sensor_open_init\n");
    wake_lock(&mt9d115_wlock);
    mt9d115_ctrl = kzalloc(sizeof(struct mt9d115_ctrl_t), GFP_KERNEL);
    if (!mt9d115_ctrl) {
        logs_err("mt9d115_init failed!\n");
        rc = -ENOMEM;
		 wake_unlock(&mt9d115_wlock);
        goto init_done;
    }
    mt9d115_ctrl->fps_divider = 1 * 0x00000400;
    mt9d115_ctrl->pict_fps_divider = 1 * 0x00000400;
    mt9d115_ctrl->fps = 30 * Q8;
    mt9d115_ctrl->set_test = TEST_OFF;
    mt9d115_ctrl->prev_res = QTR_SIZE;
    mt9d115_ctrl->pict_res = FULL_SIZE;
    mt9d115_ctrl->curr_res = INVALID_SIZE;
    csi_config = 0;			//V11 liyibo0002 : modify by liyibo for mt9d115 driver preview dead at 2011-04-14

    if (data)
        mt9d115_ctrl->sensordata = data;
    
    /* enable mclk first */
    
    msm_camio_clk_rate_set(mt9d115_DEFAULT_CLOCK_RATE);
    msleep(20);
    
    rc = mt9d115_probe_init_sensor(data);
    if (rc < 0) {
        logs_err("Calling mt9d115_sensor_open_init fail\n");
        goto init_fail;
    }
    
    rc = mt9d115_sensor_setting(REG_INIT, RES_PREVIEW);
    if (rc < 0) {
        gpio_set_value_cansleep(data->sensor_reset, 0);
        goto init_fail;
    } else
          goto init_done;
init_fail:
    logs_err(" mt9d115_sensor_open_init fail\n");
    mt9d115_probe_init_done(data);
	wake_unlock(&mt9d115_wlock);
    kfree(mt9d115_ctrl);
init_done:
    logs_err("mt9d115_sensor_open_init done\n");
    return rc;
}

static int mt9d115_init_client(struct i2c_client *client)
{
    /* Initialize the MSM_CAMI2C Chip */
    init_waitqueue_head(&mt9d115_wait_queue);
    return 0;
}

static const struct i2c_device_id mt9d115_i2c_id[] = {
    {"mt9d115", 0},
    { }
};

static int mt9d115_i2c_probe(struct i2c_client *client,
    const struct i2c_device_id *id)
{
    int rc = 0;
    log_debug("mt9d115_probe called!\n");
    
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        logs_err("i2c_check_functionality failed\n");
        goto probe_failure;
    }
    
    mt9d115_sensorw = kzalloc(sizeof(struct mt9d115_work_t), GFP_KERNEL);
    if (!mt9d115_sensorw) {
        logs_err("kzalloc failed.\n");
        rc = -ENOMEM;
        goto probe_failure;
    }
    
    i2c_set_clientdata(client, mt9d115_sensorw);
    mt9d115_init_client(client);
    mt9d115_client = client;
    
    msleep(50);
    
    log_debug("mt9d115_probe successed! rc = %d\n", rc);
    return 0;
    
probe_failure:
    logs_err("mt9d115_probe failed! rc = %d\n", rc);
    return rc;
}

static int __exit mt9d115_remove(struct i2c_client *client)
{
    struct mt9d115_work_t_t *sensorw = i2c_get_clientdata(client);
    free_irq(client->irq, sensorw);
    mt9d115_client = NULL;
    kfree(sensorw);
    return 0;
}

static struct i2c_driver mt9d115_i2c_driver = {
    .id_table = mt9d115_i2c_id,
    .probe  = mt9d115_i2c_probe,
    .remove = __exit_p(mt9d115_i2c_remove),
    .driver = {
        .name = "mt9d115",
    },
};
static int16_t mt9d115_wb = CAMERA_KER_WB_AUTO;
static int mt9d115_set_wb(int mode , int wb)
{
    long rc = 0;
    log_debug("liyibo: %s mode = %d, wb = %d\n", __func__, mode, wb);
    msleep(1);
	/* liyibo0001 : modify for reserved wb options at 2011-07-20 */
    if(wb == mt9d115_wb)
    {
        logs_info("mt9d115_set_wb :: as same as the pre setting\n!");
        return 0;
    }   
    /* liyibo0001 : modify end at 2011-07-20 */
    switch(wb)
    {
        case CAMERA_KER_WB_AUTO:
        rc = mt9d115_i2c_write_table(camera_balance_auto , sizeof(camera_balance_auto) /sizeof(camera_balance_auto[0]));
        break;
        case CAMERA_KER_WB_CLOUDY:
        rc = mt9d115_i2c_write_table(camera_balance_cloudy , sizeof(camera_balance_cloudy) /sizeof(camera_balance_cloudy[0]));
        break;
        case CAMERA_KER_WB_DAYLIGHT:
        rc = mt9d115_i2c_write_table(camera_balance_daylight , sizeof(camera_balance_daylight) /sizeof(camera_balance_daylight[0]));
        break;
	 case CAMERA_KER_WB_FLUORESCENT:
        rc = mt9d115_i2c_write_table(camera_balance_flourescant , sizeof(camera_balance_flourescant) /sizeof(camera_balance_flourescant[0]));
        break;
        case CAMERA_KER_WB_U3D:
        rc = mt9d115_i2c_write_table(camera_balance_U30 , sizeof(camera_balance_U30) /sizeof(camera_balance_U30[0]));
        break;
        case CAMERA_KER_WB_CWF:
        rc = mt9d115_i2c_write_table(camera_balance_CWF , sizeof(camera_balance_CWF) /sizeof(camera_balance_CWF[0]));
        break;
        case CAMERA_KER_WB_INCANDESCENT:
        rc = mt9d115_i2c_write_table(camera_balance_incandescent , sizeof(camera_balance_incandescent) /sizeof(camera_balance_incandescent[0]));
            break;
        default: 
            break;
    }
    mt9d115_wb = wb;
        msleep(1);
        return rc;
    
}
static int16_t  mt9d115_contrast = CAMERA_CONTRAST_ZERO;
static int mt9d115_set_contrast(int mode , int contrast)
{
    long rc = 0;
    log_debug("liyibo: %s mode = %d, contrast= %d\n", __func__, mode, contrast);
    msleep(1);
	/* liyibo0001 : modify for reserved contrast options at 2011-07-20 */
    if(contrast == mt9d115_contrast)
        return 0;
    /* liyibo0001 : modify end at 2011-07-20 */
    switch(contrast)
    {
        case CAMERA_CONTRAST_NEG_2:
        rc = mt9d115_i2c_write_table(camera_contrast_neg_2 ,  sizeof(camera_contrast_neg_2) /sizeof(camera_contrast_neg_2[0]));
        break;
        case CAMERA_CONTRAST_NEG_1:
        rc = mt9d115_i2c_write_table(camera_contrast_neg_1 ,  sizeof(camera_contrast_neg_1) /sizeof(camera_contrast_neg_1[0]));
        break;
        case CAMERA_CONTRAST_ZERO:
		//modify by liyibo for EC EC617001229696 at 2011-12-29
        rc = mt9d115_i2c_write_table(camera_contrast_zero ,  sizeof(camera_contrast_zero) /sizeof(camera_contrast_zero[0]));
        break;
        case CAMERA_CONTRAST_POSI_1:
        rc = mt9d115_i2c_write_table(camera_contrast_posi_1 ,  sizeof(camera_contrast_posi_1) /sizeof(camera_contrast_posi_1[0]));
        break;
        case CAMERA_CONTRAST_POSI_2:
        rc = mt9d115_i2c_write_table(camera_contrast_posi_2 ,  sizeof(camera_contrast_posi_2) /sizeof(camera_contrast_posi_2[0]));
        break;
        default: 
            break;

    }
    mt9d115_contrast = contrast;
        msleep(1);
        return rc;
}
/* liyibo0001 : added brigthness option at 2011-07-20 */
static int16_t  mt9d115_brightness = CAMERA_BRIGHTNESS_ZERO;
static int  mt9d115_set_brightness(int mode , int brightness)
{
    int rc = 0;
    int reg_num = MT9D115_BRIGHT_REG_NUM;
    int index;

    log_debug("liyibo: %s mode = %d, brightness= %d\n", __func__, mode, brightness);
   msleep(1);
    if(brightness >= MT9D115_MAX_BRIGHT) {
        logs_info("liyibo: brightness val is not correct!\n");
        return -1;
    }
    if(brightness == mt9d115_brightness) {
        log_debug("liyibo: there is no need to set brightness!\n");
        return 0;
    }
    
    for (index = 0; index < reg_num; index++) {
        rc = mt9d115_i2c_write(mt9d115_camera_bright_list[brightness][index].reg_addr, mt9d115_camera_bright_list[brightness][index].reg_val);
        if (rc < 0) {
            logs_err("write reg 0x%x value 0x%x failed!",mt9d115_camera_bright_list[brightness][index].reg_addr, mt9d115_camera_bright_list[brightness][index].reg_val);
            return rc;
        }
    }
    mt9d115_brightness = brightness;
    return rc;
}
/* liyibo0001 : added end at 2011-07-20 */
static int16_t  mt9d115_saturation = CAMERA_SATURATION_ZERO;
static int mt9d115_set_saturation(int mode , int saturation)
{
    int rc = 0;
    int reg_num = MT9D115_SATURATION_REG_NUM;
    int index;

    log_debug("liyibo: %s mode = %d, saturation= %d\n", __func__, mode, saturation);
    msleep(1);
	if(saturation >= MT9D115_MAX_SATURATION) {
        logs_err("liyibo: saturation val is not correct!\n");
        return -1;
    }
    /* liyibo0001 : modify for reserved saturation options at 2011-07-20 */
    if(saturation == mt9d115_saturation) {
        logs_info("liyibo: there is no need to set saturation!\n");
        return 0;
    }
    /* liyibo0001 : modify end at 2011-07-20 */
    for (index = 0; index < reg_num; index++) {
        rc = mt9d115_i2c_write(mt9d115_camera_saturation_list[saturation][index].reg_addr, mt9d115_camera_saturation_list[saturation][index].reg_val);
        if (rc < 0) {
            logs_err("write reg 0x%x value 0x%x failed!",mt9d115_camera_saturation_list[saturation][index].reg_addr, mt9d115_camera_saturation_list[saturation][index].reg_val);
            return rc;
        }
    }
    mt9d115_saturation = saturation;
    return rc;
}

static int16_t  mt9d115_sharpness = CAMERA_SHAPNESS_ZERO;
static int  mt9d115_set_sharpness(int mode , int sharpness)
{
    int rc = 0;
    int reg_num = MT9D115_SHARPNESS_REG_NUM;
    int index;

    log_debug("liyibo: %s mode = %d, sharpness= %d\n", __func__, mode, sharpness);
    msleep(1);
	if(sharpness >= MT9D115_MAX_SHARPNESS) {
        logs_err("liyibo: sharpness val is not correct!\n");
        return -1;
    }
    /* liyibo0001 : modify for reserved sharpness options at 2011-07-20 */
    if(sharpness == mt9d115_sharpness) {
        log_debug("liyibo: there is no need to set sharpness!\n");
        return 0;
    }
    /* liyibo0001 : modify end at 2011-07-20 */
    for (index = 0; index < reg_num; index++) {
        rc = mt9d115_i2c_write(mt9d115_camera_sharpness_list[sharpness][index].reg_addr, mt9d115_camera_sharpness_list[sharpness][index].reg_val);
        if (rc < 0) {
            logs_err("write reg 0x%x value 0x%x failed!",mt9d115_camera_sharpness_list[sharpness][index].reg_addr, mt9d115_camera_sharpness_list[sharpness][index].reg_val);
            return rc;
        }
    }
    mt9d115_sharpness = sharpness;
        return rc;
}

static int16_t mt9d115_effect = CAMERA_EFFECT_OFF;
static long mt9d115_set_effect_preview(int effect)
{
    long rc = 0;
    int32_t array_length;
    int i;
    switch (effect) {
        case CAMERA_EFFECT_OFF: 
        array_length = sizeof(camera_effect_off) /sizeof(camera_effect_off[0]);
        for (i = 0; i < array_length; i++) {
            rc = mt9d115_i2c_write(camera_effect_off[i].reg_addr, camera_effect_off[i].reg_val);
            if (rc < 0) return rc;
        }
        break; 
        case CAMERA_EFFECT_MONO: 
        array_length = sizeof(camera_effect_mono) /sizeof(camera_effect_mono[0]);
        for (i = 0; i < array_length; i++) {
            rc = mt9d115_i2c_write(camera_effect_mono[i].reg_addr, camera_effect_mono[i].reg_val);
            if (rc < 0)	return rc;
        }
        break;
        case CAMERA_EFFECT_SEPIA: 
        array_length = sizeof(camera_effect_sepia) /sizeof(camera_effect_sepia[0]);
        for (i = 0; i < array_length; i++) {
            rc = mt9d115_i2c_write(camera_effect_sepia[i].reg_addr, camera_effect_sepia[i].reg_val);
            if (rc < 0) return rc;
        }
        break;
        case CAMERA_EFFECT_NEGATIVE: 
        array_length = sizeof(camera_effect_negative) /sizeof(camera_effect_negative[0]);
        for (i = 0; i < array_length; i++) {
            rc = mt9d115_i2c_write(camera_effect_negative[i].reg_addr, camera_effect_negative[i].reg_val);
            if (rc < 0) return rc;
        }
        break;  
        case CAMERA_EFFECT_SOLARIZE: 
        array_length = sizeof(camera_effect_solarize) /sizeof(camera_effect_solarize[0]);
        for (i = 0; i < array_length; i++) {
            rc = mt9d115_i2c_write(camera_effect_solarize[i].reg_addr, camera_effect_solarize[i].reg_val);
            if (rc < 0) return rc;
        }
        break;
        case CAMERA_EFFECT_GREEN: 
        array_length = sizeof(camera_effect_Green) /sizeof(camera_effect_Green[0]);
        for (i = 0; i < array_length; i++) {
            rc = mt9d115_i2c_write(camera_effect_Green[i].reg_addr, camera_effect_Green[i].reg_val);
            if (rc < 0) return rc;
        }
        break;      
        default: 
        break;
    }
    return rc;
}
static long mt9d115_set_effect_capture(int effect)
{
    long rc = 0;
    int32_t array_length;
    int i;
    switch (effect) {
        case CAMERA_EFFECT_OFF: 
        array_length = sizeof(camera_effect_off_capture) /sizeof(camera_effect_off_capture[0]);
        for (i = 0; i < array_length; i++) {
            rc = mt9d115_i2c_write(camera_effect_off_capture[i].reg_addr, camera_effect_off_capture[i].reg_val);
            if (rc < 0)     return rc;
        }
        break;
        case CAMERA_EFFECT_MONO: 
        array_length = sizeof(camera_effect_mono_capture) /sizeof(camera_effect_mono_capture[0]);
        for (i = 0; i < array_length; i++) {
            rc = mt9d115_i2c_write(camera_effect_mono_capture[i].reg_addr, camera_effect_mono_capture[i].reg_val);
            if (rc < 0)     return rc;
        }
        break; 
        case CAMERA_EFFECT_SEPIA: 
        array_length = sizeof(camera_effect_sepia) /sizeof(camera_effect_sepia[0]);
        for (i = 0; i < array_length; i++) {
            rc = mt9d115_i2c_write(camera_effect_sepia_capture[i].reg_addr, camera_effect_sepia_capture[i].reg_val);
            if (rc < 0)     return rc;
        }
        break;      
        case CAMERA_EFFECT_NEGATIVE: 
        array_length = sizeof(camera_effect_negative_capture) /sizeof(camera_effect_negative_capture[0]);
        for (i = 0; i < array_length; i++) {
            rc = mt9d115_i2c_write(camera_effect_negative_capture[i].reg_addr, camera_effect_negative_capture[i].reg_val);
            if (rc < 0)     return rc;
        }
        break; 
        case CAMERA_EFFECT_SOLARIZE: 
        array_length = sizeof(camera_effect_solarize) /sizeof(camera_effect_solarize_capture[0]);
        for (i = 0; i < array_length; i++) {
            rc = mt9d115_i2c_write(camera_effect_solarize_capture[i].reg_addr, camera_effect_solarize_capture[i].reg_val);
            if (rc < 0)     return rc;
        }
        break;
        case CAMERA_EFFECT_GREEN: 
        array_length = sizeof(camera_effect_Green) /sizeof(camera_effect_Green[0]);
        for (i = 0; i < array_length; i++) {
            rc = mt9d115_i2c_write(camera_effect_Green_capture[i].reg_addr, camera_effect_Green_capture[i].reg_val);
            if (rc < 0)     return rc;
        }
        break;

        default: 
        break;
    }
    return rc;
}
static long mt9d115_set_effect(int mode, int effect)
{
    long rc = 0;
 //   int32_t array_length;
  //  int i;
    
    log_debug("liyibo: %s mode = %d, effect = %d\n", __func__, mode, effect);
    msleep(1);
    /* liyibo0001 : modify for reserved effect options at 2011-07-20 */
    if(effect == mt9d115_effect) {
    	  logs_info("mt9d115_set_effect :: as same as the pre setting\n!");
        return 0;
    }
    if(mode == SENSOR_PREVIEW_MODE)       
        mt9d115_set_effect_preview(effect); 
           
    else if (mode == SENSOR_SNAPSHOT_MODE)       
        mt9d115_set_effect_capture(effect);
    mt9d115_effect = effect;
//    mt9d115_mode = mode;
    /* liyibo0001 : modify end at 2011-07-20 */
    msleep(1);
    return rc;
}


int mt9d115_sensor_config(void __user *argp)
{
    struct sensor_cfg_data cdata;
    long   rc = 0;
    if (copy_from_user(&cdata,
        (void *)argp,
        sizeof(struct sensor_cfg_data)))
        return -EFAULT;
    mutex_lock(&mt9d115_mut);
    log_debug("mt9d115_sensor_config: cfgtype = %d\n",cdata.cfgtype);
    msleep(1);
    switch (cdata.cfgtype) {
    case CFG_SET_MODE:
        rc = mt9d115_set_sensor_mode(cdata.mode, cdata.rs);
        break;
    case CFG_PWR_DOWN:
        rc = mt9d115_power_down();
        break;
    case CFG_SET_EFFECT:        
        rc = mt9d115_set_effect(cdata.mode,cdata.cfg.effect);                
        break;
    case CFG_SET_WB:
    rc = mt9d115_set_wb(cdata.mode , cdata.cfg.wb_val);
    break;
    case CFG_SET_CONTRAST:
    rc = mt9d115_set_contrast(cdata.mode , cdata.cfg.contr_val);
    break;
    /* liyibo0001 : added brightness options at 2011-07-20 */
    case CFG_SET_BRIGHTNESS:
    rc = mt9d115_set_brightness(cdata.mode , cdata.cfg.brightness_val);
    break;
    /* liyibo0001 : added brightness end at 2011-07-20 */
    case CFG_SET_SATURATION:
    rc = mt9d115_set_saturation(cdata.mode , cdata.cfg.sat_val);
    break;
    case CFG_SET_SHARPNESS :
    rc = mt9d115_set_sharpness(cdata.mode , cdata.cfg.sharp_val);
    break;

    default:
        rc = 0;
        break;
    }  
    mutex_unlock(&mt9d115_mut);
    
    return rc;
}
static int mt9d115_sensor_release(void)
{
    int rc = -EBADF;
    mutex_lock(&mt9d115_mut);
    mt9d115_power_down();
    gpio_set_value_cansleep(mt9d115_ctrl->sensordata->sensor_reset, 0);
    gpio_free(mt9d115_ctrl->sensordata->sensor_reset);
    gpio_free(mt9d115_ctrl->sensordata->sensor_pwd);
    kfree(mt9d115_ctrl);
    mt9d115_ctrl = NULL;
    log_debug("mt9d115_release completed\n");
    mutex_unlock(&mt9d115_mut);
    wake_unlock(&mt9d115_wlock);
    return rc;
}

static int mt9d115_sensor_probe(const struct msm_camera_sensor_info *info,
        struct msm_sensor_ctrl *s)
{
    int rc = 0;
    log_debug("liyibo: i2c mt9d115\n");
    rc = i2c_add_driver(&mt9d115_i2c_driver);
    if (rc < 0 || mt9d115_client == NULL) {
        rc = -ENOTSUPP;
        goto probe_fail;
    }
    log_debug("liyibo: i2c mt9d115 111\n");
    msm_camio_clk_rate_set(mt9d115_DEFAULT_CLOCK_RATE);
    rc = mt9d115_probe_init_sensor(info);
    if (rc < 0)
        goto probe_fail;
    s->s_init = mt9d115_sensor_open_init;
    s->s_release = mt9d115_sensor_release;
    s->s_config  = mt9d115_sensor_config;
/*V11 liyibo0003 added sensor angle and modify from BACK_CAMERA_2D TO FRONT_CAMERA_2D at 2011-04-15*/
    s->s_camera_type = FRONT_CAMERA_2D;
    s->s_mount_angle = 0;
/*V11 liyibo0003 added and modify end at 2011-04-15*/
    mt9d115_probe_init_done(info);
    
	wake_lock_init(&mt9d115_wlock, WAKE_LOCK_SUSPEND, "mt9d115_wlock");
    
    return rc;
    
probe_fail:
    logs_err("mt9d115_sensor_probe: SENSOR PROBE FAILS!\n");
    i2c_del_driver(&mt9d115_i2c_driver);
    return rc;
}

static int __mt9d115_probe(struct platform_device *pdev)
{
    log_debug("liyibo: mt9d115_probe\n");
    return msm_camera_drv_start(pdev, mt9d115_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
    .probe = __mt9d115_probe,
    .driver = {
        .name = "msm_camera_mt9d115",
        .owner = THIS_MODULE,
    },
};

static int mt9d115_read_proc_info(char *page, char **start, off_t offset,
					int count, int *eof, void *data)
{	
	*eof = 1;
	return sprintf(page, "ID:0x2580\n" "Name:mt9d115\n");
}
static int mt9d115_proc_init(void)
{
	if (create_proc_read_entry("front_camera_info", 0, NULL, mt9d115_read_proc_info, NULL) == NULL) {
		logs_err("mt9d115 proc create error!\n");
	}

	return 0;
}

extern int zte_ftm_mod_ctl;
static int __init mt9d115_init(void)
{
		mt9d115_proc_init();
       if(zte_ftm_mod_ctl)
       {
          log_debug(KERN_ERR "ftm mode no regesiter mt9d115!\n");
	   return 0;	  
       }
    log_debug("liyibo: mt9d115_init\n");
    return platform_driver_register(&msm_camera_driver);
}

module_init(mt9d115_init);

MODULE_DESCRIPTION("OMNI VGA YUV sensor driver");
MODULE_LICENSE("GPL v2");

