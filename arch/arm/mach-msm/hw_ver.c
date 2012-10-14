/***********************************************************************
* Copyright (C) 2001, ZTE Corporation.
* 
* File Name:    hw_ver.c
* Description:  hw version process code
* Author:       liuzhongzhi
* Date:         2011-07-11
* 
**********************************************************************/
#include <linux/init.h>
#include <linux/platform_device.h>
#include <mach/hw_ver.h>

struct hw_ver_name
{
    int hw_ver;     /* hw version num */
    char *hw_name;  /* hw version name */
};

int hw_ver = -1;
EXPORT_SYMBOL(hw_ver);

static char *hw_version="noversion";

static struct hw_ver_name hw_ver_names[] = {
	/* V71A add here */
    {HW_VERSION_V71A_A, VERSION_STR_V71A_A}, 
    {HW_VERSION_V71A_B, VERSION_STR_V71A_B},
    {HW_VERSION_V71A_C, VERSION_STR_V71A_C},
    {HW_VERSION_V71A_D, VERSION_STR_V71A_D},
	/* V9X add here */
    {HW_VERSION_V9X_A,  VERSION_STR_V9X_A},
    {HW_VERSION_V9X_B,  VERSION_STR_V9X_B},
    {HW_VERSION_V9X_C,  VERSION_STR_V9X_C},
	/* V55 add here */
    {HW_VERSION_V55_A,  VERSION_STR_V55_A},
    {HW_VERSION_V55_B,  VERSION_STR_V55_B},
    {HW_VERSION_V55_C,  VERSION_STR_V55_C},
	/* V66 add here */
    {HW_VERSION_V66_A,  VERSION_STR_V66_A},
    {HW_VERSION_V66_B,  VERSION_STR_V66_B},
    {HW_VERSION_V66_C,  VERSION_STR_V66_C}, 
	{HW_VERSION_V66_D,  VERSION_STR_V66_D},
    {HW_VERSION_V66_E,  VERSION_STR_V66_E},
    {HW_VERSION_V66_F,  VERSION_STR_V66_F}, 
	{HW_VERSION_V66_G,  VERSION_STR_V66_G},
    {HW_VERSION_V66_H,  VERSION_STR_V66_H},
    {HW_VERSION_V66_I,  VERSION_STR_V66_I},
	/* V68 add here */
    {HW_VERSION_V68_A,  VERSION_STR_V68_A},
    {HW_VERSION_V68_B,  VERSION_STR_V68_B},
    {HW_VERSION_V68_C,  VERSION_STR_V68_C}, 
	{HW_VERSION_V68_D,  VERSION_STR_V68_D},
    {HW_VERSION_V68_E,  VERSION_STR_V68_E},
    {HW_VERSION_V68_F,  VERSION_STR_V68_F}, 
	{HW_VERSION_V68_G,  VERSION_STR_V68_G},
    {HW_VERSION_V68_H,  VERSION_STR_V68_H},
    {HW_VERSION_V68_I,  VERSION_STR_V68_I},
	/* V11A add here */
    {HW_VERSION_V11A_A, VERSION_STR_V11A_A},
    {HW_VERSION_V11A_B, VERSION_STR_V11A_B},
    {HW_VERSION_V11A_C, VERSION_STR_V11A_C},
	/* V11 add here */
    {HW_VERSION_V11_A,  VERSION_STR_V11_A},
    {HW_VERSION_V11_B,  VERSION_STR_V11_B},
};

/*
 * Get the hw_ver value
 */
static int __init hw_ver_setup(char *str)
{
    char *after;

    printk(KERN_NOTICE "%s @str:%s\n", __func__, str);
    
    hw_ver = simple_strtoul(str, &after, 16);
    
	return 1;
}
__setup("hw_ver=", hw_ver_setup);

static int __init get_hw_version(void)
{
	int i = 0;

    /* find the hw version string according to hw_ver value */
	for (i = 0; i < sizeof(hw_ver_names)/sizeof(hw_ver_names[0]); i++) {
		if(hw_ver_names[i].hw_ver == hw_ver) {
            hw_version = hw_ver_names[i].hw_name;
            break;
		}
	}
    
	printk(KERN_NOTICE "@hw_version: %s\n",hw_version);
    
	return 0;
}


/*
 * show_hw_version() - Show public hw version to user space
 */
static ssize_t show_hw_version(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	return sprintf(buf, "%s\n", hw_version);
}

static struct device_attribute hw_version_attr =
	__ATTR(hw_version, S_IRUGO | S_IWUSR, show_hw_version, NULL);

static int __devinit hw_ver_probe(struct platform_device *pdev)
{
    int ret;   

	get_hw_version();

    /* create attr file */
    ret = device_create_file(&pdev->dev, &hw_version_attr);
	if (ret) {
		dev_err(&pdev->dev, "failed: create hw_version file\n");
	}
    return 0;
}

static __devexit int hw_ver_remove(struct platform_device *pdev)
{
    device_remove_file(&pdev->dev, &hw_version_attr);	

	return 0;
}

static struct platform_driver hw_ver_driver = {
	.probe	= hw_ver_probe,
	.remove	= __devexit_p(hw_ver_remove),
	.driver = {
		.name	= "hw_version",
		.owner	= THIS_MODULE,
	},
};

static struct platform_device hw_ver_device = {
	.name = "hw_version",
	.id = -1,
};

static int __init hw_ver_init(void)
{
    int ret;

    /* register device */
    ret = platform_device_register(&hw_ver_device); 
    if (ret){
        printk(KERN_NOTICE "Unable to register hw version Device!\n");
        goto out;
    }

    /* register driver */
    ret = platform_driver_register(&hw_ver_driver);
out:
	return ret;
}


static void __exit hw_ver_exit(void)
{
    platform_device_unregister(&hw_ver_device);
	platform_driver_unregister(&hw_ver_driver);
}

module_init(hw_ver_init);
module_exit(hw_ver_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("HW Version Driver");
