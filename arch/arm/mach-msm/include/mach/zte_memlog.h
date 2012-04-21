/* ========================================================================
Copyright (c) 2001-2009 by ZTE Corporation.  All Rights Reserved.        

-------------------------------------------------------------------------------------------------
   Modify History
-------------------------------------------------------------------------------------------------
When           Who                   What 
20101203	SLF			used to power reason,SLF_BAT_20101203
20100413	JIANGFENG	Add ZTE_PLATFORM Micron, BOOT_JIANGFENG_20100413_01
=========================================================================== */
#ifndef ZTE_MEMLOG_H
#define ZTE_MEMLOG_H
#include <mach/msm_iomap.h>
#include <mach/msm_battery.h>
#define SMEM_LOG_INFO_BASE    MSM_SMEM_RAM_PHYS
#define SMEM_LOG_GLOBAL_BASE  (MSM_SMEM_RAM_PHYS + PAGE_SIZE)

#define SMEM_LOG_ENTRY_OFFSET (64*PAGE_SIZE)
#define SMEM_LOG_ENTRY_BASE   (MSM_SMEM_RAM_PHYS + SMEM_LOG_ENTRY_OFFSET)

#define ERR_DATA_MAX_SIZE 0x4000

#define MAGIC_VOLUME_DOWN_KEY 0x75898668 //"KYVD"
#define MAGIC_VOLUME_UP_KEY 0x75898680  //"KYVP"

//PM_JIANGFENG_20110211_01,start
typedef struct
{
	unsigned char display_flag;
	unsigned char img_id;
	unsigned char chg_fulled;
	unsigned char battery_capacity;
	unsigned char battery_valid;
} power_off_supply_status;
//PM_JIANGFENG_20110211_01,end

typedef struct {
  unsigned int ftm;
  unsigned int boot_reason;
  unsigned int reset_reason;
  unsigned int chg_count;
  unsigned int f3log;
  unsigned int err_fatal;
  unsigned int err_dload;
  unsigned int boot_pressed_keys[2];//0-upkey, 1-downkey; hml add to support exit ftm,
  char err_log[ERR_DATA_MAX_SIZE];
  unsigned char flash_id[2];//BOOT_JIANGFENG_20100413_01
  unsigned char sdrem_length;//0:128M; 1:256M; 2:384M; 3:512M; ...

  unsigned char reset_flag;
  unsigned char boot_success;
  //PM_JIANGFENG_20110211_01,start
	power_off_supply_status power_off_charge_info;
  //PM_JIANGFENG_20110211_01,end

  unsigned char pm_reason;//used to power reason,SLF_BAT_20101203
  
/* mboard_version be included by GPIO[173] GPIO[174] GPIO[175] in project N860-7x30,
                                  ordered by      bit[5]        bit[6]         bit[7]
*/
  unsigned int mboard_id;

 unsigned int key_is_on;  //ZTE_XJB_PM_20110706,power key event
  
  struct smem_batt_chg_t batchginfo;//YINTIANCI_BAT_20101101
} smem_global;

#endif
