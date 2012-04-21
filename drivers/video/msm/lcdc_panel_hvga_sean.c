/*
 * =====================================================================================
 *
 *       Filename:  lcdc_panel_wvga_oled.c
 *
 *    Description:  Samsung WVGA panel(480x800) driver 
 *
 *        Version:  1.0
 *        Created:  03/25/2010 02:05:46 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Lu Ya
 *        Company:  ZTE Corp.
 *
 * =====================================================================================
 */
/* ========================================================================================
when         who        what, where, why                                  comment tag
--------     ----       -----------------------------                --------------------------
2011-01-25   luya    	modify for 743	CRDB00603861 			ZTE_LCD_LUYA_20110125_001
2010-06-29   luya    	modify mdelay to msleep		 			ZTE_LCD_LUYA_20100629_001
2010-06-11   lht		project mode display panel info         	ZTE_LCD_LHT_20100611_001
2010-05-29   luya    	add delay when init finish		 			ZTE_LCD_LUYA_20100529_001
2010-05-22   luya    	modify BKL reg setting			 			ZTE_LCD_LUYA_20100522_001
2010-05-17   luya    	delete mdelay					 			ZTE_LCD_LUYA_20100517_001
2010-05-13   luya    	not init for the 1st time			 		ZTE_LCD_LUYA_20100513_001
2010-05-07   luya    	modify 729 BKL reg			 				ZTE_LCD_LUYA_20100507_001
2010-04-28   luya    	modify delay of bkl adjust		 			ZTE_LCD_LUYA_20100428_002
2010-04-14   luya    	modify for 729 bkl		 					ZTE_LCD_LUYA_20100414_001
2010-03-25   luya    	merge samsung oled driver		 			ZTE_LCD_LUYA_20100325_001

==========================================================================================*/


#include "msm_fb.h"
#include <asm/gpio.h>
#include <linux/module.h>
#include <linux/delay.h>

#define GPIO_LCD_BL_SC_OUT 2
#define GPIO_LCD_BL_EN


typedef enum
{
	LCD_PANEL_NONE = 0,
	LCD_PANEL_TRULY_HVGA,
}LCD_PANEL_TYPE;

static LCD_PANEL_TYPE g_lcd_panel_type = LCD_PANEL_NONE;

static boolean is_firsttime = true;		///ZTE_LCD_LUYA_20091221_001
extern u32 LcdPanleID;   //ZTE_LCD_LHT_20100611_001
static int spi_cs;
static int spi_sclk;
static int spi_sdi;
static int spi_sdo;
static int panel_reset;
static bool onewiremode = true;
///ZTE_LCD_LUYA_20100414_001	//ZTE_LCD_LUYA_20100507_001,,ZTE_LCD_LUYA_20100522_001
static struct msm_panel_common_pdata * lcdc_tft_pdata;

static void R61529_WriteReg(unsigned char SPI_COMMD);
static void  R61529_WriteData(unsigned char SPI_DATA);
static void lcdc_truly_init(void);
static void lcd_panel_init(void);
static void lcdc_set_bl(struct msm_fb_data_type *mfd);
void lcdc_truly_sleep(void);
static void spi_init(void);
static int lcdc_panel_on(struct platform_device *pdev);
static int lcdc_panel_off(struct platform_device *pdev);
#if 0
static void SPI_Start(void)
{
	gpio_direction_output(spi_cs, 0);
}
static void SPI_Stop(void)
{
	gpio_direction_output(spi_cs, 1);
}
#endif

////ZTE_LCD_LUYA_20100716_001
static void select_1wire_mode(void)
{
	gpio_direction_output(GPIO_LCD_BL_SC_OUT, 1);
	udelay(120);
	gpio_direction_output(GPIO_LCD_BL_SC_OUT, 0);
	udelay(280);				////ZTE_LCD_LUYA_20100226_001
	gpio_direction_output(GPIO_LCD_BL_SC_OUT, 1);
	udelay(650);				////ZTE_LCD_LUYA_20100226_001
	
}

static void send_bkl_address(void)
{
	unsigned int i,j;
	i = 0x72;
	gpio_direction_output(GPIO_LCD_BL_SC_OUT, 1);
	udelay(10);
	printk("[LY] send_bkl_address \n");
	for(j = 0; j < 8; j++)
	{
		if(i & 0x80)
		{
			gpio_direction_output(GPIO_LCD_BL_SC_OUT, 0);
			udelay(10);
			gpio_direction_output(GPIO_LCD_BL_SC_OUT, 1);
			udelay(180);
		}
		else
		{
			gpio_direction_output(GPIO_LCD_BL_SC_OUT, 0);
			udelay(180);
			gpio_direction_output(GPIO_LCD_BL_SC_OUT, 1);
			udelay(10);
		}
		i <<= 1;
	}
	gpio_direction_output(GPIO_LCD_BL_SC_OUT, 0);
	udelay(10);
	gpio_direction_output(GPIO_LCD_BL_SC_OUT, 1);

}

static void send_bkl_data(int level)
{
	unsigned int i,j;
	i = level & 0x1F;
	gpio_direction_output(GPIO_LCD_BL_SC_OUT, 1);
	udelay(10);
	printk("[LY] send_bkl_data \n");
	for(j = 0; j < 8; j++)
	{
		if(i & 0x80)
		{
			gpio_direction_output(GPIO_LCD_BL_SC_OUT, 0);
			udelay(10);
			gpio_direction_output(GPIO_LCD_BL_SC_OUT, 1);
			udelay(180);
		}
		else
		{
			gpio_direction_output(GPIO_LCD_BL_SC_OUT, 0);
			udelay(180);
			gpio_direction_output(GPIO_LCD_BL_SC_OUT, 1);
			udelay(10);
		}
		i <<= 1;
	}
	gpio_direction_output(GPIO_LCD_BL_SC_OUT, 0);
	udelay(10);
	gpio_direction_output(GPIO_LCD_BL_SC_OUT, 1);

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
    	gpio_direction_output(GPIO_LCD_BL_SC_OUT, 0);			///ZTE_LCD_LUYA_20100201_001
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
    	gpio_direction_output(GPIO_LCD_BL_SC_OUT, 0);
		mdelay(3);
		onewiremode = FALSE;
			
    }
    else 
	{
		if(!onewiremode)	//select 1 wire mode
		{
		       mdelay(100);
			printk("[LY] before select_1wire_mode\n");
			select_1wire_mode();
			onewiremode = TRUE;
		}
		send_bkl_address();
		send_bkl_data(current_lel-1);

	}
    local_irq_restore(flags);
}

static int lcdc_panel_on(struct platform_device *pdev)
{

	spi_init();
///ZTE_LCD_LUYA_20091221_001,start	ZTE_LCD_LUYA_20100513_001
	if(!is_firsttime)
	{
		lcd_panel_init();
		
	}
	else
	{
		is_firsttime = false;
	}
///ZTE_LCD_LUYA_20091221_001,end	
	return 0;
}

/***added by zte_gequn091966,20110429***/
static void R61529_WriteReg(unsigned char SPI_COMMD)
{
	unsigned short SBit,SBuffer;
	unsigned char BitCounter;
	
	SBuffer=SPI_COMMD;
	gpio_direction_output(spi_cs, 0);	//Set_CS(0); //CLR CS
	for(BitCounter=0;BitCounter<9;BitCounter++)
{
		SBit = SBuffer&0x100;
		if(SBit)
			gpio_direction_output(spi_sdo, 1);//Set_SDA(1);
		else
			gpio_direction_output(spi_sdo, 0);//Set_SDA(0);
			
		gpio_direction_output(spi_sclk, 0);//Set_SCK(0); //CLR SCL
		gpio_direction_output(spi_sclk, 1);//Set_SCK(1); //SET SCL
		SBuffer = SBuffer<<1;
	}
	gpio_direction_output(spi_cs, 1);//Set_CS(1); //SET CS
}

static void R61529_WriteData(unsigned char SPI_DATA)
{
	unsigned short SBit,SBuffer;
	unsigned char BitCounter;
	
	SBuffer=SPI_DATA | 0x100;
	gpio_direction_output(spi_cs, 0);//Set_CS(0); //CLR CS
	
	for(BitCounter=0;BitCounter<9;BitCounter++)
	{
		SBit = SBuffer&0x100;
		if(SBit)
			gpio_direction_output(spi_sdo, 1);//Set_SDA(1);
		else
			gpio_direction_output(spi_sdo, 0);//Set_SDA(0);
			
		gpio_direction_output(spi_sclk, 0);//Set_SCK(0); //CLR SCL
		gpio_direction_output(spi_sclk, 1);//Set_SCK(1); //SET SCL
		SBuffer = SBuffer<<1;
	}
	gpio_direction_output(spi_cs, 1);//Set_CS(1); //SET CS
}



void lcdc_truly_init(void)
{
   printk("gequn lcdc_truly_init\n");

   R61529_WriteReg(0xB0);
   R61529_WriteData(0x04);

   R61529_WriteReg(0x36);
   R61529_WriteData(0xC0);

   R61529_WriteReg(0x3A);
   R61529_WriteData(0x70);

   R61529_WriteReg(0xB4);
   R61529_WriteData(0x00);

   R61529_WriteReg(0x0A);
   R61529_WriteData(0x0C);

   R61529_WriteReg(0xC0);
   R61529_WriteData(0x03); //0x03 for inverse
   R61529_WriteData(0xDF);
   R61529_WriteData(0x40);
   R61529_WriteData(0x03);
   R61529_WriteData(0x10);
   R61529_WriteData(0x01);
   R61529_WriteData(0x00);
   R61529_WriteData(0x43);
   
   R61529_WriteReg(0xC1);
   R61529_WriteData(0x07);
   R61529_WriteData(0x28);
   R61529_WriteData(0x08);
   R61529_WriteData(0x08);
   R61529_WriteData(0x10);

   R61529_WriteReg(0xC4);
   R61529_WriteData(0x27);
   R61529_WriteData(0x00);
   R61529_WriteData(0x03);
   R61529_WriteData(0x01);

   R61529_WriteReg(0xC6);
   R61529_WriteData(0x05);

   R61529_WriteReg(0xC8);
   R61529_WriteData(0x00);
   R61529_WriteData(0x0E);
   R61529_WriteData(0x14);
   R61529_WriteData(0x25);
   R61529_WriteData(0x31);
   R61529_WriteData(0x4A);
   R61529_WriteData(0x3C);
   R61529_WriteData(0x23);
   R61529_WriteData(0x19);
   R61529_WriteData(0x14);
   R61529_WriteData(0x0D);
   R61529_WriteData(0x00);

   R61529_WriteData(0x00);
   R61529_WriteData(0x0E);
   R61529_WriteData(0x14);
   R61529_WriteData(0x25);
   R61529_WriteData(0x31);
   R61529_WriteData(0x4A);
   R61529_WriteData(0x3C);
   R61529_WriteData(0x23);
   R61529_WriteData(0x19);
   R61529_WriteData(0x14);
   R61529_WriteData(0x0D);
   R61529_WriteData(0x05);

   R61529_WriteReg(0xC9);
   R61529_WriteData(0x00);
   R61529_WriteData(0x0E);
   R61529_WriteData(0x14);
   R61529_WriteData(0x25);
   R61529_WriteData(0x31);
   R61529_WriteData(0x4A);
   R61529_WriteData(0x3C);
   R61529_WriteData(0x23);
   R61529_WriteData(0x19);
   R61529_WriteData(0x14);
   R61529_WriteData(0x0D);
   R61529_WriteData(0x00);

   R61529_WriteData(0x00);
   R61529_WriteData(0x0E);
   R61529_WriteData(0x14);
   R61529_WriteData(0x25);
   R61529_WriteData(0x31);
   R61529_WriteData(0x4A);
   R61529_WriteData(0x3C);
   R61529_WriteData(0x23);
   R61529_WriteData(0x19);
   R61529_WriteData(0x14);
   R61529_WriteData(0x0D);
   R61529_WriteData(0x05);

   R61529_WriteReg(0xCA);
   R61529_WriteData(0x00);
   R61529_WriteData(0x0E);
   R61529_WriteData(0x14);
   R61529_WriteData(0x25);
   R61529_WriteData(0x31);
   R61529_WriteData(0x4A);
   R61529_WriteData(0x3C);
   R61529_WriteData(0x23);
   R61529_WriteData(0x19);
   R61529_WriteData(0x14);
   R61529_WriteData(0x0D);
   R61529_WriteData(0x00);

   R61529_WriteData(0x00);
   R61529_WriteData(0x0E);
   R61529_WriteData(0x14);
   R61529_WriteData(0x25);
   R61529_WriteData(0x31);
   R61529_WriteData(0x4A);
   R61529_WriteData(0x3C);
   R61529_WriteData(0x23);
   R61529_WriteData(0x19);
   R61529_WriteData(0x14);
   R61529_WriteData(0x0D);
   R61529_WriteData(0x05);

   R61529_WriteReg(0xD0);
   R61529_WriteData(0x95);
   R61529_WriteData(0x0E);
   R61529_WriteData(0x08);
   R61529_WriteData(0x20);
   R61529_WriteData(0x31);
   R61529_WriteData(0x04);
   R61529_WriteData(0x01);
   R61529_WriteData(0x00);
   R61529_WriteData(0x08);
   R61529_WriteData(0x01);
   R61529_WriteData(0x00);
   R61529_WriteData(0x06);
   R61529_WriteData(0x01);
   R61529_WriteData(0x00);
   R61529_WriteData(0x00);
   R61529_WriteData(0x20);

   R61529_WriteReg(0xD1);
   R61529_WriteData(0x02);
   R61529_WriteData(0x32);
   R61529_WriteData(0x1F);
   R61529_WriteData(0x2C);

   R61529_WriteReg(0x2A);
   R61529_WriteData(0x00);
   R61529_WriteData(0x00);
   R61529_WriteData(0x01);
   R61529_WriteData(0x3F);

   R61529_WriteReg(0x2B);
   R61529_WriteData(0x00);
   R61529_WriteData(0x00);
   R61529_WriteData(0x01);
   R61529_WriteData(0xE0);

   R61529_WriteReg(0xB0);
   R61529_WriteData(0x03);

   R61529_WriteReg(0x11);
   mdelay(240);
   R61529_WriteReg(0x29);

}

static void spi_init(void)
{
	spi_sclk = *(lcdc_tft_pdata->gpio_num+ 5);
	spi_cs   = *(lcdc_tft_pdata->gpio_num + 3);
	spi_sdo  = *(lcdc_tft_pdata->gpio_num + 2);
	spi_sdi  = *(lcdc_tft_pdata->gpio_num + 4);
	panel_reset = *(lcdc_tft_pdata->gpio_num + 6);
	printk("spi_init\n!");

	gpio_set_value(spi_sclk, 1);
	gpio_set_value(spi_sdo, 1);
	gpio_set_value(spi_cs, 1);
//	mdelay(10);			////ZTE_LCD_LUYA_20100513_001
	msleep(20);				////ZTE_LCD_LUYA_20100629_001

}
void lcdc_truly_sleep(void)
{
	    R61529_WriteReg(0x28);  
	    msleep(100);
	    R61529_WriteReg(0x10);  
	    msleep(200);
}

static int lcdc_panel_off(struct platform_device *pdev)
{
	printk("lcdc_panel_off , g_lcd_panel_type is %d(1 LEAD. 2 TRULY. 3 OLED. )\n",g_lcd_panel_type);
	switch(g_lcd_panel_type)
	{
		case LCD_PANEL_TRULY_HVGA:
//			lcdc_truly_sleep();
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
static LCD_PANEL_TYPE lcd_panel_detect(void)
{
   spi_init();
   return LCD_PANEL_TRULY_HVGA;

}
void lcd_panel_init(void)
{
	gpio_direction_output(panel_reset, 1);
	msleep(10);						////ZTE_LCD_LUYA_20100629_001
	gpio_direction_output(panel_reset, 0);
	msleep(50);						////ZTE_LCD_LUYA_20100629_001
	gpio_direction_output(panel_reset, 1);
	msleep(50);	
	switch(g_lcd_panel_type)
	{
		case LCD_PANEL_TRULY_HVGA:
			lcdc_truly_init();
			break;		
		default:
			break;
	}
}

static struct msm_fb_panel_data lcdc_tft_panel_data = {
       .panel_info = {.bl_max = 32},
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
static int  lcdc_panel_probe(struct platform_device *pdev)
{
	struct msm_panel_info *pinfo;
	int ret;

	if(pdev->id == 0) {     
		lcdc_tft_pdata = pdev->dev.platform_data;
		lcdc_tft_pdata->panel_config_gpio(1);   

		g_lcd_panel_type = lcd_panel_detect();
		if(g_lcd_panel_type==LCD_PANEL_TRULY_HVGA)
		{
			pinfo = &lcdc_tft_panel_data.panel_info;
			pinfo->lcdc.h_back_porch = 20;
			pinfo->lcdc.h_front_porch = 50;
			pinfo->lcdc.h_pulse_width = 10;
			pinfo->lcdc.v_back_porch = 6;  
			pinfo->lcdc.v_front_porch = 8;
			pinfo->lcdc.v_pulse_width = 2;
			pinfo->lcdc.border_clr = 0;	/* blk */
			pinfo->lcdc.underflow_clr = 0xffff;	/* blue */
			pinfo->lcdc.hsync_skew = 0;
		}
		
		pinfo->xres = 320;
		pinfo->yres = 480;		
		pinfo->type = LCDC_PANEL;
		pinfo->pdest = DISPLAY_1;
		pinfo->wait_cycle = 0;
		pinfo->bpp = 24;     //18
		pinfo->fb_num = 2;
		switch(g_lcd_panel_type)
		{
			case LCD_PANEL_TRULY_HVGA:
				LcdPanleID=(u32)61;   //ZTE_LCD_LHT_20100611_001
				pinfo->clk_rate = 12288000;
				ret = platform_device_register(&this_device);
				break;
		
			default:
				break;
		}		


    	//ret = platform_device_register(&this_device);
		
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



static int  lcdc_oled_panel_init(void)
{
	int ret;

	ret = platform_driver_register(&this_driver);

	return ret;
}

module_init(lcdc_oled_panel_init);

