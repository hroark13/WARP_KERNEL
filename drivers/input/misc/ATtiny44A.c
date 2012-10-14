/*
 * Support for the capacitance ATTINY44A prox device
 *
 * Copyright (C) 2011  zhaoyang196673 <zhao.yang3@zte.com.cn>
 */

#include <linux/input-polldev.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/init.h>

//zhaoyang196673 add for capacitance ATTINY44A prox begin
#include <asm/errno.h>
#include <linux/delay.h>
#include <mach/gpio.h>
#include <linux/poll.h>
#include <linux/input.h>

#include <linux/ATtiny44A.h>
//zhaoyang196673 add for capacitance ATTINY44A prox end

/* zhaoy0012 : 2011/6/13 modify attiny44a driver to support dual device*/
#include <linux/pm.h>
/* end zhaoyang 2011/6/13 */

#define DRV_NAME "attiny44a"
#define ATTINY44A_RATE 100 /* msec */

#define MAX_PROX_VAL    65535
#define INPUT_FUZZ	    32
#define INPUT_FLAT	    32
#define DEV1_INT_VAL    1<<7
static int attiny44a_dev1_pm_gpio = 0;
static int attiny44a_dev2_pm_gpio = 0;

/* zhaoy0012 : 2011/6/13 modify attiny44a driver to support dual device*/
#define DEV2_INT_VAL    1<<8
#define SYS4DEBUG
#define PM_ACTION(GPIO)     for(i = 0; i < 2; i++) {        \
        gpio_set_value((GPIO), 0);  \
        mdelay(10);                 \
        gpio_set_value((GPIO), 1);  \
        mdelay(10);                 \
    }
/* end zhaoyang 2011/6/13 */

static void ATtiny44A_poll(struct input_polled_dev *poll_dev)
{
    /* zhaoy0012 : 2011/6/13 modify attiny44a driver to support dual device*/
    struct attiny44a_platform_data *pdata = poll_dev->private;
    int state, prox_val = 0;

    state = gpio_get_value_cansleep(pdata->dev1_int_gpio);
    prox_val = state ? 0 : DEV1_INT_VAL;

    if(pdata->dev2_int_gpio) {
        state = gpio_get_value_cansleep(pdata->dev2_int_gpio);
        prox_val |= (state ? 0 : DEV2_INT_VAL);
    }
    /* end zhaoyang 2011/6/13 */
    input_report_abs(poll_dev->input, ABS_DISTANCE , prox_val);
    input_sync(poll_dev->input);
}

static int __devinit ATtiny44A_probe(struct platform_device *pdev)
{
    struct input_polled_dev *poll_dev;
    int error;

    struct attiny44a_platform_data *pdata = pdev->dev.platform_data;
    if (pdata->enable_dev) {
        if(pdata->enable_dev() == 0) {
            printk("NO ATtiny44A device in board\n");
            return ENODEV;
        }
    }

    printk("HAVE ATtiny44A device in board\n");

    /* zhaoy0012 : 2011/6/13 modify attiny44a driver to support dual device*/
    if (pdata->dev1_int_gpio) {
        error = pdata->setup_dev1_int_gpio(1);
        if (error) {
            pr_err("%s: Failed to setup GPIOs for attiny44a_dev1 int pin\n", __func__);
            return error;
        }
        if(pdata->dev1_pm_gpio) {
            error = pdata->setup_dev1_pm_gpio(1);
            if (error) {
                pr_err("%s: Failed to setup GPIOs for attiny44a_dev1 pm pin\n", __func__);
                goto free_dev1_int;
            }
            attiny44a_dev1_pm_gpio = pdata->dev1_pm_gpio;
        }
    }

    if (pdata->dev2_int_gpio) {
        error = pdata->setup_dev2_int_gpio(1);
        if (error) {
            pr_err("%s: Failed to setup GPIOs for attiny44a_dev2 int pin\n", __func__);
            goto free_dev2_int;
        }
        if(pdata->dev2_pm_gpio) {
            error = pdata->setup_dev2_pm_gpio(1);
            if (error) {
                pr_err("%s: Failed to setup GPIOs for attiny44a_dev2 pm pin\n", __func__);
                goto free_dev2_pm;
            }
            attiny44a_dev2_pm_gpio = pdata->dev2_pm_gpio;
        }
    }
    /* end zhaoyang 2011/6/13 */
    poll_dev = input_allocate_polled_device();
    if (!poll_dev)
        return -ENOMEM;

    poll_dev->poll = ATtiny44A_poll;
    poll_dev->poll_interval = ATTINY44A_RATE;

    poll_dev->input->name = "attiny44a";
    poll_dev->input->id.bustype = BUS_HOST;
    poll_dev->input->dev.parent = &pdev->dev;

    poll_dev->private = pdata;
    set_bit(EV_ABS, poll_dev->input->evbit);
    set_bit(ABS_DISTANCE, poll_dev->input->mscbit);

    input_set_abs_params(poll_dev->input, ABS_DISTANCE, 0, MAX_PROX_VAL, INPUT_FUZZ, INPUT_FLAT);
    dev_set_drvdata(&pdev->dev, poll_dev);

    error = input_register_polled_device(poll_dev);
    if (error) {
        input_free_polled_device(poll_dev);
        return error;
    }

    return 0;

    /* zhaoy0012 : 2011/6/13 modify attiny44a driver to support dual device*/
free_dev2_pm:
    pdata->setup_dev2_int_gpio(0);
free_dev2_int:
    pdata->setup_dev1_pm_gpio(0);
free_dev1_int:
    pdata->setup_dev1_int_gpio(0);

    return error;
    /* end zhaoyang 2011/6/13 */
}

/* zhaoy0012 : 2011/6/13 modify attiny44a driver to support dual device*/
#ifdef CONFIG_PM
static int attiny44a_suspend(struct device *dev)
{
    int i;

    if(attiny44a_dev1_pm_gpio) {
        PM_ACTION(attiny44a_dev1_pm_gpio);
        gpio_set_value(attiny44a_dev1_pm_gpio, 0);
    }

    if(attiny44a_dev2_pm_gpio) {
        PM_ACTION(attiny44a_dev2_pm_gpio);
        gpio_set_value(attiny44a_dev2_pm_gpio, 0);
    }
    return 0;
}

static int attiny44a_resume(struct device *dev)
{
    int i;

    if(attiny44a_dev1_pm_gpio) {
        PM_ACTION(attiny44a_dev1_pm_gpio);
        gpio_set_value(attiny44a_dev1_pm_gpio, 1);
    }

    if(attiny44a_dev2_pm_gpio) {
        PM_ACTION(attiny44a_dev2_pm_gpio);
        gpio_set_value(attiny44a_dev2_pm_gpio, 1);
    }
    return 0;
}

static const struct dev_pm_ops attiny44a_pm_ops = {
    .suspend	= attiny44a_suspend,
    .resume		= attiny44a_resume,
};
#endif

#ifdef SYS4DEBUG
ssize_t attiny44a_store	(struct kobject *kobj,
                         struct kobj_attribute *attr,
                         const char *buf, size_t n)
{
    unsigned long val, i;
    int error;
    error = strict_strtoul(buf, 10, &val);
    if (error)
        return error;

    switch(val) {
        case 10:
            if(attiny44a_dev1_pm_gpio) {
                PM_ACTION(attiny44a_dev1_pm_gpio);
                gpio_set_value(attiny44a_dev1_pm_gpio, 0);
                printk("@@@@@@@@@@-----zhaoyang attiny44a_dev1_pm_gpio set 0-------------\n");
            }
            break;
        case 11:
            if(attiny44a_dev1_pm_gpio) {
                PM_ACTION(attiny44a_dev1_pm_gpio);
                gpio_set_value(attiny44a_dev1_pm_gpio, 1);
                printk("@@@@@@@@@@-----zhaoyang attiny44a_dev1_pm_gpio set 1-------------\n");
            }
            break;
        case 20:
            if(attiny44a_dev2_pm_gpio) {
                PM_ACTION(attiny44a_dev2_pm_gpio);
                gpio_set_value(attiny44a_dev2_pm_gpio, 0);
                printk("@@@@@@@@@@-----zhaoyang attiny44a_dev2_pm_gpio set 0-------------\n");
            }
            break;
        case 21:
            if(attiny44a_dev2_pm_gpio) {
                PM_ACTION(attiny44a_dev2_pm_gpio);
                gpio_set_value(attiny44a_dev2_pm_gpio, 1);
                printk("@@@@@@@@@@-----zhaoyang attiny44a_dev2_pm_gpio set 1-------------\n");
            }
            break;
        default:
            break;
    }

    return n;
}

EXPORT_SYMBOL(attiny44a_store);
#endif

static int __devexit ATtiny44A_remove(struct platform_device *pdev)
{
    struct input_polled_dev *poll_dev = dev_get_drvdata(&pdev->dev);

    input_unregister_polled_device(poll_dev);
    input_free_polled_device(poll_dev);
    dev_set_drvdata(&pdev->dev, NULL);

    return 0;
}

static struct platform_driver ATtiny44A_driver = {
    .probe = ATtiny44A_probe,
    .remove = __devexit_p(ATtiny44A_remove),
    .driver = {
        .name = DRV_NAME,
        .owner = THIS_MODULE,
        /* zhaoy0012 : 2011/6/13 modify attiny44a driver to support dual device*/
#ifdef CONFIG_PM
        .pm	= &attiny44a_pm_ops,
#endif
        /* end zhaoyang 2011/6/13 */
    },
};

static int __init ATtiny44A_init(void)
{
    return platform_driver_register(&ATtiny44A_driver);
}

static void __exit ATtiny44A_exit(void)
{
    platform_driver_unregister(&ATtiny44A_driver);
}

module_init(ATtiny44A_init);
module_exit(ATtiny44A_exit);

MODULE_AUTHOR("zhaoyang196673 <zhao.yang3@zte.com.cn>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Support for capacitance ATTINY44A prox device");
MODULE_ALIAS("platform:" DRV_NAME);
