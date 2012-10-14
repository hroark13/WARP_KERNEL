/******************************************************************************
  @file    max8903_charger.c
  @brief   Maxim 8903 USB/Adapter Charger Driver
  @author  liuzhongzhi 2011/05/20

  DESCRIPTION

  INITIALIZATION AND SEQUENCING REQUIREMENTS

  ---------------------------------------------------------------------------
  Copyright (c) 2011 ZTE Incorporated. All Rights Reserved. 
  ZTE Proprietary and Confidential.
  ---------------------------------------------------------------------------
******************************************************************************/
/*==============================================================================

                           EDIT HISTORY

when         who           what, where, why                                             comment tag
--------     ---------     ---------------------------             ------------
2011/06/08   liuzhongzhi    add for compatibe charger IC                         liuzhongzhi0007
==============================================================================*/

#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/power_supply.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/msm-charger.h>
#include <linux/mfd/pmic8058.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <mach/msm_hsusb.h>
#include <linux/max8903_charger.h>
#include <mach/hw_ver.h>
#include <linux/wakelock.h>

struct max8903_struct {
	struct device *dev ;
	struct delayed_work		charge_work;
	struct delayed_work		ac_charger;
	int		present;
	bool	charging;
	bool	usb_chg_enable;
	int		dock_det; /* DOCK_DET pin */
	int		cen;	/* Charger Enable input */
	int		chg;	/* Charger status output */
	int		flt;	/* Fault output */
	int		usus;	/* USB suspend */
	int		irq;	/* DC(Adapter) Power OK output */
	struct msm_hardware_charger	adapter_hw_chg;
	struct wake_lock wl;
};

#define MAX8903_CHG_PERIOD	((HZ) * 150)
#define MAX8903_BATTERY_FULL	95
#define AC_CHARGER_DETECT_DELAY   ((HZ) * 2)

/* Wall charger or USB charger */
static enum chg_type cur_chg_type = USB_CHG_TYPE__INVALID;
static struct max8903_struct *saved_msm_chg;
static int delay_ac_charger_detect = 0;

extern struct i2c_client *max17040_bak_client;
extern int max17040_get_soc(struct i2c_client *client);
extern int max17040_get_vcell(struct i2c_client *client);
extern void msm_set_source_current(struct msm_hardware_charger *hw_chg, unsigned int mA);
extern void msm_register_usb_charger_state(void (*function)(int *));

static void ac_charger_detect(struct work_struct *max8903_work)
{
	struct max8903_struct *max_chg;

	max_chg = container_of(max8903_work, struct max8903_struct,
			ac_charger.work);

	dev_info(max_chg->dev, "%s\n", __func__);

	/*
	 * if it's not a stadard USB charger
	 * think it as an AC charger
	 */
	if (delay_ac_charger_detect){
		delay_ac_charger_detect = 0;
		/* change charger type */
		max_chg->adapter_hw_chg.type = CHG_TYPE_AC;
		
		/* queue a insert envent */
		pr_info("%s: queue a insert event\n", __func__);
		msm_charger_notify_event(&max_chg->adapter_hw_chg,
			 CHG_INSERTED_EVENT);
	}
}

/*
 * max8903_chg_connected() - notify the charger connected event
 * @chg_type:	charger type 
 */
void max8903_chg_connected(enum chg_type chg_type)
{
	cur_chg_type = chg_type;
	pr_info("%s:chg type =%d\n", __func__, cur_chg_type);

	if (delay_ac_charger_detect){
		delay_ac_charger_detect = 0;
		cancel_delayed_work(&saved_msm_chg->ac_charger);
		
		pr_info("%s: queue a insert event\n", __func__);
		msm_charger_notify_event(&saved_msm_chg->adapter_hw_chg,
				CHG_INSERTED_EVENT);
	}	
}

/*
 * max8903_usb_charger_state() - determine the usb charger state
 * @mA:	the charge current from USB system
 *
 * if current is USB charger and USB charger is disabled,
 * modify the *mA to 0. That cause not to start charging
 */
static void max8903_usb_charger_state(int *mA)
{
	if ((USB_CHG_TYPE__SDP == cur_chg_type) && 
				(!saved_msm_chg->usb_chg_enable)){
		pr_info("%s:mA=%d, change to 0\n", __func__, *mA);
		*mA = 0;
	}
}
	
static void max8903_charge(struct work_struct *max8903_work)
{
	struct max8903_struct *max_chg;
	int current_capacity;
    int current_voltage;

	max_chg = container_of(max8903_work, struct max8903_struct,
			charge_work.work);

	dev_info(max_chg->dev, "%s\n", __func__);

	if (max_chg->charging) {
        current_capacity = max17040_get_soc(max17040_bak_client);
        current_voltage  = max17040_get_vcell(max17040_bak_client);
		pr_info("%s:capacity=%d voltage=%d\n ", __func__, current_capacity, current_voltage); 

        //get the charger status value
        if ((gpio_get_value(max_chg->chg) == 1) && (current_capacity > MAX8903_BATTERY_FULL))
		    msm_charger_notify_event(&max_chg->adapter_hw_chg,  CHG_DONE_EVENT);
        
		schedule_delayed_work(&max_chg->charge_work,
						MAX8903_CHG_PERIOD);
	}
}

static int max8903_start_charging(struct msm_hardware_charger *hw_chg,
		int chg_voltage, int chg_current)
{
	struct max8903_struct *max_chg;
	int ret = 0;

	max_chg = container_of(hw_chg, struct max8903_struct, adapter_hw_chg);
	if (max_chg->charging)
		/* we are already charging */
		return 0;

	//dev_dbg(max_chg->dev, "%s\n", __func__);

    gpio_direction_output(max_chg->cen, 0); //enable charger
    
	/*
	 * if usb charger is enabled, pull the USUS down
	 * else pull the USUS up
	 */
	if (max_chg->usb_chg_enable)
		gpio_direction_output(max_chg->usus, 0);
	else
		gpio_direction_output(max_chg->usus, 1);
    
	dev_info(max_chg->dev, "%s starting timed work\n",
							__func__);
	schedule_delayed_work(&max_chg->charge_work,
						MAX8903_CHG_PERIOD);
	max_chg->charging = true;

	return ret;
}

static int max8903_stop_charging(struct msm_hardware_charger *hw_chg)
{
	struct max8903_struct *max_chg;
	int ret = 0;

	max_chg = container_of(hw_chg, struct max8903_struct, adapter_hw_chg);
	if (!(max_chg->charging))
		/* we arent charging */
		return 0;

	//dev_dbg(max_chg->dev, "%s\n", __func__);

    gpio_direction_output(max_chg->cen, 0); //always enable charger

	/*
	 * if usb charger is enabled, pull the USUS down
	 * else pull the USUS up
	 */
	if (max_chg->usb_chg_enable)
		gpio_direction_output(max_chg->usus, 0);
	else
		gpio_direction_output(max_chg->usus, 1);

	dev_info(max_chg->dev, "%s stopping timed work\n",
							__func__);

	max_chg->charging = false;
	cancel_delayed_work(&max_chg->charge_work);
    
	return ret;
}

static int max8903_charging_switched(struct msm_hardware_charger *hw_chg)
{
	struct max8903_struct *max_chg;

	max_chg = container_of(hw_chg, struct max8903_struct, adapter_hw_chg);
	dev_info(max_chg->dev, "%s\n", __func__);
	return 0;
}

static irqreturn_t max8903_fault(int irq, void *dev_id)
{
	struct max8903_struct *max_chg;
    struct platform_device *pdev;

    pdev = (struct platform_device *)dev_id;
    max_chg = platform_get_drvdata(pdev);
    
    dev_err(max_chg->dev, "%s:MAX8903 Fault condition ocuured.\n", __func__);
	// do not reset the device manually for safe consideration
#if 0
    // when fault occured, reset charger
    gpio_direction_output(max_chg->cen, 1); //disable charger
    msleep(100);
    gpio_direction_output(max_chg->cen, 0); //enable charger
#endif    
	return IRQ_HANDLED;
}

static irqreturn_t max_valid_handler(int irq, void *dev_id)
{
	struct max8903_struct *max_chg;
    struct platform_device *pdev;
    struct pm8058_chip *chip;
    int state;

    pdev = (struct platform_device *)dev_id;
    max_chg = platform_get_drvdata(pdev);
    chip = get_irq_data(irq);
	
#if 0
	/* Dock insert, think it as AC charger */
	if (gpio_get_value_cansleep(max_chg->dock_det) && (BOARD_NUM(hw_ver) != BOARD_NUM_V11))
		max_chg->adapter_hw_chg.type = CHG_TYPE_USB;
	else
		max_chg->adapter_hw_chg.type = CHG_TYPE_AC;
#endif
	/* reinitialize charger type */
	if ((BOARD_NUM(hw_ver) == BOARD_NUM_V11))
		max_chg->adapter_hw_chg.type = CHG_TYPE_AC;
	else
		max_chg->adapter_hw_chg.type = CHG_TYPE_USB;

    state = pm8058_irq_get_rt_status(chip, irq);
    pr_info("%s:charge state=%d, hw_chg_type=%d\n", __func__, state, max_chg->adapter_hw_chg.type);
    if(state){
		/* delay to queue charge insert envent when charger inserted,
		 * need to detect if it is an ac charger
		 */
        //msm_charger_notify_event(&max_chg->adapter_hw_chg,
        //			 CHG_INSERTED_EVENT);
		delay_ac_charger_detect = 1;
		pr_info("%s:delay_ac_charger_detect=%d start ac charger delay work\n", __func__, delay_ac_charger_detect);
		schedule_delayed_work(&max_chg->ac_charger,
					AC_CHARGER_DETECT_DELAY);
        max_chg->present = 1;
		wake_lock(&max_chg->wl);
    }else{
    	delay_ac_charger_detect = 0;
		pr_info("%s:delay_ac_charger_detect=%d cancel ac charger delay work\n", __func__, delay_ac_charger_detect);
		cancel_delayed_work(&saved_msm_chg->ac_charger);
		
        msm_charger_notify_event(&max_chg->adapter_hw_chg,
        			 CHG_REMOVED_EVENT);
        max_chg->present = 0;
		wake_unlock(&max_chg->wl);
    }
	return IRQ_HANDLED;
}

/*
 * show_usb_chg_enable() - Show max8903 usb chg enable status
 */
static ssize_t show_usb_chg_enable(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	struct max8903_struct *max_chg;
	
	max_chg = dev_get_drvdata(dev);

	return sprintf(buf, "CHG ENABLE:%u\nCHG TYPE:%u\n", 
				max_chg->usb_chg_enable, cur_chg_type);
}

/*
 * store_usb_chg_enable() - Enable/Disable max8903 usb charge
 */
static ssize_t store_usb_chg_enable(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t count)
{
	struct max8903_struct *max_chg;	
	char *after;
	unsigned long num;
	
	max_chg = dev_get_drvdata(dev);
	
	num = simple_strtoul(buf, &after, 10);
	if (num == 0)
	{
		/* disable max8903 usb charge */
		max_chg->usb_chg_enable = 0;
	
		/*
		 * if current is USB charger, need to stop it
		 * set the source current to 0 mA
		 */
		if (USB_CHG_TYPE__SDP == cur_chg_type){
			cancel_delayed_work(&max_chg->charge_work);	/* cancel the charger work */
			msm_set_source_current(&max_chg->adapter_hw_chg, 0);
			msm_charger_notify_event(&max_chg->adapter_hw_chg,
							CHG_ENUMERATED_EVENT);
		}
		
		/* pull the USUS up */
		gpio_direction_output(max_chg->usus, 1);
	}
	else
	{
		/* enable max8903 usb charge */
		max_chg->usb_chg_enable = 1;

		/* pull the USUS down */
		gpio_direction_output(max_chg->usus, 0);
		
		/*
		 * if current is USB charger, need to start it
		 * set the source current to 500 mA
		 */
		if (USB_CHG_TYPE__SDP == cur_chg_type){
			msm_set_source_current(&max_chg->adapter_hw_chg, 500);
			msm_charger_notify_event(&max_chg->adapter_hw_chg,
							CHG_ENUMERATED_EVENT);
		}
	}
	
	return count;
}

static struct device_attribute usb_chg_enable_attr =
	__ATTR(usb_chg_enable, S_IRUGO | S_IWUSR | S_IWGRP, show_usb_chg_enable, store_usb_chg_enable);


static int __devinit max8903_probe(struct platform_device *pdev)
{
	struct max8903_struct *max_chg;
	struct device *dev = &pdev->dev;
	struct max8903_platform_data *pdata = pdev->dev.platform_data;
    struct pm8058_chip *chip;
	int ret = 0;
    
    //printk("%s\n", __func__);
	max_chg = kzalloc(sizeof(struct max8903_struct), GFP_KERNEL);
	if (max_chg == NULL) {
		dev_err(dev, "Cannot allocate memory.\n");
		return -ENOMEM;
	}
	
	saved_msm_chg = max_chg;
	
	if (pdata == NULL) {
		dev_err(&pdev->dev, "%s no platform data\n", __func__);
		ret = -EINVAL;
		goto out;
	}

	INIT_DELAYED_WORK(&max_chg->charge_work, max8903_charge);
	INIT_DELAYED_WORK(&max_chg->ac_charger, ac_charger_detect);
	wake_lock_init(&max_chg->wl, WAKE_LOCK_SUSPEND, "max8903");
    
	max_chg->dev = &pdev->dev;;
    max_chg->irq = pdata->irq;
    max_chg->cen = pdata->cen;
    max_chg->chg = pdata->chg;
    max_chg->flt = pdata->flt;
	max_chg->usus = pdata->usus;
	max_chg->dock_det = pdata->dock_det;
	max_chg->usb_chg_enable = 1;	/* enable usb charge */

	if (BOARD_NUM(hw_ver) == BOARD_NUM_V11)
		max_chg->adapter_hw_chg.type = CHG_TYPE_AC;
	else
		max_chg->adapter_hw_chg.type = CHG_TYPE_USB;
	max_chg->adapter_hw_chg.rating = 2;
	max_chg->adapter_hw_chg.name = "max8903-charger";
	max_chg->adapter_hw_chg.start_charging = max8903_start_charging;
	max_chg->adapter_hw_chg.stop_charging = max8903_stop_charging;
	max_chg->adapter_hw_chg.charging_switched = max8903_charging_switched;
    
	platform_set_drvdata(pdev, max_chg);

	ret = gpio_request(max_chg->cen, "CHARGER_CEN_N");
	if (ret) {
		dev_err(max_chg->dev, "%s gpio_request failed for %d ret=%d\n",
			__func__, max_chg->cen, ret);
		goto free_max_chg;
	}

	ret = gpio_request(max_chg->chg, "CHARGER_STATUS");
	if (ret) {
		dev_err(max_chg->dev, "%s gpio_request failed for %d ret=%d\n",
			__func__, max_chg->chg, ret);
		goto free_cen;
	}
    gpio_direction_input(max_chg->chg);

    ret = gpio_request(max_chg->flt, "CHARGER_FAULT_N");
	if (ret) {
		dev_err(max_chg->dev, "%s gpio_request failed for %d ret=%d\n",
			__func__, max_chg->flt, ret);
		goto free_chg;
	}    
    gpio_direction_input(max_chg->flt);

    ret = request_threaded_irq(gpio_to_irq(max_chg->flt),
                            NULL, max8903_fault,
                            IRQF_TRIGGER_FALLING ,"MAX8903 Fault", pdev);
    if (ret) {
        dev_err(dev, "Cannot request irq %d for Fault (%d)\n",
                gpio_to_irq(max_chg->flt), ret);
        goto free_flt;
    }

	ret = gpio_request(max_chg->usus, "USUS_CTRL");
	if (ret) {
		dev_err(max_chg->dev, "%s gpio_request failed for %d ret=%d\n",
			__func__, max_chg->usus, ret);
		goto err_flt_irq;
	}

	ret = gpio_request(max_chg->dock_det, "DOCK_DET");
	if (ret) {
		dev_err(max_chg->dev, "%s gpio_request failed for %d ret=%d\n",
			__func__, max_chg->dock_det, ret);
		goto free_usus;
	}
	gpio_direction_input(max_chg->dock_det);
    
	ret = msm_charger_register(&max_chg->adapter_hw_chg);
	if (ret) {
		dev_err(max_chg->dev,
			"%s msm_charger_register failed for ret =%d\n",
			__func__, ret);
		goto free_dock_det;
	}
	
	ret = device_create_file(max_chg->dev, &usb_chg_enable_attr);
	if (ret) {
		dev_err(max_chg->dev, "failed: create usb_chg_enable file\n");
	}
	
	ret = request_threaded_irq(max_chg->irq, NULL,
				   max_valid_handler,
				   IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
				   "max_valid_handler", pdev);
	if (ret) {
		dev_err(max_chg->dev,
			"%s request_threaded_irq failed for %d ret =%d\n",
			__func__, max_chg->irq, ret);
		goto unregister;
	}
	set_irq_wake(max_chg->irq, 1);

    chip = get_irq_data(max_chg->irq);
    ret = pm8058_irq_get_rt_status(chip, max_chg->irq);
	if (ret) {
	#if 0
		if (!gpio_get_value_cansleep(max_chg->dock_det))	/* Dock insert, think it as AC charger */
			max_chg->adapter_hw_chg.type = CHG_TYPE_AC;
		msm_charger_notify_event(&max_chg->adapter_hw_chg,
				CHG_INSERTED_EVENT);
	#else
		/* Charger inserted, but not a valid USB charger
		 * think it as a AC charger
		 */
		if (USB_CHG_TYPE__INVALID == cur_chg_type)
			max_chg->adapter_hw_chg.type = CHG_TYPE_AC;
		
		msm_charger_notify_event(&max_chg->adapter_hw_chg,
				CHG_INSERTED_EVENT);
	#endif
		max_chg->present = 1;
		wake_lock(&max_chg->wl);
	}

	msm_register_usb_charger_state(max8903_usb_charger_state);

	pr_info("%s OK chg_present=%d\n", __func__, max_chg->present);
	return 0;

unregister:
	msm_charger_unregister(&max_chg->adapter_hw_chg);
free_dock_det:
	gpio_free(max_chg->dock_det);
free_usus:
    gpio_free(max_chg->usus);
err_flt_irq:
    free_irq(gpio_to_irq(max_chg->flt), pdev);
free_flt:
    gpio_free(max_chg->flt);
free_chg:
    gpio_free(max_chg->chg);
free_cen:
    gpio_free(max_chg->cen);
free_max_chg:
	kfree(max_chg);
out:
	return ret;
}

static __devexit int max8903_remove(struct platform_device *pdev)
{
	struct max8903_struct *max_chg;
    
    max_chg = platform_get_drvdata(pdev);

	if (max_chg) {
		
		device_remove_file(max_chg->dev, &usb_chg_enable_attr);
		
    	free_irq(max_chg->irq, pdev);
        free_irq(gpio_to_irq(max_chg->flt), pdev);
        gpio_free(max_chg->flt);
        gpio_free(max_chg->chg);
        gpio_free(max_chg->cen);
    	cancel_delayed_work_sync(&max_chg->charge_work);
    	msm_charger_notify_event(&max_chg->adapter_hw_chg, CHG_REMOVED_EVENT);
    	msm_charger_unregister(&max_chg->adapter_hw_chg);
    	kfree(max_chg);            
	}

	return 0;
}

static struct platform_driver max8903_driver = {
	.probe	= max8903_probe,
	.remove	= __devexit_p(max8903_remove),
	.driver = {
		.name	= "max8903-charger",
		.owner	= THIS_MODULE,
	},
};

static int __init max8903_init(void)
{
/* liuzhongzhi0007 added by liuzhongzhi 20110608 for compatible charger IC*/
#ifdef CONFIG_ISL9519_CHARGER
    if (hw_ver == HW_VERSION_V11_A)
        return 0;
#endif
/* end liuzhongzhi0007 */
	return platform_driver_register(&max8903_driver);
}
module_init(max8903_init);

static void __exit max8903_exit(void)
{
	platform_driver_unregister(&max8903_driver);
}
module_exit(max8903_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MAX8903 Charger Driver");
MODULE_ALIAS("max8903-charger");
