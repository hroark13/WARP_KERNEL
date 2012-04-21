/*
 *  include/linux/mmc/sd.h
 *
 *  Copyright (C) 2005-2007 Pierre Ossman, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */

#ifndef MMC_SD_H
#define MMC_SD_H

#ifdef CONFIG_ZTE_DTV
#include <linux/ioctl.h>	/* for custom IOCTL for ACMD (austin.2011-04-05) */
#endif

/* SD commands                           type  argument     response */
  /* class 0 */
/* This is basically the same command as for MMC with some quirks. */
#define SD_SEND_RELATIVE_ADDR     3   /* bcr                     R6  */
#define SD_SEND_IF_COND           8   /* bcr  [11:0] See below   R7  */

  /* class 10 */
#define SD_SWITCH                 6   /* adtc [31:0] See below   R1  */

  /* Application commands */
#define SD_APP_SET_BUS_WIDTH      6   /* ac   [1:0] bus width    R1  */
#define SD_APP_SEND_NUM_WR_BLKS  22   /* adtc                    R1  */
#define SD_APP_OP_COND           41   /* bcr  [31:0] OCR         R3  */
#define SD_APP_SEND_SCR          51   /* adtc                    R1  */

#ifdef CONFIG_ZTE_DTV
  /* CPRM Application commands (austin.2011-03-22) */
#define SD_APP_GET_MKB			43
#define SD_APP_GET_MID			44
#define SD_APP_SET_CER_RN1		45
#define SD_APP_GET_CER_RN2		46
#define SD_APP_SET_CER_RES2		47
#define SD_APP_GET_CER_RES1		48
#define SD_APP_CHANGE_SECURE_AREA	49
#define SD_APP_SECURE_READ_MULTI_BLOCK		18
#define SD_APP_SECURE_WRITE_MULTI_BLOCK	25
#define SD_APP_SECURE_WRITE_MKB			26
#endif

/*
 * SD_SWITCH argument format:
 *
 *      [31] Check (0) or switch (1)
 *      [30:24] Reserved (0)
 *      [23:20] Function group 6
 *      [19:16] Function group 5
 *      [15:12] Function group 4
 *      [11:8] Function group 3
 *      [7:4] Function group 2
 *      [3:0] Function group 1
 */

/*
 * SD_SEND_IF_COND argument format:
 *
 *	[31:12] Reserved (0)
 *	[11:8] Host Voltage Supply Flags
 *	[7:0] Check Pattern (0xAA)
 */

/*
 * SCR field definitions
 */

#define SCR_SPEC_VER_0		0	/* Implements system specification 1.0 - 1.01 */
#define SCR_SPEC_VER_1		1	/* Implements system specification 1.10 */
#define SCR_SPEC_VER_2		2	/* Implements system specification 2.00 */

/*
 * SD bus widths
 */
#define SD_BUS_WIDTH_1		0
#define SD_BUS_WIDTH_4		2

/*
 * SD_SWITCH mode
 */
#define SD_SWITCH_CHECK		0
#define SD_SWITCH_SET		1

/*
 * SD_SWITCH function groups
 */
#define SD_SWITCH_GRP_ACCESS	0

/*
 * SD_SWITCH access modes
 */
#define SD_SWITCH_ACCESS_DEF	0
#define SD_SWITCH_ACCESS_HS	1

#ifdef CONFIG_ZTE_DTV
/*
 * ACMD IOCTL (austin.2011-04-05)
 */
struct sd_ioc_cmd {
    unsigned int struct_version;
#define SD_IOC_CMD_STRUCT_VERSION 0
    int write_flag;  /* implies direction of data.  true = write, false = read */
    unsigned int opcode;
    unsigned int arg;
    unsigned int flags;
    unsigned int postsleep_us;  /* apply usecond delay *after* issuing command */
    unsigned int force_timeout_ns;  /* force timeout to be force_timeout_ns ns */
    unsigned int response[4];  /* CMD response */
    unsigned int blksz;
    unsigned int blocks;
    unsigned char *data;  /* DAT buffer */
};
#define SD_IOC_ACMD _IOWR(MMC_BLOCK_MAJOR, 0, struct sd_ioc_cmd *)
#endif

#endif

