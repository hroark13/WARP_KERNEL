/*
 * leds-msm-pmic-status.c - MSM PMIC LEDs driver.
 *
 * Copyright (c) 2009, ZTE, Corporation. All rights reserved.
 *
 */
/*===========================================================================

                        EDIT HISTORY FOR MODULE


  when            who  what, where, why
  ----------   ---   -----------------------------------------------------------
===========================================================================*/


/*===========================================================================
						 INCLUDE FILES FOR MODULE
===========================================================================*/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/timer.h>
#include <linux/ctype.h>
#include <linux/sched.h>
#include <mach/pmic.h>
#include <linux/slab.h>

/*=========================================================================*/
/*                                MACROS                                   */
/*=========================================================================*/

#define ZYF_BL_TAG "[ZYF@pmic-leds]"

#define MAX_PMIC_BL_LEVEL	16
#define BLINK_LED_NUM   2

//#define LED_INFO(fmt, arg...) 	printk(KERN_INFO "WXW@LED: %s: " fmt "\n", __func__  , ## arg);
#define LED_INFO(fmt, arg...) 	

/*=========================================================================*/
/*                               TYPEDEFS & STRUCTS                                 */
/*=========================================================================*/
struct BLINK_LED_data{
       int blink_flag;
	int blink_led_flag;  // 0: off, 1:0n
	int blink_on_time;  //ms
	int blink_off_time;  //ms
	struct timer_list timer;
       struct work_struct work_led_on;
       struct work_struct work_led_off;
	struct led_classdev led;
};

struct STATUS_LED_data {
	spinlock_t data_lock;
	struct BLINK_LED_data blink_led[2];  /*green, red */
};

struct STATUS_LED_data *STATUS_LED;


/*=========================================================================*/
/*                           LOCAL FUNCITONS REALIZATION                             */
/*=========================================================================*/

//ZTE_LED_WXW_20101208_002, begin, add a adaptive API to support P730A10 & V9+
/**********************************************************************
* Function:       pmic_set_led_hal
* Description:    it is a adaptive API to support P730A10 & V9+
* Input:           int type, int level
* Output:         ret : 0 OK, !0 Error            
* Return:          int
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/12/08     V1.0      wxw       create
**********************************************************************/
static int pmic_set_led_hal(enum ledtype type, int level)
{
    int ret = 0;

    LED_INFO("the type = %d, level = %d  ++\n", type,level);
    switch( type )
        {
            case LED_KEYPAD:                //red led
                //using pmic_low_current_led_set_current instead of the pmic_set_led_intensity
                ret = pmic_low_current_led_set_current(LOW_CURRENT_LED_DRV0, level);
                LED_INFO("after call pmic_low_current_led_set_current(LOW_CURRENT_LED_DRV0, level)  \n");
                break;
            case LED_LCD:                    //green led
                #ifdef CONFIG_MACH_JOE
                    ret = pmic_low_current_led_set_current(LOW_CURRENT_LED_DRV0, level);
                #else
                    ret = pmic_low_current_led_set_current(LOW_CURRENT_LED_DRV2, level);
                #endif
                
                LED_INFO("after call pmic_low_current_led_set_current(LOW_CURRENT_LED_DRV2, level)  \n");
                break;
            default:
                pr_info("[WXW@pmic-leds] don't support the type --- %d, LED\n", type);
            break;
        }
    
        LED_INFO("the ret = %d, exit the function --\n", ret);
    return ret;
}
//ZTE_LED_WXW_20101208_002, end, add a adapt API to support P730A10 & V9+


static void pmic_red_led_on(struct work_struct *work)
{
       struct BLINK_LED_data *b_led = container_of(work, struct BLINK_LED_data, work_led_on);
       LED_INFO(" ++\n");
	 pmic_set_led_hal(LED_KEYPAD, b_led->led.brightness / MAX_PMIC_BL_LEVEL);
	 LED_INFO(" --\n");
}

static void pmic_red_led_off(struct work_struct *work)
{
    LED_INFO(" ++\n");
    pmic_set_led_hal(LED_KEYPAD, LED_OFF);
    LED_INFO(" --\n");
}

static void pmic_green_led_on(struct work_struct *work)
{
       struct BLINK_LED_data *b_led = container_of(work, struct BLINK_LED_data, work_led_on);
      LED_INFO(" ++\n");
	pmic_set_led_hal(LED_LCD, b_led->led.brightness / MAX_PMIC_BL_LEVEL);
      LED_INFO(" --\n");
    
}

static void pmic_green_led_off(struct work_struct *work)
{
	pmic_set_led_hal(LED_LCD, LED_OFF);
}

static void (*func[4])(struct work_struct *work) = {
	pmic_red_led_on,
	pmic_red_led_off,
	pmic_green_led_on,
	pmic_green_led_off,
};

static void msm_pmic_bl_led_set(struct led_classdev *led_cdev,
	enum led_brightness value)
{
	int ret;
	LED_INFO(" ++\n");
	if (!strcmp(led_cdev->name, "red")) {
		ret = pmic_set_led_hal(LED_KEYPAD, value / MAX_PMIC_BL_LEVEL);
	} else {
		ret = pmic_set_led_hal(LED_LCD, value / MAX_PMIC_BL_LEVEL);
	}
	
	if (ret)
		dev_err(led_cdev->dev, "[ZYF@PMIC LEDS]can't set pmic backlight\n");
    
      LED_INFO("the ret = %d, exit the function --\n", ret);
}

static void pmic_leds_timer(unsigned long arg)
{
      struct BLINK_LED_data *b_led = (struct BLINK_LED_data *)arg;


              if(b_led->blink_flag)
		{
		       if(b_led->blink_led_flag)
		       {
		              schedule_work(&b_led->work_led_off);
		       	b_led->blink_led_flag = 0;  
		       	mod_timer(&b_led->timer,jiffies + msecs_to_jiffies(b_led->blink_off_time));
		       }
			else
			{
			       schedule_work(&b_led->work_led_on);
		       	b_led->blink_led_flag = 1;
				mod_timer(&b_led->timer,jiffies + msecs_to_jiffies(b_led->blink_on_time));
		       }
		}	
		else
		{
		       schedule_work(&b_led->work_led_on);
		}
		
}

static ssize_t led_blink_solid_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct STATUS_LED_data *STATUS_LED;
	int idx = 1;
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	ssize_t ret = 0;

	if (!strcmp(led_cdev->name, "red"))
		idx = 0;
	else
		idx = 1;

	STATUS_LED = container_of(led_cdev, struct STATUS_LED_data, blink_led[idx].led);

	/* no lock needed for this */
	sprintf(buf, "%u\n", STATUS_LED->blink_led[idx].blink_flag);
	ret = strlen(buf) + 1;

	return ret;
}

static ssize_t led_blink_solid_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t size)
{
	struct STATUS_LED_data *STATUS_LED;
	int idx = 1;
	char *after;
	unsigned long state;
	ssize_t ret = -EINVAL;
	size_t count;

	struct led_classdev *led_cdev = dev_get_drvdata(dev);

	if (!strcmp(led_cdev->name, "red"))
		idx = 0;
	else
		idx = 1;

	STATUS_LED = container_of(led_cdev, struct STATUS_LED_data, blink_led[idx].led);

	state = simple_strtoul(buf, &after, 10);

	count = after - buf;

	if (*after && isspace(*after))
		count++;

	if (count == size) {
		ret = count;
		spin_lock(&STATUS_LED->data_lock);
		if(0==state)
		{
		       STATUS_LED->blink_led[idx].blink_flag= 0;
			pr_info(ZYF_BL_TAG "DISABLE %s led blink \n",idx?"green":"red");
		}
		else
		{
		       STATUS_LED->blink_led[idx].blink_flag= 1;
		       STATUS_LED->blink_led[idx].blink_led_flag = 0;
			schedule_work(&STATUS_LED->blink_led[idx].work_led_off);
			mod_timer(&STATUS_LED->blink_led[idx].timer,jiffies + msecs_to_jiffies(STATUS_LED->blink_led[idx].blink_off_time));
			pr_info(ZYF_BL_TAG "ENABLE %s led blink \n",idx?"green":"red");
		}
		spin_unlock(&STATUS_LED->data_lock);
	}

	return ret;
}



static ssize_t cpldled_grpfreq_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct STATUS_LED_data *STATUS_LED;
	int idx = 1;

	struct led_classdev *led_cdev = dev_get_drvdata(dev);

	if (!strcmp(led_cdev->name, "red"))
		idx = 0;
	else
		idx = 1;

	STATUS_LED = container_of(led_cdev, struct STATUS_LED_data, blink_led[idx].led);
	return sprintf(buf, "blink_on_time = %u ms \n", STATUS_LED->blink_led[idx].blink_on_time);
}

static ssize_t cpldled_grpfreq_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t size)
{
	struct STATUS_LED_data *STATUS_LED;
	int idx = 1;
	char *after;
	unsigned long state;
	ssize_t ret = -EINVAL;
	size_t count;

	struct led_classdev *led_cdev = dev_get_drvdata(dev);

	if (!strcmp(led_cdev->name, "red"))
		idx = 0;
	else
		idx = 1;

	STATUS_LED = container_of(led_cdev, struct STATUS_LED_data, blink_led[idx].led);

	state = simple_strtoul(buf, &after, 10);

	count = after - buf;

	if (*after && isspace(*after))
		count++;

	if (count == size) {
		ret = count;
		spin_lock(&STATUS_LED->data_lock);
		STATUS_LED->blink_led[idx].blink_on_time = state;  //in ms
		pr_info(ZYF_BL_TAG "Set %s led blink_on_time=%d ms \n",idx?"green":"red",STATUS_LED->blink_led[idx].blink_on_time);
		spin_unlock(&STATUS_LED->data_lock);
	}

	return ret;
}



static ssize_t cpldled_grppwm_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct STATUS_LED_data *STATUS_LED;
	int idx = 1;

	struct led_classdev *led_cdev = dev_get_drvdata(dev);

	if (!strcmp(led_cdev->name, "red"))
		idx = 0;
	else
		idx = 1;
	
	STATUS_LED = container_of(led_cdev, struct STATUS_LED_data, blink_led[idx].led);
	return sprintf(buf, "blink_off_time = %u ms\n", STATUS_LED->blink_led[idx].blink_off_time);
}

static ssize_t cpldled_grppwm_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t size)
{
	struct STATUS_LED_data *STATUS_LED;
	int idx = 1;
	char *after;
	unsigned long state;
	ssize_t ret = -EINVAL;
	size_t count;

	struct led_classdev *led_cdev = dev_get_drvdata(dev);

	if (!strcmp(led_cdev->name, "red"))
		idx = 0;
	else
		idx = 1;

	STATUS_LED = container_of(led_cdev, struct STATUS_LED_data, blink_led[idx].led);

	state = simple_strtoul(buf, &after, 10);

	count = after - buf;

	if (*after && isspace(*after))
		count++;

	if (count == size) {
		ret = count;
		spin_lock(&STATUS_LED->data_lock);
		STATUS_LED->blink_led[idx].blink_off_time= state;  //in ms
		pr_info(ZYF_BL_TAG "Set %s led blink_off_time=%d ms \n",idx?"green":"red",STATUS_LED->blink_led[idx].blink_off_time);
		spin_unlock(&STATUS_LED->data_lock);
	}

	return ret;
}


static DEVICE_ATTR(blink, 0644, led_blink_solid_show, led_blink_solid_store);
static DEVICE_ATTR(grpfreq, 0644, cpldled_grpfreq_show, cpldled_grpfreq_store);
static DEVICE_ATTR(grppwm, 0644, cpldled_grppwm_show, cpldled_grppwm_store);


static int msm_pmic_led_probe(struct platform_device *pdev)
{
	int ret = 0;
	int i, j;

	STATUS_LED = kzalloc(sizeof(struct STATUS_LED_data), GFP_KERNEL);
	if (STATUS_LED == NULL) {
		printk(KERN_ERR "STATUS_LED_probe: no memory for device\n");
		ret = -ENOMEM;
		goto err_alloc_failed;
	}
	
	STATUS_LED->blink_led[0].led.name = "red";
	STATUS_LED->blink_led[0].led.brightness_set = msm_pmic_bl_led_set;
	STATUS_LED->blink_led[0].led.brightness = LED_OFF;
	STATUS_LED->blink_led[0].blink_flag = 0;
	STATUS_LED->blink_led[0].blink_on_time = 500;
	STATUS_LED->blink_led[0].blink_off_time = 500;

	STATUS_LED->blink_led[1].led.name = "green";
	STATUS_LED->blink_led[1].led.brightness_set = msm_pmic_bl_led_set;
	STATUS_LED->blink_led[1].led.brightness = LED_OFF;
	STATUS_LED->blink_led[1].blink_flag = 0;
	STATUS_LED->blink_led[1].blink_on_time = 500;
	STATUS_LED->blink_led[1].blink_off_time = 500;

	spin_lock_init(&STATUS_LED->data_lock);

	for (i = 0; i < 2; i++) {	/* red, green */
		ret = led_classdev_register(&pdev->dev, &STATUS_LED->blink_led[i].led);
		if (ret) {
			printk(KERN_ERR
			       "STATUS_LED: led_classdev_register failed\n");
			goto err_led_classdev_register_failed;
		}
	}

	for (i = 0; i < 2; i++) {
		ret =
		    device_create_file(STATUS_LED->blink_led[i].led.dev, &dev_attr_blink);
		if (ret) {
			printk(KERN_ERR
			       "STATUS_LED: create dev_attr_blink failed\n");
			goto err_out_attr_blink;
		}
	}

	for (i = 0; i < 2; i++) {
		ret =
		    device_create_file(STATUS_LED->blink_led[i].led.dev, &dev_attr_grppwm);
		if (ret) {
			printk(KERN_ERR
			       "STATUS_LED: create dev_attr_grppwm failed\n");
			goto err_out_attr_grppwm;
		}
	}

	for (i = 0; i < 2; i++) {
		ret =
		    device_create_file(STATUS_LED->blink_led[i].led.dev, &dev_attr_grpfreq);
		if (ret) {
			printk(KERN_ERR
			       "STATUS_LED: create dev_attr_grpfreq failed\n");
			goto err_out_attr_grpfreq;
		}
	}
	dev_set_drvdata(&pdev->dev, STATUS_LED);
	
	for (i = 0; i < 2; i++)
	{
	       INIT_WORK(&STATUS_LED->blink_led[i].work_led_on, func[i*2]);
	       INIT_WORK(&STATUS_LED->blink_led[i].work_led_off, func[i*2+1]);
	       setup_timer(&STATUS_LED->blink_led[i].timer, pmic_leds_timer, (unsigned long)&STATUS_LED->blink_led[i]);
		msm_pmic_bl_led_set(&STATUS_LED->blink_led[i].led, LED_OFF);
	}
	
       return 0;
	   
err_out_attr_grpfreq:
	for (j = 0; j < i; j++)
		device_remove_file(STATUS_LED->blink_led[i].led.dev, &dev_attr_blink);
	i = 2;
	
err_out_attr_grppwm:
	for (j = 0; j < i; j++)
		device_remove_file(STATUS_LED->blink_led[i].led.dev, &dev_attr_blink);
	i = 2;
	
err_out_attr_blink:
	for (j = 0; j < i; j++)
		device_remove_file(STATUS_LED->blink_led[i].led.dev, &dev_attr_blink);
	i = 2;

err_led_classdev_register_failed:
	for (j = 0; j < i; j++)
		led_classdev_unregister(&STATUS_LED->blink_led[i].led);

err_alloc_failed:
	kfree(STATUS_LED);

	return ret;
	
}

static int __devexit msm_pmic_led_remove(struct platform_device *pdev)
{
       int i;
	   
       for (i = 0; i < 2; i++)
		led_classdev_unregister(&STATUS_LED->blink_led[i].led);

	return 0;
}

#define CONFIG_ZTE_NLED_BLINK_WHILE_APP_SUSPEND
#ifdef CONFIG_ZTE_NLED_BLINK_WHILE_APP_SUSPEND
#include "../../arch/arm/mach-msm/proc_comm.h"
#define NLED_APP2SLEEP_FLAG_RED 0x0001//red
#define NLED_APP2SLEEP_FLAG_GREEN 0x0010//green
//#define NLED_APP2SLEEP_FLAG_VIB 0x0100
#define ZTE_PROC_COMM_CMD3_NLED_BLINK_DISABLE 2
#define ZTE_PROC_COMM_CMD3_NLED_BLINK_ENABLE 3


int msm_pmic_led_config_while_app2sleep(unsigned bright_red,unsigned bright_green, unsigned set)
{
	int config_last = 0;
	if(bright_red > 0)
	{
		config_last |= NLED_APP2SLEEP_FLAG_RED;
	}
	if(bright_green > 0)
	{
		config_last |= NLED_APP2SLEEP_FLAG_GREEN;
	}
	pr_info("LED PROC:Green %d,RED%d;config2Moden 0x%x,set%d\n",bright_green,bright_red,config_last,set);
	return msm_proc_comm(PCOM_CUSTOMER_CMD3, &config_last, &set);
}
#endif

#ifdef CONFIG_PM
static int msm_pmic_led_suspend(struct platform_device *dev,
		pm_message_t state)
{
       int i;
	   #ifdef CONFIG_ZTE_NLED_BLINK_WHILE_APP_SUSPEND
	   //blink_led[0] red,blink_led[1] green
	   msm_pmic_led_config_while_app2sleep( STATUS_LED->blink_led[0].led.brightness,//red
	   										STATUS_LED->blink_led[1].led.brightness, ZTE_PROC_COMM_CMD3_NLED_BLINK_ENABLE);//green
	   #endif
       for (i = 0; i < 2; i++)
		led_classdev_suspend(&STATUS_LED->blink_led[i].led);

	return 0;
}

static int msm_pmic_led_resume(struct platform_device *dev)
{
       int i;
	#ifdef CONFIG_ZTE_NLED_BLINK_WHILE_APP_SUSPEND
	   msm_pmic_led_config_while_app2sleep( 0,0, ZTE_PROC_COMM_CMD3_NLED_BLINK_DISABLE);
	#endif
       for (i = 0; i < 2; i++)
		led_classdev_resume(&STATUS_LED->blink_led[i].led);
	

	return 0;
}
#else
#define msm_pmic_led_suspend NULL
#define msm_pmic_led_resume NULL
#endif

static struct platform_driver msm_pmic_led_driver = {
	.probe		= msm_pmic_led_probe,
	.remove		= __devexit_p(msm_pmic_led_remove),
	.suspend	= msm_pmic_led_suspend,
	.resume		= msm_pmic_led_resume,
	.driver		= {
		.name	= "pmic-leds-status",
		.owner	= THIS_MODULE,
	},
};

static int __init msm_pmic_led_init(void)
{
	return platform_driver_register(&msm_pmic_led_driver);
}
module_init(msm_pmic_led_init);

static void __exit msm_pmic_led_exit(void)
{
	platform_driver_unregister(&msm_pmic_led_driver);
}
module_exit(msm_pmic_led_exit);

MODULE_DESCRIPTION("MSM PMIC LEDs driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:pmic-leds");
