/************************************************************************
*  版权所有 (C)2011,中兴通讯股份有限公司。
* 
* 文件名称： sim_card_detector.c
* 内容摘要： sim card检测驱动
*
************************************************************************/
/*============================================================

                        EDIT HISTORY 

when          comment tag      who              what, where, why                           
----------    ------------     -----------      --------------------------      
2011/06/28                     yuanbo           add for sim card detecting
============================================================*/
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <asm/io.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/mfd/pmic8058.h>
#include <linux/sim_card_detector.h>
#include <linux/pm_runtime.h>
#include <linux/input.h>
#include <linux/workqueue.h>

#ifdef CONFIG_SIM_CARD_DETECTOR_SYSFS_ZTE
#define SIM_CARD_DETECTOR_ATTR(_name) \
    static struct kobj_attribute _name = {\
        .attr = {\
            .name = __stringify(_name),\
            .mode = 0644,\
        },	\
        .show = _name##_show,\
        .store = _name##_store,\
    }
#endif

struct sim_card_detector_dev
{
    struct cdev cdev;
    struct device *dev ;
    unsigned int gpio;
#ifdef CONFIG_SIM_CARD_DETECTOR_IRQ_ZTE
    unsigned int irq;
    unsigned int irq_flags;
    struct pm8058_chip *chip;
    int card_exist_flag;
    struct input_dev *detector;
    struct delayed_work detect_work;
    spinlock_t lock;
#endif

#ifdef CONFIG_SIM_CARD_DETECTOR_SYSFS_ZTE
    struct kobject *sim_card_detector_kobj;
#endif
};

static unsigned int sim_card_detector_gpio;

#ifdef CONFIG_SIM_CARD_DETECTOR_SYSFS_ZTE
static ssize_t  sim_card_status_show(struct kobject *kobj,
                                      struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", gpio_get_value_cansleep(sim_card_detector_gpio));
}

static ssize_t  sim_card_status_store(struct kobject *kobj,
                            struct kobj_attribute *attr, const char *buf, size_t count)
{
    return count;
}

SIM_CARD_DETECTOR_ATTR(sim_card_status);
static struct attribute *sim_card_detector_att[] = {
    &sim_card_status.attr,
    NULL
};

static struct attribute_group sim_card_detector_gr = {
    .attrs = sim_card_detector_att
};
#endif

#ifdef CONFIG_SIM_CARD_DETECTOR_IRQ_ZTE
irqreturn_t sim_card_detector_interrupt(int irq, void *dev_id)
{
    struct platform_device *pdev = (struct platform_device *)dev_id;
    struct sim_card_detector_dev *p_sim_card_detector_dev  = platform_get_drvdata(pdev);
    unsigned long flags;

    spin_lock_irqsave(&p_sim_card_detector_dev->lock, flags);
    schedule_delayed_work(&p_sim_card_detector_dev->detect_work,msecs_to_jiffies(200));/*delay for 200ms*/
    spin_unlock_irqrestore(&p_sim_card_detector_dev->lock, flags);
    return (IRQ_HANDLED); 
}

static void detect_work_fn(struct work_struct *work)
{
    struct delayed_work *detect_work = to_delayed_work(work);
    struct sim_card_detector_dev *p_sim_card_detector_dev = \
        container_of(detect_work, struct sim_card_detector_dev, detect_work);
    struct device *dev = p_sim_card_detector_dev->dev;
    int r ;

    r = pm8058_irq_get_rt_status(p_sim_card_detector_dev->chip, p_sim_card_detector_dev->irq);
    if(r != p_sim_card_detector_dev->card_exist_flag ){

        p_sim_card_detector_dev->card_exist_flag = r;
        dev_info(dev,"gpio int = %d, status = %d\n", 
               p_sim_card_detector_dev->irq, r);

        input_report_key(p_sim_card_detector_dev->detector, KEY_SIM_CARD_DETECTOR, 1);
        input_sync(p_sim_card_detector_dev->detector);
        input_report_key(p_sim_card_detector_dev->detector, KEY_SIM_CARD_DETECTOR, 0);
        input_sync(p_sim_card_detector_dev->detector);

    }

}
#endif

#ifdef CONFIG_PM
static int sim_card_detector_suspend(struct device *dev)
{
	struct sim_card_detector_dev *p_sim_card_detector_dev  = dev_get_drvdata(dev);

	if (device_may_wakeup(dev)) {
		enable_irq_wake(p_sim_card_detector_dev->irq);
	}

	return 0;
}

static int sim_card_detector_resume(struct device *dev)
{
	struct sim_card_detector_dev *p_sim_card_detector_dev = dev_get_drvdata(dev);

	if (device_may_wakeup(dev)) {
		disable_irq_wake(p_sim_card_detector_dev->irq);
	}

	return 0;
}

static struct dev_pm_ops sim_card_detector_pm_ops = {
	.suspend	= sim_card_detector_suspend,
	.resume		= sim_card_detector_resume,
};
#endif

static int __devinit sim_card_detector_probe(struct platform_device *pdev)
{

    struct device *dev = &pdev->dev;
    struct sim_card_detector_platform_data *pdata = pdev->dev.platform_data;
    int r=0;
    struct sim_card_detector_dev *p_sim_card_detector_dev;
#ifdef CONFIG_SIM_CARD_DETECTOR_IRQ_ZTE
    struct input_dev *detector;
#endif

    dev_info(dev, "Probe\n");
    
    p_sim_card_detector_dev = kzalloc(sizeof(struct sim_card_detector_dev), GFP_KERNEL);
    if (p_sim_card_detector_dev == NULL) {
		dev_err(dev, "Cannot allocate memory.\n");
		r = -ENOMEM;
       goto no_mem;
	}

    sim_card_detector_gpio = pdata->gpio;

    p_sim_card_detector_dev->gpio = pdata->gpio; /*get gpio number*/
    p_sim_card_detector_dev->dev = dev;

#ifdef CONFIG_SIM_CARD_DETECTOR_IRQ_ZTE
    INIT_DELAYED_WORK(&(p_sim_card_detector_dev->detect_work), detect_work_fn);
    p_sim_card_detector_dev->irq = pdata->irq; /*get irq number*/
    p_sim_card_detector_dev->irq_flags = pdata->irq_flags;

    p_sim_card_detector_dev->chip = get_irq_data(p_sim_card_detector_dev->irq);
    r = pm8058_irq_get_rt_status(p_sim_card_detector_dev->chip, p_sim_card_detector_dev->irq);
    dev_info(dev,"irq  = %d\n", p_sim_card_detector_dev->irq);
    p_sim_card_detector_dev->card_exist_flag= r;

    detector = input_allocate_device();
    if (!detector) {
		dev_dbg(&pdev->dev, "Can't allocate sim card detector button\n");
		r = -ENOMEM;
		goto input_dev_alc_error;
	}

    input_set_capability(detector, EV_KEY, KEY_SIM_CARD_DETECTOR);

    detector->name = "sim_card_detect_key";
	detector->phys = "sim_card_detect_key/input0";
	detector->dev.parent = &pdev->dev;

    r = input_register_device(detector);
	if (r) {
		dev_dbg(&pdev->dev, "Can't register sim card detector key: %d\n", r);
		goto free_input_dev;
	}
    p_sim_card_detector_dev->detector = detector;

    spin_lock_init(&p_sim_card_detector_dev->lock);
#endif

    /* Enable runtime PM ops, start in ACTIVE mode */
	r = pm_runtime_set_active(&pdev->dev);
	if (r < 0)
		dev_dbg(&pdev->dev, "unable to set runtime pm state\n");
	pm_runtime_enable(&pdev->dev);

    dev_info(dev,"gpio = %d\n", p_sim_card_detector_dev->gpio);
    
#ifdef CONFIG_SIM_CARD_DETECTOR_IRQ_ZTE
    r = request_threaded_irq(p_sim_card_detector_dev->irq, NULL,
                             sim_card_detector_interrupt,
                             p_sim_card_detector_dev->irq_flags,
                             "SIM_CARD_DETECTOR",
                             pdev);     

    if (r ) {
        dev_err(dev,
                "%s request_threaded_irq failed for %d ret =%d\n",
                __func__, p_sim_card_detector_dev->irq, r);
        goto irq_request_error;
    }
    set_irq_wake(p_sim_card_detector_dev->irq, 1);
#endif

#ifdef CONFIG_SIM_CARD_DETECTOR_SYSFS_ZTE
    p_sim_card_detector_dev->sim_card_detector_kobj= \
                 kobject_create_and_add("sim_card_detector", &dev->kobj);

    if (! p_sim_card_detector_dev->sim_card_detector_kobj){
        dev_err(dev,
    	          "%s create kobject error\n",
                __func__);
        r = -ENOMEM;
        goto kobject_create_error;
    }
#endif
    
    platform_set_drvdata(pdev, p_sim_card_detector_dev);

#ifdef CONFIG_SIM_CARD_DETECTOR_SYSFS_ZTE
    r = sysfs_create_group(p_sim_card_detector_dev->sim_card_detector_kobj, &sim_card_detector_gr);
    if ( r ) {
        dev_err(dev, "sysfs create file failed!\n");
        goto sys_create_error;
    }
#endif

    device_init_wakeup(&pdev->dev, 1);
    return 0;

#ifdef CONFIG_SIM_CARD_DETECTOR_SYSFS_ZTE
sys_create_error:
    kobject_put(p_sim_card_detector_dev->sim_card_detector_kobj);
    
kobject_create_error:
#endif

#ifdef CONFIG_SIM_CARD_DETECTOR_IRQ_ZTE
irq_request_error:
    input_unregister_device(detector);

free_input_dev:
    input_free_device(detector);
    
input_dev_alc_error:
#endif
    kfree(p_sim_card_detector_dev);

no_mem:
    return r;
    
}

static __devexit int sim_card_detector_remove(struct platform_device *pdev)
{
    struct sim_card_detector_dev *p_sim_card_detector_dev  = platform_get_drvdata(pdev);
    struct device *dev = &pdev->dev;
    
    dev_info(dev,"Remove\n");
#ifdef CONFIG_SIM_CARD_DETECTOR_SYSFS_ZTE
    kobject_put(p_sim_card_detector_dev->sim_card_detector_kobj);
#endif

#ifdef CONFIG_SIM_CARD_DETECTOR_IRQ_ZTE
    free_irq(p_sim_card_detector_dev->irq, pdev);
    input_unregister_device(p_sim_card_detector_dev->detector);
    input_free_device(p_sim_card_detector_dev->detector);
#endif
    kfree(p_sim_card_detector_dev);
    return 0;
}

static struct platform_driver sim_card_detector_driver = {
    .probe	= sim_card_detector_probe,
    .remove	= __devexit_p(sim_card_detector_remove),
    .driver = {
        .name	= "sim_card_detector",
        .owner	= THIS_MODULE,
#ifdef CONFIG_PM
        .pm = &sim_card_detector_pm_ops,
#endif
    },
};

static int __init sim_card_detector_init(void)
{
	return platform_driver_register(&sim_card_detector_driver);
}

static void __exit sim_card_detector_cleanup(void)
{
    platform_driver_unregister(&sim_card_detector_driver);
}

late_initcall(sim_card_detector_init);
module_exit(sim_card_detector_cleanup);

MODULE_AUTHOR("yuan bo");
MODULE_DESCRIPTION("Sim card detector driver");
