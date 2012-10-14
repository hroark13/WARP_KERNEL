/*******************************************************************************
*                                                                              *
*   File Name:    taos.c                                                      *
*   Description:   Linux device driver for Taos ambient light and         *
*   proximity sensors.                                     *
*   Author:         John Koshi                                             *
*   History:   09/16/2009 - Initial creation                          *
*           10/09/2009 - Triton version         *
*           12/21/2009 - Probe/remove mode                *
*           02/07/2010 - Add proximity          *
*                                                                              *
********************************************************************************
*    Proprietary to Taos Inc., 1001 Klein Road #300, Plano, TX 75074        *
*******************************************************************************/
/*===========================================================================
                        EDIT HISTORY FOR V11
  when        comment tag         who                  what, where, why
----------    ------------     -----------      --------------------------
2011/07/04    zhaoy0022         zhaoyang        when the driver loads successfully output SUCESS information to meet production test requirements
2011/09/02    zhaoy0032         zhaoyang        modify the parameters of the ambient light sensor gain
===========================================================================*/
// includes 
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/timer.h>
#include <asm/uaccess.h>
#include <asm/errno.h>
#include <asm/delay.h>
#include <linux/taos_common.h>
#include <linux/delay.h>
//iVIZM
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <mach/gpio.h>
#include <linux/poll.h>
#include <linux/wakelock.h>
#include <linux/input.h>

#include <linux/init.h>
#include <linux/platform_device.h>
#if defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
 
/* Early-suspend level */
#define ALS_SUSPEND_LEVEL 1
#endif

#define DEVICE_ID_REG                   0x12
#define DEVICE_27713_ID                 0x29
#define DEVICE_27711_ID                 0x20
/*驱动单个验证环境光和接近度时分别打开对应的宏即可，注意不能同时验证，正常工作时关闭*/
/*按照上层麻小强的要求打开环境光功能，不使能接近度功能*/
#define TMD27713_ALS_TEST
#define TABLET7_TYPE_1 					0x7A
#define TABLET7_TYPE_2			 		0x7B
#define TABLET10_TYPE_1		 			0xAA
#define TABLET10_TYPE_2                 0xAB

#define NUMERATOR_7A  	 				14
#define DENOMINATOR_7A	 				15
#define NUMERATOR_7B  	 				4
#define DENOMINATOR_7B	 				7
#define NUMERATOR_AA  	 				2
#define DENOMINATOR_AA	 				1
#define NUMERATOR_AB  	 				7
#define DENOMINATOR_AB	 				5

#define OFFSET_7A						1
#define OFFSET_7B 						1
#define OFFSET_AA 						1
#define OFFSET_AB 						1

static int numerator;
static int denominator;
static int offset;

#define LUX_ATTENUATION  1

//#define TMD27713_PROX_TEST
///end

// device name/id/address/counts
#define TAOS_DEVICE_ID                  "tmd27713"

#define TAOS_DEVICE_NAME                "skateFN"
#define TAOS_ID_NAME_SIZE               10
#define TAOS_TRITON_CHIPIDVAL           0x00
#define TAOS_TRITON_MAXREGS             32
#define TAOS_DEVICE_ADDR1               0x29
#define TAOS_DEVICE_ADDR2               0x39
#define TAOS_DEVICE_ADDR3               0x49
#define TAOS_MAX_NUM_DEVICES            3
#define TAOS_MAX_DEVICE_REGS            32
#define I2C_MAX_ADAPTERS                8

// TRITON register offsets
#define TAOS_TRITON_CNTRL               0x00
#define TAOS_TRITON_ALS_TIME            0X01
#define TAOS_TRITON_PRX_TIME            0x02
#define TAOS_TRITON_WAIT_TIME           0x03
#define TAOS_TRITON_ALS_MINTHRESHLO     0X04
#define TAOS_TRITON_ALS_MINTHRESHHI     0X05
#define TAOS_TRITON_ALS_MAXTHRESHLO     0X06
#define TAOS_TRITON_ALS_MAXTHRESHHI     0X07
#define TAOS_TRITON_PRX_MINTHRESHLO     0X08
#define TAOS_TRITON_PRX_MINTHRESHHI     0X09
#define TAOS_TRITON_PRX_MAXTHRESHLO     0X0A
#define TAOS_TRITON_PRX_MAXTHRESHHI     0X0B
#define TAOS_TRITON_INTERRUPT           0x0C
#define TAOS_TRITON_PRX_CFG             0x0D
#define TAOS_TRITON_PRX_COUNT           0x0E
#define TAOS_TRITON_GAIN                0x0F
#define TAOS_TRITON_REVID               0x11
#define TAOS_TRITON_CHIPID              0x12
#define TAOS_TRITON_STATUS              0x13
#define TAOS_TRITON_ALS_CHAN0LO         0x14
#define TAOS_TRITON_ALS_CHAN0HI         0x15
#define TAOS_TRITON_ALS_CHAN1LO         0x16
#define TAOS_TRITON_ALS_CHAN1HI         0x17
#define TAOS_TRITON_PRX_LO              0x18
#define TAOS_TRITON_PRX_HI              0x19
#define TAOS_TRITON_TEST_STATUS         0x1F

// Triton cmd reg masks
#define TAOS_TRITON_CMD_REG             0X80
#define TAOS_TRITON_CMD_AUTO            0x10 //iVIZM
#define TAOS_TRITON_CMD_BYTE_RW         0x00
#define TAOS_TRITON_CMD_WORD_BLK_RW     0x20
#define TAOS_TRITON_CMD_SPL_FN          0x60
#define TAOS_TRITON_CMD_PROX_INTCLR     0X05
#define TAOS_TRITON_CMD_ALS_INTCLR      0X06
#define TAOS_TRITON_CMD_PROXALS_INTCLR  0X07
#define TAOS_TRITON_CMD_TST_REG         0X08
#define TAOS_TRITON_CMD_USER_REG        0X09

// Triton cntrl reg masks
#define TAOS_TRITON_CNTL_PROX_INT_ENBL  0X20
#define TAOS_TRITON_CNTL_ALS_INT_ENBL   0X10
#define TAOS_TRITON_CNTL_WAIT_TMR_ENBL  0X08
#define TAOS_TRITON_CNTL_PROX_DET_ENBL  0X04
#define TAOS_TRITON_CNTL_ADC_ENBL       0x02
#define TAOS_TRITON_CNTL_PWRON          0x01

// Triton status reg masks
#define TAOS_TRITON_STATUS_ADCVALID     0x01
#define TAOS_TRITON_STATUS_PRXVALID     0x02
#define TAOS_TRITON_STATUS_ADCINTR      0x10
#define TAOS_TRITON_STATUS_PRXINTR      0x20

// lux constants
#define TAOS_MAX_LUX                    10000
#define TAOS_SCALE_MILLILUX             3
#define TAOS_FILTER_DEPTH               3
#define CHIP_ID                         0x3d

// forward declarations
static int taos_probe(struct i2c_client *clientp, const struct i2c_device_id *idp);
static int taos_remove(struct i2c_client *client);
static int taos_open(struct inode *inode, struct file *file);
static int taos_release(struct inode *inode, struct file *file);
static long taos_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static int taos_read(struct file *file, char *buf, size_t count, loff_t *ppos);
static int taos_write(struct file *file, const char *buf, size_t count, loff_t *ppos);
static loff_t taos_llseek(struct file *file, loff_t offset, int orig);
static int taos_get_lux(void);
static int taos_prox_poll(struct taos_prox_info *prxp);
static void taos_prox_poll_timer_func(unsigned long param);
static void taos_prox_poll_timer_start(void);
//iVIZM
static int taos_als_threshold_set(void);
static int taos_prox_threshold_set(void);
static int taos_als_get_data(void);
static int taos_interrupts_clear(void);
static int taos_sensors_als_on(void);
static int taos_sensors_prox_on(void);

DECLARE_WAIT_QUEUE_HEAD(waitqueue_read);//iVIZM

#define ALS_PROX_DEBUG //iVIZM
static unsigned int ReadEnable = 0;//iVIZM
struct ReadData { //iVIZM
    unsigned int data;
    unsigned int interrupt;
};
struct ReadData readdata[2];//iVIZM

// first device number
static dev_t taos_dev_number;

// class structure for this device
struct class *taos_class;

// module device table
static struct i2c_device_id taos_idtable[] = {
    {TAOS_DEVICE_ID, 0},
    {}
};
MODULE_DEVICE_TABLE(i2c, taos_idtable);

// board and address info   iVIZM
struct i2c_board_info taos_board_info[] = {
    {I2C_BOARD_INFO(TAOS_DEVICE_ID, TAOS_DEVICE_ADDR2),},
};

unsigned short const taos_addr_list[2] = {TAOS_DEVICE_ADDR2, I2C_CLIENT_END};//iVIZM

// client and device
struct i2c_client *my_clientp;
struct i2c_client *bad_clientp[TAOS_MAX_NUM_DEVICES];
//static int num_bad = 0;
static int device_found = 0;
//iVIZM
static char pro_buf[4]; //iVIZM
static int mcount = 0; //iVIZM
static char als_buf[4]; //iVIZM
u16 status = 0;
static int ALS_ON;

// per-device data
struct taos_data {
    struct i2c_client *client;
    struct cdev cdev;
    unsigned int addr;
    struct input_dev *input_dev;//iVIZM
    struct work_struct work;//iVIZM
    struct wake_lock taos_wake_lock;//iVIZM
    char taos_id;
    char taos_name[TAOS_ID_NAME_SIZE];
    struct semaphore update_lock;
    char valid;
    unsigned long last_updated;
#if defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend		early_suspend;
#endif
} *taos_datap;

// file operations
static struct file_operations taos_fops = {
    .owner = THIS_MODULE,
    .open = taos_open,
    .release = taos_release,
    .read = taos_read,
    .write = taos_write,
    .llseek = taos_llseek,
    .unlocked_ioctl = taos_ioctl,
};

// device configuration
static struct taos_cfg *taos_cfgp;
static u32 calibrate_target_param = 300000;
static u16 als_time_param = 200;
static u16 scale_factor_param = 1;
static u16 gain_trim_param = 512;
static u8 filter_history_param = 3;
static u8 filter_count_param = 1;
static u16 prox_threshold_hi_param = 120;
static u16 prox_threshold_lo_param = 80;
static u16 als_threshold_hi_param = 3000;//iVIZM
static u16 als_threshold_lo_param = 10;//iVIZM
static u8 prox_int_time_param = 0xEE;//50ms
static u8 prox_adc_time_param = 0xFF;
static u8 prox_wait_time_param = 0xEE;
static u8 prox_intr_filter_param = 0x23;
static u8 prox_config_param = 0x00;
static u8 prox_pulse_cnt_param = 0x08;
static u8 prox_gain_param = 0x62;

// prox info
struct taos_prox_info prox_cal_info[20];
struct taos_prox_info prox_cur_info;
struct taos_prox_info *prox_cur_infop = &prox_cur_info;
static u8 prox_history_hi = 0;
static u8 prox_history_lo = 0;
static struct timer_list prox_poll_timer;
static int prox_on = 0;
static int device_released = 0;
static u16 sat_als = 0;
static u16 sat_prox = 0;

// device reg init values
u8 taos_triton_reg_init[16] = {0x00,0xFF,0XFF,0XFF,0X00,0X00,0XFF,0XFF,0X00,0X00,0XFF,0XFF,0X00,0X00,0X00,0X00};

// lux time scale
struct time_scale_factor  {
    u16 numerator;
    u16 denominator;
    u16 saturation;
};
struct time_scale_factor TritonTime = {1, 0, 0};
struct time_scale_factor *lux_timep = &TritonTime;

// gain table
//u8 taos_triton_gain_table[] = {1, 8, 16, 120};
u8 taos_triton_gain_table[] = {1, 1, 16, 1};
// lux data

struct lux_data {
    u16 ratio;
    u16 clear;
    u16 ir;
};
struct lux_data TritonFN_lux_data[] = {
    { 9830,  8320,  15360 },
    { 12452, 10554, 22797 },
    { 14746, 6234,  11430 },
    { 17695, 3968,  6400  },
    { 0,     0,     0     }
};
struct lux_data *lux_tablep = TritonFN_lux_data;
static int lux_history[TAOS_FILTER_DEPTH] = {-ENODATA, -ENODATA, -ENODATA};//iVIZM

static irqreturn_t taos_irq_handler(int irq, void *dev_id) //iVIZM
{
    schedule_work(&taos_datap->work);
    return IRQ_HANDLED;
}

static int taos_get_data(void)//iVIZM
{
    int ret = 0;

    if ((ret = (i2c_smbus_write_byte(taos_datap->client, (TAOS_TRITON_CMD_REG | 0x13)))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte(1) failed in taos_work_func()\n");
        return (ret);
    }
    status = i2c_smbus_read_byte(taos_datap->client);
    if((status & 0x20) == 0x20) {
        ret = taos_prox_threshold_set();
        if(ret >= 0)
            ReadEnable = 1;
    } else if((status & 0x10) == 0x10) {
        ReadEnable = 1;
        taos_als_threshold_set();
        taos_als_get_data();
    }
    return ret;
}

static int taos_interrupts_clear(void)//iVIZM
{
    int ret = 0;

    if ((ret = (i2c_smbus_write_byte(taos_datap->client, (TAOS_TRITON_CMD_REG|TAOS_TRITON_CMD_SPL_FN|0x07)))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte(2) failed in taos_work_func()\n");
        return (ret);
    }


    return ret;
}
static void taos_work_func(struct work_struct * work) //iVIZM
{
    wake_lock(&taos_datap->taos_wake_lock);

    taos_get_data();
    taos_interrupts_clear();

    wake_unlock(&taos_datap->taos_wake_lock);
}
static int taos_als_get_data(void)//iVIZM
{
    int ret = 0;
    u8 reg_val;
    int lux_val = 0;

    if ((ret = (i2c_smbus_write_byte(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_CNTRL)))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte failed in ioctl als_data\n");
        return (ret);
    }

    reg_val = i2c_smbus_read_byte(taos_datap->client);
    if ((reg_val & (TAOS_TRITON_CNTL_ADC_ENBL | TAOS_TRITON_CNTL_PWRON)) != (TAOS_TRITON_CNTL_ADC_ENBL | TAOS_TRITON_CNTL_PWRON)) {
        return -ENODATA;
    }
    if ((ret = (i2c_smbus_write_byte(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_STATUS)))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte failed in ioctl als_data\n");
        return (ret);
    }
    reg_val = i2c_smbus_read_byte(taos_datap->client);

    if ((reg_val & TAOS_TRITON_STATUS_ADCVALID) != TAOS_TRITON_STATUS_ADCVALID)
        return -ENODATA;

    if ((lux_val = taos_get_lux()) < 0)
        printk(KERN_ERR "TAOS: call to taos_get_lux() returned error %d in ioctl als_data\n", lux_val);
    lux_val = (lux_val);
//   printk("********** before taos_als_get_data ********* lux_val = %d \n",lux_val);
    input_report_abs(taos_datap->input_dev,ABS_MISC,lux_val);
    input_sync(taos_datap->input_dev);
//   printk("********** taos_als_get_data ********* lux_val = %d \n",lux_val);
    return ret;
}

static int taos_als_threshold_set(void)//iVIZM
{
    int i,ret = 0;
    u8 chdata[2];
    u16 ch0;

    for (i = 0; i < 2; i++) {
        chdata[i] = (i2c_smbus_read_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_CMD_WORD_BLK_RW | (TAOS_TRITON_ALS_CHAN0LO + i))));
    }
    ch0 = chdata[0] + chdata[1]*256;
    als_threshold_hi_param = (12*ch0)/10;
    if (als_threshold_hi_param >= 65535)
        als_threshold_hi_param = 65535;
    als_threshold_lo_param = (8*ch0)/10;
    als_buf[0] = als_threshold_lo_param & 0x0ff;
    als_buf[1] = als_threshold_lo_param >> 8;
    als_buf[2] = als_threshold_hi_param & 0x0ff;
    als_buf[3] = als_threshold_hi_param >> 8;

    for( mcount=0; mcount<4; mcount++ ) {
        if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x04) + mcount, als_buf[mcount]))) < 0) {
            printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in taos als threshold set\n");
            return (ret);
        }
    }

    return ret;
}

static int taos_prox_threshold_set(void)//iVIZM
{
    int i,ret = 0;
    u8 chdata[6];
    u16 proxdata = 0;
    u16 cleardata = 0;
    int data = 0;

    for (i = 0; i < 6; i++) {
        chdata[i] = (i2c_smbus_read_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_CMD_WORD_BLK_RW| (TAOS_TRITON_ALS_CHAN0LO + i))));
    }
    cleardata = chdata[0] + chdata[1]*256;
    proxdata = chdata[4] + chdata[5]*256;
//  printk("================= proxdata = %d\n",proxdata);
    if (prox_on || proxdata < taos_cfgp->prox_threshold_lo ) {
        pro_buf[0] = 0x0;
        pro_buf[1] = 0x0;
        pro_buf[2] = taos_cfgp->prox_threshold_hi & 0x0ff;
        pro_buf[3] = taos_cfgp->prox_threshold_hi >> 8;
        data = 0;
        input_report_abs(taos_datap->input_dev,ABS_DISTANCE,data);
        input_sync(taos_datap->input_dev);
    } else if (proxdata > taos_cfgp->prox_threshold_hi ) {
        if (cleardata > ((sat_als*80)/100))
            return -ENODATA;
        pro_buf[0] = taos_cfgp->prox_threshold_lo & 0x0ff;
        pro_buf[1] = taos_cfgp->prox_threshold_lo >> 8;
        pro_buf[2] = 0xff;
        pro_buf[3] = 0xff;
        data = 1;
        input_report_abs(taos_datap->input_dev,ABS_DISTANCE,data);
        input_sync(taos_datap->input_dev);
    }

    for( mcount=0; mcount<4; mcount++ ) {
        if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x08) + mcount, pro_buf[mcount]))) < 0) {
            printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in taos prox threshold set\n");
            return (ret);
        }
    }

    prox_on = 0;
    return ret;
}
#ifdef CONFIG_PM
static int tmd27713_suspend(struct device *dev)
{
    int ret;
    struct i2c_client *client = container_of(dev, struct i2c_client, dev);

    if ((ret = (i2c_smbus_write_byte_data(client, (TAOS_TRITON_CMD_REG|0x00), 0x00))) < 0) {
        printk(KERN_ERR "tmd27713_suspend failed in power down\n");
        return (ret);
    }

    return 0;
}

static int tmd27713_resume(struct device *dev)
{
#ifdef TMD27713_ALS_TEST
    taos_sensors_als_on();
#else
    if(ALS_ON)
        taos_sensors_als_on();
#endif

#ifdef TMD27713_PROX_TEST
    taos_sensors_prox_on();
#else
    if(prox_on)
        taos_sensors_prox_on();
#endif
    msleep(50);

    return 0;
}

#if defined(CONFIG_HAS_EARLYSUSPEND)
static void tmd27713_early_suspend(struct early_suspend *h)
{
	tmd27713_suspend(&taos_datap->client->dev);
}

static void tmd27713_late_resume(struct early_suspend *h)
{
	tmd27713_resume(&taos_datap->client->dev);
}
#endif

static const struct dev_pm_ops tmd27713_pm_ops = {
#ifndef CONFIG_HAS_EARLYSUSPEND
    .suspend	= tmd27713_suspend,
    .resume		= tmd27713_resume,
#endif
};
#endif


// driver definition
static struct i2c_driver taos_driver = {
    .driver = {
        .owner = THIS_MODULE,
        .name = TAOS_DEVICE_NAME,
#ifdef CONFIG_PM
        .pm = &tmd27713_pm_ops,
#endif
    },
    .id_table = taos_idtable,
    .probe = taos_probe,
    .remove = __devexit_p(taos_remove),
};

// driver init
static int __init taos_init(void)
{
    int ret = 0;

    if ((ret = (i2c_add_driver(&taos_driver))) < 0) {
        printk(KERN_ERR "TAOS: i2c_add_driver() failed in taos_init()\n");
        return (ret);
    }
    printk(KERN_INFO "add tmd27713 i2c driver\n");

    return (ret);
}

// driver exit
static void __exit taos_exit(void)
{
    if (my_clientp)
        i2c_unregister_device(my_clientp);
    i2c_del_driver(&taos_driver);
    unregister_chrdev_region(taos_dev_number, TAOS_MAX_NUM_DEVICES);
    device_destroy(taos_class, MKDEV(MAJOR(taos_dev_number), 0));
    cdev_del(&taos_datap->cdev);
    class_destroy(taos_class);
    kfree(taos_datap);
}

extern int panel_type;
// client probe
static int taos_probe(struct i2c_client *clientp, const struct i2c_device_id *idp)
{
    int ret = 0;
    int chip_id;

    u32 error;
    struct tmd27713_irq  *tmd27713_irq;
    struct tmd27713_platform_data *pdata;

    if (device_found)
        return (-ENODEV);

    if(panel_type == TABLET7_TYPE_1) {
        numerator = NUMERATOR_7A;
        denominator = DENOMINATOR_7A;
        offset = OFFSET_7A;
    } else if(panel_type == TABLET7_TYPE_2) {
        numerator = NUMERATOR_7B;
        denominator = DENOMINATOR_7B;
        offset = OFFSET_7B;
    } else if(panel_type == TABLET10_TYPE_1) {
        numerator = NUMERATOR_AA;
        denominator = DENOMINATOR_AA;
        offset = OFFSET_AA;
    } else if(panel_type == TABLET10_TYPE_2) {
        numerator = NUMERATOR_AB;
        denominator = DENOMINATOR_AB;
        offset = OFFSET_AB;
    } else {
        printk("@@@@@@@@@@-----zhaoyang NO panel_type match!!! set default panel_type value to V55 -------------\n");
        numerator = NUMERATOR_7B;
        denominator = DENOMINATOR_7B;
        offset = OFFSET_7B;
    }

    tmd27713_irq = kzalloc(sizeof(struct tmd27713_irq), GFP_KERNEL);
    if (tmd27713_irq == NULL) {
        dev_err(&clientp->dev, "insufficient memory\n");
        return (-ENOMEM);
    }
    if ((ret = (alloc_chrdev_region(&taos_dev_number, 0, TAOS_MAX_NUM_DEVICES, TAOS_DEVICE_NAME))) < 0) {
        printk(KERN_ERR "TAOS: alloc_chrdev_region() failed in taos_init()\n");
        return (ret);
    }
    taos_class = class_create(THIS_MODULE, TAOS_DEVICE_NAME);
    taos_datap = kzalloc(sizeof(struct taos_data), GFP_KERNEL);
    if (!taos_datap) {
        printk(KERN_ERR "TAOS: kmalloc for struct taos_data failed in taos_init()\n");
        return -ENOMEM;
    }

    cdev_init(&taos_datap->cdev, &taos_fops);
    taos_datap->cdev.owner = THIS_MODULE;
    if ((ret = (cdev_add(&taos_datap->cdev, taos_dev_number, 1))) < 0) {
        printk(KERN_ERR "TAOS: cdev_add() failed in taos_init()\n");
        return (ret);
    }

    wake_lock_init(&taos_datap->taos_wake_lock, WAKE_LOCK_SUSPEND, "taos-wake-lock");
    device_create(taos_class, NULL, MKDEV(MAJOR(taos_dev_number), 0), &taos_driver ,"taos");
    pdata = clientp->dev.platform_data;

    /* Get data that is defined in board specific code. */
    tmd27713_irq->init_hw = pdata->init_irq_hw;

    if (tmd27713_irq->init_hw != NULL)
        tmd27713_irq->init_hw();

    if (!i2c_check_functionality(clientp->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        printk(KERN_ERR "TAOS: taos_probe() - i2c smbus byte data functions unsupported\n");
        return -EOPNOTSUPP;
    }
    if (!i2c_check_functionality(clientp->adapter, I2C_FUNC_SMBUS_WORD_DATA)) {
        printk(KERN_ERR "TAOS: taos_probe() - i2c smbus word data functions unsupported\n");
    }
    if (!i2c_check_functionality(clientp->adapter, I2C_FUNC_SMBUS_BLOCK_DATA)) {
        printk(KERN_ERR "TAOS: taos_probe() - i2c smbus block data functions unsupported\n");
    }
    taos_datap->client = clientp;
    i2c_set_clientdata(clientp, taos_datap);

//zhaoyang196673 add for inductance PROX sensor begin
    printk("@@@@@@@@@@-----zhaoyang read tmd27713 ALS & PROX chip_id-------------\n");
    //zhaoyang196673 add for inductance PROX sensor end

    chip_id = i2c_smbus_read_byte_data(clientp, (TAOS_TRITON_CMD_REG | (TAOS_TRITON_CNTRL + DEVICE_ID_REG))); //iVIZM  ///////????0x12??
    if((chip_id != DEVICE_27713_ID ) && (chip_id != DEVICE_27711_ID )  ) {
        printk("@@@@@zhaoyang read tmd27713 ALS & PROX chip_id error!!!, the chip_id = 0x%x\n",chip_id);
        return -ENODEV;
    } else {
        printk("@@@@@zhaoyang read tmd27713 ALS & PROX chip_idchip OK!!!\n");
        device_found = 1;
    }
    INIT_WORK(&(taos_datap->work),taos_work_func);
    sema_init(&taos_datap->update_lock, 1);

    taos_datap->input_dev = input_allocate_device();//iVIZM
    if (taos_datap->input_dev == NULL) {
        return -ENOMEM;
    }

    taos_datap->input_dev->name = TAOS_DEVICE_NAME;
    taos_datap->input_dev->id.bustype = BUS_I2C;
    set_bit(EV_ABS,taos_datap->input_dev->evbit);

	input_set_abs_params(taos_datap->input_dev, ABS_MISC, 0, 4096, 0, 0);
    input_set_capability(taos_datap->input_dev,EV_ABS,ABS_DISTANCE);
    input_set_capability(taos_datap->input_dev,EV_ABS,ABS_MISC); 
    ret = input_register_device(taos_datap->input_dev);
    if ((ret = (i2c_smbus_write_byte(clientp, (TAOS_TRITON_CMD_REG | TAOS_TRITON_CNTRL)))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte() to control reg failed in taos_probe()\n");
        return(ret);
    }
    strlcpy(clientp->name, TAOS_DEVICE_NAME, I2C_NAME_SIZE);
    strlcpy(taos_datap->taos_name, TAOS_DEVICE_NAME, TAOS_ID_NAME_SIZE);
    taos_datap->valid = 0;
    if (!(taos_cfgp = kmalloc(sizeof(struct taos_cfg), GFP_KERNEL))) {
        printk(KERN_ERR "TAOS: kmalloc for struct taos_cfg failed in taos_probe()\n");
        return -ENOMEM;
    }
    taos_cfgp->calibrate_target = calibrate_target_param;
    taos_cfgp->als_time = als_time_param;
    taos_cfgp->scale_factor = scale_factor_param;
    taos_cfgp->gain_trim = gain_trim_param;
    taos_cfgp->filter_history = filter_history_param;
    taos_cfgp->filter_count = filter_count_param;
    taos_cfgp->gain = offset;
    taos_cfgp->prox_threshold_hi = prox_threshold_hi_param;
    taos_cfgp->prox_threshold_lo = prox_threshold_lo_param;
    taos_cfgp->prox_int_time = prox_int_time_param;
    taos_cfgp->prox_adc_time = prox_adc_time_param;
    taos_cfgp->prox_wait_time = prox_wait_time_param;
    taos_cfgp->prox_intr_filter = prox_intr_filter_param;
    taos_cfgp->prox_config = prox_config_param;
    taos_cfgp->prox_pulse_cnt = prox_pulse_cnt_param;
    taos_cfgp->prox_gain = prox_gain_param;
    sat_als = (256 - taos_cfgp->prox_int_time) << 10;
    sat_prox = (256 - taos_cfgp->prox_adc_time) << 10;
    /*dmobile ::power down for init ,Rambo liu*/
    printk("Rambo::light sensor will pwr down \n");
    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x00), 0x00))) < 0) {
        printk(KERN_ERR "TAOS:Rambo, i2c_smbus_write_byte_data failed in power down\n");
        return (ret);
    }

    tmd27713_irq->irq = clientp->irq;

    if (tmd27713_irq->irq) {
        /* Try to request IRQ with falling edge first. This is
         * not always supported. If it fails, try with any edge. */

        error = request_threaded_irq(tmd27713_irq->irq, NULL,
                                     taos_irq_handler, IRQF_TRIGGER_FALLING, "tmd27713_irq", taos_datap);

        if (error < 0) {
            dev_err(&clientp->dev,
                    "failed to allocate irq %d\n", tmd27713_irq->irq);
            return (error);
        }
    }
	
#if defined(CONFIG_HAS_EARLYSUSPEND)
	taos_datap->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN +
							   ALS_SUSPEND_LEVEL;
	taos_datap->early_suspend.suspend = tmd27713_early_suspend;
	taos_datap->early_suspend.resume = tmd27713_late_resume;
	register_early_suspend(&taos_datap->early_suspend);
#endif
	
#ifdef TMD27713_ALS_TEST
    taos_sensors_als_on();
#endif
#ifdef TMD27713_PROX_TEST
    taos_sensors_prox_on();
#endif
    msleep(50);

    printk("ALS & proximity detector detect SUCCESS\n");

    return (ret);
}
static int __devexit taos_remove(struct i2c_client *client)
{
    int ret = 0;

    return (ret);
}

// open
static int taos_open(struct inode *inode, struct file *file)
{
    struct taos_data *taos_datap;
    int ret = 0;

    device_released = 0;
    taos_datap = container_of(inode->i_cdev, struct taos_data, cdev);
    if (strcmp(taos_datap->taos_name, TAOS_DEVICE_NAME) != 0) {
        printk(KERN_ERR "TAOS: device name incorrect during taos_open(), get %s\n", taos_datap->taos_name);
        ret = -ENODEV;
    }
    memset(readdata, 0, sizeof(struct ReadData)*2);//iVIZM
    return (ret);
}

// release
static int taos_release(struct inode *inode, struct file *file)
{
    struct taos_data *taos_datap;
    int ret = 0;

    device_released = 1;
    prox_on = 0;
    prox_history_hi = 0;
    prox_history_lo = 0;
    taos_datap = container_of(inode->i_cdev, struct taos_data, cdev);
    if (strcmp(taos_datap->taos_name, TAOS_DEVICE_NAME) != 0) {
        printk(KERN_ERR "TAOS: device name incorrect during taos_release(), get %s\n", taos_datap->taos_name);
        ret = -ENODEV;
    }
    return (ret);
}

// read
static int taos_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
    unsigned long flags;
    int realmax;
    int err;
    if((!ReadEnable) && (file->f_flags & O_NONBLOCK))
        return -EAGAIN;
    local_save_flags(flags);
    local_irq_disable();

    realmax = 0;
    if (down_interruptible(&taos_datap->update_lock))
        return -ERESTARTSYS;
    if (ReadEnable > 0) {
        if (sizeof(struct ReadData)*2 < count)
            realmax = sizeof(struct ReadData)*2;
        else
            realmax = count;
        err = copy_to_user(buf, readdata, realmax);
        if (err)
            return -EAGAIN;
        ReadEnable = 0;
    }
    up(&taos_datap->update_lock);
    memset(readdata, 0, sizeof(struct ReadData)*2);
    local_irq_restore(flags);
    return realmax;
}

// write
static int taos_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
    struct taos_data *taos_datap;
    u8 i = 0, xfrd = 0, reg = 0;
    u8 my_buf[TAOS_MAX_DEVICE_REGS];
    int ret = 0;

    if ((*ppos < 0) || (*ppos >= TAOS_MAX_DEVICE_REGS) || ((*ppos + count) > TAOS_MAX_DEVICE_REGS)) {
        printk(KERN_ERR "TAOS: reg limit check failed in taos_write()\n");
        return -EINVAL;
    }
    reg = (u8)*ppos;
    if ((ret =  copy_from_user(my_buf, buf, count))) {
        printk(KERN_ERR "TAOS: copy_to_user failed in taos_write()\n");
        return -ENODATA;
    }
    taos_datap = container_of(file->f_dentry->d_inode->i_cdev, struct taos_data, cdev);
    while (xfrd < count) {
        if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG | reg), my_buf[i++]))) < 0) {
            printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in taos_write()\n");
            return (ret);
        }
        reg++;
        xfrd++;
    }
    return ((int)xfrd);
}

// llseek
static loff_t taos_llseek(struct file *file, loff_t offset, int orig)
{
    int ret = 0;
    loff_t new_pos = 0;

    if ((offset >= TAOS_MAX_DEVICE_REGS) || (orig < 0) || (orig > 1)) {
        printk(KERN_ERR "TAOS: offset param limit or origin limit check failed in taos_llseek()\n");
        return -EINVAL;
    }
    switch (orig) {
        case 0:
            new_pos = offset;
            break;
        case 1:
            new_pos = file->f_pos + offset;
            break;
        default:
            return -EINVAL;
            break;
    }
    if ((new_pos < 0) || (new_pos >= TAOS_MAX_DEVICE_REGS) || (ret < 0)) {
        printk(KERN_ERR "TAOS: new offset limit or origin limit check failed in taos_llseek()\n");
        return -EINVAL;
    }
    file->f_pos = new_pos;
    return new_pos;
}
static int taos_sensors_prox_on(void)
{
    int prox_sum = 0, prox_mean = 0, prox_max = 0;
    int  ret = 0, i = 0;
    u8 reg_val = 0, reg_cntrl = 0;

    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x01), taos_cfgp->prox_int_time))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
        return (ret);
    }
    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x02), taos_cfgp->prox_adc_time))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
        return (ret);
    }
    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x03), taos_cfgp->prox_wait_time))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
        return (ret);
    }

    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x0D), taos_cfgp->prox_config))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
        return (ret);
    }

    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x0E), taos_cfgp->prox_pulse_cnt))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
        return (ret);
    }
    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x0F), taos_cfgp->prox_gain))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
        return (ret);
    }
    reg_cntrl = reg_val | (TAOS_TRITON_CNTL_PROX_DET_ENBL | TAOS_TRITON_CNTL_PWRON | TAOS_TRITON_CNTL_ADC_ENBL);
    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_CNTRL), reg_cntrl))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
        return (ret);
    }

    prox_sum = 0;
    prox_max = 0;
    for (i = 0; i < 20; i++) {
        if ((ret = taos_prox_poll(&prox_cal_info[i])) < 0) {
            printk(KERN_ERR "TAOS: call to prox_poll failed in ioctl prox_calibrate\n");
            return (ret);
        }
        prox_sum += prox_cal_info[i].prox_data;
        if (prox_cal_info[i].prox_data > prox_max)
            prox_max = prox_cal_info[i].prox_data;
        mdelay(100);
    }
    prox_mean = prox_sum/20;

    /*验证功能时不使用校验功能，以保证能产生中断*/
    taos_cfgp->prox_threshold_hi = 300;
    taos_cfgp->prox_threshold_lo = 280;
    for (i = 0; i < sizeof(taos_triton_reg_init); i++) {
        if(i !=11) {
            if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|(TAOS_TRITON_CNTRL +i)), taos_triton_reg_init[i]))) < 0) {
                printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl als_on\n");
                return (ret);
            }
        }
    }

    prox_on = 1;

    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x01), taos_cfgp->prox_int_time))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
        return (ret);
    }
    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x02), taos_cfgp->prox_adc_time))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
        return (ret);
    }
    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x03), taos_cfgp->prox_wait_time))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
        return (ret);
    }

    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x0C), taos_cfgp->prox_intr_filter))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
        return (ret);
    }
    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x0D), taos_cfgp->prox_config))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
        return (ret);
    }
    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x0E), taos_cfgp->prox_pulse_cnt))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
        return (ret);
    }
    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x0F), taos_cfgp->prox_gain))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
        return (ret);
    }
    reg_cntrl = TAOS_TRITON_CNTL_PROX_DET_ENBL | TAOS_TRITON_CNTL_PWRON | TAOS_TRITON_CNTL_PROX_INT_ENBL |
                TAOS_TRITON_CNTL_ADC_ENBL | TAOS_TRITON_CNTL_WAIT_TMR_ENBL  ;
    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_CNTRL), reg_cntrl))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
        return (ret);
    }
    taos_prox_threshold_set();//iVIZM
    return (ret);
}

static int taos_sensors_als_on(void)
{
    int  ret = 0, i = 0;
    u8 itime = 0, reg_val = 0, reg_cntrl = 0;

    for (i = 0; i < TAOS_FILTER_DEPTH; i++)
        lux_history[i] = -ENODATA;
    if ((ret = (i2c_smbus_write_byte(taos_datap->client, (TAOS_TRITON_CMD_REG|TAOS_TRITON_CMD_SPL_FN|TAOS_TRITON_CMD_ALS_INTCLR)))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte failed in ioctl als_on\n");
        return (ret);
    }
    itime = (((taos_cfgp->als_time/50) * 18) - 1);
    itime = (~itime);
    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_ALS_TIME), itime))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl als_on\n");
        return (ret);
    }
    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|TAOS_TRITON_INTERRUPT), taos_cfgp->prox_intr_filter))) < 0) {//golden
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl als_on\n");
        return (ret);
    }
    if ((ret = (i2c_smbus_write_byte(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_GAIN)))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte failed in ioctl als_on\n");
        return (ret);
    }
    reg_val = i2c_smbus_read_byte(taos_datap->client);
    reg_val = reg_val & 0xFC;
    reg_val = reg_val | (taos_cfgp->gain & 0x03);
    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_GAIN), reg_val))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl als_on\n");
        return (ret);
    }
    reg_cntrl = (TAOS_TRITON_CNTL_ADC_ENBL | TAOS_TRITON_CNTL_PWRON | TAOS_TRITON_CNTL_ALS_INT_ENBL);
    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_CNTRL), reg_cntrl))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl als_on\n");
        return (ret);
    }
    taos_als_threshold_set();//iVIZM
    return ret;
}

// ioctls
static long taos_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{

    int prox_sum = 0, prox_mean = 0, prox_max = 0;
    int lux_val = 0, ret = 0, i = 0, tmp = 0;
    u16 gain_trim_val = 0;
    u8 reg_val = 0, reg_cntrl = 0;
    int ret_check=0;
    int ret_m=0;
    u8 reg_val_temp=0;

    switch (cmd) {
        case TAOS_IOCTL_SENSOR_CHECK:
            reg_val_temp=0;
            if ((ret = (i2c_smbus_write_byte(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_CNTRL)))) < 0) {
                printk(KERN_ERR "TAOS: TAOS_IOCTL_SENSOR_CHECK failed\n");
                return (ret);
            }
            reg_val_temp = i2c_smbus_read_byte(taos_datap->client);
            printk(KERN_ERR "TAOS: TAOS_IOCTL_SENSOR_CHECK,prox_adc_time,%d~\n",reg_val_temp);
            if ((reg_val_temp & 0xFF) == 0xF)
                return -ENODATA;

            break;

        case TAOS_IOCTL_SENSOR_CONFIG:
            printk(KERN_ERR "TAOS: TAOS_IOCTL_SENSOR_CONFIG,test01~\n");
            ret = copy_from_user(taos_cfgp, (struct taos_cfg *)arg, sizeof(struct taos_cfg));
            if (ret) {
                printk(KERN_ERR "TAOS: copy_from_user failed in ioctl config_set\n");
                return -ENODATA;
            }

            break;

        case TAOS_IOCTL_SENSOR_ON:
            ret=0;
            reg_val=0;
            ret_m=0;

            printk(KERN_ERR "TAOS: TAOS_IOCTL_SENSOR_ON, test01~\n");

            /*Register init and turn off */
            for (i = 0; i < TAOS_FILTER_DEPTH; i++) {
                /*Rambo ??*/
                lux_history[i] = -ENODATA;
            }

            /*ALS interrupt clear*/
            if ((ret = (i2c_smbus_write_byte(taos_datap->client, (TAOS_TRITON_CMD_REG|TAOS_TRITON_CMD_SPL_FN|TAOS_TRITON_CMD_ALS_INTCLR)))) < 0) {
                printk(KERN_ERR "TAOS: i2c_smbus_write_byte failed in ioctl als_on\n");
                return (ret);
            }

            printk(KERN_ERR "TAOS: TAOS_IOCTL_SENSOR_ON, test02~\n");

            printk(KERN_ERR "TAOS: TAOS_IOCTL_SENSOR_ON, test03,prox_int_time,%d~\n",taos_cfgp->prox_int_time);
            if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|TAOS_TRITON_ALS_TIME), taos_cfgp->prox_int_time))) < 0) {
                printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
                return (ret);
            }

            printk(KERN_ERR "TAOS: TAOS_IOCTL_SENSOR_ON, test04,prox_adc_time,%d~\n",taos_cfgp->prox_adc_time);
            if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|TAOS_TRITON_PRX_TIME), taos_cfgp->prox_adc_time))) < 0) {
                printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
                return (ret);
            }

            printk(KERN_ERR "TAOS: TAOS_IOCTL_SENSOR_ON, test05,prox_wait_time,%d~\n", taos_cfgp->prox_wait_time);
            if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|TAOS_TRITON_WAIT_TIME), taos_cfgp->prox_wait_time))) < 0) {
                printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
                return (ret);
            }

            printk(KERN_ERR "TAOS: TAOS_IOCTL_SENSOR_ON, test05-1,prox_intr_filter,%d~\n", taos_cfgp->prox_intr_filter);
            if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|TAOS_TRITON_INTERRUPT), taos_cfgp->prox_intr_filter))) < 0) {
                printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
                return (ret);
            }

            printk(KERN_ERR "TAOS: TAOS_IOCTL_SENSOR_ON, test06,prox_config,%d~\n",taos_cfgp->prox_config);
            if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|TAOS_TRITON_PRX_CFG), taos_cfgp->prox_config))) < 0) {
                printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
                return (ret);
            }

            printk(KERN_ERR "TAOS: TAOS_IOCTL_SENSOR_ON, test06,prox_pulse_cnt,%d~\n",taos_cfgp->prox_pulse_cnt);
            if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|TAOS_TRITON_PRX_COUNT), taos_cfgp->prox_pulse_cnt))) < 0) {
                printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
                return (ret);
            }
            if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|TAOS_TRITON_GAIN), taos_cfgp->prox_gain))) < 0) {
                printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
                return (ret);
            }

            /*turn on*/
            if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|TAOS_TRITON_CNTRL), 0xF))) < 0) {
                printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
                return (ret);
            }

            break;


        case TAOS_IOCTL_SENSOR_OFF:
            ret=0;
            reg_val=0;
            ret_check=0;
            ret_m=0;

            /*turn off*/
            printk(KERN_ERR "TAOS: TAOS_IOCTL_SENSOR_OFF,test02~\n");
            if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x00), 0x00))) < 0) {
                printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_off\n");
                return (ret);
            }

            break;
        case TAOS_IOCTL_ALS_ON:
            printk("####################################\n");
            printk("######## TAOS IOCTL ALS ON #########\n");
            printk("####################################\n");

            if ((ret = (i2c_smbus_write_byte(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_CNTRL)))) < 0) {
                printk(KERN_ERR "TAOS: i2c_smbus_write_byte failed in ioctl als_calibrate\n");
                return (ret);
            }
            reg_val = i2c_smbus_read_byte(taos_datap->client);
            if ((reg_val & TAOS_TRITON_CNTL_PROX_DET_ENBL) == 0x0) {
                taos_sensors_als_on();
            }

            ALS_ON = 1;
            return (ret);
            break;
        case TAOS_IOCTL_ALS_OFF:
            printk("#####################################\n");
            printk("######## TAOS IOCTL ALS OFF #########\n");
            printk("#####################################\n");
            for (i = 0; i < TAOS_FILTER_DEPTH; i++)
                lux_history[i] = -ENODATA;
            if ((ret = (i2c_smbus_write_byte(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_CNTRL)))) < 0) {
                printk(KERN_ERR "TAOS: i2c_smbus_write_byte failed in ioctl als_calibrate\n");
                return (ret);
            }
            reg_val = i2c_smbus_read_byte(taos_datap->client);
            if ((reg_val & TAOS_TRITON_CNTL_PROX_DET_ENBL) == 0x0) {
                if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_CNTRL), 0x00))) < 0) {
                    printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl als_off\n");
                    return (ret);
                }
                cancel_work_sync(&taos_datap->work);//golden
            }
            ALS_ON = 0;
            return (ret);
            break;
        case TAOS_IOCTL_ALS_DATA:
            if ((ret = (i2c_smbus_write_byte(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_CNTRL)))) < 0) {
                printk(KERN_ERR "TAOS: i2c_smbus_write_byte failed in ioctl als_data\n");
                return (ret);
            }
            reg_val = i2c_smbus_read_byte(taos_datap->client);
            if ((reg_val & (TAOS_TRITON_CNTL_ADC_ENBL | TAOS_TRITON_CNTL_PWRON)) != (TAOS_TRITON_CNTL_ADC_ENBL | TAOS_TRITON_CNTL_PWRON))
                return -ENODATA;
            if ((ret = (i2c_smbus_write_byte(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_STATUS)))) < 0) {
                printk(KERN_ERR "TAOS: i2c_smbus_write_byte failed in ioctl als_data\n");
                return (ret);
            }
            reg_val = i2c_smbus_read_byte(taos_datap->client);
            if ((reg_val & TAOS_TRITON_STATUS_ADCVALID) != TAOS_TRITON_STATUS_ADCVALID)
                return -ENODATA;
            if ((lux_val = taos_get_lux()) < 0)
                printk(KERN_ERR "TAOS: call to taos_get_lux() returned error %d in ioctl als_data\n", lux_val);

            //lux_val = taos_lux_filter(lux_val);
            lux_val = (lux_val);
            return (lux_val);
            break;
        case TAOS_IOCTL_ALS_CALIBRATE:
            if ((ret = (i2c_smbus_write_byte(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_CNTRL)))) < 0) {
                printk(KERN_ERR "TAOS: i2c_smbus_write_byte failed in ioctl als_calibrate\n");
                return (ret);
            }
            reg_val = i2c_smbus_read_byte(taos_datap->client);
            if ((reg_val & 0x07) != 0x07)
                return -ENODATA;
            if ((ret = (i2c_smbus_write_byte(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_STATUS)))) < 0) {
                printk(KERN_ERR "TAOS: i2c_smbus_write_byte failed in ioctl als_calibrate\n");
                return (ret);
            }
            reg_val = i2c_smbus_read_byte(taos_datap->client);
            if ((reg_val & 0x01) != 0x01)
                return -ENODATA;
            if ((lux_val = taos_get_lux()) < 0) {
                printk(KERN_ERR "TAOS: call to lux_val() returned error %d in ioctl als_data\n", lux_val);
                return (lux_val);
            }
            gain_trim_val = (u16)(((taos_cfgp->calibrate_target) * 512)/lux_val);
            taos_cfgp->gain_trim = (int)gain_trim_val;
            return ((int)gain_trim_val);
            break;
        case TAOS_IOCTL_CONFIG_GET:
            ret = copy_to_user((struct taos_cfg *)arg, taos_cfgp, sizeof(struct taos_cfg));
            if (ret) {
                printk(KERN_ERR "TAOS: copy_to_user failed in ioctl config_get\n");
                return -ENODATA;
            }
            return (ret);
            break;
        case TAOS_IOCTL_CONFIG_SET:
            printk("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
            printk("^^^^^^^^^ TAOS INCTL CONFIG SET  ^^^^^^^\n");
            printk("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
            ret = copy_from_user(taos_cfgp, (struct taos_cfg *)arg, sizeof(struct taos_cfg));
            if (ret) {
                printk(KERN_ERR "TAOS: copy_from_user failed in ioctl config_set\n");
                return -ENODATA;
            }
            if(taos_cfgp->als_time < 50)
                taos_cfgp->als_time = 50;
            if(taos_cfgp->als_time > 650)
                taos_cfgp->als_time = 650;
            tmp = (taos_cfgp->als_time + 25)/50;
            taos_cfgp->als_time = tmp*50;
            sat_als = (256 - taos_cfgp->prox_int_time) << 10;
            sat_prox = (256 - taos_cfgp->prox_adc_time) << 10;
            break;
        case TAOS_IOCTL_PROX_ON:
            printk("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
            printk("^^^^^^^^^ TAOS IOCTL PROX ON  ^^^^^^^\n");
            printk("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
            prox_on = 1;

            if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x01), taos_cfgp->prox_int_time))) < 0) {
                printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
                return (ret);
            }
            if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x02), taos_cfgp->prox_adc_time))) < 0) {
                printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
                return (ret);
            }
            if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x03), taos_cfgp->prox_wait_time))) < 0) {
                printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
                return (ret);
            }

            if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x0C), taos_cfgp->prox_intr_filter))) < 0) {
                printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
                return (ret);
            }
            if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x0D), taos_cfgp->prox_config))) < 0) {
                printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
                return (ret);
            }
            if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x0E), taos_cfgp->prox_pulse_cnt))) < 0) {
                printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
                return (ret);
            }
            if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x0F), taos_cfgp->prox_gain))) < 0) {
                printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
                return (ret);
            }
            reg_cntrl = TAOS_TRITON_CNTL_PROX_DET_ENBL | TAOS_TRITON_CNTL_PWRON | TAOS_TRITON_CNTL_PROX_INT_ENBL |
                        TAOS_TRITON_CNTL_ADC_ENBL | TAOS_TRITON_CNTL_WAIT_TMR_ENBL  ;
            if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_CNTRL), reg_cntrl))) < 0) {
                printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
                return (ret);
            }
            taos_prox_threshold_set();//iVIZM
            break;
        case TAOS_IOCTL_PROX_OFF:
            printk("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
            printk("^^^^^^^^^ TAOS IOCTL PROX OFF  ^^^^^^^\n");
            printk("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
            if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_CNTRL), 0x00))) < 0) {
                printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl als_off\n");
                return (ret);
            }
            if (ALS_ON == 1) {
                taos_sensors_als_on();
            } else {
                cancel_work_sync(&taos_datap->work);//golden
            }
            prox_on = 0;
            break;
        case TAOS_IOCTL_PROX_DATA:
            if ((ret = taos_prox_poll(prox_cur_infop)) < 0) {
                printk(KERN_ERR "TAOS: call to prox_poll failed in taos_prox_poll_timer_func()\n");
                return ret;
            }
            ret = copy_to_user((struct taos_prox_info *)arg, prox_cur_infop, sizeof(struct taos_prox_info));
            if (ret) {
                printk(KERN_ERR "TAOS: copy_to_user failed in ioctl prox_data\n");
                return -ENODATA;
            }
            return (ret);
            break;
        case TAOS_IOCTL_PROX_EVENT:
            if ((ret = taos_prox_poll(prox_cur_infop)) < 0) {
                printk(KERN_ERR "TAOS: call to prox_poll failed in taos_prox_poll_timer_func()\n");
                return ret;
            }

            return (prox_cur_infop->prox_event);
            break;
        case TAOS_IOCTL_PROX_CALIBRATE:
            if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x01), taos_cfgp->prox_int_time))) < 0) {
                printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
                return (ret);
            }
            if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x02), taos_cfgp->prox_adc_time))) < 0) {
                printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
                return (ret);
            }
            if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x03), taos_cfgp->prox_wait_time))) < 0) {
                printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
                return (ret);
            }

            if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x0D), taos_cfgp->prox_config))) < 0) {
                printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
                return (ret);
            }

            if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x0E), taos_cfgp->prox_pulse_cnt))) < 0) {
                printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
                return (ret);
            }
            if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x0F), taos_cfgp->prox_gain))) < 0) {
                printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
                return (ret);
            }
            reg_cntrl = reg_val | (TAOS_TRITON_CNTL_PROX_DET_ENBL | TAOS_TRITON_CNTL_PWRON | TAOS_TRITON_CNTL_ADC_ENBL);
            if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_CNTRL), reg_cntrl))) < 0) {
                printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
                return (ret);
            }

            prox_sum = 0;
            prox_max = 0;
            for (i = 0; i < 20; i++) {
                if ((ret = taos_prox_poll(&prox_cal_info[i])) < 0) {
                    printk(KERN_ERR "TAOS: call to prox_poll failed in ioctl prox_calibrate\n");
                    return (ret);
                }
                prox_sum += prox_cal_info[i].prox_data;
                if (prox_cal_info[i].prox_data > prox_max)
                    prox_max = prox_cal_info[i].prox_data;
                mdelay(100);
            }
            prox_mean = prox_sum/20;
            taos_cfgp->prox_threshold_hi = ((((prox_max - prox_mean) * 200) + 50)/100) + prox_mean;
            taos_cfgp->prox_threshold_lo = ((((prox_max - prox_mean) * 170) + 50)/100) + prox_mean;
            printk("TAOS:------------ taos_cfgp->prox_threshold_hi = %d\n",taos_cfgp->prox_threshold_hi );
            printk("TAOS:------------ taos_cfgp->prox_threshold_lo = %d\n",taos_cfgp->prox_threshold_lo );
            for (i = 0; i < sizeof(taos_triton_reg_init); i++) {
                if(i !=11) {
                    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|(TAOS_TRITON_CNTRL +i)), taos_triton_reg_init[i]))) < 0) {
                        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl als_on\n");
                        return (ret);
                    }
                }
            }

            break;
        default:
            return -EINVAL;
            break;
    }
    return (ret);
}

// read/calculate lux value
static int taos_get_lux(void)
{
    u16 raw_clear = 0, raw_ir = 0, raw_lux = 0;
    u32 lux = 0;
    u32 ratio = 0;
    u8 dev_gain = 0;
    u16 Tint = 0;
    struct lux_data *p;
    int ret = 0;
    u8 chdata[4];
    int tmp = 0, i = 0;

    for (i = 0; i < 4; i++) {
        if ((ret = (i2c_smbus_write_byte(taos_datap->client, (TAOS_TRITON_CMD_REG | (TAOS_TRITON_ALS_CHAN0LO + i))))) < 0) {
            printk(KERN_ERR "TAOS: i2c_smbus_write_byte() to chan0/1/lo/hi reg failed in taos_get_lux()\n");
            return (ret);
        }
        chdata[i] = i2c_smbus_read_byte(taos_datap->client);
//     printk("ch(%d),data=%d\n",i,chdata[i]);
    }
//   printk("ch0=%d\n",chdata[0]+chdata[1]*256);
//   printk("ch1=%d\n",chdata[2]+chdata[3]*256);

    tmp = (taos_cfgp->als_time + 25)/50;            //if atime =100  tmp = (atime+25)/50=2.5   tine = 2.7*(256-atime)=  412.5
    TritonTime.numerator = 1;
    TritonTime.denominator = tmp;

    tmp = 300 * taos_cfgp->als_time;               //tmp = 300*atime  400
    if(tmp > 65535)
        tmp = 65535;
    TritonTime.saturation = tmp;
    raw_clear = chdata[1];
    raw_clear <<= 8;
    raw_clear |= chdata[0];
    raw_ir    = chdata[3];
    raw_ir    <<= 8;
    raw_ir    |= chdata[2];

    raw_clear *= (taos_cfgp->scale_factor);
    raw_ir *= (taos_cfgp->scale_factor );

    if(raw_ir > raw_clear) {
        raw_lux = raw_ir;
        raw_ir = raw_clear;
        raw_clear = raw_lux;
    }
    dev_gain = taos_triton_gain_table[taos_cfgp->gain & 0x3];
    if(raw_clear >= lux_timep->saturation)
        return(TAOS_MAX_LUX);
    if(raw_ir >= lux_timep->saturation)
        return(TAOS_MAX_LUX);
    if(raw_clear == 0)
        return(0);
    if(dev_gain == 0 || dev_gain > 127) {
        printk(KERN_ERR "TAOS: dev_gain = 0 or > 127 in taos_get_lux()\n");
        return -1;
    }
    if(lux_timep->denominator == 0) {
        printk(KERN_ERR "TAOS: lux_timep->denominator = 0 in taos_get_lux()\n");
        return -1;
    }
    ratio = (raw_ir<<15)/raw_clear;
    for (p = lux_tablep; p->ratio && p->ratio < ratio; p++);
    if(!p->ratio) {//iVIZM
        if(lux_history[0] < 0)
            return 0;
        else
            return lux_history[0];
    }
    Tint = taos_cfgp->als_time;
    raw_clear = ((raw_clear*400 + (dev_gain>>1))/dev_gain + (Tint>>1))/Tint;
    raw_ir = ((raw_ir*400 +(dev_gain>>1))/dev_gain + (Tint>>1))/Tint;
    lux = ((raw_clear*(p->clear)) - (raw_ir*(p->ir)));
    lux = (lux + 32000)/64000;
    if(lux > TAOS_MAX_LUX) {
        lux = TAOS_MAX_LUX;
    }
    lux = (lux * numerator )/ denominator;

    return(lux);
}
// verify device

// proximity poll
static int taos_prox_poll(struct taos_prox_info *prxp)
{
    int i = 0, ret = 0;
    u8 chdata[6];
    for (i = 0; i < 6; i++) {
        chdata[i] = (i2c_smbus_read_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_CMD_AUTO | (TAOS_TRITON_ALS_CHAN0LO + i))));
    }
    prxp->prox_clear = chdata[1];
    prxp->prox_clear <<= 8;
    prxp->prox_clear |= chdata[0];
    if (prxp->prox_clear > ((sat_als*80)/100))
        return -ENODATA;
    prxp->prox_data = chdata[5];
    prxp->prox_data <<= 8;
    prxp->prox_data |= chdata[4];

    return (ret);
}

// prox poll timer function
static void taos_prox_poll_timer_func(unsigned long param)
{
    int ret = 0;

    if (!device_released) {
        if ((ret = taos_prox_poll(prox_cur_infop)) < 0) {
            printk(KERN_ERR "TAOS: call to prox_poll failed in taos_prox_poll_timer_func()\n");
            return;
        }
        taos_prox_poll_timer_start();
    }
    return;
}

// start prox poll timer
static void taos_prox_poll_timer_start(void)
{
    init_timer(&prox_poll_timer);
    prox_poll_timer.expires = jiffies + (HZ/10);
    prox_poll_timer.function = taos_prox_poll_timer_func;
    add_timer(&prox_poll_timer);
    return;
}

MODULE_AUTHOR("John Koshi - Surya Software");
MODULE_DESCRIPTION("TAOS ambient light and proximity sensor driver");
MODULE_LICENSE("GPL");

module_init(taos_init);
module_exit(taos_exit);
