/*
 *  Copyright (C) 2009-2011 ZTE Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
 /*===========================================================================
                         EDIT HISTORY FOR V11
   when        comment tag         who                  what, where, why
 ----------    ------------     -----------      --------------------------
 2011/07/04    zhaoy0022         zhaoyang        when the driver loads successfully output SUCESS information to meet production test requirements
 ===========================================================================*/
// includes
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/timer.h>
#include <asm/uaccess.h>
#include <asm/errno.h>
#include <asm/delay.h>
#include <linux/taos_common.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/input-polldev.h>

#define TAOS_DEVICE_NAME		"taos"
#define TAOS_DEVICE_ID			"tritonFN"
#define TAOS_ID_NAME_SIZE		    10
#define TAOS_TRITON_CHIPIDVAL   	0x00
#define TAOS_TRITON_MAXREGS     	32
#define TAOS_DEVICE_ADDR1		    0x29
#define TAOS_DEVICE_ADDR2       	0x39
#define TAOS_DEVICE_ADDR3       	0x49
#define TAOS_MAX_NUM_DEVICES		3
#define TAOS_MAX_DEVICE_REGS		32
#define I2C_MAX_ADAPTERS		    8

// TRITON register offsets
#define TAOS_TRITON_CNTRL 		    0x00
#define TAOS_TRITON_ALS_TIME 		0X01
#define TAOS_TRITON_PRX_TIME		0x02
#define TAOS_TRITON_WAIT_TIME		0x03
#define TAOS_TRITON_ALS_MINTHRESHLO	0X04
#define TAOS_TRITON_ALS_MINTHRESHHI 0X05
#define TAOS_TRITON_ALS_MAXTHRESHLO	0X06
#define TAOS_TRITON_ALS_MAXTHRESHHI	0X07
#define TAOS_TRITON_PRX_MINTHRESHLO 0X08
#define TAOS_TRITON_PRX_MINTHRESHHI 0X09
#define TAOS_TRITON_PRX_MAXTHRESHLO 0X0A
#define TAOS_TRITON_PRX_MAXTHRESHHI 0X0B
#define TAOS_TRITON_INTERRUPT		0x0C
#define TAOS_TRITON_PRX_CFG		    0x0D
#define TAOS_TRITON_PRX_COUNT		0x0E
#define TAOS_TRITON_GAIN		    0x0F
#define TAOS_TRITON_REVID		    0x11
#define TAOS_TRITON_CHIPID      	0x12
#define TAOS_TRITON_STATUS		    0x13
#define TAOS_TRITON_ALS_CHAN0LO		0x14
#define TAOS_TRITON_ALS_CHAN0HI		0x15
#define TAOS_TRITON_ALS_CHAN1LO		0x16
#define TAOS_TRITON_ALS_CHAN1HI		0x17
#define TAOS_TRITON_PRX_LO		    0x18
#define TAOS_TRITON_PRX_HI		    0x19
#define TAOS_TRITON_TEST_STATUS		0x1F

// Triton cmd reg masks
#define TAOS_TRITON_CMD_REG		    0X80
#define TAOS_TRITON_CMD_BYTE_RW		0x00
#define TAOS_TRITON_CMD_WORD_BLK_RW	0x20
#define TAOS_TRITON_CMD_SPL_FN		0x60
#define TAOS_TRITON_CMD_PROX_INTCLR	0X05
#define TAOS_TRITON_CMD_ALS_INTCLR	0X06
#define TAOS_TRITON_CMD_PROXALS_INTCLR 	0X07
#define TAOS_TRITON_CMD_TST_REG		0X08
#define TAOS_TRITON_CMD_USER_REG	0X09

// Triton cntrl reg masks
#define TAOS_TRITON_CNTL_PROX_INT_ENBL	0X20
#define TAOS_TRITON_CNTL_ALS_INT_ENBL	0X10
#define TAOS_TRITON_CNTL_WAIT_TMR_ENBL	0X08
#define TAOS_TRITON_CNTL_PROX_DET_ENBL	0X04
#define TAOS_TRITON_CNTL_ADC_ENBL	    0x02
#define TAOS_TRITON_CNTL_PWRON		    0x01

// Triton status reg masks
#define TAOS_TRITON_STATUS_ADCVALID	0x01
#define TAOS_TRITON_STATUS_PRXVALID	0x02
#define TAOS_TRITON_STATUS_ADCINTR	0x10
#define TAOS_TRITON_STATUS_PRXINTR	0x20

// lux constants
#define	TAOS_MAX_LUX			65535000
#define TAOS_SCALE_MILLILUX		3
#define TAOS_FILTER_DEPTH		3
/*
 * Defines
 */
// device configuration
static struct taos_cfg *taos_cfgp;

#define assert(expr)\
    if(!(expr)) {\
        printk( "Assertion failed! %s,%d,%s,%s\n",\
                __FILE__,__LINE__,__func__,#expr);\
    }

#define TMD26713_DRV_NAME	"tmd26713"
#define TMD26713_I2C_ADDR	0x39

#define TMD26713_ID		0x29
#define TMD26711_ID		0x20
#define MAX_PROX_VAL    65535

static struct device *hwmon_dev;
static struct i2c_client *tmd26713_i2c_client;

/*
 * Initialization function
 */

static struct input_polled_dev *tmd26713_idev;
#define POLL_INTERVAL		100
#define INPUT_FUZZ	32
#define INPUT_FLAT	32
static int report_prox_val(void)
{
    u16 prox_val, i, ret;
    u8 chdata[2];

    for (i = 0; i < 2; i++) {
        if ((ret = (i2c_smbus_write_byte(tmd26713_i2c_client, (TAOS_TRITON_CMD_REG | (TAOS_TRITON_PRX_LO + i))))) < 0) {
            printk(KERN_ERR "TAOS: i2c_smbus_write_byte() to als/prox data reg failed in report_prox_val()\n");
            return (ret);
        }
        chdata[i] = i2c_smbus_read_byte(tmd26713_i2c_client);
    }
    prox_val = chdata[1];
    prox_val <<= 8;
    prox_val|= chdata[0];

    input_report_abs(tmd26713_idev->input, ABS_DISTANCE , prox_val);
    input_sync(tmd26713_idev->input);

    return 0;
}

static void tmd26713_dev_poll(struct input_polled_dev *dev)
{
    report_prox_val();
}

static int tmd26713_chip_off(void)
{
    int ret = 0;
    if ((ret = (i2c_smbus_write_byte_data(tmd26713_i2c_client, (TAOS_TRITON_CMD_REG|0x00), 0x00))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in tmd26713_chip_off\n");
        return (ret);
    }
    return (ret);
}

static int tmd26713_chip_on(void)
{
    int ret = 0;
    if ((ret = (i2c_smbus_write_byte_data(tmd26713_i2c_client, (TAOS_TRITON_CMD_REG|0x00), 0x00))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in tmd26713_chip_on\n");
        return (ret);
    }
    if ((ret = (i2c_smbus_write_byte_data(tmd26713_i2c_client, (TAOS_TRITON_CMD_REG|0x01), taos_cfgp->prox_int_time))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in tmd26713_chip_on\n");
        return (ret);
    }
    if ((ret = (i2c_smbus_write_byte_data(tmd26713_i2c_client, (TAOS_TRITON_CMD_REG|0x02), taos_cfgp->prox_adc_time))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in tmd26713_chip_on\n");
        return (ret);
    }
    if ((ret = (i2c_smbus_write_byte_data(tmd26713_i2c_client, (TAOS_TRITON_CMD_REG|0x03), taos_cfgp->prox_wait_time))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in tmd26713_chip_on\n");
        return (ret);
    }
    if ((ret = (i2c_smbus_write_byte_data(tmd26713_i2c_client, (TAOS_TRITON_CMD_REG|0x0C), taos_cfgp->prox_intr_filter))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in tmd26713_chip_on\n");
        return (ret);
    }
    if ((ret = (i2c_smbus_write_byte_data(tmd26713_i2c_client, (TAOS_TRITON_CMD_REG|0x0D), taos_cfgp->prox_config))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in tmd26713_chip_on\n");
        return (ret);
    }
    if ((ret = (i2c_smbus_write_byte_data(tmd26713_i2c_client, (TAOS_TRITON_CMD_REG|0x0E), taos_cfgp->prox_pulse_cnt))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in tmd26713_chip_on\n");
        return (ret);
    }
    if ((ret = (i2c_smbus_write_byte_data(tmd26713_i2c_client, (TAOS_TRITON_CMD_REG|0x0F), taos_cfgp->prox_gain))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in tmd26713_chip_on\n");
        return (ret);
    }
    if ((ret = (i2c_smbus_write_byte_data(tmd26713_i2c_client, (TAOS_TRITON_CMD_REG|0x00), 0x0F))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in tmd26713_chip_on\n");
        return (ret);
    }
    return (ret);
}
/*
 * I2C init/probing/exit functions
 */

extern struct input_polled_dev *input_allocate_polled_device(void);

static int __devinit tmd26713_probe(struct i2c_client *client,
                                    const struct i2c_device_id *id)
{
    int result;
    struct i2c_adapter *adapter ;
    struct input_dev *idev;

    tmd26713_i2c_client = client;
    adapter = to_i2c_adapter(client->dev.parent);

    result = i2c_check_functionality(adapter,
                                     I2C_FUNC_SMBUS_BYTE|I2C_FUNC_SMBUS_BYTE_DATA);
    assert(result);

    printk(KERN_INFO "check tmd26713 chip ID\n");
    result = i2c_smbus_read_byte_data(client, (TAOS_TRITON_CMD_REG |TAOS_TRITON_CHIPID));
    if (TMD26713_ID != (result)) {  //compare the address value
        dev_err(&client->dev,"read chip ID 0x%x is not equal to TMD26713_ID 0x%x!\n", result,TMD26713_ID);
        printk(KERN_INFO "read chip ID failed\n");
        return (-ENODEV);
    } else {
        printk(KERN_INFO "@@@@@@@@@zhaoyang196673 read TMD26713 chip ID OK!\n");
    }

    if (!(taos_cfgp = kmalloc(sizeof(struct taos_cfg), GFP_KERNEL))) {
        printk(KERN_ERR "TAOS: kmalloc for struct taos_cfg failed in taos_probe()\n");
        return -ENOMEM;
    }
    taos_cfgp->calibrate_target = 300000;
    taos_cfgp->als_time = 100;
    taos_cfgp->scale_factor = 1;
    taos_cfgp->gain_trim = 512;
    taos_cfgp->filter_history = 3;
    taos_cfgp->filter_count = 1;
    taos_cfgp->gain = 1;
    taos_cfgp->prox_threshold_hi = 12000;
    taos_cfgp->prox_threshold_lo = 10000;
    taos_cfgp->prox_int_time = 0xFC;
    taos_cfgp->prox_adc_time = 0xFF;
    taos_cfgp->prox_wait_time = 0xF2;
    taos_cfgp->prox_intr_filter = 0x00;
    taos_cfgp->prox_config = 0x00;
    taos_cfgp->prox_pulse_cnt = 0x08;
    taos_cfgp->prox_gain = 0x20;

    /* Initialize & open the tmd26713 chip */
    result = tmd26713_chip_on();
    assert(result==0);

    hwmon_dev = hwmon_device_register(&client->dev);
    assert(!(IS_ERR(hwmon_dev)));

    dev_info(&client->dev, "build time %s %s\n", __DATE__, __TIME__);

    /*input poll device register */
    tmd26713_idev = input_allocate_polled_device();
    if (!tmd26713_idev) {
        dev_err(&client->dev, "alloc poll device failed!\n");
        result = -ENOMEM;
        return result;
    }
    tmd26713_idev->poll = tmd26713_dev_poll;
    tmd26713_idev->poll_interval = POLL_INTERVAL;
    idev = tmd26713_idev->input;
    idev->name = TMD26713_DRV_NAME;
    idev->id.bustype = BUS_I2C;
    idev->dev.parent = &client->dev;
    set_bit(EV_ABS, idev->evbit);
    set_bit(ABS_DISTANCE, idev->mscbit);

    input_set_abs_params(idev, ABS_DISTANCE, 0, MAX_PROX_VAL, INPUT_FUZZ, INPUT_FLAT);

    result = input_register_polled_device(tmd26713_idev);
    if (result) {
        dev_err(&client->dev, "register poll device failed!\n");
        return result;
    }

    printk("proximity detector detect SUCCESS\n");

    return result;
}

static int __devexit tmd26713_remove(struct i2c_client *client)
{
    int result;

    result = tmd26713_chip_off();
    assert(result==0);

    hwmon_device_unregister(hwmon_dev);
    return result;
}


static const struct i2c_device_id tmd26713_id[] = {
    { TMD26713_DRV_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, tmd26713_id);

//zhaoyang196673 add for TMD26713 proximity begin
static int tmd26713_suspend(struct i2c_client *client, pm_message_t mesg)
{
    return tmd26713_chip_off();
}

static int tmd26713_resume(struct i2c_client *client)
{
    return tmd26713_chip_on();
}
//zhaoyang196673 add for TMD26713 proximity end

static struct i2c_driver tmd26713_driver = {
    .driver = {
        .name	= TMD26713_DRV_NAME,
        .owner	= THIS_MODULE,
    },
    .probe	= tmd26713_probe,
    .remove	= __devexit_p(tmd26713_remove),
    .suspend = tmd26713_suspend,
    .resume = tmd26713_resume,
    .id_table = tmd26713_id,
};

static int __init tmd26713_init(void)
{
    /* register driver */
    int res;

    res = i2c_add_driver(&tmd26713_driver);
    if (res < 0) {
        printk(KERN_INFO "add tmd26713 i2c driver failed\n");
        return -ENODEV;
    }
    printk(KERN_INFO "add tmd26713 i2c driver\n");

    return (res);
}

static void __exit tmd26713_exit(void)
{
    printk(KERN_INFO "remove tmd26713 i2c driver.\n");
    i2c_del_driver(&tmd26713_driver);
}

MODULE_AUTHOR("Zhao Yang <zhao.yang3@zte.com.cn>");
MODULE_DESCRIPTION("TMD26713 digital proximity detector sensor driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.1");

module_init(tmd26713_init);
module_exit(tmd26713_exit);
