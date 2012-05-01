/* drivers/video/msm/src/panel/lcdc/lcdc_panel.c
 *
 * Copyright (C) 2008 QUALCOMM USA, INC.
 * 
 * All source code in this file is licensed under the following license
 * except where indicated.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can find it at http://www.fsf.org
 */
 /* 
========================================================================
when         who           what, where, why                                                 comment tag
--------     ----          -------------------------------------    ------------------------------------
2011-04-19   luya		modify for BKL abnormal when wakeup	CRDB00639193	ZTE_LCD_LUYA_20110419_001
2010-11-04   lkej 	   modify  the fuction lcdc_panel_on() for bootup image	ZTE_LCD_LKEJ_20101104_001					
2010-06-11   lht		   project mode display panel info         	                         ZTE_LCD_LHT_20100611_001
2010-06-09   lht		   change max bkl level to 25           	                         ZTE_LCD_LHT_20100609_001
2010-05-21   lht		   early suspand current adjust           	                         ZTE_LCD_LHT_20100521_001
2010-05-19   lht		   sleep current adjust           			                         ZTE_LCD_LHT_20100519_001
2010-03-19   liuhaitao     for lcd dithering while shake 			                  ZTE_LCD_LHT_20100319_001
2010-03-01   liuhaitao     for sleep/wakeup bkl black
2010-01-26   luya		   fix white-screen(permanent) when wakeup	            ZTE_LCD_LUYA_20100126_001
2010-01-14   luya		   fix white-screen when wakeup(cause:bkl 
						   on before lcd)							            ZTE_LCD_LUYA_20100114_001
2009-12-18   lixiongwei    fix the lcd wave problem                                        ZTE_BACKLIGHT_LXW_001
2009-11-28   hp            fix transient white screen                                          ZTE_BACKLIGHT_HP_001
2009-11-27   weilanying    ajust backlight                                                     ZTE_BACKLIGHT_WLY_004 
2009-10-30   weilanying    backlight go to dim                                               ZTE_BACKLIGHT_WLY_002 
2009-10-31   weilanying    backlight set 32 class                                            ZTE_BACKLIGHT_WLY_003
=======================================================
*/

#include "msm_fb.h"
#include <asm/gpio.h>
#include <linux/module.h>
#include <linux/delay.h>

#define GPIO_LCD_BL_PWM_OUT  2 


#define LCD_BL_LEVEL 32
#define lcd_bl_max   lcd_bl_level-1
#define lcd_bl_min   0

extern u32 LcdPanleID;   //ZTE_LCD_LHT_20100611_001

//static DEFINE_SPINLOCK(tps61061_lock);

static int himax_panel_initialized = 1;

static int spi_cs;
static int spi_sclk;
static int lcd_id;
static int spi_sdi;
static int spi_sdo;
static int panel_reset;

typedef enum
{
	LCD_PANEL_NONE = 0,
	LCD_PANEL_LEAD_WVGA,
	LCD_PANEL_TRULY_WVGA,
}LCD_PANEL_TYPE;

static LCD_PANEL_TYPE g_lcd_panel_type = LCD_PANEL_NONE;



static bool onewiremode = TRUE;		///ZTE_LCD_LUYA_20110419_001
static struct msm_panel_common_pdata * lcdc_tft_pdata;
static void gpio_lcd_lead_emuspi_write_one_index(unsigned short addr);
static void gpio_lcd_lead_emuspi_write_one_data(unsigned short data);
static void gpio_lcd_truly_emuspi_read_one_index(unsigned int addr,unsigned int *data);
static void gpio_lcd_truly_emuspi_write_one_index(unsigned int addr,unsigned short data);
static void gpio_lcd_truly_emuspi_write_cmd(unsigned int addr,unsigned short data);	
void lcdc_lead_sleep(void);
void lcdc_truly_sleep(void);
static void lcdc_truly_init(void);
static void lcdc_lead_init(void);  
static void lcdc_set_bl(struct msm_fb_data_type *mfd);

static void SPI_Start(void);
static void SPI_Stop(void);
static void spi_init(void);
void lcd_panel_init(void); 
static int lcdc_panel_on(struct platform_device *pdev);
static int lcdc_panel_off(struct platform_device *pdev);


static void select_1wire_mode(void)
{
	gpio_direction_output(GPIO_LCD_BL_PWM_OUT, 1);
	udelay(120);
	gpio_direction_output(GPIO_LCD_BL_PWM_OUT, 0);
	udelay(280);
	gpio_direction_output(GPIO_LCD_BL_PWM_OUT, 1);
	udelay(650);
	
}

static void send_bkl_address(void)
{
	unsigned int i,j;
	i = 0x72;
	gpio_direction_output(GPIO_LCD_BL_PWM_OUT, 1);
	udelay(10);
	printk("[LY] send_bkl_address \n");
	for(j = 0; j < 8; j++)
	{
		if(i & 0x80)
		{
			gpio_direction_output(GPIO_LCD_BL_PWM_OUT, 0);
			udelay(10);
			gpio_direction_output(GPIO_LCD_BL_PWM_OUT, 1);
			udelay(180);
		}
		else
		{
			gpio_direction_output(GPIO_LCD_BL_PWM_OUT, 0);
			udelay(180);
			gpio_direction_output(GPIO_LCD_BL_PWM_OUT, 1);
			udelay(10);
		}
		i <<= 1;
	}
	gpio_direction_output(GPIO_LCD_BL_PWM_OUT, 0);
	udelay(10);
	gpio_direction_output(GPIO_LCD_BL_PWM_OUT, 1);

}

static void send_bkl_data(int level)
{
	unsigned int i,j;
	i = level & 0x1F;
	gpio_direction_output(GPIO_LCD_BL_PWM_OUT, 1);
	udelay(10);
	printk("[LY] send_bkl_data \n");
	for(j = 0; j < 8; j++)
	{
		if(i & 0x80)
		{
			gpio_direction_output(GPIO_LCD_BL_PWM_OUT, 0);
			udelay(10);
			gpio_direction_output(GPIO_LCD_BL_PWM_OUT, 1);
			udelay(180);
		}
		else
		{
			gpio_direction_output(GPIO_LCD_BL_PWM_OUT, 0);
			udelay(180);
			gpio_direction_output(GPIO_LCD_BL_PWM_OUT, 1);
			udelay(10);
		}
		i <<= 1;
	}
	gpio_direction_output(GPIO_LCD_BL_PWM_OUT, 0);
	udelay(10);
	gpio_direction_output(GPIO_LCD_BL_PWM_OUT, 1);

}


static void lcdc_set_bl(struct msm_fb_data_type *mfd)
{
       /*value range is 1--32*/
    int current_lel = mfd->bl_level;
    unsigned long flags;


    printk("[ZYF] lcdc_set_bl level=%d, %d\n", 
		   current_lel , mfd->panel_power_on);


    if(!mfd->panel_power_on)
	{
    	gpio_direction_output(GPIO_LCD_BL_PWM_OUT, 0);			///ZTE_LCD_LUYA_20100201_001
	    return;
    }

    if(current_lel < 1)
    {
        current_lel = 0;
    }
    if(current_lel > 32)
    {
        current_lel = 32;
    }

    /*ZTE_BACKLIGHT_WLY_005,@2009-11-28, set backlight as 32 levels, end*/
    local_irq_save(flags);
    if(current_lel==0)
    {
    	gpio_direction_output(GPIO_LCD_BL_PWM_OUT, 0);
		mdelay(3);
		onewiremode = FALSE;
			
    }
    else 
	{
		if(!onewiremode)	//select 1 wire mode
		{
			printk("[LY] before select_1wire_mode\n");
			select_1wire_mode();
			onewiremode = TRUE;
		}
		send_bkl_address();
		send_bkl_data(current_lel-1);

	}
    local_irq_restore(flags);
}


static void lcdc_lead_init(void)
{//T38KPS00 CPT3.8"+HX8363A 18BIT RGB INTERFACE, LEAD LCD
	//msleep(150);//Delay(10*150);
	SPI_Start();
  
	gpio_lcd_lead_emuspi_write_one_index(0xB9);   //Set_EXTC
	gpio_lcd_lead_emuspi_write_one_data(0xFF);          //
	gpio_lcd_lead_emuspi_write_one_data(0x83);          //
	gpio_lcd_lead_emuspi_write_one_data(0x63);          //
	SPI_Stop();
	SPI_Start();
	gpio_lcd_lead_emuspi_write_one_index(0xB1);   //Set_POWER
	gpio_lcd_lead_emuspi_write_one_data(0x81);          //
	gpio_lcd_lead_emuspi_write_one_data(0x30);          //
	gpio_lcd_lead_emuspi_write_one_data(0x03);          //
	gpio_lcd_lead_emuspi_write_one_data(0x34);          //
	gpio_lcd_lead_emuspi_write_one_data(0x01);          //02
	gpio_lcd_lead_emuspi_write_one_data(0x13);          //
	gpio_lcd_lead_emuspi_write_one_data(0x11);          //
	gpio_lcd_lead_emuspi_write_one_data(0x00);          //
	gpio_lcd_lead_emuspi_write_one_data(0x35);          //
	gpio_lcd_lead_emuspi_write_one_data(0x3E);          //
	gpio_lcd_lead_emuspi_write_one_data(0x1c);          //
	gpio_lcd_lead_emuspi_write_one_data(0x1c);          //
	SPI_Stop();
	SPI_Start();
	gpio_lcd_lead_emuspi_write_one_index(0x11);	
	SPI_Stop();
	msleep(150);//Delay(10*120);
	SPI_Start();	
	gpio_lcd_lead_emuspi_write_one_index(0x3A);   //COLMOD
	//spi_HX8363_Data(0x77);          //24bit
	gpio_lcd_lead_emuspi_write_one_data(0x66);  		//18bit
	SPI_Stop();
	SPI_Start();
	gpio_lcd_lead_emuspi_write_one_index(0xB3);   //Set_RGBIF
	gpio_lcd_lead_emuspi_write_one_data(0x09);          //
	SPI_Stop();
	SPI_Start();
	gpio_lcd_lead_emuspi_write_one_index(0xB4);   //Set_CYC
	gpio_lcd_lead_emuspi_write_one_data(0x00);          //
	gpio_lcd_lead_emuspi_write_one_data(0x03);
	gpio_lcd_lead_emuspi_write_one_data(0x7E);
	gpio_lcd_lead_emuspi_write_one_data(0x02);
	gpio_lcd_lead_emuspi_write_one_data(0x01);
	gpio_lcd_lead_emuspi_write_one_data(0x12);
	gpio_lcd_lead_emuspi_write_one_data(0x64);
	gpio_lcd_lead_emuspi_write_one_data(0x01);
	gpio_lcd_lead_emuspi_write_one_data(0x60);
	SPI_Stop();
	SPI_Start();
	gpio_lcd_lead_emuspi_write_one_index(0xB6);   //Set_VCOM
	gpio_lcd_lead_emuspi_write_one_data(0x33);         // 3c
	SPI_Stop();
	SPI_Start();
	gpio_lcd_lead_emuspi_write_one_index(0xCC);   //Set_PANEL
	gpio_lcd_lead_emuspi_write_one_data(0x09);          //
	SPI_Stop();
	msleep(5);//Delay(10*5);
	SPI_Start();
	gpio_lcd_lead_emuspi_write_one_index(0xE0);   //Gamma 2.2
	gpio_lcd_lead_emuspi_write_one_data(0x00);          //
	gpio_lcd_lead_emuspi_write_one_data(0x1E);          //
	gpio_lcd_lead_emuspi_write_one_data(0x23);          //
	gpio_lcd_lead_emuspi_write_one_data(0x32);          //
	gpio_lcd_lead_emuspi_write_one_data(0x36);          //
	gpio_lcd_lead_emuspi_write_one_data(0x3F);          //
	gpio_lcd_lead_emuspi_write_one_data(0x08);          //
	gpio_lcd_lead_emuspi_write_one_data(0xCC);          //
	gpio_lcd_lead_emuspi_write_one_data(0x0E);          //
	gpio_lcd_lead_emuspi_write_one_data(0x92);          //
	gpio_lcd_lead_emuspi_write_one_data(0x54);          //
	gpio_lcd_lead_emuspi_write_one_data(0x15);          //
	gpio_lcd_lead_emuspi_write_one_data(0x18);          //
	gpio_lcd_lead_emuspi_write_one_data(0x4F);          //
	gpio_lcd_lead_emuspi_write_one_data(0x19);          //
	gpio_lcd_lead_emuspi_write_one_data(0x00);          //
	gpio_lcd_lead_emuspi_write_one_data(0x1E);          //
	gpio_lcd_lead_emuspi_write_one_data(0x23);          //
	gpio_lcd_lead_emuspi_write_one_data(0x32);          //
	gpio_lcd_lead_emuspi_write_one_data(0x36);          //
	gpio_lcd_lead_emuspi_write_one_data(0x3F);          //
	gpio_lcd_lead_emuspi_write_one_data(0x08);          //
	gpio_lcd_lead_emuspi_write_one_data(0xCC);          //
	gpio_lcd_lead_emuspi_write_one_data(0x0E);          //
	gpio_lcd_lead_emuspi_write_one_data(0x92);          //
	gpio_lcd_lead_emuspi_write_one_data(0x54);          //
	gpio_lcd_lead_emuspi_write_one_data(0x15);          //
	gpio_lcd_lead_emuspi_write_one_data(0x18);          //
	gpio_lcd_lead_emuspi_write_one_data(0x4F);          //
	gpio_lcd_lead_emuspi_write_one_data(0x19);          //
	SPI_Stop();
	msleep(5);//Delay(10*5);
	SPI_Start();
	gpio_lcd_lead_emuspi_write_one_index(0x21);     //INVON
	SPI_Stop();
	SPI_Start();
	gpio_lcd_lead_emuspi_write_one_index(0x36);   
	//LCD_data_SPI8bit(0x08);  	
	gpio_lcd_lead_emuspi_write_one_data(0x03);     //GS=1
	SPI_Stop();
	SPI_Start();
	gpio_lcd_lead_emuspi_write_one_index(0x29);  
	SPI_Stop();
	
	printk(KERN_INFO "fxy lcd module TFT Lead init finish\n!");
	

}
static void lcdc_truly_init(void)
{
#if 1   // 2.0

gpio_lcd_truly_emuspi_write_one_index(0xF000,0x55);
gpio_lcd_truly_emuspi_write_one_index(0xF001,0xAA);
gpio_lcd_truly_emuspi_write_one_index(0xF002,0x52);
gpio_lcd_truly_emuspi_write_one_index(0xF003,0x08);
gpio_lcd_truly_emuspi_write_one_index(0xF004,0x01);

gpio_lcd_truly_emuspi_write_one_index(0xBC00,0x00);
gpio_lcd_truly_emuspi_write_one_index(0xBC01,0x96);
gpio_lcd_truly_emuspi_write_one_index(0xBC02,0x00);
gpio_lcd_truly_emuspi_write_one_index(0xBD00,0x00);
gpio_lcd_truly_emuspi_write_one_index(0xBD01,0x96);
gpio_lcd_truly_emuspi_write_one_index(0xBD02,0x00);
gpio_lcd_truly_emuspi_write_one_index(0xBE01,0x2D);


gpio_lcd_truly_emuspi_write_one_index(0xD100,0x00 );
gpio_lcd_truly_emuspi_write_one_index(0xD101,0x35 );
gpio_lcd_truly_emuspi_write_one_index(0xD102,0x00 );
gpio_lcd_truly_emuspi_write_one_index(0xD103,0x90 );
gpio_lcd_truly_emuspi_write_one_index(0xD104,0x00 );
gpio_lcd_truly_emuspi_write_one_index(0xD105,0x90 );
gpio_lcd_truly_emuspi_write_one_index(0xD106,0x00 );
gpio_lcd_truly_emuspi_write_one_index(0xD107,0x91 );
gpio_lcd_truly_emuspi_write_one_index(0xD108,0x00 );
gpio_lcd_truly_emuspi_write_one_index(0xD109,0x91 );
gpio_lcd_truly_emuspi_write_one_index(0xD10A,0x00 );
gpio_lcd_truly_emuspi_write_one_index(0xD10B,0x98 );
gpio_lcd_truly_emuspi_write_one_index(0xD10C,0x00 );
gpio_lcd_truly_emuspi_write_one_index(0xD10D,0xB0 );
gpio_lcd_truly_emuspi_write_one_index(0xD10E,0x00 );
gpio_lcd_truly_emuspi_write_one_index(0xD10F,0xD7 );
gpio_lcd_truly_emuspi_write_one_index(0xD110,0x00 );
gpio_lcd_truly_emuspi_write_one_index(0xD111,0xFC );
gpio_lcd_truly_emuspi_write_one_index(0xD112,0x01 );
gpio_lcd_truly_emuspi_write_one_index(0xD113,0x2C );
gpio_lcd_truly_emuspi_write_one_index(0xD114,0x01 );
gpio_lcd_truly_emuspi_write_one_index(0xD115,0x4C );
gpio_lcd_truly_emuspi_write_one_index(0xD116,0x01 );
gpio_lcd_truly_emuspi_write_one_index(0xD117,0x84 );
gpio_lcd_truly_emuspi_write_one_index(0xD118,0x01 );
gpio_lcd_truly_emuspi_write_one_index(0xD119,0xAD );
gpio_lcd_truly_emuspi_write_one_index(0xD11A,0x01 );
gpio_lcd_truly_emuspi_write_one_index(0xD11B,0xAE );
gpio_lcd_truly_emuspi_write_one_index(0xD11C,0x01 );
gpio_lcd_truly_emuspi_write_one_index(0xD11D,0xD8 );
gpio_lcd_truly_emuspi_write_one_index(0xD11E,0x01 );
gpio_lcd_truly_emuspi_write_one_index(0xD11F,0xFD );
gpio_lcd_truly_emuspi_write_one_index(0xD120,0x02 );
gpio_lcd_truly_emuspi_write_one_index(0xD121,0x11 );
gpio_lcd_truly_emuspi_write_one_index(0xD122,0x02 );
gpio_lcd_truly_emuspi_write_one_index(0xD123,0x2C );
gpio_lcd_truly_emuspi_write_one_index(0xD124,0x02 );
gpio_lcd_truly_emuspi_write_one_index(0xD125,0x42 );
gpio_lcd_truly_emuspi_write_one_index(0xD126,0x02 );
gpio_lcd_truly_emuspi_write_one_index(0xD127,0x65 );
gpio_lcd_truly_emuspi_write_one_index(0xD128,0x02 );
gpio_lcd_truly_emuspi_write_one_index(0xD129,0x7F );
gpio_lcd_truly_emuspi_write_one_index(0xD12A,0x02 );
gpio_lcd_truly_emuspi_write_one_index(0xD12B,0xB8 );
gpio_lcd_truly_emuspi_write_one_index(0xD12C,0x02 );
gpio_lcd_truly_emuspi_write_one_index(0xD12D,0xBE );
gpio_lcd_truly_emuspi_write_one_index(0xD12E,0x02 );
gpio_lcd_truly_emuspi_write_one_index(0xD12F,0xC2 );
gpio_lcd_truly_emuspi_write_one_index(0xD130,0x03 );
gpio_lcd_truly_emuspi_write_one_index(0xD131,0x5E );
gpio_lcd_truly_emuspi_write_one_index(0xD132,0x03 );
gpio_lcd_truly_emuspi_write_one_index(0xD133,0xC1 );


gpio_lcd_truly_emuspi_write_one_index(0xD200,0x00 );
gpio_lcd_truly_emuspi_write_one_index(0xD201,0x35 );
gpio_lcd_truly_emuspi_write_one_index(0xD202,0x00 );
gpio_lcd_truly_emuspi_write_one_index(0xD203,0x90 );
gpio_lcd_truly_emuspi_write_one_index(0xD204,0x00 );
gpio_lcd_truly_emuspi_write_one_index(0xD205,0x90 );
gpio_lcd_truly_emuspi_write_one_index(0xD206,0x00 );
gpio_lcd_truly_emuspi_write_one_index(0xD207,0x91 );
gpio_lcd_truly_emuspi_write_one_index(0xD208,0x00 );
gpio_lcd_truly_emuspi_write_one_index(0xD209,0x91 );
gpio_lcd_truly_emuspi_write_one_index(0xD20A,0x00 );
gpio_lcd_truly_emuspi_write_one_index(0xD20B,0x98 );
gpio_lcd_truly_emuspi_write_one_index(0xD20C,0x00 );
gpio_lcd_truly_emuspi_write_one_index(0xD20D,0xB0 );
gpio_lcd_truly_emuspi_write_one_index(0xD20E,0x00 );
gpio_lcd_truly_emuspi_write_one_index(0xD20F,0xD7 );
gpio_lcd_truly_emuspi_write_one_index(0xD210,0x00 );
gpio_lcd_truly_emuspi_write_one_index(0xD211,0xFC );
gpio_lcd_truly_emuspi_write_one_index(0xD212,0x01 );
gpio_lcd_truly_emuspi_write_one_index(0xD213,0x2C );
gpio_lcd_truly_emuspi_write_one_index(0xD214,0x01 );
gpio_lcd_truly_emuspi_write_one_index(0xD215,0x4C );
gpio_lcd_truly_emuspi_write_one_index(0xD216,0x01 );
gpio_lcd_truly_emuspi_write_one_index(0xD217,0x84 );
gpio_lcd_truly_emuspi_write_one_index(0xD218,0x01 );
gpio_lcd_truly_emuspi_write_one_index(0xD219,0xAD );
gpio_lcd_truly_emuspi_write_one_index(0xD21A,0x01 );
gpio_lcd_truly_emuspi_write_one_index(0xD21B,0xAE );
gpio_lcd_truly_emuspi_write_one_index(0xD21C,0x01 );
gpio_lcd_truly_emuspi_write_one_index(0xD21D,0xD8 );
gpio_lcd_truly_emuspi_write_one_index(0xD21E,0x01 );
gpio_lcd_truly_emuspi_write_one_index(0xD21F,0xFD );
gpio_lcd_truly_emuspi_write_one_index(0xD220,0x02 );
gpio_lcd_truly_emuspi_write_one_index(0xD221,0x11 );
gpio_lcd_truly_emuspi_write_one_index(0xD222,0x02 );
gpio_lcd_truly_emuspi_write_one_index(0xD223,0x2C );
gpio_lcd_truly_emuspi_write_one_index(0xD224,0x02 );
gpio_lcd_truly_emuspi_write_one_index(0xD225,0x42 );
gpio_lcd_truly_emuspi_write_one_index(0xD226,0x02 );
gpio_lcd_truly_emuspi_write_one_index(0xD227,0x65 );
gpio_lcd_truly_emuspi_write_one_index(0xD228,0x02 );
gpio_lcd_truly_emuspi_write_one_index(0xD229,0x7F );
gpio_lcd_truly_emuspi_write_one_index(0xD22A,0x02 );
gpio_lcd_truly_emuspi_write_one_index(0xD22B,0xB8 );
gpio_lcd_truly_emuspi_write_one_index(0xD22C,0x02 );
gpio_lcd_truly_emuspi_write_one_index(0xD22D,0xBE );
gpio_lcd_truly_emuspi_write_one_index(0xD22E,0x02 );
gpio_lcd_truly_emuspi_write_one_index(0xD22F,0xC2 );
gpio_lcd_truly_emuspi_write_one_index(0xD230,0x03 );
gpio_lcd_truly_emuspi_write_one_index(0xD231,0x5E );
gpio_lcd_truly_emuspi_write_one_index(0xD232,0x03 );
gpio_lcd_truly_emuspi_write_one_index(0xD233,0xC1 );

gpio_lcd_truly_emuspi_write_one_index(0xD300,0x00);
gpio_lcd_truly_emuspi_write_one_index(0xD301,0x35);
gpio_lcd_truly_emuspi_write_one_index(0xD302,0x00);
gpio_lcd_truly_emuspi_write_one_index(0xD303,0x90);
gpio_lcd_truly_emuspi_write_one_index(0xD304,0x00);
gpio_lcd_truly_emuspi_write_one_index(0xD305,0x90);
gpio_lcd_truly_emuspi_write_one_index(0xD306,0x00);
gpio_lcd_truly_emuspi_write_one_index(0xD307,0x91);
gpio_lcd_truly_emuspi_write_one_index(0xD308,0x00);
gpio_lcd_truly_emuspi_write_one_index(0xD309,0x91);
gpio_lcd_truly_emuspi_write_one_index(0xD30A,0x00);
gpio_lcd_truly_emuspi_write_one_index(0xD30B,0x98);
gpio_lcd_truly_emuspi_write_one_index(0xD30C,0x00);
gpio_lcd_truly_emuspi_write_one_index(0xD30D,0xB0);
gpio_lcd_truly_emuspi_write_one_index(0xD30E,0x00);
gpio_lcd_truly_emuspi_write_one_index(0xD30F,0xD7);
gpio_lcd_truly_emuspi_write_one_index(0xD310,0x00);
gpio_lcd_truly_emuspi_write_one_index(0xD311,0xFC);
gpio_lcd_truly_emuspi_write_one_index(0xD312,0x01);
gpio_lcd_truly_emuspi_write_one_index(0xD313,0x2C);
gpio_lcd_truly_emuspi_write_one_index(0xD314,0x01);
gpio_lcd_truly_emuspi_write_one_index(0xD315,0x4C);
gpio_lcd_truly_emuspi_write_one_index(0xD316,0x01);
gpio_lcd_truly_emuspi_write_one_index(0xD317,0x84);
gpio_lcd_truly_emuspi_write_one_index(0xD318,0x01);
gpio_lcd_truly_emuspi_write_one_index(0xD319,0xAD);
gpio_lcd_truly_emuspi_write_one_index(0xD31A,0x01);
gpio_lcd_truly_emuspi_write_one_index(0xD31B,0xAE);
gpio_lcd_truly_emuspi_write_one_index(0xD31C,0x01);
gpio_lcd_truly_emuspi_write_one_index(0xD31D,0xD8);
gpio_lcd_truly_emuspi_write_one_index(0xD31E,0x01);
gpio_lcd_truly_emuspi_write_one_index(0xD31F,0xFD);
gpio_lcd_truly_emuspi_write_one_index(0xD320,0x02);
gpio_lcd_truly_emuspi_write_one_index(0xD321,0x11);
gpio_lcd_truly_emuspi_write_one_index(0xD322,0x02);
gpio_lcd_truly_emuspi_write_one_index(0xD323,0x2C);
gpio_lcd_truly_emuspi_write_one_index(0xD324,0x02);
gpio_lcd_truly_emuspi_write_one_index(0xD325,0x42);
gpio_lcd_truly_emuspi_write_one_index(0xD326,0x02);
gpio_lcd_truly_emuspi_write_one_index(0xD327,0x65);
gpio_lcd_truly_emuspi_write_one_index(0xD328,0x02);
gpio_lcd_truly_emuspi_write_one_index(0xD329,0x7F);
gpio_lcd_truly_emuspi_write_one_index(0xD32A,0x02);
gpio_lcd_truly_emuspi_write_one_index(0xD32B,0xB8);
gpio_lcd_truly_emuspi_write_one_index(0xD32C,0x02);
gpio_lcd_truly_emuspi_write_one_index(0xD32D,0xBE);
gpio_lcd_truly_emuspi_write_one_index(0xD32E,0x02);
gpio_lcd_truly_emuspi_write_one_index(0xD32F,0xC2);
gpio_lcd_truly_emuspi_write_one_index(0xD330,0x03);
gpio_lcd_truly_emuspi_write_one_index(0xD331,0x5E);
gpio_lcd_truly_emuspi_write_one_index(0xD332,0x03);
gpio_lcd_truly_emuspi_write_one_index(0xD333,0xC1);


gpio_lcd_truly_emuspi_write_one_index(0xD400,0x00);
gpio_lcd_truly_emuspi_write_one_index(0xD401,0x35);
gpio_lcd_truly_emuspi_write_one_index(0xD402,0x00);
gpio_lcd_truly_emuspi_write_one_index(0xD403,0x90);
gpio_lcd_truly_emuspi_write_one_index(0xD404,0x00);
gpio_lcd_truly_emuspi_write_one_index(0xD405,0x90);
gpio_lcd_truly_emuspi_write_one_index(0xD406,0x00);
gpio_lcd_truly_emuspi_write_one_index(0xD407,0x91);
gpio_lcd_truly_emuspi_write_one_index(0xD408,0x00);
gpio_lcd_truly_emuspi_write_one_index(0xD409,0x91);
gpio_lcd_truly_emuspi_write_one_index(0xD40A,0x00);
gpio_lcd_truly_emuspi_write_one_index(0xD40B,0x98);
gpio_lcd_truly_emuspi_write_one_index(0xD40C,0x00);
gpio_lcd_truly_emuspi_write_one_index(0xD40D,0xB0);
gpio_lcd_truly_emuspi_write_one_index(0xD40E,0x00);
gpio_lcd_truly_emuspi_write_one_index(0xD40F,0xD7);
gpio_lcd_truly_emuspi_write_one_index(0xD410,0x00);
gpio_lcd_truly_emuspi_write_one_index(0xD411,0xFC);
gpio_lcd_truly_emuspi_write_one_index(0xD412,0x01);
gpio_lcd_truly_emuspi_write_one_index(0xD413,0x2C);
gpio_lcd_truly_emuspi_write_one_index(0xD414,0x01);
gpio_lcd_truly_emuspi_write_one_index(0xD415,0x4C);
gpio_lcd_truly_emuspi_write_one_index(0xD416,0x01);
gpio_lcd_truly_emuspi_write_one_index(0xD417,0x84);
gpio_lcd_truly_emuspi_write_one_index(0xD418,0x01);
gpio_lcd_truly_emuspi_write_one_index(0xD419,0xAD);
gpio_lcd_truly_emuspi_write_one_index(0xD41A,0x01);
gpio_lcd_truly_emuspi_write_one_index(0xD41B,0xAE);
gpio_lcd_truly_emuspi_write_one_index(0xD41C,0x01);
gpio_lcd_truly_emuspi_write_one_index(0xD41D,0xD8);
gpio_lcd_truly_emuspi_write_one_index(0xD41E,0x01);
gpio_lcd_truly_emuspi_write_one_index(0xD41F,0xFD);
gpio_lcd_truly_emuspi_write_one_index(0xD420,0x02);
gpio_lcd_truly_emuspi_write_one_index(0xD421,0x11);
gpio_lcd_truly_emuspi_write_one_index(0xD422,0x02);
gpio_lcd_truly_emuspi_write_one_index(0xD423,0x2C);
gpio_lcd_truly_emuspi_write_one_index(0xD424,0x02);
gpio_lcd_truly_emuspi_write_one_index(0xD425,0x42);
gpio_lcd_truly_emuspi_write_one_index(0xD426,0x02);
gpio_lcd_truly_emuspi_write_one_index(0xD427,0x65);
gpio_lcd_truly_emuspi_write_one_index(0xD428,0x02);
gpio_lcd_truly_emuspi_write_one_index(0xD429,0x7F);
gpio_lcd_truly_emuspi_write_one_index(0xD42A,0x02);
gpio_lcd_truly_emuspi_write_one_index(0xD42B,0xB8);
gpio_lcd_truly_emuspi_write_one_index(0xD42C,0x02);
gpio_lcd_truly_emuspi_write_one_index(0xD42D,0xBE);
gpio_lcd_truly_emuspi_write_one_index(0xD42E,0x02);
gpio_lcd_truly_emuspi_write_one_index(0xD42F,0xC2);
gpio_lcd_truly_emuspi_write_one_index(0xD430,0x03);
gpio_lcd_truly_emuspi_write_one_index(0xD431,0x5E);
gpio_lcd_truly_emuspi_write_one_index(0xD432,0x03);
gpio_lcd_truly_emuspi_write_one_index(0xD433,0xC1);


gpio_lcd_truly_emuspi_write_one_index(0xD500,0x00 );
gpio_lcd_truly_emuspi_write_one_index(0xD501,0x35 );
gpio_lcd_truly_emuspi_write_one_index(0xD502,0x00 );
gpio_lcd_truly_emuspi_write_one_index(0xD503,0x90 );
gpio_lcd_truly_emuspi_write_one_index(0xD504,0x00 );
gpio_lcd_truly_emuspi_write_one_index(0xD505,0x90 );
gpio_lcd_truly_emuspi_write_one_index(0xD506,0x00 );
gpio_lcd_truly_emuspi_write_one_index(0xD507,0x91 );
gpio_lcd_truly_emuspi_write_one_index(0xD508,0x00 );
gpio_lcd_truly_emuspi_write_one_index(0xD509,0x91 );
gpio_lcd_truly_emuspi_write_one_index(0xD50A,0x00 );
gpio_lcd_truly_emuspi_write_one_index(0xD50B,0x98 );
gpio_lcd_truly_emuspi_write_one_index(0xD50C,0x00 );
gpio_lcd_truly_emuspi_write_one_index(0xD50D,0xB0 );
gpio_lcd_truly_emuspi_write_one_index(0xD50E,0x00 );
gpio_lcd_truly_emuspi_write_one_index(0xD50F,0xD7 );
gpio_lcd_truly_emuspi_write_one_index(0xD510,0x00 );
gpio_lcd_truly_emuspi_write_one_index(0xD511,0xFC );
gpio_lcd_truly_emuspi_write_one_index(0xD512,0x01 );
gpio_lcd_truly_emuspi_write_one_index(0xD513,0x2C );
gpio_lcd_truly_emuspi_write_one_index(0xD514,0x01 );
gpio_lcd_truly_emuspi_write_one_index(0xD515,0x4C );
gpio_lcd_truly_emuspi_write_one_index(0xD516,0x01 );
gpio_lcd_truly_emuspi_write_one_index(0xD517,0x84 );
gpio_lcd_truly_emuspi_write_one_index(0xD518,0x01 );
gpio_lcd_truly_emuspi_write_one_index(0xD519,0xAD );
gpio_lcd_truly_emuspi_write_one_index(0xD51A,0x01 );
gpio_lcd_truly_emuspi_write_one_index(0xD51B,0xAE );
gpio_lcd_truly_emuspi_write_one_index(0xD51C,0x01 );
gpio_lcd_truly_emuspi_write_one_index(0xD51D,0xD8 );
gpio_lcd_truly_emuspi_write_one_index(0xD51E,0x01 );
gpio_lcd_truly_emuspi_write_one_index(0xD51F,0xFD );
gpio_lcd_truly_emuspi_write_one_index(0xD520,0x02 );
gpio_lcd_truly_emuspi_write_one_index(0xD521,0x11 );
gpio_lcd_truly_emuspi_write_one_index(0xD522,0x02 );
gpio_lcd_truly_emuspi_write_one_index(0xD523,0x2C );
gpio_lcd_truly_emuspi_write_one_index(0xD524,0x02 );
gpio_lcd_truly_emuspi_write_one_index(0xD525,0x42 );
gpio_lcd_truly_emuspi_write_one_index(0xD526,0x02 );
gpio_lcd_truly_emuspi_write_one_index(0xD527,0x65 );
gpio_lcd_truly_emuspi_write_one_index(0xD528,0x02 );
gpio_lcd_truly_emuspi_write_one_index(0xD529,0x7F );
gpio_lcd_truly_emuspi_write_one_index(0xD52A,0x02 );
gpio_lcd_truly_emuspi_write_one_index(0xD52B,0xB8 );
gpio_lcd_truly_emuspi_write_one_index(0xD52C,0x02 );
gpio_lcd_truly_emuspi_write_one_index(0xD52D,0xBE );
gpio_lcd_truly_emuspi_write_one_index(0xD52E,0x02 );
gpio_lcd_truly_emuspi_write_one_index(0xD52F,0xC2 );
gpio_lcd_truly_emuspi_write_one_index(0xD530,0x03 );
gpio_lcd_truly_emuspi_write_one_index(0xD531,0x5E );
gpio_lcd_truly_emuspi_write_one_index(0xD532,0x03 );
gpio_lcd_truly_emuspi_write_one_index(0xD533,0xC1 );

gpio_lcd_truly_emuspi_write_one_index(0xD600,0x00);
gpio_lcd_truly_emuspi_write_one_index(0xD601,0x35);
gpio_lcd_truly_emuspi_write_one_index(0xD602,0x00);
gpio_lcd_truly_emuspi_write_one_index(0xD603,0x90);
gpio_lcd_truly_emuspi_write_one_index(0xD604,0x00);
gpio_lcd_truly_emuspi_write_one_index(0xD605,0x90);
gpio_lcd_truly_emuspi_write_one_index(0xD606,0x00);
gpio_lcd_truly_emuspi_write_one_index(0xD607,0x91);
gpio_lcd_truly_emuspi_write_one_index(0xD608,0x00);
gpio_lcd_truly_emuspi_write_one_index(0xD609,0x91);
gpio_lcd_truly_emuspi_write_one_index(0xD60A,0x00);
gpio_lcd_truly_emuspi_write_one_index(0xD60B,0x98);
gpio_lcd_truly_emuspi_write_one_index(0xD60C,0x00);
gpio_lcd_truly_emuspi_write_one_index(0xD60D,0xB0);
gpio_lcd_truly_emuspi_write_one_index(0xD60E,0x00);
gpio_lcd_truly_emuspi_write_one_index(0xD60F,0xD7);
gpio_lcd_truly_emuspi_write_one_index(0xD610,0x00);
gpio_lcd_truly_emuspi_write_one_index(0xD611,0xFC);
gpio_lcd_truly_emuspi_write_one_index(0xD612,0x01);
gpio_lcd_truly_emuspi_write_one_index(0xD613,0x2C);
gpio_lcd_truly_emuspi_write_one_index(0xD614,0x01);
gpio_lcd_truly_emuspi_write_one_index(0xD615,0x4C);
gpio_lcd_truly_emuspi_write_one_index(0xD616,0x01);
gpio_lcd_truly_emuspi_write_one_index(0xD617,0x84);
gpio_lcd_truly_emuspi_write_one_index(0xD618,0x01);
gpio_lcd_truly_emuspi_write_one_index(0xD619,0xAD);
gpio_lcd_truly_emuspi_write_one_index(0xD61A,0x01);
gpio_lcd_truly_emuspi_write_one_index(0xD61B,0xAE);
gpio_lcd_truly_emuspi_write_one_index(0xD61C,0x01);
gpio_lcd_truly_emuspi_write_one_index(0xD61D,0xD8);
gpio_lcd_truly_emuspi_write_one_index(0xD61E,0x01);
gpio_lcd_truly_emuspi_write_one_index(0xD61F,0xFD);
gpio_lcd_truly_emuspi_write_one_index(0xD620,0x02);
gpio_lcd_truly_emuspi_write_one_index(0xD621,0x11);
gpio_lcd_truly_emuspi_write_one_index(0xD622,0x02);
gpio_lcd_truly_emuspi_write_one_index(0xD623,0x2C);
gpio_lcd_truly_emuspi_write_one_index(0xD624,0x02);
gpio_lcd_truly_emuspi_write_one_index(0xD625,0x42);
gpio_lcd_truly_emuspi_write_one_index(0xD626,0x02);
gpio_lcd_truly_emuspi_write_one_index(0xD627,0x65);
gpio_lcd_truly_emuspi_write_one_index(0xD628,0x02);
gpio_lcd_truly_emuspi_write_one_index(0xD629,0x7F);
gpio_lcd_truly_emuspi_write_one_index(0xD62A,0x02);
gpio_lcd_truly_emuspi_write_one_index(0xD62B,0xB8);
gpio_lcd_truly_emuspi_write_one_index(0xD62C,0x02);
gpio_lcd_truly_emuspi_write_one_index(0xD62D,0xBE);
gpio_lcd_truly_emuspi_write_one_index(0xD62E,0x02);
gpio_lcd_truly_emuspi_write_one_index(0xD62F,0xC2);
gpio_lcd_truly_emuspi_write_one_index(0xD630,0x03);
gpio_lcd_truly_emuspi_write_one_index(0xD631,0x5E);
gpio_lcd_truly_emuspi_write_one_index(0xD632,0x03);
gpio_lcd_truly_emuspi_write_one_index(0xD633,0xC1);

gpio_lcd_truly_emuspi_write_one_index(0xB000,0x06);   //AVDD Voltage Setting        
gpio_lcd_truly_emuspi_write_one_index(0xB001,0x06);
gpio_lcd_truly_emuspi_write_one_index(0xB002,0x06);

gpio_lcd_truly_emuspi_write_one_index(0xB100,0x02);   //AVEE Voltage
gpio_lcd_truly_emuspi_write_one_index(0xB101,0x02);
gpio_lcd_truly_emuspi_write_one_index(0xB102,0x02);

gpio_lcd_truly_emuspi_write_one_index(0xBA00,0x14);    //VGLX  Voltage Setting
gpio_lcd_truly_emuspi_write_one_index(0xBA01,0x14);
gpio_lcd_truly_emuspi_write_one_index(0xBA02,0x14);

gpio_lcd_truly_emuspi_write_one_index(0xB900,0x34);    //VGH  Voltage Setting
gpio_lcd_truly_emuspi_write_one_index(0xB901,0x34);
gpio_lcd_truly_emuspi_write_one_index(0xB902,0x34);
gpio_lcd_truly_emuspi_write_one_index(0xB800,0x05);    //VCL Power Control

gpio_lcd_truly_emuspi_write_one_index(0xF000,0x55);    //Enable Page 0
gpio_lcd_truly_emuspi_write_one_index(0xF001,0xAA);
gpio_lcd_truly_emuspi_write_one_index(0xF002,0x52);
gpio_lcd_truly_emuspi_write_one_index(0xF003,0x08);
gpio_lcd_truly_emuspi_write_one_index(0xF004,0x00);

gpio_lcd_truly_emuspi_write_one_index(0xB000,0x08);
gpio_lcd_truly_emuspi_write_one_index(0xB400,0x10);
gpio_lcd_truly_emuspi_write_one_index(0xB600,0x02);  //Source Output Data hold time

gpio_lcd_truly_emuspi_write_one_index(0xB100,0xCC);

gpio_lcd_truly_emuspi_write_one_index(0xBC00,0x05);   //Z-Inversion
gpio_lcd_truly_emuspi_write_one_index(0xBC01,0x05);
gpio_lcd_truly_emuspi_write_one_index(0xBC02,0x05);

gpio_lcd_truly_emuspi_write_one_index(0xB700,0x22);   //Gate EQ
gpio_lcd_truly_emuspi_write_one_index(0xB701,0x22);

gpio_lcd_truly_emuspi_write_one_index(0xB800,0x01);  //Source EQ
gpio_lcd_truly_emuspi_write_one_index(0xB801,0x03);
gpio_lcd_truly_emuspi_write_one_index(0xB802,0x03);
gpio_lcd_truly_emuspi_write_one_index(0xB803,0x03);

gpio_lcd_truly_emuspi_write_one_index(0xC80B,0x8C);  //Display Timing Control
gpio_lcd_truly_emuspi_write_one_index(0xC80C,0x8C);
gpio_lcd_truly_emuspi_write_one_index(0xC80F,0x8C);
gpio_lcd_truly_emuspi_write_one_index(0xC810,0x8C);

gpio_lcd_truly_emuspi_write_one_index(0xBD02,0x07);
gpio_lcd_truly_emuspi_write_one_index(0xBD03,0x31);
gpio_lcd_truly_emuspi_write_one_index(0xBE02,0x07);
gpio_lcd_truly_emuspi_write_one_index(0xBE03,0x31);
gpio_lcd_truly_emuspi_write_one_index(0xBF02,0x07);
gpio_lcd_truly_emuspi_write_one_index(0xBF03,0x31);

gpio_lcd_truly_emuspi_write_one_index(0xFF00,0xAA);
gpio_lcd_truly_emuspi_write_one_index(0xFF01,0x55);
gpio_lcd_truly_emuspi_write_one_index(0xFF02,0x25);
gpio_lcd_truly_emuspi_write_one_index(0xFF03,0x01);

gpio_lcd_truly_emuspi_write_one_index(0xF306,0x10);
gpio_lcd_truly_emuspi_write_one_index(0xF408,0x00);

gpio_lcd_truly_emuspi_write_one_index(0x3500,0x00);
gpio_lcd_truly_emuspi_write_one_index(0x3a00,0X66);
gpio_lcd_truly_emuspi_write_one_index(0x3600,0XD4);
gpio_lcd_truly_emuspi_write_cmd(0x1100,0x00);	
msleep(120);
gpio_lcd_truly_emuspi_write_cmd(0x2900,0x00);

#endif 	
printk(KERN_INFO "lht lcd module TFT Truly init finish\n!");
	
}

static void spi_init(void)
{
	/* Setting the Default GPIO's */
	lcd_id = *(lcdc_tft_pdata->gpio_num+1);
	spi_sclk = *(lcdc_tft_pdata->gpio_num+5);
	spi_cs   = *(lcdc_tft_pdata->gpio_num + 3);
	spi_sdi  = *(lcdc_tft_pdata->gpio_num + 2);
	spi_sdo  = *(lcdc_tft_pdata->gpio_num + 4);
	panel_reset = *(lcdc_tft_pdata->gpio_num + 6);

	/* Set the output so that we dont disturb the slave device */
	gpio_set_value(spi_sclk, 0);
	gpio_set_value(spi_sdi, 0);

	/* Set the Chip Select De-asserted */
	gpio_set_value(spi_cs, 0);

}

static int lcdc_panel_on(struct platform_device *pdev)
{

	spi_init();   //ZTE_LCD_LHT_20100519_001
///ZTE_LCD_LUYA_20100126_001
	if(himax_panel_initialized==1) 
	{
		himax_panel_initialized = 0;
		//ZTE_LCD_LKEJ_20101104_001,start
		//lcdc_himax_init();   
		//ZTE_LCD_LKEJ_20101104_001,end
	}
	else 
	{
		//spi_init();
		lcd_panel_init();
		
	}
	return 0;
}

void lcdc_lead_sleep(void)
{
	SPI_Start(); 
	    gpio_lcd_lead_emuspi_write_one_index(0x28);  
	SPI_Stop(); 
	SPI_Start(); 
	    gpio_lcd_lead_emuspi_write_one_index(0x10);  
	SPI_Stop(); 
	msleep(120);
}
void lcdc_truly_sleep(void)
{
	gpio_lcd_truly_emuspi_write_one_index(0x2800,0x00);
	gpio_lcd_truly_emuspi_write_one_index(0x1000,0x00);	
	msleep(200);	
}
static int lcdc_panel_off(struct platform_device *pdev)
{
	printk("lcdc_panel_off , g_lcd_panel_type is %d(1 LEAD. 2 TRULY. 3 OLED. )\n",g_lcd_panel_type);
	switch(g_lcd_panel_type)
	{
		case LCD_PANEL_TRULY_WVGA:
			lcdc_truly_sleep();
			break;
		case LCD_PANEL_LEAD_WVGA:
			//lcdc_lead_sleep();
			break;
		default:
			break;
	}
	
	gpio_direction_output(panel_reset, 0);
	//ZTE_LCD_LHT_20100521_001
	gpio_direction_output(spi_sclk, 0);
	gpio_direction_output(spi_sdi, 0);
	gpio_direction_output(spi_sdo, 0);
	gpio_direction_output(spi_cs, 0);
	return 0;
}

void lcd_panel_init(void)
{
	gpio_direction_output(panel_reset, 1);
	msleep(10);						////ZTE_LCD_LUYA_20100629_001
	gpio_direction_output(panel_reset, 0);
	msleep(20);						////ZTE_LCD_LUYA_20100629_001
	gpio_direction_output(panel_reset, 1);
	msleep(20);	
	switch(g_lcd_panel_type)
	{
		case LCD_PANEL_LEAD_WVGA:
			lcdc_lead_init();
			break;
		case LCD_PANEL_TRULY_WVGA:		
			lcdc_truly_init();
			break;
		default:
			break;
	}
}
static void gpio_lcd_truly_emuspi_write_one_index(unsigned int addr,unsigned short data)
{
	unsigned int i;
	int j;

	i=0x20;
	gpio_direction_output(spi_sclk, 0);	
	gpio_direction_output(spi_cs, 0);
	for (j = 0; j < 8; j++) 
	{
		if (i & 0x80)
			gpio_direction_output(spi_sdo, 1);	
		else
			gpio_direction_output(spi_sdo, 0);	
		gpio_direction_output(spi_sclk, 0);	
		gpio_direction_output(spi_sclk, 1);	
		i <<= 1;
	}
	i = addr ;
	for (j = 0; j < 8; j++) {

		if (i & 0x8000)
			gpio_direction_output(spi_sdo, 1);	
		else
			gpio_direction_output(spi_sdo, 0);	
		gpio_direction_output(spi_sclk, 0);	
		gpio_direction_output(spi_sclk, 1);	
		i <<= 1;
	}
	gpio_direction_output(spi_cs, 1);
	gpio_direction_output(spi_cs, 0);

	i=0x00;
	for (j = 0; j < 8; j++) 
	{

		if (i & 0x80)
			gpio_direction_output(spi_sdo, 1);	
		else
			gpio_direction_output(spi_sdo, 0);	

		gpio_direction_output(spi_sclk, 0);	
		gpio_direction_output(spi_sclk, 1);	
		//gpio_direction_output(spi_sclk, 0);	
		/*udelay(4);*/
		i <<= 1;
	}
	i = addr ;
	for (j = 0; j < 8; j++) {

		if (i & 0x80)
			gpio_direction_output(spi_sdo, 1);	
		else
			gpio_direction_output(spi_sdo, 0);	
		gpio_direction_output(spi_sclk, 0);	
		gpio_direction_output(spi_sclk, 1);	
		i <<= 1;
	}

	gpio_direction_output(spi_cs, 1);
	gpio_direction_output(spi_cs, 0);
	i=0x40;
	for (j = 0; j < 8; j++) 
	{

		if (i & 0x80)
			gpio_direction_output(spi_sdo, 1);	
		else
			gpio_direction_output(spi_sdo, 0);	

		gpio_direction_output(spi_sclk, 0);	
		gpio_direction_output(spi_sclk, 1);
		i <<= 1;
	}
	i = data ;
	for (j = 0; j < 8; j++) {

		if (i & 0x80)
			gpio_direction_output(spi_sdo, 1);	
		else
			gpio_direction_output(spi_sdo, 0);	

		gpio_direction_output(spi_sclk, 0);	
		gpio_direction_output(spi_sclk, 1);
		i <<= 1;
	}
	gpio_direction_output(spi_cs, 1);
}
static void gpio_lcd_truly_emuspi_write_cmd(unsigned int addr,unsigned short data)
{
	unsigned int i;
	int j;

	i=0x20;
	gpio_direction_output(spi_sclk, 0);	
	gpio_direction_output(spi_cs, 0);
	for (j = 0; j < 8; j++) 
	{
		if (i & 0x80)
			gpio_direction_output(spi_sdo, 1);	
		else
			gpio_direction_output(spi_sdo, 0);	
		gpio_direction_output(spi_sclk, 0);	
		gpio_direction_output(spi_sclk, 1);
		i <<= 1;
	}
	i = addr ;
	for (j = 0; j < 8; j++) {

		if (i & 0x8000)
			gpio_direction_output(spi_sdo, 1);	
		else
			gpio_direction_output(spi_sdo, 0);	
		gpio_direction_output(spi_sclk, 0);	
		gpio_direction_output(spi_sclk, 1);	
		i <<= 1;
	}
	gpio_direction_output(spi_cs, 1);
	gpio_direction_output(spi_cs, 0);

	i=0x00;
	for (j = 0; j < 8; j++) 
	{

		if (i & 0x80)
			gpio_direction_output(spi_sdo, 1);	
		else
			gpio_direction_output(spi_sdo, 0);	
		gpio_direction_output(spi_sclk, 0);	
		gpio_direction_output(spi_sclk, 1);	
		i <<= 1;
	}
	i = addr ;
	for (j = 0; j < 8; j++) {

		if (i & 0x80)
			gpio_direction_output(spi_sdo, 1);	
		else
			gpio_direction_output(spi_sdo, 0);	
		gpio_direction_output(spi_sclk, 0);	
		gpio_direction_output(spi_sclk, 1);	
		i <<= 1;
	}
	gpio_direction_output(spi_cs, 1);
	
}

static void gpio_lcd_truly_emuspi_read_one_index(unsigned int addr,unsigned int *data)
{
	unsigned int i;
	int j;
       unsigned int dbit,bits1;
	i=0x20;
	gpio_direction_output(spi_sclk, 0);	
	gpio_direction_output(spi_cs, 0);
	for (j = 0; j < 8; j++) 
	{

		if (i & 0x80)
			gpio_direction_output(spi_sdo, 1);	
		else
			gpio_direction_output(spi_sdo, 0);	
		gpio_direction_output(spi_sclk, 0);	
		gpio_direction_output(spi_sclk, 1);
		i <<= 1;
	}
	i = addr ;
	for (j = 0; j < 8; j++) {

		if (i & 0x8000)
			gpio_direction_output(spi_sdo, 1);	
		else
			gpio_direction_output(spi_sdo, 0);	
		gpio_direction_output(spi_sclk, 0);	
		gpio_direction_output(spi_sclk, 1);	
		i <<= 1;
	}
	gpio_direction_output(spi_cs, 1);
	msleep(1);
	gpio_direction_output(spi_cs, 0);
	i=0x00;
	for (j = 0; j < 8; j++) 
	{

		if (i & 0x80)
			gpio_direction_output(spi_sdo, 1);	
		else
			gpio_direction_output(spi_sdo, 0);	

		gpio_direction_output(spi_sclk, 0);	
		gpio_direction_output(spi_sclk, 1);
		i <<= 1;
	}
	i = addr ;
	for (j = 0; j < 8; j++) {

		if (i & 0x80)
			gpio_direction_output(spi_sdo, 1);	
		else
			gpio_direction_output(spi_sdo, 0);	

		gpio_direction_output(spi_sclk, 0);	
		gpio_direction_output(spi_sclk, 1);	
		i <<= 1;
	}

	gpio_direction_output(spi_cs, 1);
	msleep(1);
	gpio_direction_output(spi_cs, 0);

	i=0xc0;
	for (j = 0; j < 8; j++) 
	{

		if (i & 0x80)
			gpio_direction_output(spi_sdo, 1);	
		else
			gpio_direction_output(spi_sdo, 0);	
		gpio_direction_output(spi_sclk, 0);	
		gpio_direction_output(spi_sclk, 1);	
		i <<= 1;
	}
	bits1=0;
	gpio_direction_input(spi_sdi);
	for (j = 0; j < 8; j++) {
		gpio_direction_output(spi_sclk, 0);	
		gpio_direction_output(spi_sclk, 1);	
		dbit=gpio_get_value(spi_sdi);		
		bits1 = 2*bits1+dbit;
	}
	*data =  bits1;
	gpio_direction_output(spi_sdo,1);
	gpio_direction_output(spi_cs, 1);
}

static void gpio_lcd_lead_emuspi_write_one_index(unsigned short addr)
{
	unsigned short  i;
	int j;

	i = addr | 0x000;
	for (j = 0; j < 9; j++) {

		if ((i << j) & 0x100)
			gpio_direction_output(spi_sdo, 1);	
		else
			gpio_direction_output(spi_sdo, 0);	

		gpio_direction_output(spi_sclk, 0);	
		gpio_direction_output(spi_sclk, 1);	
	}

}
static void gpio_lcd_lead_emuspi_write_one_data(unsigned short data)
{
	unsigned short  i;
	int j;

	i = data | 0x100;
	for (j = 0; j < 9; j++) {

		if ((i << j) & 0x100)
			gpio_direction_output(spi_sdo, 1);	
		else
			gpio_direction_output(spi_sdo, 0);	

		gpio_direction_output(spi_sclk, 0);	
		gpio_direction_output(spi_sclk, 1);	
	}

}

static LCD_PANEL_TYPE lcd_panel_detect(void)
{
	unsigned int id_h,id_l;
	LCD_PANEL_TYPE panel_type;
	boolean read_truly_id=true;
	spi_init();
	if(read_truly_id)
	{
		gpio_lcd_truly_emuspi_read_one_index(0xDA00,&id_h);
		gpio_lcd_truly_emuspi_read_one_index(0xDB00,&id_l);
		printk("lead id is 0x%x%x\n",id_h,id_l);
	}
	else
	{		
	}
	
	if(((id_h==0x00)&&(id_l==0x80)))
	{
		panel_type=LCD_PANEL_TRULY_WVGA;
		return panel_type;
	}
	panel_type=LCD_PANEL_LEAD_WVGA;
	return panel_type;
}

static void SPI_Start(void)
{
	gpio_direction_output(spi_cs, 0);
}
static void SPI_Stop(void)
{
	gpio_direction_output(spi_cs, 1);

}

static struct msm_fb_panel_data lcdc_tft_panel_data = {
    /* ZTE_BACKLIGHT_WLY_004 @2009-11-27 ajust backlight, begin */
    .panel_info = {.bl_max = 27},      //ZTE_LCD_LHT_20100609_001
    /* ZTE_BACKLIGHT_WLY_004 @2009-11-27 ajust backlight, begin */
	.on = lcdc_panel_on,
	.off = lcdc_panel_off,
       .set_backlight = lcdc_set_bl,
};

static struct platform_device this_device = {
	.name   = "lcdc_panel_wvga",
	.id	= 1,
	.dev	= {
		.platform_data = &lcdc_tft_panel_data,  
	}
};

static int __devinit lcdc_panel_probe(struct platform_device *pdev)
{

	struct msm_panel_info *pinfo;
	int ret;

	if(pdev->id == 0) {     
		lcdc_tft_pdata = pdev->dev.platform_data;
		lcdc_tft_pdata->panel_config_gpio(1);   

		g_lcd_panel_type = lcd_panel_detect(); //temporary
		if(g_lcd_panel_type==LCD_PANEL_LEAD_WVGA)
		{
			pinfo = &lcdc_tft_panel_data.panel_info;
			pinfo->lcdc.h_back_porch = 10;
			pinfo->lcdc.h_front_porch = 8;
			pinfo->lcdc.h_pulse_width = 10;
			pinfo->lcdc.v_back_porch = 2;
			pinfo->lcdc.v_front_porch = 2;
			pinfo->lcdc.v_pulse_width = 2;
			pinfo->lcdc.border_clr = 0;	/* blk */
			pinfo->lcdc.underflow_clr = 0xff;	/* blue */
			pinfo->lcdc.hsync_skew = 0;
			LcdPanleID= 72;
		}
		else
		{
			pinfo = &lcdc_tft_panel_data.panel_info;
			pinfo->lcdc.h_back_porch = 8;
			pinfo->lcdc.h_front_porch = 8;
			pinfo->lcdc.h_pulse_width = 2;
			pinfo->lcdc.v_back_porch = 8;
			pinfo->lcdc.v_front_porch = 8;
			pinfo->lcdc.v_pulse_width = 2;
			pinfo->lcdc.border_clr = 0;	/* blk */
			pinfo->lcdc.underflow_clr = 0xffff;	/* blue */
			pinfo->lcdc.hsync_skew = 0;
			LcdPanleID=70;
		}
		pinfo->xres = 480;
		pinfo->yres = 800;		
		pinfo->type = LCDC_PANEL;
		pinfo->pdest = DISPLAY_1;
		pinfo->wait_cycle = 0;
		pinfo->bpp = 18;
		pinfo->fb_num = 2;
		pinfo->clk_rate = 24576000;
    	ret = platform_device_register(&this_device);		
		return 0;
	 	
	}
	msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = lcdc_panel_probe,
	.driver = {
		.name   = "lcdc_panel_wvga",
	},
};


static int __devinit lcdc_lcdc_panel_init(void)
{
	int ret;

	ret = platform_driver_register(&this_driver);

	return ret;
}

module_init(lcdc_lcdc_panel_init);
