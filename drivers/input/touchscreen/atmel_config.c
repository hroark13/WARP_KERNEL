
/* ========================================================================================
when            who       what, where, why                         		comment tag
--------     ----       -------------------------------------    --------------------------
2010-12-28	 zfj		 add the configuratio for P855A10			ZTE_TS_ZFJ_20101228
2010-12-13   wly         v9£´ƒ¨»œ ˙∆¡                               ZTE_WLY_CRDB00586327
2010-11-24   wly         Ω‚æˆ ÷’∆‘⁄∆¡…œ£¨ÀØ√ﬂªΩ–—∫Û ˝æ›¬“±®Œ Ã‚     ZTE_WLY_CRDB00577718
========================================================================================*/

#include <linux/module.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <mach/board.h>
#include <asm/mach-types.h>
#include <linux/atmel_qt602240.h>
#include <linux/jiffies.h>
#include <mach/msm_hsusb.h>
#include <mach/vreg.h>

#if defined(CONFIG_MACH_BLADEPLUS)
static int atmel_platform_power(int on)
{
	int rc = -EINVAL;
	struct vreg *vreg_ldo8, 	*vreg_ldo15;

	vreg_ldo15 = vreg_get(NULL, "gp6");
	vreg_ldo8 = vreg_get(NULL, "gp7");

	if (!vreg_ldo8) {
		pr_err("%s: VREG L8 get failed\n", __func__);
		return rc;
	}
	if (!vreg_ldo15) {
		pr_err("%s: VREG L15 get failed\n", __func__);
		return rc;
	}
	
	rc = vreg_set_level(vreg_ldo8, 1850);
	if (rc) {
		pr_err("%s: VREG L8 set failed\n", __func__);
		goto ldo8_put;
	}
	if (on){

		rc = vreg_enable(vreg_ldo8);
		if (rc) {
			pr_err("%s: VREG L8 enable failed\n", __func__);
			goto ldo8_put;
		}
	}
	else 
	{
		rc = vreg_disable(vreg_ldo8);
		if (rc) {
			pr_err("%s: VREG L8 enable failed\n", __func__);
			goto ldo8_put;
		}
	}

	
	rc = vreg_set_level(vreg_ldo15, 3000);
	if (rc) {
		pr_err("%s: VREG L15 set failed\n", __func__);
		goto ldo15_put;
	}
	if (on){

		rc = vreg_enable(vreg_ldo15);
		if (rc) {
			pr_err("%s: VREG L15 enable failed\n", __func__);
			goto ldo15_put;
		}
	}
	else 
	{
		rc = vreg_disable(vreg_ldo15);
		if (rc) {
			pr_err("%s: VREG L15 enable failed\n", __func__);
			goto ldo15_put;
		}
	}
	return 0;

	ldo8_put:
	vreg_put(vreg_ldo8);
	ldo15_put:
	vreg_put(vreg_ldo15);
	return rc;
}
#elif defined(CONFIG_MACH_V9)
static int atmel_platform_power(int on)
{
	int rc = -EINVAL;
	struct vreg *vreg_ldo15;

	vreg_ldo15 = vreg_get(NULL, "gp6");

	if (!vreg_ldo15) {
		pr_err("%s: VREG L15 get failed\n", __func__);
		return rc;
	}
	
	rc = vreg_set_level(vreg_ldo15, 3000);
	if (rc) {
		pr_err("%s: VREG L15 set failed\n", __func__);
		goto ldo15_put;
	}
	if (on){

		rc = vreg_enable(vreg_ldo15);
		if (rc) {
			pr_err("%s: VREG L15 enable failed\n", __func__);
			goto ldo15_put;
		}
	}
	else 
	{
		rc = vreg_disable(vreg_ldo15);
		if (rc) {
			pr_err("%s: VREG L15 enable failed\n", __func__);
			goto ldo15_put;
		}
		}
	return 0;

	ldo15_put:
	vreg_put(vreg_ldo15);
	return rc;
	}
#elif defined( CONFIG_MACH_V9PLUS)
static int atmel_platform_power(int on)
{
	int rc = -EINVAL;
	struct vreg *vreg_ldo15;

	vreg_ldo15 = vreg_get(NULL, "gp6");

	if (!vreg_ldo15) {
		pr_err("%s: VREG L15 get failed\n", __func__);
		return rc;
	}
	
	rc = vreg_set_level(vreg_ldo15, 2600);
	if (rc) {
		pr_err("%s: VREG L15 set failed\n", __func__);
		goto ldo15_put;
	}
	if (on){

		rc = vreg_enable(vreg_ldo15);
		if (rc) {
			pr_err("%s: VREG L15 enable failed\n", __func__);
			goto ldo15_put;
		}
	}
	else 
	{
		rc = vreg_disable(vreg_ldo15);
		if (rc) {
			pr_err("%s: VREG L15 enable failed\n", __func__);
			goto ldo15_put;
		}
		}
	return 0;

	ldo15_put:
	vreg_put(vreg_ldo15);
	return rc;
	}
#elif defined(CONFIG_MACH_ARTHUR)||defined(CONFIG_MACH_SKATEPLUS)
static int atmel_platform_power(int on)
{
	int rc = -EINVAL;
	struct vreg *vreg_ldo15;	

	vreg_ldo15 = vreg_get(NULL, "gp6");

	if (!vreg_ldo15) {
		pr_err("%s: VREG L15get failed\n", __func__);
		return rc;
	}

		rc = vreg_set_level(vreg_ldo15, 3000);
		if (rc) {
			pr_err("%s: VREG L15 set failed\n", __func__);
		goto ldo15_put;
	}
	if (on){

		rc = vreg_enable(vreg_ldo15);
		if (rc) {
			pr_err("%s: VREG L15 enable failed\n", __func__);
			goto ldo15_put;
		}
		}
	else 
	{
		rc = vreg_disable(vreg_ldo15);
		if (rc) {
			pr_err("%s: VREG L15 disable failed\n", __func__);
			goto ldo15_put;
		}
	}
	return 0;

ldo15_put:
	vreg_put(vreg_ldo15);

	return rc;
}

#else
static int atmel_platform_power(int on)
{
	int rc = -EINVAL;
	struct vreg *vreg_ldo15;

	vreg_ldo15 = vreg_get(NULL, "gp6");

	if (!vreg_ldo15) {
		pr_err("%s: VREG L15 get failed\n", __func__);
		return rc;
	}
	
	rc = vreg_set_level(vreg_ldo15, 3000);
	if (rc) {
		pr_err("%s: VREG L15 set failed\n", __func__);
		goto ldo15_put;
	}
	if (on){

		rc = vreg_enable(vreg_ldo15);
		if (rc) {
			pr_err("%s: VREG L15 enable failed\n", __func__);
			goto ldo15_put;
		}
	}
	else 
	{
		rc = vreg_disable(vreg_ldo15);
		if (rc) {
			pr_err("%s: VREG L15 enable failed\n", __func__);
			goto ldo15_put;
		}
	}
	return 0;

	ldo15_put:
	vreg_put(vreg_ldo15);
	return rc;
}
#endif

struct atmel_i2c_platform_data atmel_data = {
#ifdef CONFIG_MACH_BLADEPLUS 
#ifdef CONFIG_TOUCHSCREEN_MXT224_P855D10
  .version = 0x16,
  .source = 1,
  .abs_x_min = 0,
  .abs_x_max = 479,
  .abs_y_max = 799,
  .abs_y_min = 0,
  .x_line=16,
  .y_line=10,
  .abs_pressure_min = 0,
  .abs_pressure_max = 255,
  .abs_width_min = 0,
  .abs_width_max = 15,
  .gpio_irq = 55,
  .power = atmel_platform_power,
  .config_T7[0] = 30,
  .config_T7[1] = 255,
  .config_T7[2] = 25,
  .config_T8[0] = 8,
  .config_T8[1] = 0,
  .config_T8[2] = 3,
  .config_T8[3] = 1,
  .config_T8[4] = 10,
  .config_T8[5] = 0,
  .config_T8[6] = 0,
  .config_T8[7] = 0,
  .config_T8[8] = 15,
  .config_T8[9] = 0xc0,

  .config_T9[0] = 131,
  .config_T9[1] = 0,
  .config_T9[2] = 0,
  .config_T9[3] = 16,
  .config_T9[4] = 10,
  .config_T9[5] = 1,
  .config_T9[6] = 0x10,
  .config_T9[7] = 40,
  //.config_T9[7] = 35,
  .config_T9[8] = 3,
  //.config_T9[9] = 6,//∫·∆¡
  .config_T9[9] = 1,  //Modity driection for P855A10
  .config_T9[10] = 10,
  .config_T9[11] = 6,
  .config_T9[12] = 3,
  .config_T9[13] = 79,//movefilter
  .config_T9[14] = 5,//touch numbers
  .config_T9[15] = 10,
  .config_T9[16] = 30,
  .config_T9[17] = 30,
  .config_T9[18] = 0x1F,
  .config_T9[19] = 03,
  .config_T9[20] = 0xDF,
  .config_T9[21] = 01,
	.config_T9[22] = 10,
	.config_T9[23] = 5,
	.config_T9[24] = 20,
	.config_T9[25] = 23,
	.config_T9[26] = 208,
	.config_T9[27] = 58,
	.config_T9[28] = 144,
	.config_T9[29] = 86,
  .config_T9[30] = 15,
  .config_T9[31] = 10,

  .config_T15[0] = 0,
  .config_T15[1] = 0,
  .config_T15[2] = 0,
  .config_T15[3] = 0,
  .config_T15[4] = 0,
  .config_T15[5] = 0,
  .config_T15[6] = 0,
  .config_T15[7] = 0,
  .config_T15[8] = 0,
  .config_T18[0] = 0,
  .config_T18[1] = 0,
  .config_T19[0] = 0,
  .config_T19[1] = 0,
  .config_T19[2] = 0,
  .config_T19[3] = 0,
  .config_T19[4] = 0,
  .config_T19[5] = 0,
  .config_T19[6] = 0,
  .config_T19[7] = 0,
  .config_T19[8] = 0,
  .config_T19[9] = 0,
  .config_T19[10] = 0,
  .config_T19[11] = 0,
  .config_T20[0] = 0x13,
  .config_T20[1] = 1,
  .config_T20[2] = 1,
  .config_T20[3] = 1,
  .config_T20[4] = 1,
  .config_T20[5] = 10,
  .config_T20[6] = 0,
  .config_T20[7] = 25,
  .config_T20[8] = 40,
  .config_T20[9] = 4,
  .config_T20[10] = 15,
  .config_T20[11] = 0,
  .config_T22[0] = 0x0d,
  .config_T22[1] = 0,
  .config_T22[2] = 0,
  .config_T22[3] = 0x19,
  .config_T22[4] = 0x00,
  .config_T22[5] = 0xe7,
  .config_T22[6] = 0xff,
  .config_T22[7] = 4,
  .config_T22[8] = 30,
  .config_T22[9] = 0,
  .config_T22[10] = 0,
  .config_T22[11] = 11,
  .config_T22[12] = 24,
  .config_T22[13] = 25,
  .config_T22[14] = 31,
  .config_T22[15] = 51,
  .config_T22[16] = 4,
  .config_T23[0] = 0,
  .config_T23[1] = 0,
  .config_T23[2] = 0,
  .config_T23[3] = 0,
  .config_T23[4] = 0,
  .config_T23[5] = 0,
  .config_T23[6] = 0,
  .config_T23[7] = 0,
  .config_T23[8] = 0,
  .config_T23[9] = 0,
  .config_T23[10] = 0,
  .config_T23[11] = 0,
  .config_T23[12] = 0,
  .config_T24[0] = 0,
  .config_T24[1] = 0,
  .config_T24[2] = 0,
  .config_T24[3] = 0,
  .config_T24[4] = 0,
  .config_T24[5] = 0,
  .config_T24[6] = 0,
  .config_T24[7] = 0,
  .config_T24[8] = 0,
  .config_T24[9] = 0,
  .config_T24[10] = 0,
  .config_T24[11] = 0,
  .config_T24[12] = 0,
  .config_T24[13] = 0,
  .config_T24[14] = 0,
  .config_T24[15] = 0,
  .config_T24[16] = 0,
  .config_T24[17] = 0,
  .config_T24[18] = 0,
  .config_T25[0] = 0,
  .config_T25[1] = 0,
  .config_T25[2] = 0xe0,
  .config_T25[3] = 0x2e,
  .config_T25[4] = 0x58,
  .config_T25[5] = 0x1b,
  .config_T25[6] = 0xe0,
  .config_T25[7] = 0x2e,
  .config_T25[8] = 0x58,
  .config_T25[9] = 0x1b,
  .config_T27[0] = 0,
  .config_T27[1] = 0,
  .config_T27[2] = 0,
  .config_T27[3] = 0,
  .config_T27[4] = 0,
  .config_T27[5] = 0,
  .config_T27[6] = 0,
  .config_T28[0] = 128,
  .config_T28[1] = 0,
  .config_T28[2] = 0,
  .config_T28[3] = 16,
  .config_T28[4] = 16,
  .config_T28[5] = 0x1e,
  .config_T35[0] = 7,
  .config_T35[1] = 45,  
  .config_T35[2] = 20,
  .config_T35[3] = 100, 
  .config_T35[4] = 1,
	.config_T38[0] = 1,
	.config_T38[1] = 2,  
	.config_T38[2] = 3,
	.config_T38[3] = 4, 
	.config_T38[4] = 5,
	.config_T38[5] = 8,  
	.config_T38[6] = 7,
	.config_T38[7] = 8, 
	.config_T59[0] = 7,
	.config_T59[1] = 36,  
	.config_T59[2] = 255,
	.config_T59[3] = 10, 
	.config_T59[4] = 1,
	.config_T59[5] = 15,  

  #ifdef CONFIG_ATMEL_TS_USB_NOTIFY
			.config_T9_charge[0] = 70,//10	 //t9[7]
			.config_T9_charge[1] = 10,//10	  //t9[31]
			.config_T9_charge[2] = 10,//10	//t9[11]
			.config_T9_charge[3] = 10,//10	//t9[12]
			.config_T28_charge[0] = 16,//0x0a
			.config_T28_charge[1] = 16,//0x0a	
#endif
  .object_crc[0] = 94,
  .object_crc[1] = 233,
  .object_crc[2] = 182,
  /*.cable_config[0] = config_T9[7],
  .cable_config[1] = config_T22[8],
  .cable_config[2] = config_T28[3],
  .cable_config[3] = config_T28[4],*/
  .cable_config[0] = 30,
  .cable_config[1] = 20,
  .cable_config[2] = 4,
  .cable_config[3] = 8,
  .GCAF_level[0] = 4,
  .GCAF_level[1] = 16,
  .GCAF_level[2] = 0,
  .GCAF_level[3] = 0,
  .filter_level[0] = 100,
  .filter_level[1] = 100,
  .filter_level[2] = 100,
  .filter_level[3] = 100,
  .display_width = 1024,	/* display width in pixel */
  .display_height = 1024,	/* display height in pixel */

#else
  .version = 0x16,
  .source = 1,
  .abs_x_min = 0,
  .abs_x_max = 479,
  .abs_y_max = 799,
  .abs_y_min = 0,
  .abs_pressure_min = 0,
  .abs_pressure_max = 255,
  .abs_width_min = 0,
  .abs_width_max = 15,
  .gpio_irq = 55,
  .power = atmel_platform_power,
  .config_T7[0] = 30,
  .config_T7[1] = 255,
  .config_T7[2] = 25,
  .config_T8[0] = 8,
  .config_T8[1] = 0,
  .config_T8[2] = 10,
  .config_T8[3] = 10,
  .config_T8[4] = 10,
  .config_T8[5] = 0,
  .config_T8[6] = 0,
  .config_T8[7] = 0,
  .config_T8[8] = 15,
  .config_T8[9] = 0xc0,

  .config_T9[0] = 131,
  .config_T9[1] = 0,
  .config_T9[2] = 0,
  .config_T9[3] = 16,
  .config_T9[4] = 10,
  .config_T9[5] = 1,
  .config_T9[6] = 0x10,
  .config_T9[7] = 40,
  //.config_T9[7] = 35,
  .config_T9[8] = 3,
  //.config_T9[9] = 6,//∫·∆¡
  .config_T9[9] = 1,  //Modity driection for P855A10
  .config_T9[10] = 10,
  .config_T9[11] = 6,
  .config_T9[12] = 3,
  .config_T9[13] = 79,//movefilter
  .config_T9[14] = 5,//touch numbers
  .config_T9[15] = 10,
  .config_T9[16] = 30,
  .config_T9[17] = 30,
  .config_T9[18] = 0x1F,
  .config_T9[19] = 03,
  .config_T9[20] = 0xDF,
  .config_T9[21] = 01,
	.config_T9[22] = 10,
	.config_T9[23] = 5,
	.config_T9[24] = 20,
	.config_T9[25] = 23,
	.config_T9[26] = 208,
	.config_T9[27] = 58,
	.config_T9[28] = 144,
	.config_T9[29] = 86,
  .config_T9[30] = 15,
  .config_T9[31] = 10,

  .config_T15[0] = 0,
  .config_T15[1] = 0,
  .config_T15[2] = 0,
  .config_T15[3] = 0,
  .config_T15[4] = 0,
  .config_T15[5] = 0,
  .config_T15[6] = 0,
  .config_T15[7] = 0,
  .config_T15[8] = 0,
  .config_T18[0] = 0,
  .config_T18[1] = 0,
  .config_T19[0] = 0,
  .config_T19[1] = 0,
  .config_T19[2] = 0,
  .config_T19[3] = 0,
  .config_T19[4] = 0,
  .config_T19[5] = 0,
  .config_T19[6] = 0,
  .config_T19[7] = 0,
  .config_T19[8] = 0,
  .config_T19[9] = 0,
  .config_T19[10] = 0,
  .config_T19[11] = 0,
	.config_T20[0] = 0x13,
	.config_T20[1] = 1,
	.config_T20[2] = 1,
	.config_T20[3] = 1,
	.config_T20[4] = 1,
	.config_T20[5] = 10,
	.config_T20[6] = 0,
	.config_T20[7] = 25,
	.config_T20[8] = 40,
	.config_T20[9] = 4,
	.config_T20[10] = 15,
	.config_T20[11] = 0,

  .config_T22[0] = 0x0d,
  .config_T22[1] = 0,
  .config_T22[2] = 0,
  .config_T22[3] = 0x19,
  .config_T22[4] = 0x00,
  .config_T22[5] = 0xe7,
  .config_T22[6] = 0xff,
  .config_T22[7] = 4,
  .config_T22[8] = 30,
  .config_T22[9] = 1,
  .config_T22[10] = 0,
  .config_T22[11] = 24,
  .config_T22[12] = 51,
  .config_T22[13] = 13,
  .config_T22[14] = 25,
  .config_T22[15] = 0,
  .config_T22[16] = 4,
  .config_T23[0] = 0,
  .config_T23[1] = 0,
  .config_T23[2] = 0,
  .config_T23[3] = 0,
  .config_T23[4] = 0,
  .config_T23[5] = 0,
  .config_T23[6] = 0,
  .config_T23[7] = 0,
  .config_T23[8] = 0,
  .config_T23[9] = 0,
  .config_T23[10] = 0,
  .config_T23[11] = 0,
  .config_T23[12] = 0,
  .config_T24[0] = 0,
  .config_T24[1] = 0,
  .config_T24[2] = 0,
  .config_T24[3] = 0,
  .config_T24[4] = 0,
  .config_T24[5] = 0,
  .config_T24[6] = 0,
  .config_T24[7] = 0,
  .config_T24[8] = 0,
  .config_T24[9] = 0,
  .config_T24[10] = 0,
  .config_T24[11] = 0,
  .config_T24[12] = 0,
  .config_T24[13] = 0,
  .config_T24[14] = 0,
  .config_T24[15] = 0,
  .config_T24[16] = 0,
  .config_T24[17] = 0,
  .config_T24[18] = 0,
  .config_T25[0] = 0,
  .config_T25[1] = 0,
  .config_T25[2] = 0xe0,
  .config_T25[3] = 0x2e,
  .config_T25[4] = 0x58,
  .config_T25[5] = 0x1b,
  .config_T25[6] = 0xe0,
  .config_T25[7] = 0x2e,
  .config_T25[8] = 0x58,
  .config_T25[9] = 0x1b,
  .config_T27[0] = 0,
  .config_T27[1] = 1,
  .config_T27[2] = 0,
  .config_T27[3] = 0,
  .config_T27[4] = 0,
  .config_T27[5] = 0,
  .config_T27[6] = 0,
  .config_T28[0] = 0,
  .config_T28[1] = 0,
  .config_T28[2] = 0,
  .config_T28[3] = 16,
  .config_T28[4] = 16,
  .config_T28[5] = 0x1e,
	.config_T59[0] = 7,
	.config_T59[1] = 36,  
	.config_T59[2] = 255,
	.config_T59[3] = 10, 
	.config_T59[4] = 1,
	.config_T59[5] = 15,    
  #ifdef CONFIG_ATMEL_TS_USB_NOTIFY
			.config_T9_charge[0] = 70,//10	 //t9[7]
			.config_T9_charge[1] = 10,//10	  //t9[31]
			.config_T9_charge[2] = 10,//10	//t9[11]
			.config_T9_charge[3] = 10,//10	//t9[12]
			.config_T28_charge[0] = 16,//0x0a
			.config_T28_charge[1] = 16,//0x0a	
#endif
  .object_crc[0] = 94,
  .object_crc[1] = 233,
  .object_crc[2] = 182,
  /*.cable_config[0] = config_T9[7],
  .cable_config[1] = config_T22[8],
  .cable_config[2] = config_T28[3],
  .cable_config[3] = config_T28[4],*/
  .cable_config[0] = 30,
  .cable_config[1] = 20,
  .cable_config[2] = 4,
  .cable_config[3] = 8,
  .GCAF_level[0] = 4,
  .GCAF_level[1] = 16,
  .GCAF_level[2] = 0,
  .GCAF_level[3] = 0,
  .filter_level[0] = 100,
  .filter_level[1] = 100,
  .filter_level[2] = 100,
  .filter_level[3] = 100,
  .display_width = 1024,	/* display width in pixel */
  .display_height = 1024,	/* display height in pixel */
#endif
#elif defined( CONFIG_MACH_V9PLUS)
  .version = 0x16,
  .source = 1,
  .abs_x_min = 0,
  .abs_x_max = 599,
  .abs_y_max = 1023,
  .abs_y_min = 0,
  .abs_pressure_min = 0,
  .abs_pressure_max = 255,
  .abs_width_min = 0,
  .abs_width_max = 15,
  .gpio_irq = 55,
  .power = atmel_platform_power,
  .config_T7[0] = 30,
  .config_T7[1] = 255,
  .config_T7[2] = 25,
  .config_T8[0] = 10,
  .config_T8[1] = 0,
  .config_T8[2] = 5,
  .config_T8[3] = 5,
  .config_T8[4] = 0,
  .config_T8[5] = 0,
  .config_T8[6] = 5,
  .config_T8[7] = 32,
  .config_T9[0] = 131,
  .config_T9[1] = 0,
  .config_T9[2] = 0,
  .config_T9[3] = 18,
  .config_T9[4] = 11,
  .config_T9[5] = 1,
  .config_T9[6] = 1,
  .config_T9[7] = 35,
  //.config_T9[7] = 35,
  .config_T9[8] = 3,
  //.config_T9[9] = 6,//∫·∆¡
  .config_T9[9] = 3,  // ˙∆¡
  .config_T9[10] = 10,
  .config_T9[11] = 1,
  .config_T9[12] = 1,
  .config_T9[13] = 0x30,//movefilter
  .config_T9[14] = 5,//touch numbers
  .config_T9[15] = 10,
  .config_T9[16] = 30,
  .config_T9[17] = 30,
  .config_T9[18] = 0,
  .config_T9[19] = 0,
  .config_T9[20] = 87,
  .config_T9[21] = 2,
  .config_T9[22] = 31,
  .config_T9[23] = 31,
  .config_T9[24] = -3,
  .config_T9[25] = 0,
  .config_T9[26] = 208,
  .config_T9[27] = 31,
  .config_T9[28] = 72,
  .config_T9[29] = 31,
  .config_T9[30] = 18,
  .config_T15[0] = 3,
  .config_T15[1] = 0,
  .config_T15[2] = 11,
  .config_T15[3] = 3,
  .config_T15[4] = 1,
  .config_T15[5] = 2,
  .config_T15[6] = 1,
  .config_T15[7] = 35,
  .config_T15[8] = 2,
  .config_T18[0] = 0,
  .config_T18[1] = 0,
  .config_T19[0] = 0,
  .config_T19[1] = 0,
  .config_T19[2] = 0,
  .config_T19[3] = 0,
  .config_T19[4] = 0,
  .config_T19[5] = 0,
  .config_T19[6] = 0,
  .config_T19[7] = 0,
  .config_T19[8] = 0,
  .config_T19[9] = 0,
  .config_T19[10] = 0,
  .config_T19[11] = 0,
  .config_T20[0] = 0,
  .config_T20[1] = 100,
  .config_T20[2] = 100,
  .config_T20[3] = 100,
  .config_T20[4] = 100,
  .config_T20[5] = 0,
  .config_T20[6] = 0,
  .config_T20[7] = 0,
  .config_T20[8] = 0,
  .config_T20[9] = 0,
  .config_T20[10] = 0,
  .config_T20[11] = 0,
  .config_T22[0] = 0x0d,
  .config_T22[1] = 0,
  .config_T22[2] = 0,
  .config_T22[3] = 0x19,
  .config_T22[4] = 0x00,
  .config_T22[5] = 0xe7,
  .config_T22[6] = 0xff,
  .config_T22[7] = 4,
  .config_T22[8] = 20,
  .config_T22[9] = 1,
  .config_T22[10] = 0,
  .config_T22[11] = 16,
  .config_T22[12] = 31,
  .config_T22[13] = 57,
  .config_T22[14] = 65,
  .config_T22[15] = 0,
  .config_T22[16] = 4,
  .config_T23[0] = 0,
  .config_T23[1] = 0,
  .config_T23[2] = 0,
  .config_T23[3] = 0,
  .config_T23[4] = 0,
  .config_T23[5] = 0,
  .config_T23[6] = 0,
  .config_T23[7] = 0,
  .config_T23[8] = 0,
  .config_T23[9] = 0,
  .config_T23[10] = 0,
  .config_T23[11] = 0,
  .config_T23[12] = 0,
  .config_T24[0] = 0,
  .config_T24[1] = 0,
  .config_T24[2] = 0,
  .config_T24[3] = 0,
  .config_T24[4] = 0,
  .config_T24[5] = 100,
  .config_T24[6] = 100,
  .config_T24[7] = 1,
  .config_T24[8] = 10,
  .config_T24[9] = 20,
  .config_T24[10] = 40,
  .config_T24[11] = 0,
  .config_T24[12] = 0,
  .config_T24[13] = 0,
  .config_T24[14] = 0,
  .config_T24[15] = 0,
  .config_T24[16] = 0,
  .config_T24[17] = 0,
  .config_T24[18] = 0,
  .config_T25[0] = 0,
  .config_T25[1] = 0,
  .config_T25[2] = 0xe0,
  .config_T25[3] = 0x2e,
  .config_T25[4] = 0x58,
  .config_T25[5] = 0x1b,
  .config_T25[6] = 0xe0,
  .config_T25[7] = 0x2e,
  .config_T25[8] = 0x58,
  .config_T25[9] = 0x1b,
  .config_T27[0] = 0,
  .config_T27[1] = 1,
  .config_T27[2] = 0,
  .config_T27[3] = 0,
  .config_T27[4] = 0,
  .config_T27[5] = 0,
  .config_T27[6] = 0,
  .config_T28[0] = 0,
  .config_T28[1] = 0,
  .config_T28[2] = 2,
  .config_T28[3] = 12,
  .config_T28[4] = 12,
  .config_T28[5] = 0x1e,
  .object_crc[0] = 22,
  .object_crc[1] = 245,
  .object_crc[2] = 101,
  /*.cable_config[0] = config_T9[7],
  .cable_config[1] = config_T22[8],
  .cable_config[2] = config_T28[3],
  .cable_config[3] = config_T28[4],*/
  .cable_config[0] = 30,
  .cable_config[1] = 20,
  .cable_config[2] = 4,
  .cable_config[3] = 8,
  .GCAF_level[0] = 4,
  .GCAF_level[1] = 16,
  .GCAF_level[2] = 0,
  .GCAF_level[3] = 0,
  .filter_level[0] = 100,
  .filter_level[1] = 100,
  .filter_level[2] = 100,
  .filter_level[3] = 100,
  .display_width = 1024,	/* display width in pixel */
  .display_height = 1024,	/* display height in pixel */
#elif defined( CONFIG_MACH_ARTHUR)
  .version = 0x16,
  .source = 1,
  .abs_x_min = 0,
  .abs_x_max = 479,
  .abs_y_max = 799,
  .abs_y_min = 0,
  .abs_pressure_min = 0,
  .abs_pressure_max = 255,
  .abs_width_min = 0,
  .abs_width_max = 15,
  .gpio_irq = 55,
  .power = atmel_platform_power,
  .config_T7[0] = 30,
  .config_T7[1] = 255,
  .config_T7[2] = 25,
  .config_T8[0] = 8,
  .config_T8[1] = 0,
  .config_T8[2] = 20,
  .config_T8[3] = 20,
  .config_T8[4] = 0,
  .config_T8[5] = 0,
  .config_T8[6] = 5,
  .config_T8[7] = 15,
  .config_T9[0] = 131,
  .config_T9[1] = 0,
  .config_T9[2] = 0,
  .config_T9[3] = 19,
  .config_T9[4] = 11,
  .config_T9[5] = 1,
  .config_T9[6] = 0x10,
  .config_T9[7] = 38,
  .config_T9[8] = 3,
  .config_T9[9] = 1,
  .config_T9[10] = 10,
  .config_T9[11] = 6,
  .config_T9[12] = 3,
  .config_T9[13] = 0x30,//movefilter
  .config_T9[14] = 5,//touch numbers
  .config_T9[15] = 30,
  .config_T9[16] = 30,
  .config_T9[17] = 30,
  .config_T9[18] = 0x5B,
  .config_T9[19] = 03,
  .config_T9[20] = 0xDF,
  .config_T9[21] = 01,
	.config_T9[22] = 0,//31,
	.config_T9[23] = 0,//31,
	.config_T9[24] = 0,
	.config_T9[25] = 0,
	.config_T9[26] = 0,//208,
	.config_T9[27] = 0,//31,
	.config_T9[28] = 0,//80,
	.config_T9[29] = 0,//31,
  .config_T9[30] = 18,
  .config_T15[0] = 0,
  .config_T15[1] = 0,
  .config_T15[2] = 0,
  .config_T15[3] = 0,
  .config_T15[4] = 0,
  .config_T15[5] = 0,
  .config_T15[6] = 0,
  .config_T15[7] = 0,
  .config_T15[8] = 0,
  .config_T18[0] = 0,
  .config_T18[1] = 0,
  .config_T19[0] = 0,
  .config_T19[1] = 0,
  .config_T19[2] = 0,
  .config_T19[3] = 0,
  .config_T19[4] = 0,
  .config_T19[5] = 0,
  .config_T19[6] = 0,
  .config_T19[7] = 0,
  .config_T19[8] = 0,
  .config_T19[9] = 0,
  .config_T19[10] = 0,
  .config_T19[11] = 0,
  .config_T20[0] = 25,
  .config_T20[1] = 1,
  .config_T20[2] = 1,
  .config_T20[3] = 1,
  .config_T20[4] = 1,
  .config_T20[5] = 0,
  .config_T20[6] = 0,
  .config_T20[7] = 0,
  .config_T20[8] = 0,
  .config_T20[9] = 0,
  .config_T20[10] = 0,
  .config_T20[11] = 0,
  .config_T22[0] = 141,
  .config_T22[1] = 0,
  .config_T22[2] = 0,
  .config_T22[3] = 0x19,
  .config_T22[4] = 0x00,
  .config_T22[5] = 0xe7,
  .config_T22[6] = 0xff,
  .config_T22[7] = 4,
  .config_T22[8] = 20,
  .config_T22[9] = 0,
  .config_T22[10] = 1,
  .config_T22[11] = 5,
  .config_T22[12] = 10,
  .config_T22[13] = 15,
  .config_T22[14] = 20,
  .config_T22[15] = 25,
  .config_T22[16] = 4,
  .config_T23[0] = 0,
  .config_T23[1] = 0,
  .config_T23[2] = 0,
  .config_T23[3] = 0,
  .config_T23[4] = 0,
  .config_T23[5] = 0,
  .config_T23[6] = 0,
  .config_T23[7] = 0,
  .config_T23[8] = 0,
  .config_T23[9] = 0,
  .config_T23[10] = 0,
  .config_T23[11] = 0,
  .config_T23[12] = 0,
  .config_T24[0] = 0,
  .config_T24[1] = 0,
  .config_T24[2] = 0,
  .config_T24[3] = 0,
  .config_T24[4] = 0,
  .config_T24[5] = 0,
  .config_T24[6] = 0,
  .config_T24[7] = 0,
  .config_T24[8] = 0,
  .config_T24[9] = 0,
  .config_T24[10] = 0,
  .config_T24[11] = 0,
  .config_T24[12] = 0,
  .config_T24[13] = 0,
  .config_T24[14] = 0,
  .config_T24[15] = 0,
  .config_T24[16] = 0,
  .config_T24[17] = 0,
  .config_T24[18] = 0,
  .config_T25[0] = 0,
  .config_T25[1] = 0,
  .config_T25[2] = 0xe0,
  .config_T25[3] = 0x2e,
  .config_T25[4] = 0x58,
  .config_T25[5] = 0x1b,
  .config_T25[6] = 0xe0,
  .config_T25[7] = 0x2e,
  .config_T25[8] = 0x58,
  .config_T25[9] = 0x1b,
  .config_T27[0] = 0,
  .config_T27[1] = 0,
  .config_T27[2] = 0,
  .config_T27[3] = 0,
  .config_T27[4] = 0,
  .config_T27[5] = 0,
  .config_T27[6] = 0,
  .config_T28[0] = 0,
  .config_T28[1] = 0,
  .config_T28[2] = 3,
  .config_T28[3] = 16,
  .config_T28[4] = 16,
  .config_T28[5] = 0x1e,
  .object_crc[0] = 117,
  .object_crc[1] = 8,
  .object_crc[2] = 119,
  .cable_config[0] = 30,
  .cable_config[1] = 20,
  .cable_config[2] = 4,
  .cable_config[3] = 8,
  .GCAF_level[0] = 4,
  .GCAF_level[1] = 16,
  .GCAF_level[2] = 0,
  .GCAF_level[3] = 0,
  .filter_level[0] = 100,
  .filter_level[1] = 100,
  .filter_level[2] = 100,
  .filter_level[3] = 100,
  .display_width = 1024,	/* display width in pixel */
  .display_height = 1024,	/* display height in pixel */
#elif defined(CONFIG_MACH_SKATEPLUS)
.version = 0x16,
.source = 1,
.abs_x_min = 0,
.abs_x_max = 479,
.abs_y_max = 799,
.abs_y_min = 0,
.abs_pressure_min = 0,
.abs_pressure_max = 255,
.abs_width_min = 0,
.abs_width_max = 15,
.gpio_irq = 55,
.power = atmel_platform_power,
.config_T7[0] = 30,
.config_T7[1] = 255,
.config_T7[2] = 25,
.config_T8[0] = 8,
.config_T8[1] = 0,
.config_T8[2] = 20,
.config_T8[3] = 20,
.config_T8[4] = 0,
.config_T8[5] = 0,
.config_T8[6] = 5,
.config_T8[7] = 15,
.config_T9[0] = 131,
.config_T9[1] = 0,
.config_T9[2] = 0,
.config_T9[3] = 19,
.config_T9[4] = 11,
.config_T9[5] = 1,
.config_T9[6] = 0x10,
.config_T9[7] = 38,
.config_T9[8] = 3,
.config_T9[9] = 1,
.config_T9[10] = 10,
.config_T9[11] = 6,
.config_T9[12] = 3,
.config_T9[13] = 0x30,//movefilter
.config_T9[14] = 5,//touch numbers
.config_T9[15] = 30,
.config_T9[16] = 30,
.config_T9[17] = 30,
.config_T9[18] = 0x5B,
.config_T9[19] = 03,
.config_T9[20] = 0xDF,
.config_T9[21] = 01,
  .config_T9[22] = 5,//31,
  .config_T9[23] = 5,//31,
  .config_T9[24] = 20,
  .config_T9[25] = 20,
  .config_T9[26] = 208,//208,
  .config_T9[27] = 56,//31,
  .config_T9[28] = 208,//80,
  .config_T9[29] = 86,//31,
.config_T9[30] = 18,
.config_T15[0] = 0,
.config_T15[1] = 0,
.config_T15[2] = 0,
.config_T15[3] = 0,
.config_T15[4] = 0,
.config_T15[5] = 0,
.config_T15[6] = 0,
.config_T15[7] = 0,
.config_T15[8] = 0,
.config_T18[0] = 0,
.config_T18[1] = 0,
.config_T19[0] = 0,
.config_T19[1] = 0,
.config_T19[2] = 0,
.config_T19[3] = 0,
.config_T19[4] = 0,
.config_T19[5] = 0,
.config_T19[6] = 0,
.config_T19[7] = 0,
.config_T19[8] = 0,
.config_T19[9] = 0,
.config_T19[10] = 0,
.config_T19[11] = 0,
.config_T20[0] = 25,
.config_T20[1] = 1,
.config_T20[2] = 1,
.config_T20[3] = 1,
.config_T20[4] = 1,
.config_T20[5] = 0,
.config_T20[6] = 0,
.config_T20[7] = 0,
.config_T20[8] = 0,
.config_T20[9] = 0,
.config_T20[10] = 0,
.config_T20[11] = 0,
.config_T22[0] = 141,
.config_T22[1] = 0,
.config_T22[2] = 0,
.config_T22[3] = 0x19,
.config_T22[4] = 0x00,
.config_T22[5] = 0xe7,
.config_T22[6] = 0xff,
.config_T22[7] = 4,
.config_T22[8] = 20,
.config_T22[9] = 0,
.config_T22[10] = 1,
.config_T22[11] = 5,
.config_T22[12] = 10,
.config_T22[13] = 15,
.config_T22[14] = 20,
.config_T22[15] = 25,
.config_T22[16] = 4,
.config_T23[0] = 0,
.config_T23[1] = 0,
.config_T23[2] = 0,
.config_T23[3] = 0,
.config_T23[4] = 0,
.config_T23[5] = 0,
.config_T23[6] = 0,
.config_T23[7] = 0,
.config_T23[8] = 0,
.config_T23[9] = 0,
.config_T23[10] = 0,
.config_T23[11] = 0,
.config_T23[12] = 0,
.config_T24[0] = 0,
.config_T24[1] = 0,
.config_T24[2] = 0,
.config_T24[3] = 0,
.config_T24[4] = 0,
.config_T24[5] = 0,
.config_T24[6] = 0,
.config_T24[7] = 0,
.config_T24[8] = 0,
.config_T24[9] = 0,
.config_T24[10] = 0,
.config_T24[11] = 0,
.config_T24[12] = 0,
.config_T24[13] = 0,
.config_T24[14] = 0,
.config_T24[15] = 0,
.config_T24[16] = 0,
.config_T24[17] = 0,
.config_T24[18] = 0,
.config_T25[0] = 0,
.config_T25[1] = 0,
.config_T25[2] = 0xe0,
.config_T25[3] = 0x2e,
.config_T25[4] = 0x58,
.config_T25[5] = 0x1b,
.config_T25[6] = 0xe0,
.config_T25[7] = 0x2e,
.config_T25[8] = 0x58,
.config_T25[9] = 0x1b,
.config_T27[0] = 0,
.config_T27[1] = 0,
.config_T27[2] = 0,
.config_T27[3] = 0,
.config_T27[4] = 0,
.config_T27[5] = 0,
.config_T27[6] = 0,
.config_T28[0] = 0,
.config_T28[1] = 0,
.config_T28[2] = 3,
.config_T28[3] = 16,
.config_T28[4] = 16,
.config_T28[5] = 0x1e,
  #ifdef CONFIG_ATMEL_TS_USB_NOTIFY
				.config_T9_charge[0] = 70,//10	 //t9[7]
				.config_T9_charge[1] = 0,//10	  //t9[31]
				.config_T9_charge[2] = 10,//10	//t9[11]
				.config_T9_charge[3] = 10,//10	//t9[12]
				.config_T28_charge[0] = 16,//0x0a
				.config_T28_charge[1] = 16,//0x0a	
#endif

.object_crc[0] = 117,
.object_crc[1] = 8,
.object_crc[2] = 119,
.cable_config[0] = 30,
.cable_config[1] = 20,
.cable_config[2] = 4,
.cable_config[3] = 8,
.GCAF_level[0] = 4,
.GCAF_level[1] = 16,
.GCAF_level[2] = 0,
.GCAF_level[3] = 0,
.filter_level[0] = 100,
.filter_level[1] = 100,
.filter_level[2] = 100,
.filter_level[3] = 100,
.display_width = 1024,	  /* display width in pixel */
.display_height = 1024,   /* display height in pixel */


#elif defined( CONFIG_MACH_SEAN)
	.version = 0x16,
	.source = 1,
	.abs_x_min = 0,
	.abs_x_max = 319,
	.abs_y_min = 0,
	.abs_y_max = 479,
	.abs_pressure_min = 0,
	.abs_pressure_max = 255,
	.abs_width_min = 0,
	.abs_width_max = 15,
	.gpio_irq = 55,
	.power = atmel_platform_power,
	.config_T7[0] = 0x32,
	.config_T7[1] = 12,
	.config_T7[2] = 25,
	.config_T8[0] = 10,
	.config_T8[1] = 5,
	.config_T8[2] = 20,
	.config_T8[3] = 20,
	.config_T8[4] = 0,
	.config_T8[5] = 0,
	.config_T8[6] = 10,//9,
	.config_T8[7] = 6,//0x0f,
	.config_T8[8] = 0,//9,
	.config_T8[9] = 0,//0x0f,	
	
	.config_T9[0] = 0x8b,//0x8f
	.config_T9[1] = 0,
	.config_T9[2] = 2,
	.config_T9[3] = 14,//hwrev>5
	.config_T9[4] = 9,//hwrev>5
	.config_T9[5] = 0,//AKSCFG
	.config_T9[6] = 16,
	.config_T9[7] = 35,
	.config_T9[8] = 3, //2 touch detect integration 05-19
	//.config_T9[9] = 6,//∫·∆¡
	.config_T9[9] = 1,	// ˙∆¡
	.config_T9[10] = 10,
	.config_T9[11] = 6, //3 05-19
	.config_T9[12] = 6, //1 05-19
	.config_T9[13] = 79,//5
	.config_T9[14] = 5,
	.config_T9[15] = 10,
	.config_T9[16] = 30, //20 05-19
	.config_T9[17] = 30,
	.config_T9[18] = 0xfd,
	.config_T9[19] = 1,
	.config_T9[20] = 0x3f,
	.config_T9[21] = 1,
	//.config_T9[18] = 0x40,//320 low 16bits
	//.config_T9[19] = 0x01,// 320 high 16bits
	//.config_T9[20] = 0xdf,
	//.config_T9[21] = 0x01,
	//.config_T9[18] = 223,//31,
	//.config_T9[19] = 1,//3,
	//.config_T9[20] = 31,//223,
	//.config_T9[21] = 3,//1,  
	.config_T9[22] = 0,
	.config_T9[23] = 0,
	.config_T9[24] = 0,
	.config_T9[25] = 0,
	.config_T9[26] = 64,
	.config_T9[27] = 0,
	.config_T9[28] = 64,
	.config_T9[29] = 0,
	.config_T9[30] = 15,//10
	.config_T9[31] = 9,//10

	.config_T15[0] = 0,
	.config_T15[1] = 0,
	.config_T15[2] = 0,
	.config_T15[3] = 0,//4
	.config_T15[4] = 0,
	.config_T15[5] = 0,
	.config_T15[6] = 0,
	.config_T15[7] = 0,//25
	.config_T15[8] = 0,
	.config_T18[0] = 0,
	.config_T18[1] = 0,
	.config_T19[0] = 0,
	.config_T19[1] = 0,
	.config_T19[2] = 0,
	.config_T19[3] = 0,
	.config_T19[4] = 0,
	.config_T19[5] = 0,
	.config_T19[6] = 0,
	.config_T19[7] = 0,
	.config_T19[8] = 0,
	.config_T19[9] = 0,
	.config_T19[10] = 0,
	.config_T19[11] = 0,
	.config_T20[0] = 25,//0x13,
	.config_T20[1] = 1,
	.config_T20[2] = 1,
	.config_T20[3] = 1,
	.config_T20[4] = 1,
	.config_T20[5] = 0,//max finger number
	.config_T20[6] = 0,
	.config_T20[7] = 0,
	.config_T20[8] = 0,
	.config_T20[9] = 0,
	.config_T20[10] = 0,
	.config_T20[11] = 0,
	.config_T22[0] = 0x0d,
	.config_T22[1] = 0,
	.config_T22[2] = 0,
	.config_T22[3] = 0x19,//??
	.config_T22[4] = 0x0,//??
	.config_T22[5] = 0xE7,//??
	.config_T22[6] = 0xFF,//??
	.config_T22[7] = 8,
	.config_T22[8] = 25,
	.config_T22[9] = 0,
	.config_T22[10] = 1,
	.config_T22[11] = 19,
	.config_T22[12] = 0,
	.config_T22[13] = 25,
	.config_T22[14] = 45,
	.config_T22[15] = 0,
	.config_T22[16] = 8,
	.config_T23[0] = 0,
	.config_T23[1] = 0,
	.config_T23[2] = 0,
	.config_T23[3] = 0,
	.config_T23[4] = 0,
	.config_T23[5] = 0,
	.config_T23[6] = 0,
	.config_T23[7] = 0,
	.config_T23[8] = 0,
	.config_T23[9] = 0,
	.config_T23[10] = 0,
	.config_T23[11] = 0,
	.config_T23[12] = 0,
	.config_T24[0] = 0,
	.config_T24[1] = 0,
	.config_T24[2] = 0,
	.config_T24[3] = 0,
	.config_T24[4] = 0,
	.config_T24[5] = 0,
	.config_T24[6] = 0,
	.config_T24[7] = 0,
	.config_T24[8] = 0,
	.config_T24[9] = 0,
	.config_T24[10] = 0,
	.config_T24[11] = 0,
	.config_T24[12] = 0,
	.config_T24[13] = 0,
	.config_T24[14] = 0,
	.config_T24[15] = 0,
	.config_T24[16] = 0,
	.config_T24[17] = 0,
	.config_T24[18] = 0,
	.config_T25[0] = 0,
	.config_T25[1] = 0,
	.config_T25[2] = 0,
	.config_T25[3] = 0,
	.config_T25[4] = 0,
	.config_T25[5] = 0,
	.config_T25[5] = 0,
	.config_T25[6] = 0,
	.config_T25[7] = 0,
	.config_T25[8] = 0,
	.config_T25[9] = 0,
	.config_T27[0] = 0,
	.config_T27[1] = 1,
	.config_T27[2] = 0,
	.config_T27[3] = 0,
	.config_T27[4] = 0,
	.config_T27[5] = 0,
	.config_T27[6] = 0,
	.config_T28[0] = 0,
	.config_T28[1] = 0,
	.config_T28[2] = 1,
	.config_T28[3] = 16,
	.config_T28[4] = 16,//32,
	.config_T28[5] = 0,//0x0a
	.object_crc[0] = 117,
	.object_crc[1] = 8,
	.object_crc[2] = 119,
/*.cable_config[0] = config_T9[7],
.cable_config[1] = config_T22[8],
.cable_config[2] = config_T28[3],
.cable_config[3] = config_T28[4],*/
	.cable_config[0] = 30,
	.cable_config[1] = 20,
	.cable_config[2] = 4,
	.cable_config[3] = 8,
	.GCAF_level[0] = 4,
	.GCAF_level[1] = 16,
	.GCAF_level[2] = 0,
	.GCAF_level[3] = 0,
	.filter_level[0] = 100,
	.filter_level[1] = 100,
	.filter_level[2] = 100,
	.filter_level[3] = 100,
	.display_width = 1024,  /* display width in pixel */
	.display_height = 1024, /* display height in pixel */


#else
  .version = 0x16,
  .source = 1,
  .abs_x_min = 0,
  .abs_x_max = 599,
  .abs_y_max = 1023,
  .abs_y_min = 0,
  .abs_pressure_min = 0,
  .abs_pressure_max = 255,
  .abs_width_min = 0,
  .abs_width_max = 15,
  .gpio_irq = 55,
  .power = atmel_platform_power,
  .config_T7[0] = 30,
  .config_T7[1] = 255,
  .config_T7[2] = 25,
  .config_T8[0] = 10,
  .config_T8[1] = 0,
  .config_T8[2] = 20,
  .config_T8[3] = 20,
  .config_T8[4] = 0,
  .config_T8[5] = 0,
  .config_T8[6] = 5,
  .config_T8[7] = 32,
  .config_T9[0] = 131,
  .config_T9[1] = 0,
  .config_T9[2] = 0,
  .config_T9[3] = 18,
  .config_T9[4] = 11,
  .config_T9[5] = 1,
  .config_T9[6] = 1,
  .config_T9[7] = 35,
  //.config_T9[7] = 35,
  .config_T9[8] = 3,
  //.config_T9[9] = 6,//∫·∆¡
  .config_T9[9] = 3,  // ˙∆¡
  .config_T9[10] = 10,
  .config_T9[11] = 2,
  .config_T9[12] = 2,
  .config_T9[13] = 0x30,//movefilter
  .config_T9[14] = 5,//touch numbers
  .config_T9[15] = 10,
  .config_T9[16] = 10,
  .config_T9[17] = 10,
  .config_T9[18] = 0,
  .config_T9[19] = 0,
  .config_T9[20] = 81,
  .config_T9[21] = 2,
  .config_T9[22] = 0,
  .config_T9[23] = 0,
  .config_T9[24] = 0,
  .config_T9[25] = 0,
  .config_T9[26] = 0,
  .config_T9[27] = 0,
  .config_T9[28] = 0,
  .config_T9[29] = 0,
  .config_T9[30] = 30,
  .config_T15[0] = 3,
  .config_T15[1] = 0,
  .config_T15[2] = 11,
  .config_T15[3] = 3,
  .config_T15[4] = 1,
  .config_T15[5] = 2,
  .config_T15[6] = 1,
  .config_T15[7] = 35,
  .config_T15[8] = 2,
  .config_T18[0] = 0,
  .config_T18[1] = 0,
  .config_T19[0] = 0,
  .config_T19[1] = 0,
  .config_T19[2] = 0,
  .config_T19[3] = 0,
  .config_T19[4] = 0,
  .config_T19[5] = 0,
  .config_T19[6] = 0,
  .config_T19[7] = 0,
  .config_T19[8] = 0,
  .config_T19[9] = 0,
  .config_T19[10] = 0,
  .config_T19[11] = 0,
  .config_T20[0] = 0,
  .config_T20[1] = 100,
  .config_T20[2] = 100,
  .config_T20[3] = 100,
  .config_T20[4] = 100,
  .config_T20[5] = 0,
  .config_T20[6] = 0,
  .config_T20[7] = 0,
  .config_T20[8] = 0,
  .config_T20[9] = 0,
  .config_T20[10] = 0,
  .config_T20[11] = 0,
  .config_T22[0] = 0x0d,
  .config_T22[1] = 0,
  .config_T22[2] = 0,
  .config_T22[3] = 0x19,
  .config_T22[4] = 0x00,
  .config_T22[5] = 0xe7,
  .config_T22[6] = 0xff,
  .config_T22[7] = 4,
  .config_T22[8] = 20,
  .config_T22[9] = 1,
  .config_T22[10] = 0,
  .config_T22[11] = 5,
  .config_T22[12] = 10,
  .config_T22[13] = 15,
  .config_T22[14] = 20,
  .config_T22[15] = 25,
  .config_T22[16] = 4,
  .config_T23[0] = 0,
  .config_T23[1] = 0,
  .config_T23[2] = 0,
  .config_T23[3] = 0,
  .config_T23[4] = 0,
  .config_T23[5] = 0,
  .config_T23[6] = 0,
  .config_T23[7] = 0,
  .config_T23[8] = 0,
  .config_T23[9] = 0,
  .config_T23[10] = 0,
  .config_T23[11] = 0,
  .config_T23[12] = 0,
  .config_T24[0] = 0,
  .config_T24[1] = 0,
  .config_T24[2] = 0,
  .config_T24[3] = 0,
  .config_T24[4] = 0,
  .config_T24[5] = 100,
  .config_T24[6] = 100,
  .config_T24[7] = 1,
  .config_T24[8] = 10,
  .config_T24[9] = 20,
  .config_T24[10] = 40,
  .config_T24[11] = 0,
  .config_T24[12] = 0,
  .config_T24[13] = 0,
  .config_T24[14] = 0,
  .config_T24[15] = 0,
  .config_T24[16] = 0,
  .config_T24[17] = 0,
  .config_T24[18] = 0,
  .config_T25[0] = 0,
  .config_T25[1] = 0,
  .config_T25[2] = 0xe0,
  .config_T25[3] = 0x2e,
  .config_T25[4] = 0x58,
  .config_T25[5] = 0x1b,
  .config_T25[6] = 0xe0,
  .config_T25[7] = 0x2e,
  .config_T25[8] = 0x58,
  .config_T25[9] = 0x1b,
  .config_T27[0] = 0,
  .config_T27[1] = 1,
  .config_T27[2] = 0,
  .config_T27[3] = 0,
  .config_T27[4] = 0,
  .config_T27[5] = 0,
  .config_T27[6] = 0,
  .config_T28[0] = 0,
  .config_T28[1] = 0,
  .config_T28[2] = 2,
  .config_T28[3] = 16,
  .config_T28[4] = 16,
  .config_T28[5] = 0x1e,
  .object_crc[0] = 117,
  .object_crc[1] = 8,
  .object_crc[2] = 119,
  /*.cable_config[0] = config_T9[7],
  .cable_config[1] = config_T22[8],
  .cable_config[2] = config_T28[3],
  .cable_config[3] = config_T28[4],*/
  .cable_config[0] = 30,
  .cable_config[1] = 20,
  .cable_config[2] = 4,
  .cable_config[3] = 8,
  .GCAF_level[0] = 4,
  .GCAF_level[1] = 16,
  .GCAF_level[2] = 0,
  .GCAF_level[3] = 0,
  .filter_level[0] = 100,
  .filter_level[1] = 100,
  .filter_level[2] = 100,
  .filter_level[3] = 100,
  .display_width = 1024,	/* display width in pixel */
  .display_height = 1024,	/* display height in pixel */
#endif
};
