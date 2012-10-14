/*******************************************************************************
 *                                                                              *
 *       File Name:      taos_v3.c (Version 3.5)                                *
 *       Description:    Linux device driver for Taos Ambient Light Sensor      *
 *                                                                              *
 *       Author:         TAOS Inc.                                              *
 *       								       *
 ********************************************************************************
 *       Proprietary to Taos Inc., 1001 Klein Road #300, Plano, TX 75074        *
 *******************************************************************************/
/**
 * Copyright (C) 2011, Texas Advanced Optoelectronic Solutions (R).
 * All rights reserved.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/*===========================================================================
                        EDIT HISTORY FOR V11
  when        comment tag         who                  what, where, why
----------    ------------     -----------      --------------------------
2011/03/28    zhaoy0002         zhaoyang        modify for resolve the problem that without mma8452, the kernel panic
2011/07/04    zhaoy0022         zhaoyang        when the driver loads successfully output SUCESS information to meet production test requirements
2011/08/02    zhaoy0026         zhaoyang        modify the properties of ambient light sensors sysfs interface
===========================================================================*/

#include <linux/mutex.h>
#include <linux/pm_runtime.h>
#include <linux/wait.h>

/* Includes */
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
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <linux/gpio.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/ioport.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>

#include <linux/workqueue.h>
#include <linux/firmware.h>
#include <linux/string.h>
#include <linux/kmod.h>
#include <linux/ctype.h>
#include <linux/input.h>
#include <linux/taos_v3.h>    //zhaoyang196673 changed for tsl2583

/**
 * definitions/declarations used in this code
 * u8 = unsigned char (8 bit)
 * s8 = signed   char (8 bit)
 * unsigned int= unsigned int  (16 bit)
 * s16= signed   int  (16 bit) - aka int
 */

//....................................................................................................
#define DRIVER_VERSION_ID	"3.6"

#define TAOS258x		2

//This build will be for..
#define DRIVER_TYPE		TAOS258x

//....................................................................................................
//Debug related
//#define TAOS_DEBUG			0
//#define TAOS_DEBUG_SMBUS		1	//prints the data written or read from a smbus r/w
//#define TAOS_IRQ_DEBUG			1	//prints special messages from interrupt BHs console
//....................................................................................................
//User notification method
//#define ASYNC_NOTIFY			1	//Interrupt generated application notification via fasync_
#define EVENT_NOTIFY			1	//Interrupt generated input_event update in /dev/input/event_
//....................................................................................................
//Module defines

/* Device name/id/address */
#define DEVICE_NAME			"taos"
#ifdef TAOS258x
#define DEVICE_ID			"skateFN"
#else
#define DEVICE_ID			"tritonFN"
#endif

#define ID_NAME_SIZE			10
#define DEVICE_ADDR1			0x29
#define DEVICE_ADDR2			0x39
#define DEVICE_ADDR3			0x49
#define MAX_NUM_DEVICES			3
#define MAX_DEVICE_REGS			32
#define I2C_MAX_ADAPTERS		8

/* Triton register offsets */
#ifdef TAOS258x
#define	TAOS_REG_MAX	8
#else
#define	TAOS_REG_MAX	16
#endif

//zhaoyang196673 add for tsl2583 begin
#define assert(expr)\
    if(!(expr)) {\
        printk( "Assertion failed! %s,%d,%s,%s\n",\
                __FILE__,__LINE__,__func__,#expr);\
    }
//zhaoyang196673 add for tsl2583 end


/* Power management */
//zhaoyang196673 add for tsl2583 begin
//#define taos_suspend			NULL
//#define taos_resume			NULL
//zhaoyang196673 add for tsl2583 end
#define taos_shutdown			NULL

/* Operational mode */
//#define POLLED_MODE			1
//#define INTERRUPT_MODE			2

#define TRUE				1
#define FALSE				0

#define	MAXI2C	32
#define	CMD_ADDRESS	0x80
#define	TAOS_ERROR	-1
#define TAOS_SUCCESS	0

#define	MAX_SAMPLES_CAL	200

/* Prototypes */
extern int i2c_add_driver(struct i2c_driver *driver);
extern struct i2c_adapter *i2c_get_adapter(int id);

static int taos_probe(struct i2c_client *clientp,
                      const struct i2c_device_id *idp);
static irqreturn_t taos_interrupt(int irq, void *dev_id);

//zhaoyang196673 add for tsl2583 begin
static int taos_suspend(struct i2c_client *client, pm_message_t mesg);
static int taos_resume(struct i2c_client *client);
//zhaoyang196673 add for tsl2583 end

void taos_als_int_tasklet(struct work_struct *work);

static int taos_remove(struct i2c_client *client);
static int taos_open(struct inode *inode, struct file *file);
static int taos_release(struct inode *inode, struct file *file);
static int taos_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
                      unsigned long arg);
static int taos_read(struct file *file, char *buf, size_t count, loff_t *ppos);
static int taos_write(struct file *file, const char *buf, size_t count,
                      loff_t *ppos);
static loff_t taos_llseek(struct file *file, loff_t offset, int orig);
static int taos_fasync(int fd, struct file *file, int mode);
//static int taos_lux_filter(int raw_lux);
static int taos_device_name(unsigned char *bufp, char **device_name);
static int taos_als_calibrate(unsigned long als_cal_target);
int taos_parse_config_file(char *x);

//....................................................................................................
// Various /misc. emunerations
typedef enum {
    TAOS_CHIP_UNKNOWN = 0,
    TAOS_CHIP_WORKING = 1,
    TAOS_CHIP_SLEEP = 2
} TAOS_CHIP_WORKING_STATUS;

//....................................................................................................

char driver_version_id[] = DRIVER_VERSION_ID;

/* Module parameter */
//static char *mode = "default";
//module_param(mode, charp, S_IRUGO);
//MODULE_PARM_DESC(mode, "Mode of operation - polled or interrupt");

//------------------------------------------------------------------------------------------------------------------

/* First device number */
static dev_t taos_dev_number;

/* Class structure for this device */
static struct class *taos_class;

/* Module device table */
static struct i2c_device_id taos_idtable[] = {
    {
        DEVICE_ID, 0
    },
    {
    }
};
MODULE_DEVICE_TABLE( i2c, taos_idtable);

/* Client and device */
static struct i2c_client *my_clientp;
static struct device *devp;
static int device_found;

/* Driver definition */
static struct i2c_driver taos_driver = {
    .driver =
    {
        .owner = THIS_MODULE, .name = DEVICE_NAME,
    },
    .id_table = taos_idtable,
    .probe = taos_probe,
    .remove = __devexit_p(taos_remove),
    .suspend =  taos_suspend,
    .shutdown = taos_shutdown,
    .resume = taos_resume,
};


/* Per-device data */
static struct taos_data {
    struct i2c_client *client;
    struct cdev cdev;
    struct fasync_struct *async_queue;
    unsigned int addr;
    char taos_id;
    char taos_name[ID_NAME_SIZE];
    struct semaphore update_lock;
    char valid;
    unsigned long last_updated;
}*taos_datap;

/* File operations */
static struct file_operations taos_fops = {
    .owner = THIS_MODULE,
    .open = taos_open,
    .release = taos_release,
    .read = taos_read,
    .write = taos_write,
    .llseek = taos_llseek,
    .fasync = taos_fasync,
    .ioctl = taos_ioctl,
};

static u8 init_done = 0;

// ................ Als info ...................../
struct taos_als_info als_cur_info;
EXPORT_SYMBOL( als_cur_info);

//Next two vars are also used to determine irq type - in addition/lieu status info
volatile bool als_on = 0;

static int device_released = 0;
static unsigned int irqnum = 0;
static DECLARE_WAIT_QUEUE_HEAD( taos_int_wait);

/* Lux time scale */
struct time_scale_factor {
    unsigned int numerator;
    unsigned int denominator;
    unsigned int saturation;
};

//---------------------------------------------- Mutex ---------------------------------------
DEFINE_MUTEX( als_mutex);

//Structs & vars
int taos_chip_status = TAOS_CHIP_UNKNOWN; //Unknown = uninitialized to start
int taos_cycle_type; // what is the type of cycle being run: ALS, PROX, or BOTH

struct taos_settings taos_settings;
EXPORT_SYMBOL( taos_settings);

/* Device configuration */
#define MAX_SETTING_MEMBERS	6
static struct taos_settings *taos_settingsP; //Pointer Needed for ioctl(s)

//More Prototypes
unsigned long taos_isqrt(unsigned long x);
int taos_register_dump(void);

int taos_i2c_read(u8 reg, u8 *val, unsigned int len);
int taos_i2c_write(u8 reg, u8 *val);
int taos_i2c_write_command(u8 reg);

int taos_parse_lux_file(char *x);

//-------------------------- Work Queues used for bottom halves of IRQs -----------------------------
static struct workqueue_struct *taos_wq0;

typedef struct {
    struct work_struct my_work;
    int x;
} my_work_t;

my_work_t *als;

//..................................................................................................................
// Initial values for device - this values can/will be changed by driver (via chip_on and other)
// and applications as needed.
// These values are dynamic.

//UNCOMMENT BELOW FOR SKATE
#ifdef TAOS258x
u8 taos_config[8] = {
    0x00, 0xee, 0x00, 0x03, 0x00, 0xFF, 0xFF, 0x00
};
//	cntrl atime intC  Athl0 Athl1 Athh0 Athh1 gain
#else

//UNCOMMENT BELOW FOR TRITON
//u8 taos_config[16] = {
0x00, 0xee, 0xff, 0xf5, 0x10, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x03, 0x30, 0x00, 0x0a, 0x20
};
cntrl atime ptime wtime Athl0 Athl1 Athh0 Athh1 Pthl0 Pthl1 Pthh0 Pthh1 Prst pcfg pcnt gain

// initial lux equation table

#endif

#define LUX_TABLE_MIN_RECS 3	//Default minimum number of records in the following table
//Note table updates must always include complete table.


//This structure is intentionally large to accommodate updates via
//ioctl and proc input methods.
struct taos_lux {
    unsigned int ratio;
    unsigned int ch0;
    unsigned int ch1;
} taos_device_lux[] = {
    {9830, 8520, 15729},
    {12452, 10807, 23344},
    {14746, 6383, 11705},
    {17695, 4063, 6554},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0}
};

struct taos_lux taos_lux;
EXPORT_SYMBOL( taos_lux);

int als_time_scale; // computed, ratios lux due to als integration time
int als_saturation; // computed, set to 90% of full scale of als integration
//int	als_thresh_high;	// als threshold (trigger point) - when als >
//int	als_thresh_low;		// als threshold (trigger point) - when als <

#define MAX_GAIN_STAGES	4	//Index = ( 0 - 3) Used to validate the gain selection index
struct gainadj {
    s16 ch0;
    s16 ch1;
} gainadj[] = {
    {1, 1},
    {8, 8},
    {16, 16},
    {107, 115}
};

struct taos_lux *taos_device_luxP;
struct taos_lux *copy_of_taos_device_luxP;

//Input_dev
static struct input_dev *taos_dev;
//zhaoyang196673 add for tsl2583 begin
//sysfs - interface functions
static ssize_t taos_device_id(struct device *dev,
                              struct device_attribute *attr, char *buf);

static DEVICE_ATTR( device_id, 0644, taos_device_id, NULL);

static ssize_t taos_power_state_show(struct device *dev,
                                     struct device_attribute *attr, char *buf);
static ssize_t taos_power_state_store(struct device *dev,
                                      struct device_attribute *attr, const char *buf, size_t len);

static DEVICE_ATTR(device_state, 0644,
                   taos_power_state_show,
                   taos_power_state_store);

static ssize_t taos_gain_show(struct device *dev,
                              struct device_attribute *attr, char *buf);
static ssize_t taos_gain_store(struct device *dev,
                               struct device_attribute *attr, const char *buf, size_t len);
static DEVICE_ATTR(als_gain, 0644,
                   taos_gain_show,
                   taos_gain_store);

static ssize_t taos_interrupt_show(struct device *dev,
                                   struct device_attribute *attr, char *buf);
static ssize_t taos_interrupt_store(struct device *dev,
                                    struct device_attribute *attr, const char *buf, size_t len);
static DEVICE_ATTR(als_interrupt, 0644,
                   taos_interrupt_show,
                   taos_interrupt_store);

static ssize_t taos_als_time_show(struct device *dev,
                                  struct device_attribute *attr, char *buf);
static ssize_t taos_als_time_store(struct device *dev,
                                   struct device_attribute *attr, const char *buf, size_t len);
static DEVICE_ATTR(als_time, 0644,
                   taos_als_time_show,
                   taos_als_time_store);

static ssize_t taos_als_trim_show(struct device *dev,
                                  struct device_attribute *attr, char *buf);
static ssize_t taos_als_trim_store(struct device *dev,
                                   struct device_attribute *attr, const char *buf, size_t len);
static DEVICE_ATTR(als_trim, 0644,
                   taos_als_trim_show,
                   taos_als_trim_store);

static ssize_t taos_als_persistence_show(struct device *dev,
        struct device_attribute *attr, char *buf);
static ssize_t taos_als_persistence_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t len);
static DEVICE_ATTR(als_persistence, 0644,
                   taos_als_persistence_show,
                   taos_als_persistence_store);

static ssize_t taos_als_cal_target_show(struct device *dev,
                                        struct device_attribute *attr, char *buf);
static ssize_t taos_als_cal_target_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t len);
static DEVICE_ATTR(als_target, 0644,
                   taos_als_cal_target_show,
                   taos_als_cal_target_store);

static ssize_t taos_lux_show(struct device *dev, struct device_attribute *attr,
                             char *buf);
static DEVICE_ATTR( lux, 0644, taos_lux_show, NULL);

static ssize_t taos_do_calibrate(struct device *dev,
                                 struct device_attribute *attr, const char *buf, size_t len);
static DEVICE_ATTR( calibrate, 0644, NULL, taos_do_calibrate);

static ssize_t taos_als_thresh_low_show(struct device *dev,
                                        struct device_attribute *attr, char *buf);
static ssize_t taos_als_thresh_low_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t len);
static DEVICE_ATTR(als_lowT, 0644,
                   taos_als_thresh_low_show,
                   taos_als_thresh_low_store);

static ssize_t taos_als_thresh_high_show(struct device *dev,
        struct device_attribute *attr, char *buf);
static ssize_t taos_als_thresh_high_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t len);
static DEVICE_ATTR(als_highT, 0644,
                   taos_als_thresh_high_show,
                   taos_als_thresh_high_store);

//zhaoyang196673 add for tsl2583 end

static struct attribute *sysfs_attrs_ctrl[] = {
    &dev_attr_device_id.attr,
    &dev_attr_device_state.attr,
    &dev_attr_als_gain.attr,
    &dev_attr_als_interrupt.attr,
    &dev_attr_als_time.attr,
    &dev_attr_als_trim.attr,
    &dev_attr_als_persistence.attr,
    &dev_attr_als_target.attr,
    &dev_attr_lux.attr,
    &dev_attr_calibrate.attr,
    &dev_attr_als_lowT.attr,
    &dev_attr_als_highT.attr,
    NULL
};

static struct attribute_group apds990x_attribute_group[] = {
    {
        .attrs = sysfs_attrs_ctrl
    },
};

// Interrupt pin declaration - must be changed to suit H/W
//#define GPIO_PS_ALS_INT_IRQ 	S5PV210_GPH0(4)
//#define HW_IRQ_PIN				S5PV210_GPH0(4)
#define HW_IRQ_NO				34

//===========================================================================================================
//
//                                          FUNCTIONS BEGIN HERE
//
//===========================================================================================================

//===========================================================================================================
/**@defgroup Initialization Driver Initialization
 * The sections applies to functions for driver initialization, instantiation, _exit,\n
 * and user-land application invoking (ie. open).\n
 * Also included in this section is the initial interrupt handler (handler bottom halves are in the respective
 * sections).
 * @{
 */

//-----------------------------------------------------------------------------------------------------------
/** Driver initialization - device probe is initiated here, to identify
 * a valid device if present on any of the i2c buses and at any address.
 * \param  none
 * \return: int (0 = OK)
 * \note  	H/W Interrupt are device/product dependent.  Attention is required to the definition and
 * configuration.
 */
//-----------------------------------------------------------------------------------------------------------
static int __init taos_init(void)
{
    int ret = 0;
//zhaoyang196673 add for tsl2583 begin
    if ((ret = (i2c_add_driver(&taos_driver))) < 0) {
        printk(KERN_ERR "taos_device_drvr: i2c_add_driver() failed in "
               "taos_init()\n");
        return -ENODEV;
    }
    printk(KERN_INFO "add lts2583 i2c driver\n");
//zhaoyang196673 add for tsl2583 end
    return (ret);
}

/**
 * Driver exit
 */
static void __exit taos_exit(void)
{
    printk(KERN_INFO "TAOS driver __exit\n");
    if (als)
        kfree((void *)als);

    if (taos_wq0) {
        flush_workqueue( taos_wq0 );
        destroy_workqueue( taos_wq0 );
    }

    input_free_device(taos_dev);

    if (my_clientp)
        i2c_unregister_device(my_clientp);
    i2c_del_driver(&taos_driver);
    device_destroy(taos_class, MKDEV(MAJOR(taos_dev_number), 0));
    cdev_del(&taos_datap->cdev);
    kfree(taos_datap);
    class_destroy(taos_class);
    unregister_chrdev_region(taos_dev_number, MAX_NUM_DEVICES);

}

/**
 * Client probe function - When a valid device is found, the driver's device
 * data structure is updated, and initialization completes successfully.
 */
static int taos_probe(struct i2c_client *clientp,
                      const struct i2c_device_id *idp)
{
    int error, ret = 0;
    unsigned char buf[MAX_DEVICE_REGS] = {0};
    char *device_name;
    //zhaoyang196673 add for tsl2583 begin
    u32 err;
    struct tsl2583_irq  *tsl;
    struct tsl2583_platform_data *pdata;
    //zhaoyang196673 add for tsl2583 end

    if (device_found)
        return (-ENODEV);

    tsl = kzalloc(sizeof(struct tsl2583_irq), GFP_KERNEL);
    if (tsl == NULL) {
        dev_err(&clientp->dev, "insufficient memory\n");
        return (-ENOMEM);
    }

    //zhaoyang196673 add for tsl2583 begin
    taos_datap = kzalloc(sizeof(struct taos_data), GFP_KERNEL);
    if (taos_datap == NULL) {
        dev_err(&clientp->dev, "insufficient memory at taos_datap\n");
        return (-ENOMEM);
    }
    //zhaoyang196673 add for tsl2583 end

    //zhaoyang196673 add for tsl2583 begin
    pdata = clientp->dev.platform_data;

    /* Get data that is defined in board specific code. */
    tsl->init_hw = pdata->init_irq_hw;

    if (tsl->init_hw != NULL)
        tsl->init_hw();

    //zhaoyang196673 add for tsl2583 end
    if (!i2c_check_functionality(clientp->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_err(&clientp->dev, "taos_probe() - i2c smbus byte data "
                "functions unsupported\n");
        return (-EOPNOTSUPP);
    }

    if (!i2c_check_functionality(clientp->adapter, I2C_FUNC_SMBUS_WORD_DATA)) {
        dev_warn(&clientp->dev, "taos_probe() - i2c smbus word data "
                 "functions unsupported\n");
    }

    if (!i2c_check_functionality(clientp->adapter, I2C_FUNC_SMBUS_BLOCK_DATA)) {
        dev_err(&clientp->dev, "taos_probe() - i2c smbus block data "
                "functions unsupported\n");
    }

    if(clientp == NULL)
        return (-ENODEV);

    taos_datap->client = clientp;

    i2c_set_clientdata(clientp, taos_datap);

    if ((ret = taos_device_name(buf, &device_name)) == 0) {
        dev_info(&clientp->dev, "i2c device found - does not match "
                 "expected id in taos_probe() 1\n");
    }

    //zhaoyang196673 add for tsl2583 begin
    if (strcmp(device_name, "skateFN") && strcmp(device_name, "tritonFN") )
        //zhaoyang196673 add for tsl2583 end
    {
        dev_info(&clientp->dev, "i2c device found - does not match "
                 "expected id in taos_probe()2\n");
        return (-ENODEV);
    } else {
        dev_info(&clientp->dev, "i2c device found - matches expected "
                 "id of %s in taos_probe()\n", device_name);
        device_found = 1;
    }
    if ((ret = (i2c_smbus_write_byte(clientp, (CMD_REG | CNTRL)))) < 0) {
        dev_err(&clientp->dev, "i2c_smbus_write_byte() to cmd reg "
                "failed in taos_probe(), err = %d\n", ret);
        return (ret);
    }
    //zhaoyang196673 add for tsl2583 begin
    tsl->irq = clientp->irq;

    if (tsl->irq) {
        /* Try to request IRQ with falling edge first. This is
         * not always supported. If it fails, try with any edge. */

        //zhaoyang196673 add for tsl2583 begin
        error = request_threaded_irq(tsl->irq, NULL,
                                     taos_interrupt,
                                     IRQF_TRIGGER_FALLING,
                                     clientp->dev.driver->name,
                                     tsl);
        //zhaoyang196673 add for tsl2583 end
        if (error < 0) {
            dev_err(&clientp->dev,
                    "failed to allocate irq %d\n", tsl->irq);
            return (error);
        }
    }
    //zhaoyang196673 add for tsl2583 end
    strlcpy(clientp->name, DEVICE_ID, I2C_NAME_SIZE);
    strlcpy(taos_datap->taos_name, DEVICE_ID, ID_NAME_SIZE);

    taos_datap->valid = 0;
    init_MUTEX(&taos_datap->update_lock);

    if (!(taos_settingsP = kmalloc(sizeof(struct taos_settings), GFP_KERNEL))) {
        dev_err(&clientp->dev,
                "kmalloc for struct taos_settings failed in taos_probe()\n");
        return (-ENOMEM);
    }

    //zhaoyang196673 add for tsl2583 begin
    //.............................................................................................................
    //Create the work queue and allocate kernel memory for the scheduled workers (tasklets)
    taos_wq0 = create_workqueue("taos_work_queue");
    if (taos_wq0) {
        als = (my_work_t *)kmalloc(sizeof(my_work_t), GFP_KERNEL);
    }
    //.............................................................................................................
    if ((ret = (alloc_chrdev_region(&taos_dev_number, 0, MAX_NUM_DEVICES,
                                    DEVICE_NAME))) < 0) {
        printk(KERN_ERR "taos_device_drvr: alloc_chrdev_region() failed in "
               "taos_init()\n");
        return (ret);
    }
    taos_class = class_create(THIS_MODULE, DEVICE_NAME);

    cdev_init(&taos_datap->cdev, &taos_fops);
    taos_datap->cdev.owner = THIS_MODULE;

    if ((ret = (cdev_add(&taos_datap->cdev, taos_dev_number, 1))) < 0) {
        printk(KERN_ERR "taos_device_drvr: cdev_add() failed in "
               "taos_init()\n");
        goto exit_2;
    }

    device_create(taos_class, NULL, MKDEV(MAJOR(taos_dev_number), 0),
                  &taos_driver ,DEVICE_NAME);

//zhaoyang196673 add for tsl2583 begin
    devp = &my_clientp->dev;
    goto exit_4;
//zhaoyang196673 add for tsl2583 end

exit_2:
    cdev_del(&taos_datap->cdev);

    kfree(taos_datap);
    //zhaoyang196673 add for tsl2583 begin
    class_destroy(taos_class);
    //zhaoyang196673 add for tsl2583 end
    unregister_chrdev_region(taos_dev_number, MAX_NUM_DEVICES);
exit_4:
    //Load up the V2 defaults (these are hard coded defaults for now)
    taos_defaults();

    //Set any other defaults that are needed to initialize (such as taos_cycle_type).
    //Note: these settings can/will be changed based on user requirements.
    taos_cycle_type = LIGHT;
    //Initialize the work queues

    //zhaoyang196673 add for tsl2583 begin
    taos_chip_on();
    //zhaoyang196673 add for tsl2583 end

    INIT_WORK( (struct work_struct *)als, taos_als_int_tasklet );
    //. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
    //This section populates the input structure and registers the device
#ifdef EVENT_NOTIFY
    taos_dev = input_allocate_device();
    if (!taos_dev) {
        printk(KERN_ERR "__init: Not enough memory for input_dev\n");
        return(ret);
    }

    //Since we now have the struct, populate.
    taos_dev->name = DEVICE_ID;
    taos_dev->id.bustype = BUS_I2C;
    taos_dev->id.vendor = 0x0089;
    taos_dev->id.product = 0x2581;
    taos_dev->id.version = 3;
    taos_dev->phys = "/dev/taos";

    // This device one value (LUX)
    // Other devices may have LUX and PROX.
    // Thus we use the ABS_X so ABS_Y is logically available.
    set_bit(EV_ABS, taos_dev->evbit);
    set_bit(ABS_MISC, taos_dev->mscbit);

    //Only returns only positive LUX values
    //No noise filter, no flat level
    //zhaoyang196673 add for tsl2583 begin
    input_set_abs_params(taos_dev, ABS_MISC, 0, MAX_LUX, 0, 0);
    //zhaoyang196673 add for tsl2583 end

    //And register..
    ret = input_register_device(taos_dev);
    if (ret) {
        printk(KERN_ERR "button.c: Failed to register input_dev\n");
        goto err_free_dev;
    }
#endif	//End of input_event

    printk("ALS detect SUCCESS\n");

    //invoke sysfs
    err = sysfs_create_group(&taos_datap->client->dev.kobj,
                             apds990x_attribute_group);
    if (err < 0) {
        dev_err(&taos_datap->client->dev, "Sysfs registration failed\n");
        goto err_free_dev;
    }

    return (ret); //Normal return

    //In the event we had a problem REGISTERING the device for input_events
err_free_dev:
    input_free_device(taos_dev);
    return (ret);
    //zhaoyang196673 add for tsl2583 end
    return (ret);
}

/**
 * Client remove
 */
static int __devexit taos_remove(struct i2c_client *client)
{
    return (0);
}

/**
 * Device open function
 */
static int taos_open(struct inode *inode, struct file *file)
{
    int ret = 0;
    struct taos_data *taos_datap;

    //For debug - Make it ez to c on the console
#ifdef TAOS_DEBUG
    printk("***************************************************\n");
    printk("*                                                 *\n");
    printk("*                  TAOS DEVICE                    *\n");
    printk("*                    DRIVER                       *\n");
    printk("*                     OPEN                        *\n");
    printk("***************************************************\n");
#endif

    taos_datap = container_of(inode->i_cdev, struct taos_data, cdev);
    if (strcmp(taos_datap->taos_name, DEVICE_ID) != 0) {
        dev_err(devp, "device name error in taos_open(), shows %s\n",
                taos_datap->taos_name);
        return (-ENODEV);
    }

    device_released = 0;
    return (ret);
}

/**
 * Run-time interrupt handler - depending on whether the device is in ambient
 * light sensing interrupt mode, this handler queues up
 * the bottom-half tasklet, to handle all valid interrupts.
 *
 * \param 	int	IRQ number from H/W
 * \param 	void*	*dev_id	pointer to device id - not used
 * \return 	 IRQ_HANDLED
 */
irqreturn_t taos_interrupt(int irq, void *dev_id)
{
    int ret;

    if (taos_wq0) {
        ret = queue_work( taos_wq0, (struct work_struct *)als );
    }
#ifdef TAOS_DEBUG
    else
        printk(KERN_INFO "\nNO taos_wq0\n\n");

    printk(KERN_INFO "Device interrupt\n");
#endif
    return (IRQ_HANDLED); //Work queued - we're outa here
}

/**
 * Device release
 */
static int taos_release(struct inode *inode, struct file *file)
{
    int ret = 0;
    struct taos_data *taos_datap;

    device_released = 1;
    taos_datap = container_of(inode->i_cdev, struct taos_data, cdev);
    if (strcmp(taos_datap->taos_name, DEVICE_ID) != 0) {
        dev_err(devp, "device id incorrect in taos_release(), shows "
                "%s\n", taos_datap->taos_name);
        ret = -ENODEV;
    }
    taos_fasync(-1, file, 0);
    free_irq(irqnum, taos_datap);
    return (ret);
}

/**
 * Name verification - uses default register values to identify the Taos device
 */
static int taos_device_name(unsigned char *bufp, char **device_name)
{
    if (((bufp[0x12] & 0xf0) == 0x00) || (bufp[0x12] == 0x08)) {
        *device_name = "tritonFN";
        return (1);
    } else if (((bufp[0x12] & 0xf0) == 0x90)) {
        *device_name = "skateFN";
        return (1);
    }
    *device_name = "unknown";
    printk(KERN_INFO "Identified %s\n",*device_name);
    return (0);
}

/**
 * Device read - reads are permitted only within the range of the accessible
 * registers of the device. The data read is copied safely to user space.
 */
static int taos_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
    int ret = 0;
    struct taos_data *taos_datap;
    u8 my_buf[MAX_DEVICE_REGS];
    u8 reg, i = 0, xfrd = 0;

    //Make sure we not in the middle of accessing the device
    if (mutex_trylock(&als_mutex) == 0) {
        printk(KERN_INFO "Can't get ALS mutex\n");
        return (-1); // busy, so return LAST VALUE
    }

    if ((*ppos < 0) || (*ppos >= MAX_DEVICE_REGS) || (count > MAX_DEVICE_REGS)) {
        dev_err(devp, "reg limit check failed in taos_read()\n");
        mutex_unlock(&als_mutex);
        return (-EINVAL);
    }
    reg = (u8) * ppos;
    taos_datap = container_of(file->f_dentry->d_inode->i_cdev,
                              struct taos_data, cdev);
    while (xfrd < count) {
        if ((ret = (i2c_smbus_write_byte(taos_datap->client, (CMD_REG | reg))))
            < 0) {
            dev_err(devp, "i2c_smbus_write_byte to cmd reg failed "
                    "in taos_read(), err = %d\n", ret);
            mutex_unlock(&als_mutex);
            return (ret);
        }
        my_buf[i++] = i2c_smbus_read_byte(taos_datap->client);
        reg++;
        xfrd++;
    }
    if ((ret = copy_to_user(buf, my_buf, xfrd))) {
        dev_err(devp, "copy_to_user failed in taos_read()\n");
        mutex_unlock(&als_mutex);
        return (-ENODATA);
    }

    mutex_unlock(&als_mutex);
    return ((int) xfrd);
}

/**
 * Device write - writes are permitted only within the range of the accessible
 * registers of the device. The data written is copied safely from user space.
 */
static int taos_write(struct file *file, const char *buf, size_t count,
                      loff_t *ppos)
{
    int ret = 0;
    struct taos_data *taos_datap;
    u8 my_buf[MAX_DEVICE_REGS];
    u8 reg, i = 0, xfrd = 0;

    if (mutex_trylock(&als_mutex) == 0) {
        printk(KERN_INFO "Can't get ALS mutex\n");
        return (-1); // busy, so return LAST VALUE
    }

    if ((*ppos < 0) || (*ppos >= MAX_DEVICE_REGS) || ((*ppos + count)
            > MAX_DEVICE_REGS)) {
        dev_err(devp, "reg limit check failed in taos_write()\n");
        mutex_unlock(&als_mutex);
        return (-EINVAL);
    }
    reg = (u8) * ppos;
    if ((ret = copy_from_user(my_buf, buf, count))) {
        dev_err(devp, "copy_from_user failed in taos_write()\n");
        mutex_unlock(&als_mutex);
        return (-ENODATA);
    }
    taos_datap = container_of(file->f_dentry->d_inode->i_cdev,
                              struct taos_data, cdev);
    while (xfrd < count) {
        if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (CMD_REG
                                              | reg), my_buf[i++]))) < 0) {
            dev_err(devp, "i2c_smbus_write_byte_data failed in "
                    "taos_write()\n");
            mutex_unlock(&als_mutex);
            return (ret);
        }
        reg++;
        xfrd++;
    }

    mutex_unlock(&als_mutex);
    return ((int) xfrd);
}

/**
 * Device seek - seeks are permitted only within the range of the accessible
 * registers of the device. The new offset is returned.
 */
static loff_t taos_llseek(struct file *file, loff_t offset, int orig)
{
    loff_t new_pos;

    if ((offset >= MAX_DEVICE_REGS) || (orig < 0) || (orig > 1)) {
        dev_err(devp, "offset param limit or whence limit check failed "
                "in taos_llseek()\n");
        return (-EINVAL);
    }
    switch (orig) {
        case 0:
            new_pos = offset;
            break;
        case 1:
            new_pos = file->f_pos + offset;
            break;
        default:
            return (-EINVAL);
            break;
    }
    if ((new_pos < 0) || (new_pos >= MAX_DEVICE_REGS)) {
        dev_err(devp, "new offset check failed in taos_llseek()\n");
        return (-EINVAL);
    }
    file->f_pos = new_pos;
    return (new_pos);
}

/**
 * Fasync function - called when the FASYNC flag in the file descriptor of the
 * device indicates that a new user program wants to be added to the wait queue
 * of the device - calls fasync_helper() to add the new entry to the queue.
 */
static int taos_fasync(int fd, struct file *file, int mode)
{
    taos_datap = container_of(file->f_dentry->d_inode->i_cdev, struct
                              taos_data, cdev);
    return (fasync_helper(fd, file, mode, &taos_datap->async_queue));
}

/**
 * Provides initial operational parameter defaults.\n
 * These defaults may be changed by the following:\n
 * - system deamon
 * - user application (via ioctl)
 * - directly writing to the taos procfs file
 * - external kernel level modules / applications
 */
void taos_defaults()
{
    //Operational parameters
    taos_cycle_type = LIGHT; // default is ALS only
    taos_settings.als_time = 100; // must be a multiple of 50mS
    taos_settings.als_gain = 0; // this is actually an index into the gain tableXXXX
    // assume clear glass as default
    taos_settings.als_gain_trim = 100; // default gain trim to account for aperture effects???
    taos_settings.als_persistence = 3; // default number of 'out of bounds' b4 interrupt
    taos_settings.als_cal_target = 130; // Known external ALS reading used for calibration???
//    taos_settings.interrupts_enabled = 0; // Interrupts enabled (ALS) 0 = none
    taos_settings.interrupts_enabled = CNTL_ALS_INT_ENBL; // Interrupts enabled (ALS) 0 = none  //zhaoyang changed

    //Initialize ALS data to defaults
    als_cur_info.als_ch0 = 0;
    als_cur_info.als_ch1 = 0;
    als_cur_info.lux = 0;
    //zhaoyang196673 add for tsl2583 begin
    taos_settings.als_thresh_high = 2;
    taos_settings.als_thresh_low = 1;
    //zhaoyang196673 add for tsl2583 end

#ifdef TAOS_DEBUG
    printk    (KERN_INFO "\nDEFAULTS LOADED\n");
#endif
}
EXPORT_SYMBOL( taos_defaults);
//..................................................................................................................
//@}	//End of Initial


//==================================================================================================================
/**@defgroup Tasklets IRQ Tasklets (BH)
 * The sections applies to ALS tasklets.\n
 * These tasklets are performed via queued workspace methods.
 * @{
 */

/**
 * Ambient light transition sense interrupt tasklet - called when the ambient
 * light falls above or below a band of ambient light. A signal is issued to
 * any waiting user mode threads, and the above band is adjusted up or down?
 * The ALS interrupt filter is initially set to 0x00 when ALS_ON is called, to
 * force the first interrupt, after which it is set to the configured value.
 * ALS data is stored into the "als_cur_info" structure.
 * \param 	als	Not used
 * \return	Nothing
 */
void taos_als_int_tasklet(struct work_struct *als)
{
    int ret = 0;
    int i;
    u8 als_int_thresh[4];
    u8 chdata[4];
    unsigned int raw_ch0, raw_ch1, cdelta;

#ifdef TAOS_IRQ_DEBUG
    printk(KERN_INFO "taos_als_int_tasklet\n");
#endif
    //Get the data - this will force an update of the 'als_cur_info' struct.
    taos_get_lux();

    //Clear the status AFTER we get the LUX
    if ((ret = taos_i2c_write_command(CMD_REG | CMD_SPL_FN | CMD_ALS_INTCLR))
        < 0) {
        printk(KERN_INFO "taos_i2c_write_command failed to clear ALS irq, err = %d\n", ret);
    }

    //. . . . . . . . . . . . USER NOTIFICATION . . . . . . . . . . . . . . .
#ifdef ASYNC_NOTIFY
    //Notify the user-land app (if any)
    if(taos_datap->async_queue)
        kill_fasync(&taos_datap->async_queue, SIGIO, POLL_IN);
#endif

#ifdef EVENT_NOTIFY
    input_report_abs(taos_dev, ABS_MISC, (int)als_cur_info.lux);
    input_sync(taos_dev);
#endif
    //. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

    //And finally, re-adjust our upper and lower thresholds
    raw_ch0 = als_cur_info.als_ch0;
    raw_ch1 = als_cur_info.als_ch1;
    if (raw_ch0 == 0) {
        taos_settings.als_thresh_low = 0;
        taos_settings.als_thresh_high = 1;
    } else if (raw_ch0 < 10) {
        taos_settings.als_thresh_low = raw_ch0 -1;
        taos_settings.als_thresh_high = raw_ch0;
    } else {
        //zhaoyang196673 add for tsl2583 begin
        cdelta = (raw_ch0 * 30) / 100;
        //zhaoyang196673 add for tsl2583 end

        taos_settings.als_thresh_low = raw_ch0 - cdelta;
        taos_settings.als_thresh_high = raw_ch0 + cdelta;
        if (taos_settings.als_thresh_high > 0xFFFF)
            taos_settings.als_thresh_high = 0xFFFF;
    }

    als_int_thresh[0] = (taos_settings.als_thresh_low) & 0xFF;
    als_int_thresh[1] = (taos_settings.als_thresh_low >> 8) & 0xFF;
    als_int_thresh[2] = (taos_settings.als_thresh_high) & 0xFF;
    als_int_thresh[3] = (taos_settings.als_thresh_high >> 8) & 0xFF;

    for (i = 0; i < 4; i++) {
        if ((ret = taos_i2c_write((CMD_REG |(ALS_MINTHRESHLO + i)), &als_int_thresh[i]))< 0) {
            printk(KERN_INFO "FAILED: taos_i2c_write to update the ALS LOW THRESH (B).\n");
        }
    }
    //zhaoyang196673 add for tsl2583 end

    //Is this the very first interrupt?
    if (!init_done) { //Yes - so do it one time Pump has been primed
        //Maintain the persistence value that was there if there was one
        if ((ret = taos_i2c_read((CMD_REG |(INTERRUPT)),&chdata[0],1))< 0) {
            printk(KERN_INFO "Failed to get the persistence register value\n");
        }
        if ((chdata[0] & 0x0F) == 0x00) { //We need to set it to something higher
            chdata[0] |= taos_settings.als_persistence; //ALS Interrupt after 'n' consecutive readings out of range
        }
        if ((ret = taos_i2c_write((CMD_REG |INTERRUPT), &chdata[0]))< 0) {
            printk(KERN_INFO "FAILED: taos_i2c_write to update the ALS LOW THRESH (B).\n");
        }

        //Not necessary for Skate but helps other devices
        //Clear the status here again - to force a persistence count update.
        if( (ret=taos_i2c_write_command(CMD_REG|CMD_SPL_FN|CMD_ALS_INTCLR)) < 0) {
            printk(KERN_INFO "taos_i2c_write_command failed to clear ALS irq, err = %d\n", ret);
        }

        init_done = 1;
    }

    return;
}

//..................................................................................................................
//@} //End of group Tasklets

//==================================================================================================================
/**@defgroup IOCTL User ioctl Functions
 * The section applies to 'ioctl' calls used primarily to support user-land applications.\n
 * @{
 */
/**
 * Ioctl functions - each one is documented below.
 */
static int taos_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
                      unsigned long arg)
{
    int ret = 0;
    struct taos_data *taos_datap;
    int tmp;
    u8 reg_val;

    taos_datap = container_of(inode->i_cdev, struct taos_data, cdev);
    switch (cmd) {
            /**
             * - ALS_ON - called to set the device in ambient light sense mode.
             * configured values of light integration time, initial interrupt
             * filter, gain, and interrupt thresholds (if interrupt driven) are
             * initialized. Then power, adc, (and interrupt if needed) are enabled.
             */
        case TAOS_IOCTL_ALS_ON:
            taos_cycle_type = LIGHT;
            printk(KERN_INFO "ALS On\n");
            return (taos_chip_on());
            break;
            /**
             * - ALS_OFF - called to stop ambient light sense mode of operation.
             * Clears the filter history, and clears the control register.
             */
        case TAOS_IOCTL_ALS_OFF:
            taos_chip_off();
            break;

            /**
             * - ALS_DATA - request for current ambient light data. If correctly
             * enabled and valid data is available at the device, the function
             * for lux conversion is called, and the result returned if valid.
             */
        case TAOS_IOCTL_ALS_DATA:
            //Are we actively updating the struct?
            if ((taos_settings.interrupts_enabled & CNTL_ALS_INT_ENBL) == 0x00)
                return (taos_get_lux()); //No - so get fresh data
            else
                return (als_cur_info.lux); //Yes - data is fresh as last change
            break;

            /**
             * - ALS_CALIBRATE - called to run one calibration cycle, during assembly.
             * The lux value under a known intensity light source is used to obtain
             * a "gain trim" multiplier which is to be used to normalize subsequent
             * lux data read from the device, after the calibration is completed.
             * This 'known intensity should be the value of als_cal_target.
             * If not - make is so!
             */
        case TAOS_IOCTL_ALS_CALIBRATE:
            return (taos_als_calibrate(taos_settings.als_cal_target));
            break;

            /**
             * - CONFIG-GET - returns the current device configuration values to the
             * caller. The user mode application can display or store configuration
             * values, for future reconfiguration of the device, as needed.
             * Refer to structure "taos_settings" in '.h' file.
             */
        case TAOS_IOCTL_CONFIG_GET:
            ret = copy_to_user((struct taos_settings *) arg, &taos_settings,
                               sizeof(struct taos_settings));
            if (ret) {
                dev_err(devp, "copy_to_user() failed in ioctl config_get\n");
                return (-ENODATA);
            }
            return (ret);
            break;

            /**
             * - GET_ALS - returns the current ALS structure values to the
             * caller. The values returned represent the last call to
             * taos_get_lux().  This ioctl can be used when the driver
             * is operating in the interrupt mode.
             * Refer to structure "als_cur_info" in '.h' file.
             */
        case TAOS_IOCTL_GET_ALS:
            ret = copy_to_user((struct taos_als_info *) arg, &als_cur_info,
                               sizeof(struct taos_als_info));
            if (ret) {
                dev_err(devp,
                        "copy_to_user() failed in ioctl to get taos_als_info\n");
                return (-ENODATA);
            }
            return (ret);
            break;

            /**
             * - CONFIG-SET - used by a user mode application to set the desired
             * values in the driver's in memory copy of the device configuration
             * data. Light integration times are aligned optimally, and driver
             * global variables dependent on configured values are updated here.
             * Refer to structure "taos_settings" in '.h' file.
             */
        case TAOS_IOCTL_CONFIG_SET:
            ret = copy_from_user(&taos_settings, (struct taos_settings *) arg,
                                 sizeof(struct taos_settings));
            if (ret) {
                dev_err(devp, "copy_from_user() failed in ioctl "
                        "config_set\n");
                return (-ENODATA);
            }
            return (ret);
            break;

            /**
             * - LUX_TABLE-GET - returns the current LUX coefficients table to the
             * caller. The user mode application can display or store the table
             * values, for future re-calibration of the device, as needed.
             * Refer to structure "taos_lux" in '.h' file.
             */
        case TAOS_IOCTL_LUX_TABLE_GET:
            ret = copy_to_user((struct taos_lux *) arg, &taos_device_lux,
                               sizeof(taos_device_lux));
            if (ret) {
                dev_err(devp,
                        "copy_to_user() failed in ioctl TAOS_IOCTL_LUX_TABLE_GET\n");
                return (-ENODATA);
            }
            return (ret);
            break;

            /**
             * - LUX TABLE-SET - used by a user mode application to set the desired
             * LUX table values in the driver's in memory copy of the lux table
             * Refer to structure "taos_lux" in '.h' file.
             */
        case TAOS_IOCTL_LUX_TABLE_SET:
            ret = copy_from_user(&taos_lux, (struct taos_lux *) arg,
                                 sizeof(taos_device_lux));
            if (ret) {
                dev_err(devp, "copy_from_user() failed in ioctl "
                        "TAOS_IOCTL_LUX_TABLE_SET\n");
                return (-ENODATA);
            }
            return (ret);
            break;

            /**
             * - TAOS_IOCTL_ID - used to query driver name & version
             */
        case TAOS_IOCTL_ID:
            ret = copy_to_user((char *) arg, &driver_version_id,
                               sizeof(driver_version_id));
            printk(KERN_INFO "%s\n",DRIVER_VERSION_ID);
            return (ret);
            break;

            /**
             * - REGISTER_DUMP - dumps registers to the console device.
             */
        case TAOS_IOCTL_REG_DUMP:

            taos_register_dump();
            return (ret /*TAOS_SUCCESS*/);
            break;

            /*
             * - Enable/Disable ALS interrupt
             *
             */
        case TAOS_IOCTL_INT_SET:
            get_user(tmp, (int *) arg);
            switch (tmp) {
                case 1: //ALS INTERRUPT - ON
#ifdef TAOS_IRQ_DEBUG
                    printk(KERN_INFO "ALS Interrupts on\n");
#endif
                    //Make sure we start off the very 1st interrupt by setting persistence to 0
                    reg_val = 0x00;
                    reg_val |= CNTL_ALS_INT_ENBL; //Set up for Level interrupt
                    //and write it
                    if ((ret = taos_i2c_write((CMD_REG | (INTERRUPT)), &reg_val)) < 0) {
                        printk(KERN_INFO "FAILED taos_i2c_write to update the persistance register.\n");
                    }

                    taos_settings.interrupts_enabled = CNTL_ALS_INT_ENBL;
                    break;

                case 0: //ALS - OFF
                    taos_settings.interrupts_enabled = 0x00;
                    reg_val = 0x01; //Set up for Level interrupt
                    //and write it
                    if ((ret = taos_i2c_write((CMD_REG |(INTERRUPT)), &reg_val))< 0) {
                        printk(KERN_INFO "FAILED: taos_i2c_write to update the Interrupts OFF  register.\n");
                    }
                    taos_chip_off();
                    break;
            }
            return (TAOS_SUCCESS);
            break;

            /**
             * - TAOS_IOCTL_SET_GAIN - used to set/change the device analog gain.
             * The value passed into here from the user is the index into the table.
             * Index = ( 0 - 3) Used to validate the gain selection index
             */
        case TAOS_IOCTL_SET_GAIN:
            get_user(tmp,(int *)arg);
            if (tmp > MAX_GAIN_STAGES)
                return(-1);

            taos_settings.als_gain = tmp;
            if ((ret = taos_i2c_write(CMD_REG|GAIN, (u8 *)&tmp))< 0) {
                printk(KERN_INFO "taos_i2c_write to turn on device failed in taos_chip_on.\n");
                return (-1);
            }
            return (ret);
            break;

        default:
            return (-EINVAL);
            break;
    }

    return (ret);

}
//.............................................................................................................
//@}	//end of IOCTL section


//==================================================================================================================
/**@defgroup ALS Ambient Light Sense (ALS)
 * The section applies to the ALS related functions.\n
 * Other ALS releated functions may appear elsewhere in the code.\n
 * @{
 */

//..................................................................................................................

/**
 * Reads and calculates current lux value.
 * The raw ch0 and ch1 values of the ambient light sensed in the last
 * integration cycle are read from the device.
 * Time scale factor array values are adjusted based on the integration time.
 * The raw values are multiplied by a scale factor, and device gain is obtained
 * using gain index. Limit checks are done next, then the ratio of a multiple
 * of ch1 value, to the ch0 value, is calculated. The array taos_device_luxP[]
 * declared above is then scanned to find the first ratio value that is just
 * above the ratio we just calculated. The ch0 and ch1 multiplier constants in
 * the array are then used along with the time scale factor array values, to
 * calculate the lux.
 *
 *	\param 	none
 *	\return int	-1 = Failure, Lux Value = success
 */
int taos_get_lux(void)
{
    u32 ch0, ch1; /* separated ch0/ch1 data from device */
    s32 lux; /* raw lux calculated from device data */
    u32 ratio;
    u8 buf[5];
    struct taos_lux *p;

    int i, ret;

    s32 ch0lux;
    s32 ch1lux;

    if (mutex_trylock(&als_mutex) == 0) {
        printk(KERN_INFO "Can't get ALS mutex\n");
        return (als_cur_info.lux); // busy, so return LAST VALUE
    }

    if (taos_chip_status != TAOS_CHIP_WORKING) {
        // device is not enabled
        printk(KERN_INFO "device is not enabled\n");
        mutex_unlock(&als_mutex);
        return (-1);
    }

    if ((taos_cycle_type & LIGHT) == 0) {
        // device not in ALS mode
        printk(KERN_INFO "device not in ALS mode\n");
        mutex_unlock(&als_mutex);
        return (-1);
    }

    if ((ret = taos_i2c_read((CMD_REG), &buf[0], 1)) < 0) {
        printk(KERN_INFO "taos_i2c_read() to CMD_REG reg failed in taos_get_lux()\n");
        mutex_unlock(&als_mutex);
        return -1;
    }
    /* is data new & valid */
    if (!(buf[0] & STA_ADCINTR)) {
        printk(KERN_INFO "Data not valid, so return LAST VALUE\n");
        mutex_unlock(&als_mutex);
        return (als_cur_info.lux); // have no data, so return LAST VALUE
    }

    for (i = 0; i < 4; i++) {
        if ((ret = taos_i2c_read((CMD_REG | (ALS_CHAN0LO + i)), &buf[i], 1))
            < 0) {
            printk(KERN_INFO "taos_i2c_read() to (CMD_REG |(ALS_CHAN0LO + i) regs failed in taos_get_lux()\n");
            mutex_unlock(&als_mutex);
            return -1;
        }
    }

    /* clear status, really interrupt status (interrupts are off, but we use the bit anyway */
    if ((ret = taos_i2c_write_command(CMD_REG | CMD_SPL_FN | CMD_ALS_INTCLR))
        < 0) {
        printk(KERN_INFO "taos_i2c_write_command failed in taos_chip_on, err = %d\n", ret);
        mutex_unlock(&als_mutex);
        return -1; // have no data, so return fail
    }

    /* extract ALS/lux data */
    ch0 = (buf[1] << 8) | buf[0];
    ch1 = (buf[3] << 8) | buf[2];

    als_cur_info.als_ch0 = ch0;
    als_cur_info.als_ch1 = ch1;

#ifdef TAOS_DEBUG
    printk(KERN_INFO " ch0=%d/0x%x ch1=%d/0x%x\n",ch0,ch0,ch1,ch1);
#endif

    if ((ch0 >= als_saturation) || (ch1 >= als_saturation)) {
    return_max:
        als_cur_info.lux = MAX_LUX;
        mutex_unlock(&als_mutex);
        return (als_cur_info.lux);
    }

    if (ch0 == 0) {
        als_cur_info.lux = 0;
#ifdef TAOS_DEBUG
        printk(KERN_INFO "ch0==0\n");
#endif
        mutex_unlock(&als_mutex);
        return (als_cur_info.lux); // have no data, so return LAST VALUE
    }

    /* calculate ratio */
    ratio = (ch1 << 15) / ch0;

    /* convert to unscaled lux using the pointer to the table */
    for (p = (struct taos_lux *) taos_device_lux; p->ratio != 0 && p->ratio< ratio; p++)
        ;

    if (p->ratio == 0) {
#ifdef TAOS_DEBUG
        printk(KERN_INFO "ratio = %04x   p->ratio = %04x\n",ratio,p->ratio);
#endif
        lux = 0;
    } else { //	    lux = ch0*p->ch0 - ch1*p->ch1;
        ch0lux = ((ch0 * p->ch0) + (gainadj[taos_settings.als_gain].ch0 >> 1))
                 / gainadj[taos_settings.als_gain].ch0;
        ch1lux = ((ch1 * p->ch1) + (gainadj[taos_settings.als_gain].ch1 >> 1))
                 / gainadj[taos_settings.als_gain].ch1;
        lux = ch0lux - ch1lux;
    }

    /* note: lux is 31 bit max at this point */
    if (lux < 0) {
        printk(KERN_INFO "No Data - Return last value\n");
        als_cur_info.lux = 0;
        mutex_unlock(&als_mutex);
        return (als_cur_info.lux); // have no data, so return LAST VALUE
    }
    /* adjust for active time scale */
    lux = (lux + (als_time_scale >> 1)) / als_time_scale;
    /* adjust for active gain scale */
    lux >>= 13; /* tables have factor of 8192 builtin for accuracy */

    lux = (lux * taos_settings.als_gain_trim + 50) / 100;

#ifdef TAOS_DEBUG
    printk(KERN_INFO "lux=%d\n",lux);
#endif
    if (lux > MAX_LUX) /* check for overflow */
        goto return_max;

    als_cur_info.lux = lux; //Update the structure with the latest VALID lux.
    mutex_unlock(&als_mutex);
    return (lux);
}
EXPORT_SYMBOL( taos_get_lux);

/**
 * Obtain single reading and calculate the als_gain_trim (later used to derive actual lux)
 *\param   ulong	als_cal_target	ALS target (real world)
 *\return  int		updated gain_trim value
 */
int taos_als_calibrate(unsigned long als_cal_target)
{
    u8 reg_val;
    unsigned int gain_trim_val;
    int ret;
    int lux_val;

    //This may seem redundant but..
    //We need this next line in case we call this function from an external application who has
    //a different target value than our defaults.
    //In which case, make 'that' our new default/settings.
    taos_settings.als_cal_target = als_cal_target;

    if ((ret = (i2c_smbus_write_byte(taos_datap->client, (CMD_REG | CNTRL))))
        < 0) {
        dev_err(devp,
                "i2c_smbus_write_byte to cmd reg failed in ioctl als_calibrate\n");
        return (ret);
    }
    reg_val = i2c_smbus_read_byte(taos_datap->client);

    if ((reg_val & (CNTL_ADC_ENBL | CNTL_PWRON))
        != (CNTL_ADC_ENBL | CNTL_PWRON)) {
        dev_err(
            devp,
            "reg_val & (CNTL_ADC_ENBL | CNTL_PWRON)) != (CNTL_ADC_ENBL | CNTL_PWRON) in ioctl als_calibrate\n");
        return (-ENODATA);
    }

    if ((ret = (i2c_smbus_write_byte(taos_datap->client, (CMD_REG | STATUS))))
        < 0) {
        dev_err(devp,
                "i2c_smbus_write_byte to cmd reg failed in ioctl als_calibrate\n");
        return (ret);
    }
    reg_val = i2c_smbus_read_byte(taos_datap->client);

    if ((reg_val & STA_ADCVALID) != STA_ADCVALID) {
        dev_err(devp,
                "(reg_val & STA_ADCVALID) != STA_ADCVALID in ioctl als_calibrate\n");
        return (-ENODATA);
    }

    if ((lux_val = taos_get_lux()) < 0) {
        dev_err(
            devp,
            "taos_get_lux() returned bad value %d in ioctl als_calibrate\n",
            lux_val);
        return (lux_val);
    }
    gain_trim_val = (unsigned int) (((taos_settings.als_cal_target)
                                     * taos_settings.als_gain_trim) / lux_val);

    printk(KERN_INFO "\n\ntaos_settings.als_cal_target = %d\ntaos_settings.als_gain_trim = %d\nlux_val = %d\n",
           taos_settings.als_cal_target,taos_settings.als_gain_trim,lux_val);

    if ((gain_trim_val < 25) || (gain_trim_val > 400)) {
        dev_err(
            devp,
            "ALS calibrate result %d out of range in ioctl als_calibrate\n",
            gain_trim_val);
        return (-ENODATA);
    }
    taos_settings.als_gain_trim = (int) gain_trim_val;

    return ((int) gain_trim_val);
}

EXPORT_SYMBOL( taos_als_calibrate);

//==================================================================================================================
/**@defgroup DEV_CTRL Device Control Functions
 * @{
 */

//..................................................................................................................
/**
 * Turn the device on.
 * Configuration and taos_cycle_type must be set before calling this function.
 * \param 	none.
 * \return 	int	0 = success, < 0 = failure
 */
int taos_chip_on()
{
    int i;
    int ret = 0;
    u8 *uP;
    u8 utmp;
    int als_count;
    int als_time;

    //make sure taos_cycle_type is set.
    if ((taos_cycle_type != LIGHT)) {
        return (-1);
    }

    //and make sure we're not already on
    if (taos_chip_status == TAOS_CHIP_WORKING) {
        // if forcing a register update - turn off, then on
        printk(KERN_INFO "device is already enabled\n");
        return (-1);
    }

    //. . . . . . . . . . . . . . . SHADOW REGISTER INITIALIZATION . . . . . . . . . . .
    //Note:
    //If interrupts are enabled, keeping persist at 0 should prime the pump.
    //So don't set persist in shadow register. Just enable if required.
    taos_config[INTERRUPT] = (taos_settings.interrupts_enabled & 0xF0);

    // determine als integration regster
    als_count = (taos_settings.als_time * 100 + 135) / 270;
    if (als_count == 0)
        als_count = 1; // ensure at least one cycle

    //LIGHT
    // convert back to time (encompasses overrides)
    if (taos_cycle_type & LIGHT) {
        als_time = (als_count * 27 + 5) / 10;
        taos_config[ALS_TIME] = 256 - als_count;
    } else
        taos_config[ALS_TIME] = 0xff;

    //Set the gain based on taos_settings struct
    taos_config[GAIN] = taos_settings.als_gain;

    // set globals re scaling and saturation
    als_saturation = als_count * 922; // 90% of full scale
    als_time_scale = als_time / 50;

    //. . . . . . . . . . . . . . . . . . . . . . . . . . .  . . . . .  . . . . . . . . . . .
    //SKATE Specific power-on / adc enable sequence
    // Power on the device 1st.
    utmp = CNTL_PWRON;
    //
    //

    if ((ret = taos_i2c_write(CMD_REG | CNTRL, &utmp)) < 0) {
        printk(KERN_INFO "taos_i2c_write to turn on device failed in taos_chip_on.\n");
        return (-1);
    }

    //User the following shadow copy for our delay before enabling ADC
    //Write ALL THE REGISTERS
    for (i = 0, uP = taos_config; i < TAOS_REG_MAX; i++) {
        if ((ret = taos_i2c_write(CMD_REG + i, uP++)) < 0) {
            printk(KERN_INFO "taos_i2c_write to reg %d failed in taos_chip_on.\n",i);
            return (-1);
        }
    }

    //NOW enable the ADC
    // initialize the desired mode of operation
    utmp = CNTL_PWRON | CNTL_ADC_ENBL;
    //
    //
    if ((ret = taos_i2c_write(CMD_REG | CNTRL, &utmp)) < 0) {
        printk(KERN_INFO "taos_i2c_write to turn on device failed in taos_chip_on.\n");
        return (-1);
    }
    taos_chip_status = TAOS_CHIP_WORKING;

#ifdef TAOS_DEBUG
    printk(KERN_INFO "chip_on() called\n\n");
#endif

    return (ret); // returns result of last i2cwrite
}
EXPORT_SYMBOL( taos_chip_on);

//..................................................................................................................
/**
 * Turn the device OFF.
 * \param	none.
 * \return	int	0 = success, < 0 = failure
 */
int taos_chip_off()
{
    int ret;
    u8 utmp;

    // turn device off
    taos_chip_status = TAOS_CHIP_SLEEP;
    //zhaoyang196673 add for tsl2583 begin
    ret = taos_i2c_write_command(0xE1);
    assert(ret == 0);
    //zhaoyang196673 add for tsl2583 end

    utmp = 0x00;
    ret = taos_i2c_write(CMD_REG | CNTRL, &utmp);
    als_on = FALSE;
    init_done = 0x00; //used by ALS irq as one/first shot
#ifdef TAOS_DEBUG
    printk(KERN_INFO "chip_off() called\n");
#endif

    return (ret);

}
EXPORT_SYMBOL( taos_chip_off);
//zhaoyang196673 add for tsl2583 begin
static int taos_suspend(struct i2c_client *client, pm_message_t mesg)
{
    return taos_chip_off();
}

static int taos_resume(struct i2c_client *client)
{
    return taos_chip_on();
}
//zhaoyang196673 add for tsl2583 end
//..................................................................................................................
//@} End of Device Control Section


//==================================================================================================================

//==================================================================================================================
/**@defgroup I2C-HAL I2C/SMBus Communication Functions
 * The sections pertains to the driver/device communication functions\n
 * facilitated via SMBus in an I2C manner.\n
 * These functions represent several layers of abstraction.
 * Also note, some i2c controllers do not support block read/write.
 * @{
 */

//  Abstraction layer calls
/**
 * Read a number of bytes starting at register (reg) location.
 *
 * \param 	unsigned char 	reg - device register
 * \param 	unsigned char	*val - location to store results
 * \param 	unsigned int 	number of bytes to read
 * \return	TAOS_SUCCESS, or i2c_smbus_write_byte ERROR code
 *
 */
int taos_i2c_read(u8 reg, u8 *val, unsigned int len)
{
    int result;
    int i;

    if (len > MAXI2C)
        len = MAXI2C;
    for (i = 0; i < len; i++) {
        /* select register to write */
        if (reg >= 0) {
            if ((result = (i2c_smbus_write_byte(taos_datap->client, (CMD_REG
                                                | (reg + i))))) < 0) {
                dev_err(devp,
                        "FAILED: i2c_smbus_write_byte_data in taos_io_read()\n");
                return (result);
            }
            /* read the data */
            *val = i2c_smbus_read_byte(taos_datap->client);
#ifdef TAOS_DEBUG_SMBUS
            printk(KERN_INFO "READ FROM REGISTER [%02X] -->%02x<--\n",(CMD_REG | (reg + i)) ,*data);
#endif
            val++;
        }
    }
    return (TAOS_SUCCESS);
}

/**
 * This function is used to send a an register address followed by a data value
 * When calling this function remember to set the register MSBit (if applicable).
 * \param  unsigned char register
 * \param  unsigned char * pointer to data value(s) to be written
 * \return TAOS_SUCCESS, or i2c_smbus_write_byte error code.
 */
int taos_i2c_write(u8 reg, u8 *val)
{
    int retval;
    if ((retval = (i2c_smbus_write_byte_data(taos_datap->client, reg, (*val))))
        < 0) {
        dev_err(devp, "FAILED: i2c_smbus_write_byte_data\n");
        return (retval);
    }

    return (TAOS_SUCCESS);
}

/**
 * This function is used to send a command to device command/control register
 * All bytes sent using this command have their MSBit set - it's a command!
 * \param  unsigned char register to write
 * \return TAOS_SUCCESS, or i2c_smbus_write_byte error code.
 */
int taos_i2c_write_command(u8 reg)
{
    int result;
    u8 *p;
    u8 buf;

    p = &buf;
    *p = reg |= CMD_REG;
    // write the data
    if ((result = (i2c_smbus_write_byte(taos_datap->client, (*p)))) < 0) {
        dev_err(devp, "FAILED: i2c_smbus_write_byte\n");
        return (result);
    }
    return (TAOS_SUCCESS);
}

//..................................................................................................................
//@}	//end of I2C-HAL section


//==================================================================================================================
/**@defgroup group5 Ancillary Functions
 * The following functions are for use within the context of of the driver module.\n
 * These functions are not intended to called externally.  These functions are\n
 * not exported.\n
 * @{
 */

/**
 * Integer Square Root
 * We need an integer version since 1st Floating point is not allowed in driver world, 2nd, cannot
 * count on the devices having a FPU, and 3rd software FP emulation may be excessive.
 */
unsigned long taos_isqrt(unsigned long x)
{
    register unsigned long op, res, one;
    op = x;
    res = 0;

    // "one" starts at the highest power of four <= than the argument.
    one = 1 << 30; // second-to-top bit set
    while (one > op)
        one >>= 2;

    while (one != 0) {
        if (op >= res + one) {
            op -= res + one;
            res += one << 1;
        }
        res >>= 1;
        one >>= 2;
    }
    return res;
}

/**
 * Simple dump of device registers to console via printk.
 */
int taos_register_dump(void)
{
    int i = 0;
    int ret = 0;
    u8 chdata[0x20];

    if (mutex_trylock(&als_mutex) == 0) {
        printk(KERN_INFO "Register Dump: Can't get ALS mutex\n");
        return (-1);
    }

    for (i = 0; i < 0x20; i++) {
        if ((ret = taos_i2c_read((CMD_REG | (CNTRL + i)), &chdata[i], 1)) < 0) {
            printk(KERN_INFO "i2c_smbus_read_byte() failed in taos_register_dump()\n"); //dev_err(devp, "");
            mutex_unlock(&als_mutex);
            return (-1);
        }
    }

    printk(KERN_INFO "** taos register dump *** \n");
    for (i = 0; i < 0x24; i += 2) {
        printk(KERN_INFO "(0x%02x)0x%02x, (0x%02x)0x%02x", i, chdata[i], (i+1), chdata[(i+1)]);
    }
    printk(KERN_INFO "\n");

    mutex_unlock (&als_mutex);
    return 0;
}

//@}	//End of ancillary documentation - gp5
//==================================================================================================================

/**@defgroup group6 Sysfs Interface Functions
 * The following functions are for access via the sysfs style interface.\n
 * Use 'cat' to read, 'echo >' to write\n
 * @{
 */

//..................................................................................................................
// sysfs interface functions begin here

static ssize_t taos_device_id(struct device *dev,
                              struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%s %s\n", DEVICE_ID, DRIVER_VERSION_ID);
}

static ssize_t taos_power_state_show(struct device *dev,
                                     struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", taos_chip_status);
    return 0;
}

static ssize_t taos_power_state_store(struct device *dev,
                                      struct device_attribute *attr, const char *buf, size_t len)
{
    unsigned long value;
    int ret;

    if (strict_strtoul(buf, 0, &value)) {
        return -EINVAL;
    }
    if (value == 1) {
        ret = taos_chip_on();
    } else {
        ret = taos_chip_off();
    }
    return len;
}

static ssize_t taos_gain_show(struct device *dev,
                              struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", taos_settings.als_gain);
    return 0;
}

static ssize_t taos_gain_store(struct device *dev,
                               struct device_attribute *attr, const char *buf, size_t len)
{
    unsigned long value;
    if (strict_strtoul(buf, 0, &value)) {
        return -EINVAL;
    }
    if (value) {
        if (value > 4) {
            printk(KERN_INFO "Invalid Gain Index\n");
            return (-1);
        } else {
            taos_settings.als_gain = value;
        }
    }
    return len;
}

static ssize_t taos_interrupt_show(struct device *dev,
                                   struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", taos_settings.interrupts_enabled);
    return 0;
}

static ssize_t taos_interrupt_store(struct device *dev,
                                    struct device_attribute *attr, const char *buf, size_t len)
{
    unsigned long value;
    if (strict_strtoul(buf, 0, &value)) {
        return -EINVAL;
    }
    if (value) {
        if (value > 1) {
            printk(KERN_INFO "Invalid: 0 or 1\n");
            return (-1);
        } else {
            taos_settings.interrupts_enabled = CNTL_ALS_INT_ENBL;
        }
    }
    return len;
}

static ssize_t taos_als_time_show(struct device *dev,
                                  struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", taos_settings.als_time);
    return 0;
}

static ssize_t taos_als_time_store(struct device *dev,
                                   struct device_attribute *attr, const char *buf, size_t len)
{
    unsigned long value;
    if (strict_strtoul(buf, 0, &value)) {
        return -EINVAL;
    }
    if (value) {
        taos_settings.als_time = value;
    }
    return len;
}

static ssize_t taos_als_trim_show(struct device *dev,
                                  struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", taos_settings.als_gain_trim);
    return 0;
}

static ssize_t taos_als_trim_store(struct device *dev,
                                   struct device_attribute *attr, const char *buf, size_t len)
{
    unsigned long value;
    if (strict_strtoul(buf, 0, &value)) {
        return -EINVAL;
    }
    if (value) {
        taos_settings.als_gain_trim = value;
    }
    return len;
}

static ssize_t taos_als_persistence_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", taos_settings.als_persistence);
    return 0;
}

static ssize_t taos_als_persistence_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t len)
{
    unsigned long value;
    if (strict_strtoul(buf, 0, &value)) {
        return -EINVAL;
    }
    if (value) {
        taos_settings.als_persistence = value;
    }
    return len;
}

static ssize_t taos_als_cal_target_show(struct device *dev,
                                        struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", taos_settings.als_cal_target);
    return 0;
}

static ssize_t taos_als_cal_target_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t len)
{
    unsigned long value;
    if (strict_strtoul(buf, 0, &value)) {
        return -EINVAL;
    }
    if (value) {
        taos_settings.als_cal_target = value;
    }
    return len;
}

static ssize_t taos_lux_show(struct device *dev, struct device_attribute *attr,
                             char *buf)
{
    int lux = 0;
    if ((taos_settings.interrupts_enabled & CNTL_ALS_INT_ENBL) == 0x00)
        lux = taos_get_lux(); //get fresh data
    else
        lux = als_cur_info.lux; //data is fresh as last change

    return sprintf(buf, "%d\n", lux);
    return 0;
}

static ssize_t taos_do_calibrate(struct device *dev,
                                 struct device_attribute *attr, const char *buf, size_t len)
{
    unsigned long value;
    if (strict_strtoul(buf, 0, &value)) {
        return -EINVAL;
    }
    if (value == 1) {
        taos_als_calibrate(taos_settings.als_cal_target);
    }
    return len;
}

static ssize_t taos_als_thresh_low_show(struct device *dev,
                                        struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", taos_settings.als_thresh_low);
    return 0;
}

static ssize_t taos_als_thresh_low_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t len)
{
    unsigned long value;
    if (strict_strtoul(buf, 0, &value)) {
        return -EINVAL;
    }
    if (value) {
        taos_settings.als_thresh_low = value;
    }
    return len;
}

static ssize_t taos_als_thresh_high_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", taos_settings.als_thresh_high);
    return 0;
}

static ssize_t taos_als_thresh_high_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t len)
{
    unsigned long value;
    if (strict_strtoul(buf, 0, &value)) {
        return -EINVAL;
    }
    if (value) {
        taos_settings.als_thresh_high = value;
    }
    return len;
}

//@}	//End of Sysfs Interface Functions - gp6
//==================================================================================================================


MODULE_AUTHOR("Jon Brenner<jbrenner@taosinc.com>");
MODULE_DESCRIPTION("TAOS 258x ambient light sensor driver");
MODULE_LICENSE("GPL");

module_init( taos_init);
module_exit( taos_exit);

