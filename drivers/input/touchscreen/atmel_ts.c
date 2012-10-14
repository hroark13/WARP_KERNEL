/*
 *  Atmel maXTouch Touchscreen Controller Driver
 * 
 *  
 *  Copyright (C) 2010 Atmel Corporation
 *  Copyright (C) 2009 Raphael Derosso Pereira <raphaelpereira@gmail.com>
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

                        EDIT HISTORY FOR ATMEL TOUCHSCREEN

when               comment tag       who                  what, where, why                           
----------     -----------      -----------     --------------------------                
2011/03/17     wangweiping0007       wangweiping      modify for ts controler will lost msgs under some conditions using muti-touch
2011/05/24     wangweiping0010       wangweiping      modify for re-optimize ts driver code in order to apply all platform
2011/06/01     wangweiping0013       wangweiping      modify touchscreen probe for optimize ts code
2011/06/09     wangweiping0017       wangweiping      modify touchscreen for 7" calibration when power on or resume
2011/09/23     yuehongliang0005      yuehongliang     added check calibration for 10" touchscreen
2011/09/27     yuehongliang0006      yuehongliang     added for get touchscreen infomation
2011/10/19     yuehongliang0007      yuehongliang     modify for bending issues and charger noise suppression
2011/10/20     yuehongliang0008      yuehongliang     added for sliding on touchscreen restore sensitivity 
2011/10/29     yuehongliang0009      yuehongliang     modify for shutdown power when sleep
2011/11/01     yuehongliang0010      yuehongliang     modify for 10 inch touchscreen power when sleep
2011/11/23     yuehongliang0011      yuehongliang     decline senstivitiy for 10 inch touchscreen
===========================================================================*/

/*
 * 
 * Driver for Atmel maXTouch family of touch controllers.
 *
 */
#include <linux/input/mt.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/debugfs.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <asm/uaccess.h>
#include <linux/atmel_ts.h>
#include <linux/delay.h> 
#include <linux/timer.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <mach/hw_ver.h> 
#if defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#endif
#define DRIVER_VERSION "0.9a"
//#undef ABS_MT_TRACKING_ID
//#define ABS_MT_TRACKING_ID
#define CHECK_TS_CONFIG_DATA_ZTE

#define GET_TOUCHSCREEN_INFORMATION

static int debug = DEBUG_TRACE;
static int comms = 0; 

static int8_t cal_check_flag = 0u;
static int report_message_flag;

module_param(debug, int, 0644);
module_param(comms, int, 0644);

MODULE_PARM_DESC(debug, "Activate debugging output");
MODULE_PARM_DESC(comms, "Select communications mode");
//static struct class *touch_class;
/* Device Info descriptor */
/* Parsed from maXTouch "Id information" inside device */
struct mxt_device_info {
        u8   family_id;
        u8   variant_id;
        u8   major;
        u8   minor;
        u8   build;
        u8   num_objs;
        u8   x_size;
        u8   y_size;
        char family_name[16];	 /* Family name */
        char variant_name[16];    /* Variant name */
        u16  num_nodes;           /* Number of sensor nodes */
};

/* object descriptor table, parsed from maXTouch "object table" */
struct mxt_object {
        u16 chip_addr;
        u8  type;
        u8  size;
        u8  instances;
        u8  num_report_ids;
};
/* Mapping from report id to object type and instance */
struct report_id_map {
        u8  object;
        u8  instance;
        /*
        * This is the first report ID belonging to object. It enables us to
        * find out easily the touch number: each touch has different report
        * ID (which are assigned to touches in increasing order). By
        * subtracting the first report ID from current, we get the touch
        * number.
        */
        u8  first_rid;
};

struct atmel_finger_data {
        int x;
        int y;
        int w;
        int z;
};
struct mxt_finger {
	int status;
	int x;
	int y;
	int area;
	int pressure;
};
/* Driver datastructure */
struct mxt_data {
        struct i2c_client              *client;
        struct input_dev           *input;
        //struct input_dev *input_dev;
		char                                phys_name[32];
        struct workqueue_struct  *atmel_wq;
        struct work_struct           work;
        int                                   irq;
		struct mxt_finger finger[MXT_MAX_NUM_TOUCHES];
        struct timer_list              timer;
        u16                                 last_read_addr;
        int                                   message_counter;
        int                                   read_fail_counter;
        u8                                   numtouch;

        u8                                   finger_count;
        u16                                 finger_pressed;
        struct atmel_finger_data  finger_data[MXT_MAX_NUM_TOUCHES];
              
        struct mxt_device_info    device_info;

        u32	                                info_block_crc;
        u32                                 configuration_crc;
        u16                                 report_id_count;
        struct report_id_map      *rid_map;
        struct mxt_object           *object_table;

        u16                                msg_proc_addr;
        u8                                  message_size;

        u16                                max_x_val;
        u16                                max_y_val;
        
        int                                  pre_data[3];
        uint8_t                            calibration_confirm;
        unsigned int                     timestamp;//uint64_t                          timestamp;

        int                   (*init_hw)(void);
        int                   (*exit_hw)(void);
        int                   (*read_chg)(void);

        int                   (*power_on)(int);

#if defined(CONFIG_HAS_EARLYSUSPEND)
        struct early_suspend	early_suspend;
#endif		
};


#define I2C_RETRY_COUNT 5
#define I2C_PAYLOAD_SIZE 254

#define	TOUCHSCREEN_TIMEOUT	(msecs_to_jiffies(1000))

/* Maps a report ID to an object type (object type number). */
#define	REPORT_ID_TO_OBJECT(rid, mxt)			\
	(((rid) == 0xff) ? 0 : mxt->rid_map[rid].object)

/* Maps a report ID to an object type (string). */
#define	REPORT_ID_TO_OBJECT_NAME(rid, mxt)			\
	object_type_name[REPORT_ID_TO_OBJECT(rid, mxt)]

/* Returns non-zero if given object is a touch object */
#define IS_CONFIG_OBJECT(object) \
        ((object == MXT_GEN_POWERCONFIG_T7) || \
         (object == MXT_GEN_ACQUIRECONFIG_T8) || \
         (object == MXT_TOUCH_MULTITOUCHSCREEN_T9) || \
         (object == MXT_TOUCH_KEYARRAY_T15) || \
         (object == MXT_SPT_COMMSCONFIG_T18) || \
         (object == MXT_PROCI_GRIPFACESUPPRESSION_T20) || \
         (object == MXT_PROCG_NOISESUPPRESSION_T22) || \
         (object == MXT_PROCI_ONETOUCHGESTUREPROCESSOR_T24) || \
         (object == MXT_SPT_SELFTEST_T25) || \
         (object == MXT_PROCI_TWOTOUCHGESTUREPROCESSOR_T27) || \
         (object == MXT_SPT_CTECONFIG_T28) || \
         (object == MXT_PROCI_GRIPSUPPRESSION_T40) || \
         (object == MXT_PROCI_PALMSUPPRESSION_T41) || \
         (object == MXT_SPT_DIGITIZER_T43) ? 1 : 0)
         
#define mxt_debug(level, ...) \
        do { \
        	if (debug >= (level)) \
        		pr_debug(__VA_ARGS__); \
        } while (0) 

/* 
 * Check whether we have multi-touch enabled kernel; if not, report just the
 * first touch (on mXT224, the maximum is 10 simultaneous touches).
 * Because just the 1st one is reported, it might seem that the screen is not
 * responding to touch if the first touch is removed while the screen is being
 * touched by another finger, so beware. 
 *
 * TODO: investigate if there is any standard set of input events that uppper
 * layers are expecting from a touchscreen? These can however be different for
 * different platforms, and customers may have different opinions too about
 * what should be interpreted as right-click, for example. 
 *
 */

static inline void report_gesture(int data, struct mxt_data *mxt)
{
        input_event(mxt->input, EV_MSC, MSC_GESTURE, data); 
}

static const u8	*object_type_name[] = {
        [0]  = "Reserved",
        [5]  = "GEN_MESSAGEPROCESSOR_T5",
        [6]  = "GEN_COMMANDPROCESSOR_T6",
        [7]  = "GEN_POWERCONFIG_T7",
        [8]  = "GEN_ACQUIRECONFIG_T8",
        [9]  = "TOUCH_MULTITOUCHSCREEN_T9",
        [15] = "TOUCH_KEYARRAY_T15",
        [18] = "SPT_COMMSCONFIG_T18",
        [19] = "SPT_GPIOPWM_T19",
        [20] = "PROCI_GRIPFACESUPPRESSION_T20",
        [22] = "PROCG_NOISESUPPRESSION_T22",
        [23] = "TOUCH_PROXIMITY_T23",
        [24] = "PROCI_ONETOUCHGESTUREPROCESSOR_T24",
        [25] = "SPT_SELFTEST_T25",
        [27] = "PROCI_TWOTOUCHGESTUREPROCESSOR_T27",
        [28] = "SPT_CTECONFIG_T28",
        [37] = "DEBUG_DIAGNOSTICS_T37",
        [38] = "SPT_USER_DATA_T38",
        [40] = "PROCI_GRIPSUPPRESSION_T40",
        [41] = "PROCI_PALMSUPPRESSION_T41",
        [42] = "PROCI_FACESUPPRESSION_T42",
        [43] = "SPT_DIGITIZER_T43",
        [44] = "SPT_MESSAGECOUNT_T44",
};

/*
 * Reads a block of bytes from given address from mXT chip. If we are
 * reading from message window, and previous read was from message window,
 * there's no need to write the address pointer: the mXT chip will
 * automatically set the address pointer back to message window start.
 */
static int mxt_read_block(struct i2c_client *client,
		   u16 addr,
		   u16 length,
		   u8 *value)
{
        struct i2c_adapter *adapter = client->adapter;
        struct i2c_msg msg[2];
        __le16	le_addr;
        struct mxt_data *mxt;

        mxt = i2c_get_clientdata(client);

        if (mxt != NULL) 
        {
                if ((mxt->last_read_addr == addr) &&
                	(addr == mxt->msg_proc_addr)) 
                {
                        if  (i2c_master_recv(client, value, length) == length)
                                return length;
                        else
                                return -EIO;
                } else 
                {
                        mxt->last_read_addr = addr;
                }
        }

        mxt_debug(DEBUG_TRACE, "Writing address pointer & reading %d bytes "
                "in on i2c transaction...\n", length); 
        le_addr = cpu_to_le16(addr);
              
        msg[0].addr  = client->addr;
        msg[0].flags = 0x00;
        msg[0].len   = 2;
        msg[0].buf   = (u8 *) &le_addr;

        msg[1].addr  = client->addr;
        msg[1].flags = I2C_M_RD;
        msg[1].len   = length;
        msg[1].buf   = (u8 *) value;
        if  (i2c_transfer(adapter, msg, 2) == 2)
        	return length;
        else
        	return -EIO;

}

/* Writes one byte to given address in mXT chip. */
static int mxt_write_byte(struct i2c_client *client, u16 addr, u8 value)
{
        struct {
        	__le16 le_addr;
        	u8 data;

        } i2c_byte_transfer;

        struct mxt_data *mxt;

        mxt = i2c_get_clientdata(client);
        if (mxt != NULL)
        	mxt->last_read_addr = -1;
        i2c_byte_transfer.le_addr = cpu_to_le16(addr);
        i2c_byte_transfer.data = value;
        if  (i2c_master_send(client, (u8 *) &i2c_byte_transfer, 3) == 3)
        	return 0;
        else
        	return -EIO;
}


/* Writes a block of bytes (max 256) to given address in mXT chip. */
static int mxt_write_block(struct i2c_client *client,
		    u16 addr,
		    u16 length,
		    u8 *value)
{
        int i;
        struct {
        	__le16	le_addr;
        	u8	data[256];

        } i2c_block_transfer;

        struct mxt_data *mxt;

        mxt_debug(DEBUG_TRACE, "Writing %d bytes to %d...", length, addr);
        if (length > 256)
        	return -EINVAL;
        mxt = i2c_get_clientdata(client);
        if (mxt != NULL)
        	mxt->last_read_addr = -1;
        for (i = 0; i < length; i++)
        	i2c_block_transfer.data[i] = *value++;
        i2c_block_transfer.le_addr = cpu_to_le16(addr);
        i = i2c_master_send(client, (u8 *) &i2c_block_transfer, length + 2);
        if (i == (length + 2))
        	return length;
        else
        	return -EIO;
}

static u8 get_object_size(struct mxt_data *mxt, u8 object_type)
{
        u8 loop_i;
        for (loop_i = 0; loop_i < mxt->device_info.num_objs; loop_i++) 
        {
                if (mxt->object_table[loop_i].type == object_type)
                        return mxt->object_table[loop_i].size;
        }
        return 0;
}

/* Returns object address in mXT chip, or zero if object is not found */
static u16 get_object_address(struct mxt_data *mxt, uint8_t object_type)
{
        uint8_t object_table_index = 0;
        uint8_t address_found = 0;
        uint16_t address = 0;
        uint8_t instance = 0;
        struct mxt_object *object_table = mxt->object_table;
        struct mxt_object *obj;
        int max_objs = mxt->device_info.num_objs;

        while ((object_table_index < max_objs) && !address_found) 
        {
                obj = &object_table[object_table_index];
                if (obj->type == object_type) 
                {
                        address_found = 1;
                        /* Are there enough instances defined in the FW? */
                        if (obj->instances >= instance) 
                        {
                                address = obj->chip_addr +
                                        (obj->size + 1) * instance;
                        } 
                        else 
                        {
                                return 0;
                        }
                }
                object_table_index++;
        }
        return address;
}

/* Calculates the 24-bit CRC sum. */
static u32 CRC_24(u32 crc, u8 byte1, u8 byte2)
{
        static const u32 crcpoly = 0x80001B;
        u32 result;
        u32 data_word;

        data_word = ((((u16) byte2) << 8u) | byte1);
        result = ((crc << 1u) ^ data_word);
        if (result & 0x1000000)
                result ^= crcpoly;
        return result;
}

/* Calculates the CRC value for mXT infoblock. */
int calculate_infoblock_crc(u32 *crc_result, u8 *data, int crc_area_size)
{
        u32 crc = 0;
        int i;

        for (i = 0; i < (crc_area_size - 1); i = i + 2)
                crc = CRC_24(crc, *(data + i), *(data + i + 1));
        /* If uneven size, pad with zero */
        if (crc_area_size & 0x0001)
                crc = CRC_24(crc, *(data + i), 0);
        /* Return only 24 bits of CRC. */
        *crc_result = (crc & 0x00FFFFFF);

        return 0;
}

/* Calculates the CRC value for mXT config data. */
int calculate_config_data_crc(u32 *crc_result, u8 *data, int crc_area_size)
{
        u32 crc = 0;
        int i;

        for (i = 0; i < (crc_area_size - 1); i = i + 2)
                crc = CRC_24(crc, *(data + i), *(data + i + 1));
        /* If uneven size, pad with zero */
        if (crc_area_size & 0x0001)
                crc = CRC_24(crc, *(data + i), 0);
        /* Return only 24 bits of CRC. */
        *crc_result = (crc & 0x00FFFFFF);

        return 0;
}

static void mxt_clean_touch_state(struct mxt_data *mxt)
{
        //u8 loop_i;
        if(!mxt->finger_count)
        {
                mxt->finger_pressed = 0;
                return;
         }
        
        /*for (loop_i = 0; loop_i < mxt->numtouch; loop_i++) 
        {
                if (mxt->finger_pressed & BIT(loop_i)) 
                {
                        #ifdef ABS_MT_TRACKING_ID
                        input_report_abs(mxt->input, ABS_MT_TOUCH_MAJOR, 0);
						input_report_abs(mxt->input, BTN_TOUCH, 0);
                        input_mt_sync(mxt->input); 
                        #else
                        input_report_key(mxt->input, BTN_TOUCH, 0);
                        #endif

                }
        }
        input_sync(mxt->input);*/
        mxt->finger_pressed = 0;
        mxt->finger_count = 0;
        memset(&mxt->finger_data, 0, sizeof(struct atmel_finger_data) * mxt->numtouch);

}

uint8_t calibrate_chip(struct mxt_data *mxt)
{
        int ret = 1;
        uint8_t config_T8_cal_clear[2] = {5, 45};
          
        if(debug >= DEBUG_TRACE)
                printk(KERN_INFO "[TSP][%s] \n", __FUNCTION__);
        if(cal_check_flag == 0)
        {     
        	/* Write temporary acquisition config to chip. */
              mxt_write_block(mxt->client, get_object_address(mxt, MXT_GEN_ACQUIRECONFIG_T8) + 6, 
                                 2, config_T8_cal_clear);
        }        

        /* send calibration command to the chip */
        /* change calibration suspend settings to zero until calibration confirmed good */
        mxt_write_byte(mxt->client, get_object_address(mxt, MXT_GEN_COMMANDPROCESSOR_T6) +
                          MXT_ADR_T6_CALIBRATE, 1);

        /* set flag to show we must still confirm if calibration was good or bad */
        if((mxt->device_info.family_id == MXT224_FAMILYID)&&(mxt->device_info.major == 1))
        {
                cal_check_flag = 1u;      
        }
        
        mxt_clean_touch_state(mxt);
        return ret;
}


int read_mxtouch_nt_t_num(struct mxt_data *mxt, uint16_t *tch, uint16_t *atch)
{
        uint8_t data_buffer[100] = { 0 };
        uint8_t try_ctr = 0;
        uint8_t data_byte = 0xF3; /* dianostic command to get touch flags */
        uint16_t diag_address;
        uint8_t tch_ch = 0, atch_ch = 0;
        uint8_t check_mask;
        uint8_t i;
        uint8_t j;
        uint8_t x_line_limit;
        int error = 0;

        /* we have had the first touchscreen or face suppression message 
         * after a calibration - check the sensor state and try to confirm if
         * cal was good or bad */

        /* get touch flags from the chip using the diagnostic object */
        /* write command to command processor to get touch flags - 0xF3 Command required to do this */
        error = mxt_write_byte(mxt->client, get_object_address(mxt, MXT_GEN_COMMANDPROCESSOR_T6) +
                          DIAGNOSTIC_OFFSET, data_byte);
        if(error < 0)
        {
                printk(KERN_INFO "maxTouch write command T6 fail\n");
                return -1;
        }
        /* get the address of the diagnostic object so we can get the data we need */
        diag_address = get_object_address(mxt,MXT_DEBUG_DIAGNOSTIC_T37);

        msleep(10); 

        /* read touch flags from the diagnostic object - clear buffer so the while loop can run first time */
        memset( data_buffer , 0xFF, sizeof( data_buffer ) );

        /* wait for diagnostic object to update */
        while(!((data_buffer[0] == 0xF3) && (data_buffer[1] == 0x00)))
        {
        	//printk(KERN_INFO "try_ctr = %d\n",try_ctr);
        	/* wait for data to be valid  */
        	if(try_ctr > 10) //0318 hugh 100-> 10
        	{
                        /* Failed! */
                        if(debug >= DEBUG_TRACE)
                                printk(KERN_DEBUG "[TSP] Diagnostic Data did not update!!\n");
                        //qt_timer_state = 0;//0430 hugh
                        break;
        	}
        	msleep(2); //0318 hugh  3-> 2
        	try_ctr++; /* timeout counter */
        	mxt_read_block(mxt->client,diag_address,2,data_buffer);
            #if 0
		    if(debug >= DEBUG_TRACE)
        	        printk(KERN_INFO "[TSP] Waiting for diagnostic data to update, try %d\n", try_ctr);
            #endif
        }

        /* data is ready - read the detection flags */
        mxt_read_block(mxt->client,diag_address,82,data_buffer);
        /* data array is 20 x 16 bits for each set of flags, 2 byte header, 40 bytes for touch flags 40 bytes for antitouch flags*/

        /* count up the channels/bits if we recived the data properly */
        if((data_buffer[0] == 0xF3) && (data_buffer[1] == 0x00))
        {

                /* mode 0 : 16 x line, mode 1 : 17 etc etc upto mode 4.*/
                x_line_limit = 16 + 2; 
                if(x_line_limit > 20)
                {
                        /* hard limit at 20 so we don't over-index the array */
                        x_line_limit = 20;
                }

                /* double the limit as the array is in bytes not words */
                x_line_limit = x_line_limit << 1;

                /* count the channels and print the flags to the log */
                for(i = 0; i < x_line_limit; i+=2) /* check X lines - data is in words so increment 2 at a time */
                {
                /* print the flags to the log - only really needed for debugging */
//printk("[TSP] Detect Flags X%d, %x%x, %x%x \n", i>>1,data_buffer[3+i],data_buffer[2+i],data_buffer[43+i],data_buffer[42+i]);

                        /* count how many bits set for this row */
                        for(j = 0; j < 8; j++)
                        {
                                /* create a bit mask to check against */
                                check_mask = 1 << j;

                                /* check detect flags */
                                if(data_buffer[2+i] & check_mask)
                                {
                                        tch_ch++;
                                }
                                if(data_buffer[3+i] & check_mask)
                                {
                                        tch_ch++;
                                }

                                /* check anti-detect flags */
                                if(data_buffer[42+i] & check_mask)
                                {
                                        atch_ch++;
                                }
                                if(data_buffer[43+i] & check_mask)
                                {
                                        atch_ch++;
                                }
                        }
        	}

              *tch = tch_ch;
              *atch = atch_ch;
        	/* print how many channels we counted */
              if(debug >= DEBUG_TRACE)
        	        printk(KERN_INFO "[TSP] Flags Counted channels: t:%d a:%d \n", tch_ch, atch_ch);

        	/* send page up command so we can detect when data updates next time,
        	 * page byte will sit at 1 until we next send F3 command */
        	data_byte = 0x01;
        	error = mxt_write_byte(mxt->client, get_object_address(mxt, MXT_GEN_COMMANDPROCESSOR_T6)+DIAGNOSTIC_OFFSET, 
        	                             data_byte);
              if(error < 0)
              {
                      printk(KERN_INFO "maxTouch send page up command faill\n");
                      return -1;
              }

        }
        return 0;
}

int read_atch_tch_num(struct mxt_data *mxt, uint8_t xchannel, uint8_t ychannel, uint16_t *tch, uint16_t *atch)
{
        uint8_t data_buffer[132] = { 0 };
        uint8_t try_ctr = 0;
        uint8_t data_byte = 0xF3; /* dianostic command to get touch flags */
        uint16_t diag_address;
        uint16_t tch_ch = 0, atch_ch = 0;
        uint8_t check_mask;
        uint16_t i;
        uint16_t j;
        int error = 0;
        uint8_t need_bytes = ychannel / 8 + (((ychannel % 8) == 0) ? 0 : 1);
        uint8_t remain_bytes = (xchannel * 2 * need_bytes) % 128;
        uint8_t total_page_num = ((xchannel * need_bytes) / 128 + ((remain_bytes == 0) ? 0 : 1)) * 2;
        uint8_t current_page_num = 0;

#if 1
        printk(KERN_INFO "Atmel read_atch_tch_num() need_bytes=%d, remain_bytes=%d total_page_num=%d\n",
            need_bytes, remain_bytes, total_page_num);
#endif 
        /* we have had the first touchscreen or face suppression message 
         * after a calibration - check the sensor state and try to confirm if
         * cal was good or bad */

        /* get touch flags from the chip using the diagnostic object */
        /* write command to command processor to get touch flags - 0xF3 Command required to do this */
        error = mxt_write_byte(mxt->client, get_object_address(mxt, MXT_GEN_COMMANDPROCESSOR_T6) +
                          DIAGNOSTIC_OFFSET, data_byte);
        if(error < 0)
        {
                printk(KERN_INFO "maxTouch write command T6 fail\n");
                return -1;
        }
            
        /* get the address of the diagnostic object so we can get the data we need */
        diag_address = get_object_address(mxt,MXT_DEBUG_DIAGNOSTIC_T37);

        do
        {
            msleep(10); 

            /* read touch flags from the diagnostic object - clear buffer so the while loop can run first time */
            memset( data_buffer , 0xFF, sizeof( data_buffer ) );

            /* wait for diagnostic object to update */
            while(!((data_buffer[0] == 0xF3) && (data_buffer[1] == (0x00 + current_page_num))))
            {
            	//printk(KERN_INFO "try_ctr = %d\n",try_ctr);
            	/* wait for data to be valid  */
            	if(try_ctr > 10) //0318 hugh 100-> 10
            	{
                        /* Failed! */
                        if(debug >= DEBUG_TRACE)
                                printk(KERN_DEBUG "[TSP] Diagnostic Data did not update!!\n");
                        //qt_timer_state = 0;//0430 hugh
                        break;
            	}
            	msleep(2); //0318 hugh  3-> 2
            	try_ctr++; /* timeout counter */
            	mxt_read_block(mxt->client, diag_address, 2, data_buffer);

            }

            /* data is ready - read the detection flags */
            mxt_read_block(mxt->client, diag_address, 130, data_buffer);
            
#if 0
            printk("data_buffer:");
            for (i = 0; i < 132; i += 2)
                printk(" 0x%02x%02x  ", data_buffer[i], data_buffer[i+1]);
            printk("\n");
#endif            
            /* count up the channels/bits if we recived the data properly */
            if((data_buffer[0] == 0xF3) && (data_buffer[1] == (0x00+ current_page_num)))
            {

                    /* count the channels and print the flags to the log */
                    for(i = 0; i < 128; i++) /* check X lines - data is in words so increment 2 at a time */
                    {
                    /* print the flags to the log - only really needed for debugging */
                   
                            /* count how many bits set for this row */
                            for(j = 0; j < 8; j++)
                            {
                                    /* create a bit mask to check against */
                                    check_mask = 1 << j;

                                    if (current_page_num  < (total_page_num / 2))
                                    {
                                        /* check detect flags */
                                        if(data_buffer[2+i] & check_mask)
                                        {
                                                tch_ch++;
                                        }
                                    }
                                    else
                                    {
                                        /* check anti-detect flags */
                                        if(data_buffer[2+i] & check_mask)
                                        {
                                                atch_ch++;
                                        }
                                    }

                            }
                            //printk("Atmel %d byte tch_ch=%d, atch_ch=%d\n", i, tch_ch, atch_ch);
            	        }
             }
             /* send page up command so we can detect when data updates next time,
            	 * page byte will sit at 1 until we next send F3 command */
            data_byte = 0x01;
            error = mxt_write_byte(mxt->client, get_object_address(mxt, MXT_GEN_COMMANDPROCESSOR_T6)+DIAGNOSTIC_OFFSET, 
            	                             data_byte);
             if(error < 0)
             {
                     printk(KERN_INFO "maxTouch send page up command faill\n");
                     return -1;
             }

              current_page_num++;
        } while(current_page_num < total_page_num); 

        *tch = tch_ch;
        *atch = atch_ch;
        /* print how many channels we counted */
        if(debug >= DEBUG_TRACE)
                printk(KERN_INFO "Atmel [TSP] Flags Counted channels: t:%d a:%d \n", tch_ch, atch_ch);

        return 0;
}

void check_chip1_0_calibration(struct mxt_data *mxt)
{
            uint16_t tch_ch = 0, atch_ch = 0;
            int error = 0;
            error = read_mxtouch_nt_t_num(mxt, &tch_ch, &atch_ch);
            if(error < 0)
            {
                    return;
            }

/* process counters and decide if we must re-calibrate or if cal was good */      
              if(atch_ch >= 2)		//jwlee add 0325
        	{
        	          if(debug >= DEBUG_TRACE)
                                printk(KERN_DEBUG "[TSP] calibration was bad\n");
                        /* cal was bad - must recalibrate and check afterwards */
                        calibrate_chip(mxt);
        	}
        	else 
              {
                        #if 0
                        if(debug >= DEBUG_TRACE)
                                printk(KERN_INFO "[TSP] calibration was not decided yet\n");
                        #endif
                        /* we cannot confirm if good or bad - we must wait for next touch  message to confirm */
                        cal_check_flag = 1u;
               }
}

void check_chip2_0_calibration(struct mxt_data *mxt)
{
        uint16_t tch_ch = 0, atch_ch = 0;
        int error = 0;

        if(mxt->device_info.family_id == MXT1386_FAMILYID)
        {
            if(debug >= DEBUG_TRACE)
                printk(KERN_INFO "Atmel mxt1386 check chip calibration.\n");
            error = read_atch_tch_num(mxt, 33, 42, &tch_ch, &atch_ch);
            if(error < 0)
            {
                printk(KERN_ERR "Atmel mxt1386 read tch and atch error.\n");
                return;
            }
        }
        else
        {
            if(debug >= DEBUG_TRACE)
                printk(KERN_INFO "Atmel mxt224 check chip calibration.\n");
            error = read_mxtouch_nt_t_num(mxt, &tch_ch, &atch_ch);
            if(error < 0)
            {
                printk(KERN_ERR "Atmel mxt224 read tch and atch error..\n");
                return;
            }
        }

        /* process counters and decide if we must re-calibrate or if cal was good */      
        if(atch_ch !=0 ||tch_ch !=0)		//jwlee add 0325
        {
              if(debug >= DEBUG_TRACE)
              {          
                    printk(KERN_DEBUG "[TSP] calibration was bad\n");
              }
                /* cal was bad - must recalibrate and check afterwards */
              calibrate_chip(mxt);
        }
        else
        {
              if(debug >= DEBUG_TRACE)
              {
                    printk(KERN_DEBUG "[TSP] calibration was good\n");
              }
        }

}

static void confirm_calibration(struct mxt_data *mxt)
{
        uint8_t ATCH_NOR[4] = {255, 1, 0, 0};
        uint8_t objectT9 = 0;
        uint8_t objectT22 = 0;
        int error;

        //mxt224 firmware 1.6 or 2.0
        //if((mxt->device_info.family_id == MXT224_FAMILYID)&&(mxt->device_info.major == 1))
        if(mxt->device_info.family_id == MXT224_FAMILYID)
        {
                objectT9 = 35;//30-35;
                objectT22 = 25;//20-25;
                error = mxt_write_block(mxt->client,
                                   get_object_address(mxt, GEN_ACQUISITIONCONFIG_T8) + 6, 2, ATCH_NOR);
                if(error < 0)
                {
                        printk(KERN_INFO "Atmel maxTouch calibration write ATCH_NOR err!\n");
                }
             
                error = mxt_write_byte(mxt->client,
                                get_object_address(mxt, MXT_TOUCH_MULTITOUCHSCREEN_T9) + 7, objectT9);
                if(error < 0)
                {
                        printk(KERN_INFO "Atmel maxTouch write T9 err!\n");
                }
                                
                error = mxt_write_byte(mxt->client,
                                get_object_address(mxt, MXT_PROCG_NOISESUPPRESSION_T22) + 8, objectT22);
                if(error < 0)
                {
                        printk(KERN_INFO "Atmel maxTouch write T22 err!\n");
                }                
        }
        else//mxt1386
        {
                objectT9 = 40;
                objectT22 = 20;
                
                error = mxt_write_block(mxt->client,
                                   get_object_address(mxt, GEN_ACQUISITIONCONFIG_T8) + 6, 4, ATCH_NOR);
                if(error < 0)
                {
                        printk(KERN_INFO "Atmel maxTouch calibration write ATCH_NOR err!\n");
                }
                if(BOARD_NUM(hw_ver) != BOARD_NUM_V11) {
                        error = mxt_write_byte(mxt->client,
                              get_object_address(mxt, MXT_TOUCH_MULTITOUCHSCREEN_T9) + 7, objectT9);
                        if(error < 0)
                        {
                            printk(KERN_INFO "Atmel maxTouch write T9 err!\n");
                        }
                }
               // #if 0
                error = mxt_write_byte(mxt->client,
                                get_object_address(mxt, MXT_PROCG_NOISESUPPRESSION_T22) + 8, objectT22);
                if(error < 0)
                {
                        printk(KERN_INFO "Atmel maxTouch write T22 err!\n");
                }
               // #endif
        }

        mxt->pre_data[0] = 2;
        cal_check_flag = 0;

        if(debug >= DEBUG_TRACE)
                printk(KERN_INFO "Atmel calibration confirm\n");
}

void report_muti_ts_key(u8 *message, struct mxt_data *mxt)
{
        u8              key_status;
        u8              key0_7;
        static u8      key_flag = 0;
        key_status = (message[MXT_MSG_T15_STATUS] >>7)& 0x01;
        key0_7      = message[MXT_MSG_T15_KEY0_7];
        if(key0_7 ==1)
        {
                input_report_key(mxt->input, KEY_HOME,  1);
                key_flag = 1;
        }
        else if(key0_7 ==2)
        {
                input_report_key(mxt->input, KEY_MENU,   1);
                key_flag = 2;
        }
        else if(key0_7 ==4)
        {
                input_report_key(mxt->input, KEY_BACK,   1);
                key_flag = 4;
        }
        else
        {
                if( key_flag == 1)
                {
                        input_report_key(mxt->input, KEY_HOME,   0);
                }
                if( key_flag == 2)
                {
                        input_report_key(mxt->input, KEY_MENU,   0);
                }
                if( key_flag == 4)
                {
                        input_report_key(mxt->input, KEY_BACK,   0);

                }
                key_flag = 0;
        }
        input_sync(mxt->input);

}

/* 
modify the way of report touch for it can not play angry birds
*/
static void report_muti_touchs(struct mxt_data *mxt)
{
        u8 loop_i;
        int finger_num = 0;
                for (loop_i = 0; loop_i < mxt->numtouch; loop_i++) 
                {
                        if (mxt->finger_pressed & BIT(loop_i)) 
								{
                            finger_num++;    
								input_report_abs(mxt->input, ABS_MT_TOUCH_MAJOR, mxt->finger_data[loop_i].z);
                                input_report_abs(mxt->input, ABS_MT_WIDTH_MAJOR, mxt->finger_data[loop_i].w);
								input_report_abs(mxt->input, ABS_MT_PRESSURE, mxt->finger_data[loop_i].z);
                                input_report_abs(mxt->input, ABS_MT_POSITION_X, mxt->finger_data[loop_i].x);
                                input_report_abs(mxt->input, ABS_MT_POSITION_Y, mxt->finger_data[loop_i].y);
                                input_report_abs(mxt->input, ABS_MT_TRACKING_ID, loop_i); 
								input_mt_sync(mxt->input);
                                }
                } 
        input_report_key(mxt->input, BTN_TOUCH, finger_num > 0);
		
		if (mxt->finger_pressed) {
		input_report_abs(mxt->input, ABS_X, mxt->finger_data[0].x);
		input_report_abs(mxt->input, ABS_Y, mxt->finger_data[0].y);
		input_report_abs(mxt->input,	ABS_PRESSURE, mxt->finger_data[0].z);
	}
        input_mt_sync(mxt->input);
		input_sync(mxt->input);
}

static void msg_process_finger_data(struct atmel_finger_data *fdata, u8  *data)
{
        fdata->x =  data[MXT_MSG_T9_XPOSMSB] * 16 +
        		((data[MXT_MSG_T9_XYPOSLSB] >> 4) & 0xF);
        fdata->y = data[MXT_MSG_T9_YPOSMSB] * 16 +
        		((data[MXT_MSG_T9_XYPOSLSB] >> 0) & 0xF);
        fdata->y >>= 2;
        fdata->w = data[MXT_MSG_T9_TCHAREA];
        fdata->z = data[MXT_MSG_T9_TCHAMPLITUDE];
}

static void process_T9_message(u8 *message, struct mxt_data *mxt)
{

        struct input_dev *input;
        u8  status;
        u8  touch_number;
        u8  report_id;

        input = mxt->input;
        status = message[MXT_MSG_T9_STATUS];
        report_id = message[0];
        touch_number = message[MXT_MSG_REPORTID] - mxt->rid_map[report_id].first_rid;
        if((touch_number >= 0) && (touch_number < mxt->numtouch))
        {
                msg_process_finger_data(&mxt->finger_data[touch_number], message);
                if (status & (MXT_MSGB_T9_RELEASE|MXT_MSGB_T9_SUPPRESS) )
                {
                        if(mxt->finger_pressed & BIT(touch_number))
                        {
                                mxt->finger_count--;
                                mxt->finger_pressed &= ~BIT(touch_number);
                        }
                        
                        if(mxt->pre_data[0] < 2)
                        {
                                if (mxt->finger_count == 0 &&
                                        (jiffies_to_msecs(jiffies) > (mxt->timestamp + 200) &&
                                        (touch_number == 0 && ((abs(mxt->finger_data[0].y - mxt->pre_data[ 2]) > 135)
                                        || (abs(mxt->finger_data[0].x - mxt->pre_data[1]) > 135)))))
                                 {
                                                confirm_calibration(mxt);
                                 }
                                
                                if (!mxt->finger_count && mxt->pre_data[0] == 1)
                                {
                                        mxt->pre_data[0] = 0;
                                }
                        }
                } 
                else if (status & (MXT_MSGB_T9_DETECT|MXT_MSGB_T9_PRESS))  
                {
                        if (!(mxt->finger_pressed & BIT(touch_number)))
                        {
                                mxt->finger_count++;
                                mxt->finger_pressed |= BIT(touch_number);
                        }
                        
                        if(mxt->pre_data[0] < 2)
                        {
                                if (!mxt->pre_data[0] && mxt->finger_count ==1)
                                {
                                        mxt->pre_data[0] = 1;
                                        mxt->pre_data[1] = mxt->finger_data[0].x;
                                        mxt->pre_data[2] = mxt->finger_data[0].y;
                                        //mxt->timestamp = jiffies;
                                        mxt->timestamp = jiffies_to_msecs(jiffies);
                                }
                                else if(mxt->pre_data[0] && mxt->finger_count !=1)
                                {
                                        mxt->pre_data[0] = 0;
                                }
                                
                        }
                }
        }
        return;
}


static int process_message(u8 *message, u8 object, struct mxt_data *mxt)
{
        struct i2c_client *client;
        u8   status;
        u16 xpos = 0xFFFF;
        u16 ypos = 0xFFFF;
        u8   event;
        u8   direction;
        u16 distance;
        u8   report_id;

        client = mxt->client;
        report_id = message[0];

        switch (object) {
                case MXT_GEN_COMMANDPROCESSOR_T6:
                        status = message[1];
                        if (status & MXT_MSGB_T6_COMSERR) 
                        {
                                if (debug >= DEBUG_TRACE)
                                        dev_err(&client->dev, "maXTouch checksum error\n");
                        }
                        if (status & MXT_MSGB_T6_CFGERR) 
                        {
                                /* 
                                 * Configuration error. A proper configuration
                                 * needs to be written to chip and backed up. Refer
                                 * to protocol document for further info.
                                 */
                                 if (debug >= DEBUG_TRACE)
                                        dev_err(&client->dev, "maXTouch configuration error\n");
                        }
                        if (status & MXT_MSGB_T6_CAL) 
                        {
                                /* Calibration in action, no need to react */
                                if (debug >= DEBUG_TRACE)
                                {
                                        dev_info(&client->dev, "maXTouch calibration in progress\n");
                                }
                                if((mxt->device_info.family_id == MXT224_FAMILYID)&&(mxt->device_info.major == 1))
                                {
                                        cal_check_flag = 1u;
                                }
                                else
                                {
                                       //check_chip2_0_calibration(mxt);
                                }
                                check_chip2_0_calibration(mxt); 
                                mxt->pre_data[0] = 0;
                                mxt_clean_touch_state(mxt);
                                
                        }
                        if (status & MXT_MSGB_T6_SIGERR) 
                        {
                                /* 
                                * Signal acquisition error, something is seriously
                                * wrong, not much we can in the driver to correct
                                * this
                                */
                                if (debug >= DEBUG_TRACE)
                                        dev_err(&client->dev, "maXTouch acquisition error\n");
                        }
                        if (status & MXT_MSGB_T6_OFL) 
                        {
                                /*
                                 * Cycle overflow, the acquisition is too short.
                                 * Can happen temporarily when there's a complex
                                 * touch shape on the screen requiring lots of
                                 * processing.
                                 */
                                 if (debug >= DEBUG_TRACE)
                                        dev_err(&client->dev, "maXTouch cycle overflow\n");
                        }
                        if (status & MXT_MSGB_T6_RESET) 
                        {
                                /* Chip has reseted, no need to react. */
                                if (debug >= DEBUG_TRACE)
                                        dev_info(&client->dev, "maXTouch chip reset\n");
                        }
                        if (status == 0) {
                                /* Chip status back to normal. */
                                if (debug >= DEBUG_TRACE)
                                        dev_info(&client->dev, "maXTouch status normal\n");
                        }
                        break;

                case MXT_TOUCH_MULTITOUCHSCREEN_T9:
                        process_T9_message(message, mxt);
                        report_message_flag = 1;
                        break;

                case MXT_SPT_GPIOPWM_T19:
                        if (debug >= DEBUG_TRACE)
        	                dev_info(&client->dev, "Receiving GPIO message\n");
                        break;
                        
                case MXT_TOUCH_KEYARRAY_T15:
                        report_muti_ts_key(message, mxt);
                        break;
                        
                case MXT_PROCI_GRIPFACESUPPRESSION_T20:
                        if (debug >= DEBUG_TRACE)
                                dev_info(&client->dev, "Receiving face suppression msg\n");
                        break;

                case MXT_PROCG_NOISESUPPRESSION_T22:
                        if (debug >= DEBUG_TRACE)
                                dev_info(&client->dev, "Receiving noise suppression msg\n");
                        status = message[MXT_MSG_T22_STATUS];
                        if (status & MXT_MSGB_T22_FHCHG) 
                        {
                                if (debug >= DEBUG_TRACE)
                                        dev_info(&client->dev, "maXTouch: Freq changed\n");
                        }
                        if (status & MXT_MSGB_T22_GCAFERR) 
                        {
                                if (debug >= DEBUG_TRACE)
                                        dev_info(&client->dev, "maXTouch: High noise ""level\n");
                        }
                        if (status & MXT_MSGB_T22_FHERR) 
                        {
                                if (debug >= DEBUG_TRACE)
                                        dev_info(&client->dev, "maXTouch: Freq changed -Noise level too high\n");
                        }
                        break;

                case MXT_PROCI_ONETOUCHGESTUREPROCESSOR_T24:
                        if (debug >= DEBUG_TRACE)
                                dev_info(&client->dev, "Receiving one-touch gesture msg\n");

                        event = message[MXT_MSG_T24_STATUS] & 0x0F;
                        xpos = message[MXT_MSG_T24_XPOSMSB] * 16 +
        	                ((message[MXT_MSG_T24_XYPOSLSB] >> 4) & 0x0F);
                        ypos = message[MXT_MSG_T24_YPOSMSB] * 16 +
        	                ((message[MXT_MSG_T24_XYPOSLSB] >> 0) & 0x0F);
                        xpos >>= 2;
                        ypos >>= 2;
                        direction = message[MXT_MSG_T24_DIR];
                        distance = message[MXT_MSG_T24_DIST] +
        	                (message[MXT_MSG_T24_DIST + 1] << 16);

                        report_gesture((event << 24) | (direction << 16) | distance, mxt);
                        report_gesture((xpos << 16) | ypos, mxt);

                        break;

                case MXT_SPT_SELFTEST_T25:
                        if (debug >= DEBUG_TRACE)
                                dev_info(&client->dev, "Receiving Self-Test msg\n");

                        if (message[MXT_MSG_T25_STATUS] == MXT_MSGR_T25_OK) 
                        {
                                if (debug >= DEBUG_TRACE)
                                        dev_info(&client->dev, "maXTouch: Self-Test OK\n");
                        } 
                        else  
                        {
                                dev_err(&client->dev,
                                        "maXTouch: Self-Test Failed [%02x]:"
                                        "{%02x,%02x,%02x,%02x,%02x}\n",
                                        message[MXT_MSG_T25_STATUS],
                                        message[MXT_MSG_T25_STATUS + 0],
                                        message[MXT_MSG_T25_STATUS + 1],
                                        message[MXT_MSG_T25_STATUS + 2],
                                        message[MXT_MSG_T25_STATUS + 3],
                                        message[MXT_MSG_T25_STATUS + 4]);
                        }
                        break;

                case MXT_PROCI_TWOTOUCHGESTUREPROCESSOR_T27:
                        if (debug >= DEBUG_TRACE)
                                dev_info(&client->dev, "Receiving 2-touch gesture message\n");

                        event = message[MXT_MSG_T27_STATUS] & 0xF0;
                        xpos = message[MXT_MSG_T27_XPOSMSB] * 16 +
                                ((message[MXT_MSG_T27_XYPOSLSB] >> 4) & 0x0F);
                        ypos = message[MXT_MSG_T27_YPOSMSB] * 16 +
                                ((message[MXT_MSG_T27_XYPOSLSB] >> 0) & 0x0F);
                        xpos >>= 2;
                        ypos >>= 2;
                        direction = message[MXT_MSG_T27_ANGLE];
                        distance = message[MXT_MSG_T27_SEPARATION] +
                        	   (message[MXT_MSG_T27_SEPARATION + 1] << 16);

                        report_gesture((event << 24) | (direction << 16) | distance, mxt);
                        report_gesture((xpos << 16) | ypos, mxt);
                        break;

                case MXT_SPT_CTECONFIG_T28:
                        if (debug >= DEBUG_TRACE)
        	                dev_info(&client->dev, "Receiving CTE message...\n");
                        status = message[MXT_MSG_T28_STATUS];
                        if (status & MXT_MSGB_T28_CHKERR)
                                dev_err(&client->dev, "maXTouch: Power-Up CRC failure\n");
                        break;
                        
                default:
                        if (debug >= DEBUG_TRACE)
                                dev_info(&client->dev, "maXTouch: Unknown message!\n");
                        break;
                
        }

        return 0;
}

/*
 * Processes messages when the interrupt line (CHG) is asserted. Keeps
 * reading messages until a message with report ID 0xFF is received,
 * which indicates that there is no more new messages.
 *
 */

static void mxt_work_func(struct work_struct *work)
{
        struct	mxt_data *mxt;
        struct	i2c_client *client;

        u8	*message;
        u16	message_length;
        u16	message_addr;
        u8	report_id;
        u8	object;
        int	error;
        int   i;

        message = NULL;
        mxt = container_of(work, struct mxt_data, work);
        client = mxt->client;
        message_addr = mxt->msg_proc_addr;
        message_length = 7;//mxt->message_size;
        
        if (message_length < 256) 
        {
                message = kmalloc(message_length, GFP_KERNEL);
                if (message == NULL) 
                {
                        dev_err(&client->dev, "Error allocating memory\n");
                        return;
                }
        } 
        else 
        {
                dev_err(&client->dev, "Message length larger than 256 bytes not supported\n");
                return;
        }

        do 
        {
                /* Read next message, reread on failure. */
                mxt->message_counter++;
                for (i = 1; i < I2C_RETRY_COUNT; i++) 
                {
                        error = mxt_read_block(client, message_addr, message_length, message);
                        if (error >= 0)
                                break;
                        mxt->read_fail_counter++;
                        if (debug >= DEBUG_TRACE)
                                printk(KERN_ERR "Failure reading i2c i = %d\n",i);
                }
                if (error < 0) 
                {
                        mxt_clean_touch_state(mxt);
                        kfree(message);
                        enable_irq(mxt->client->irq);
                        return;
                }
                report_id = message[0];
                if ((report_id != MXT_END_OF_MESSAGES) && (report_id != 0)) 
                {
                        /* Get type of object and process the message */
                        object = mxt->rid_map[report_id].object;
                        process_message(message, object, mxt);
                }

        } while (comms ? (mxt->read_chg() == 0) : 
                                ((report_id != MXT_END_OF_MESSAGES) && (report_id != 0)));

        if(report_message_flag)
        {
                report_muti_touchs(mxt);

                if(cal_check_flag )
                {
                        check_chip1_0_calibration(mxt);
                }
                report_message_flag =0;
        }

        mod_timer(&mxt->timer, jiffies + TOUCHSCREEN_TIMEOUT);
        
        kfree(message);
        enable_irq(mxt->client->irq); 
        
}


/*
 * The maXTouch device will signal the host about a new message by asserting
 * the CHG line. This ISR schedules a worker routine to read the message when
 * that happens.
 */
static irqreturn_t mxt_irq_handler(int irq, void *_mxt)
{
        struct mxt_data *mxt = _mxt;

        /* Send the signal only if falling edge generated the irq. */
        disable_irq_nosync(mxt->client->irq); 
        queue_work(mxt->atmel_wq, &mxt->work);

        return IRQ_HANDLED;
}

static void mxt_timer(unsigned long handle)
{
        struct mxt_data *mxt = (struct mxt_data *) handle;

        mxt_clean_touch_state(mxt);

        return;
}


/******************************************************************************/
/* Initialization of driver                                                   */
/******************************************************************************/
static int __devinit mxt_identify(struct i2c_client *client,
				  struct mxt_data *mxt,
				  u8 *id_block_data)
{
        u8 buf[7];
        int error;
        int identified;

        identified = 0;
        /* Read Device info to check if chip is valid */
        error = mxt_read_block(client, MXT_ADDR_INFO_BLOCK, MXT_ID_BLOCK_SIZE,
        		       (u8 *) buf);
        if (error < 0) 
        {
                int fail_count = 1;
                mxt->read_fail_counter++;
                do{
                        printk(KERN_DEBUG "the %d times atmel touchscreen identify fail, delay 200ms and retry it \n", fail_count);
                        msleep(200);
                        error = mxt_read_block(client, MXT_ADDR_INFO_BLOCK, MXT_ID_BLOCK_SIZE,
        		       (u8 *) buf);
                        if(fail_count == 3)
                                break;
                        fail_count++;
                }while(error < 0);
        }
        
        if(error < 0)
        {
               printk(KERN_DEBUG "Failure accessing maXTouch device\n");
               return -EIO;
        }
        memcpy(id_block_data, buf, MXT_ID_BLOCK_SIZE);

        mxt->device_info.family_id   = buf[0];
        mxt->device_info.variant_id  = buf[1];
        mxt->device_info.major        = ((buf[2] >> 4) & 0x0F);
        mxt->device_info.minor        = (buf[2] & 0x0F);
        mxt->device_info.build          = buf[3];
        mxt->device_info.x_size        = buf[4];
        mxt->device_info.y_size        = buf[5];
        mxt->device_info.num_objs   = buf[6];
        mxt->device_info.num_nodes  = mxt->device_info.x_size * mxt->device_info.y_size;
        
        /*
         * Check Family & Variant Info; warn if not recognized but
         * still continue.
         */
        /* MXT224 */
        if (mxt->device_info.family_id == MXT224_FAMILYID) 
        {
                strcpy(mxt->device_info.family_name, "mXT224");
                if (mxt->device_info.variant_id == MXT224_CAL_VARIANTID) 
                {
                        strcpy(mxt->device_info.variant_name, "Calibrated");
                }
                else if (mxt->device_info.variant_id == MXT224_UNCAL_VARIANTID) 
                {
                        strcpy(mxt->device_info.variant_name, "Uncalibrated");
                }
                else 
                {
                        dev_err(&client->dev,
                        	        "Warning: maXTouch Variant ID [%d] not "
                        	        "supported\n",
                        	        mxt->device_info.variant_id);
                        strcpy(mxt->device_info.variant_name, "UNKNOWN");
                        /* identified = -ENXIO; */
                }
        /* MXT1386 */
        } 
        else if (mxt->device_info.family_id == MXT1386_FAMILYID) 
        {
                strcpy(mxt->device_info.family_name, "mXT1386");
                if (mxt->device_info.variant_id == MXT1386_CAL_VARIANTID) 
                {
                        strcpy(mxt->device_info.variant_name, "Calibrated");
                } 
                else 
                {
                        dev_err(&client->dev,
                        	        "Warning: maXTouch Variant ID [%d] not "
                        	        "supported\n",
                        	        mxt->device_info.variant_id);
                        strcpy(mxt->device_info.variant_name, "UNKNOWN");
                        /* identified = -ENXIO; */
                }
        /* Unknown family ID! */
        } 
        else 
        {
                dev_err(&client->dev,
                	"Warning: maXTouch Family ID [%d] not supported\n",
                	mxt->device_info.family_id);
                strcpy(mxt->device_info.family_name, "UNKNOWN");
                strcpy(mxt->device_info.variant_name, "UNKNOWN");
                identified = -ENXIO;
        }
        dev_info(
        	&client->dev,
        	"Atmel maXTouch (Family %s (%X), Variant %s (%X)) Firmware "
        	"version [%d.%d] Build %d\n",
        	mxt->device_info.family_name,
        	mxt->device_info.family_id,
        	mxt->device_info.variant_name,
        	mxt->device_info.variant_id,
        	mxt->device_info.major,
        	mxt->device_info.minor,
        	mxt->device_info.build
        );
        dev_info(
        	&client->dev,
        	"Atmel maXTouch Configuration "
        	"[X: %d] x [Y: %d]\n",
        	mxt->device_info.x_size,
        	mxt->device_info.y_size
        );
        return identified;
}

#ifdef CHECK_TS_CONFIG_DATA_ZTE
 static int  mxt_check_config_data(struct i2c_client *client,
                                                                struct mxt_data *mxt)
{
        u8 *mem_crc = NULL;
        u32 config_crc = 0;
        u32 my_config_crc = 0;
        u8	object_type;
        u16	object_address;
        u16	object_size;
        int	i;
        int	error = 0;
        int maxsize_object = 0;
        struct mxt_platform_data *pdata;

        pdata = client->dev.platform_data;
        mxt->configuration_crc = 1;
        
        for (i = 0; i < mxt->device_info.num_objs; i++) 
        {
                object_size = mxt->object_table[i].size;
                if(object_size > maxsize_object)
                {
                        maxsize_object = object_size;
                }
        }
        
        mem_crc = kzalloc(maxsize_object, GFP_KERNEL);
        if (mem_crc == NULL) 
        {
                printk(KERN_WARNING "maXTouch: Can't allocate memory!\n");
                error = -ENOMEM;
                goto err_allocate_mem;
        }
        
        for (i = 0; i < mxt->device_info.num_objs; i++) 
        {
                object_type = mxt->object_table[i].type;
                object_address = mxt->object_table[i].chip_addr;
                object_size = mxt->object_table[i].size;
                error = mxt_read_block(client, object_address,
                		       object_size, mem_crc);
                if (error < 0) 
                {
                        mxt->read_fail_counter++;
                        printk(KERN_DEBUG "maXTouch Object %d could not be read!\n", i);
                        error = -EIO;
                        goto err_read_mxt_touch;
                }
                
                if(!IS_CONFIG_OBJECT(object_type))
                {        
                        continue;
                }
                mxt_debug(DEBUG_TRACE, "calculate config crc from chip!\n");
                calculate_config_data_crc(&config_crc, mem_crc, object_size);

                switch(object_type){
                        case MXT_GEN_POWERCONFIG_T7:
                                calculate_config_data_crc(&my_config_crc, pdata->config_T7, object_size);
                                break;
                                
                        case MXT_GEN_ACQUIRECONFIG_T8:
                                calculate_config_data_crc(&my_config_crc, pdata->config_T8, object_size);
                                break;
                                
                        case MXT_TOUCH_MULTITOUCHSCREEN_T9:
                                calculate_config_data_crc(&my_config_crc, pdata->config_T9, object_size);
                                break;
                                
                        case MXT_TOUCH_KEYARRAY_T15:
                                calculate_config_data_crc(&my_config_crc, pdata->config_T15, object_size);
                                break;
                                
                        case MXT_SPT_COMMSCONFIG_T18:
                                calculate_config_data_crc(&my_config_crc, pdata->config_T18, object_size);
                                break;
								
                        case MXT_PROCI_GRIPFACESUPPRESSION_T20:
                                calculate_config_data_crc(&my_config_crc, pdata->config_T20, object_size);
                                break;
                                
                        case MXT_PROCG_NOISESUPPRESSION_T22:
                                calculate_config_data_crc(&my_config_crc, pdata->config_T22, object_size);
                                break;
                                
                        case MXT_PROCI_ONETOUCHGESTUREPROCESSOR_T24:
                                calculate_config_data_crc(&my_config_crc, pdata->config_T24, object_size);
                                break;
                                
                        case MXT_SPT_SELFTEST_T25:
                                calculate_config_data_crc(&my_config_crc, pdata->config_T25, object_size);
                                break;
                                
                        case MXT_PROCI_TWOTOUCHGESTUREPROCESSOR_T27:
                                calculate_config_data_crc(&my_config_crc, pdata->config_T27, object_size);
                                break;
                                
                        case MXT_SPT_CTECONFIG_T28:
                                calculate_config_data_crc(&my_config_crc, pdata->config_T28, object_size);
                                break;
                                
                        case MXT_PROCI_GRIPSUPPRESSION_T40:
                                calculate_config_data_crc(&my_config_crc, pdata->config_T40, object_size);
                                break;
                                
                        case MXT_PROCI_PALMSUPPRESSION_T41:
                                calculate_config_data_crc(&my_config_crc, pdata->config_T41, object_size);
                                break;
                                
                        case MXT_SPT_DIGITIZER_T43:
                                calculate_config_data_crc(&my_config_crc, pdata->config_T43, object_size);
                                break;
                                
                        default:
                                mxt_debug(DEBUG_TRACE, "warning: there are other object!\n");
                                break;
                }
                if(my_config_crc != config_crc)
                {
                        mxt_debug(DEBUG_TRACE, "the config is not matched, object_type:%d, chip crc:%d, crc:%d\n", 
                                           object_type, config_crc, my_config_crc);
                        mxt->configuration_crc = 0;
                        break;
                }
        }
        error = 0;
        
err_read_mxt_touch:
        if(mem_crc != NULL)
        {
                kfree(mem_crc);
                mem_crc = NULL;
        }
err_allocate_mem:
        
        return error;
}

static int  mxt_reload_config_data(struct i2c_client *client,
                                                                struct mxt_data *mxt)
{
        int	i;
        int	error;
        u8	object_type;
        u16	object_address;
        struct mxt_platform_data *pdata;

        error = 0;
        pdata = client->dev.platform_data;

        for (i = 0; i < mxt->device_info.num_objs; i++) 
        {
                object_type = mxt->object_table[i].type;
                object_address = mxt->object_table[i].chip_addr;
                if(!IS_CONFIG_OBJECT(object_type))
                {        
                        continue;
                }
                switch(object_type){
                        case MXT_GEN_POWERCONFIG_T7:
                                error = mxt_write_block(mxt->client, 
                                                        get_object_address(mxt, MXT_GEN_POWERCONFIG_T7), 
                                                        get_object_size(mxt, MXT_GEN_POWERCONFIG_T7), 
                                                        pdata->config_T7);
                                break;
                        case MXT_GEN_ACQUIRECONFIG_T8:
                                error = mxt_write_block(mxt->client, 
                                                        get_object_address(mxt, MXT_GEN_ACQUIRECONFIG_T8), 
                                                        get_object_size(mxt, MXT_GEN_ACQUIRECONFIG_T8), 
                                                        pdata->config_T8);
                                break;
                        case MXT_TOUCH_MULTITOUCHSCREEN_T9:
                                error = mxt_write_block(mxt->client, 
                                                        get_object_address(mxt, MXT_TOUCH_MULTITOUCHSCREEN_T9), 
                                                        get_object_size(mxt, MXT_TOUCH_MULTITOUCHSCREEN_T9), 
                                                        pdata->config_T9);
                                break;
                        case MXT_TOUCH_KEYARRAY_T15:
                                error = mxt_write_block(mxt->client, 
                                                        get_object_address(mxt, MXT_TOUCH_KEYARRAY_T15), 
                                                        get_object_size(mxt, MXT_TOUCH_KEYARRAY_T15), 
                                                        pdata->config_T15);
                                break;
                        case MXT_SPT_COMMSCONFIG_T18:
                                error = mxt_write_block(mxt->client, 
                                                        get_object_address(mxt, MXT_SPT_COMMSCONFIG_T18), 
                                                        get_object_size(mxt, MXT_SPT_COMMSCONFIG_T18), 
                                                        pdata->config_T18);
                                break;
                        case MXT_PROCI_GRIPFACESUPPRESSION_T20:
                                error = mxt_write_block(mxt->client, 
                                                        get_object_address(mxt, MXT_PROCI_GRIPFACESUPPRESSION_T20), 
                                                        get_object_size(mxt, MXT_PROCI_GRIPFACESUPPRESSION_T20), 
                                                        pdata->config_T20);
                                break;
                                
                        case MXT_PROCG_NOISESUPPRESSION_T22:
                                error = mxt_write_block(mxt->client, 
                                                        get_object_address(mxt, MXT_PROCG_NOISESUPPRESSION_T22), 
                                                        get_object_size(mxt, MXT_PROCG_NOISESUPPRESSION_T22), 
                                                        pdata->config_T22);
                                break;
                         case MXT_PROCI_ONETOUCHGESTUREPROCESSOR_T24:
                                error = mxt_write_block(mxt->client, 
                                                        get_object_address(mxt, MXT_PROCI_ONETOUCHGESTUREPROCESSOR_T24), 
                                                        get_object_size(mxt, MXT_PROCI_ONETOUCHGESTUREPROCESSOR_T24), 
                                                        pdata->config_T24);
                                break;
                         case MXT_SPT_SELFTEST_T25:
                                error = mxt_write_block(mxt->client, 
                                                        get_object_address(mxt, MXT_SPT_SELFTEST_T25), 
                                                        get_object_size(mxt, MXT_SPT_SELFTEST_T25), 
                                                        pdata->config_T25);
                                break;
                         case MXT_PROCI_TWOTOUCHGESTUREPROCESSOR_T27:
                                error = mxt_write_block(mxt->client, 
                                                        get_object_address(mxt, MXT_PROCI_TWOTOUCHGESTUREPROCESSOR_T27), 
                                                        get_object_size(mxt, MXT_PROCI_TWOTOUCHGESTUREPROCESSOR_T27), 
                                                        pdata->config_T27);
                                break;
                         case MXT_SPT_CTECONFIG_T28:
                                error = mxt_write_block(mxt->client, 
                                                        get_object_address(mxt, MXT_SPT_CTECONFIG_T28), 
                                                        get_object_size(mxt, MXT_SPT_CTECONFIG_T28), 
                                                        pdata->config_T28);
                                break;
                        case MXT_PROCI_GRIPSUPPRESSION_T40:
                                error = mxt_write_block(mxt->client, 
                                                        get_object_address(mxt, MXT_PROCI_GRIPSUPPRESSION_T40), 
                                                        get_object_size(mxt, MXT_PROCI_GRIPSUPPRESSION_T40), 
                                                        pdata->config_T40);
                                break;
                        case MXT_PROCI_PALMSUPPRESSION_T41:
                                error = mxt_write_block(mxt->client, 
                                                        get_object_address(mxt, MXT_PROCI_PALMSUPPRESSION_T41), 
                                                        get_object_size(mxt, MXT_PROCI_PALMSUPPRESSION_T41), 
                                                        pdata->config_T41);
                                break;
                        case MXT_SPT_DIGITIZER_T43:
                                error = mxt_write_block(mxt->client, 
                                                        get_object_address(mxt, MXT_SPT_DIGITIZER_T43), 
                                                        get_object_size(mxt, MXT_SPT_DIGITIZER_T43), 
                                                        pdata->config_T43);
                                break;

                        }
                        if (error < 0) 
                        {
                                printk(KERN_DEBUG "maXTouch Object could not be writed!\n");
                                error = -EIO;
                                return error;
                        }
        
        }
        mxt_write_byte(mxt->client, 
                        get_object_address(mxt, MXT_GEN_COMMANDPROCESSOR_T6) + MXT_ADR_T6_BACKUPNV, 0x55);
        msleep(10);

        mxt_write_byte(mxt->client, 
                        get_object_address(mxt, MXT_GEN_COMMANDPROCESSOR_T6) + MXT_ADR_T6_RESET, 0x11);
        msleep(200);
        mxt_debug(DEBUG_TRACE, " reload the Config data completed\n");
        
        return error;
        
}
#endif

/*
 * Reads the object table from maXTouch chip to get object data like
 * address, size, report id. For Info Block CRC calculation, already read
 * id data is passed to this function too (Info Block consists of the ID
 * block and object table).
 *
 */
static int __devinit mxt_read_object_table(struct i2c_client *client,
					   struct mxt_data *mxt,
					   u8 *raw_id_data)
{
        u16	report_id_count;
        u8	buf[MXT_OBJECT_TABLE_ELEMENT_SIZE];
        u8   *raw_ib_data;
        u8	object_type;
        u16	object_address;
        u16	object_size;
        u8	object_instances;
        u8	object_report_ids;
        u16	object_info_address;
        u32	crc;
        u32 calculated_crc;
        int	i;
        int	error;

        u8	object_instance;
        u8	object_report_id;
        u8	report_id;
        int   first_report_id;
        int     ib_pointer;
        struct mxt_object *object_table;

        object_table = kzalloc(sizeof(struct mxt_object) *
        		       mxt->device_info.num_objs,
        		       GFP_KERNEL);
        if (object_table == NULL) 
        {
                printk(KERN_WARNING "maXTouch: Memory allocation failed!\n");
                error = -ENOMEM;
                goto err_object_table_alloc;
        }

        raw_ib_data = kmalloc(MXT_OBJECT_TABLE_ELEMENT_SIZE *
        		mxt->device_info.num_objs + MXT_ID_BLOCK_SIZE,
        		GFP_KERNEL);
        if (raw_ib_data == NULL) 
        {
                printk(KERN_WARNING "maXTouch: Memory allocation failed!\n");
                error = -ENOMEM;
                goto err_ib_alloc;
        }

        /* Copy the ID data for CRC calculation. */
        memcpy(raw_ib_data, raw_id_data, MXT_ID_BLOCK_SIZE);
        ib_pointer = MXT_ID_BLOCK_SIZE;

        mxt->object_table = object_table;
        object_info_address = MXT_ADDR_OBJECT_TABLE;

        report_id_count = 0;
        for (i = 0; i < mxt->device_info.num_objs; i++) 
        {
                mxt_debug(DEBUG_TRACE, "Reading maXTouch at [0x%04x]: ",
                	  object_info_address);

                error = mxt_read_block(client, object_info_address,
                		       MXT_OBJECT_TABLE_ELEMENT_SIZE, buf);

                if (error < 0) 
                {
                        mxt->read_fail_counter++;
                        dev_err(&client->dev,
                        	"maXTouch Object %d could not be read\n", i);
                        error = -EIO;
                        goto err_object_read;
                }

                memcpy(raw_ib_data + ib_pointer, buf, 
                        MXT_OBJECT_TABLE_ELEMENT_SIZE);
                ib_pointer += MXT_OBJECT_TABLE_ELEMENT_SIZE;

                object_type       =  buf[0];
                object_address    = (buf[2] << 8) + buf[1];
                object_size         =  buf[3] + 1;
                object_instances  =  buf[4] + 1;
                object_report_ids =  buf[5];
                mxt_debug(DEBUG_TRACE, "Type=%03d, Address=0x%04x, "
                	  "Size=0x%02x, %d instances, %d report id's\n",
                	  object_type,
                	  object_address,
                	  object_size,
                	  object_instances,
                	  object_report_ids
                );

                /* Save frequently needed info. */
                if (object_type == MXT_GEN_MESSAGEPROCESSOR_T5) 
                {
                	mxt->msg_proc_addr = object_address;
                	mxt->message_size = object_size;
                }

                object_table[i].type            = object_type;
                object_table[i].chip_addr       = object_address;
                object_table[i].size            = object_size;
                object_table[i].instances       = object_instances;
                object_table[i].num_report_ids  = object_report_ids;
                report_id_count += object_instances * object_report_ids;
                object_info_address += MXT_OBJECT_TABLE_ELEMENT_SIZE;

        }

        mxt->rid_map =
                kzalloc(sizeof(struct report_id_map) * (report_id_count + 1),
        		/* allocate for report_id 0, even if not used */
        		GFP_KERNEL);
        if (mxt->rid_map == NULL) 
        {
                printk(KERN_WARNING "maXTouch: Can't allocate memory!\n");
                error = -ENOMEM;
                goto err_rid_map_alloc;
        }

        mxt->report_id_count = report_id_count;
        if (report_id_count > 254) 
        {	/* 0 & 255 are reserved */
                dev_err(&client->dev,
                        "Too many maXTouch report id's [%d]\n",
                        report_id_count);
                error = -ENXIO;
                goto err_max_rid;
        }

        /* Create a mapping from report id to object type */
        report_id = 1; /* Start from 1, 0 is reserved. */

        /* Create table associating report id's with objects & instances */
        for (i = 0; i < mxt->device_info.num_objs; i++) 
        {
                for (object_instance = 0;
                        object_instance < object_table[i].instances;
                        object_instance++)
                {
                        first_report_id = report_id;
                        for (object_report_id = 0;
                                object_report_id < object_table[i].num_report_ids;
                                object_report_id++) 
                        {
                                mxt->rid_map[report_id].object =
                                        object_table[i].type;
                                mxt->rid_map[report_id].instance =
                                        object_instance;
                                mxt->rid_map[report_id].first_rid =
                                        first_report_id;
                                report_id++;
                        }
                }
        }

        /* Read 3 byte CRC */
        error = mxt_read_block(client, object_info_address, 3, buf);
        if (error < 0) 
        {
        	mxt->read_fail_counter++;
        	dev_err(&client->dev, "Error reading CRC\n");
        }

        crc = (buf[2] << 16) | (buf[1] << 8) | buf[0];

        if (calculate_infoblock_crc(&calculated_crc, raw_ib_data,
        			    ib_pointer)) 
        {
        	printk(KERN_WARNING "Error while calculating CRC!\n");
        	calculated_crc = 0;
        }
        kfree(raw_ib_data);

        mxt_debug(DEBUG_TRACE, "Reported info block CRC = 0x%6X\n", crc);
        mxt_debug(DEBUG_TRACE, "Calculated info block CRC = 0x%6X\n\n", calculated_crc);

        if (crc == calculated_crc) 
        {
                mxt->info_block_crc = crc;
        } 
        else 
        {
                mxt->info_block_crc = 0;
                printk(KERN_ALERT "maXTouch: Info block CRC invalid!\n");
        }
        if (debug >= DEBUG_VERBOSE) 
        {
                dev_info(&client->dev, "maXTouch: %d Objects\n",
                	mxt->device_info.num_objs);

                for (i = 0; i < mxt->device_info.num_objs; i++) 
                {
                        dev_info(&client->dev, "Type:\t\t\t[%d]: %s\n",
                                object_table[i].type,
                                object_type_name[object_table[i].type]);
                        dev_info(&client->dev, "\tAddress:\t0x%04X\n",
                                object_table[i].chip_addr);
                        dev_info(&client->dev, "\tSize:\t\t%d Bytes\n",
                                object_table[i].size);
                        dev_info(&client->dev, "\tInstances:\t%d\n",
                                object_table[i].instances);
                        dev_info(&client->dev, "\tReport Id's:\t%d\n",
                                object_table[i].num_report_ids);
                }
        }
        return 0;

err_max_rid:
        kfree(mxt->rid_map);
err_rid_map_alloc:
err_object_read:
        kfree(raw_ib_data);
err_ib_alloc:
        kfree(object_table);
err_object_table_alloc:
        
        return error;
}

static int mxt_suspend(struct device *dev)
{
        unsigned ret = -ENODEV;
        int error;
        int8_t config_T7[3] = {0, 0, 0};
        struct mxt_data *mxt = dev_get_drvdata(dev);
         
        if (device_may_wakeup(dev))
        {
                if (mxt->client->irq)
                        enable_irq_wake(mxt->client->irq);
        }
        del_timer(&mxt->timer);
        /* disable worker */
        disable_irq_nosync(mxt->client->irq);
        ret = cancel_work_sync(&mxt->work);

        if (ret)
                enable_irq(mxt->client->irq);

        mxt->finger_pressed = 0;
        mxt->finger_count = 0;

        mxt->pre_data[0] =0;
        //qt_timer_state=0;
        
        //mxt->power_on(0);
        if (mxt->device_info.family_id == MXT224_FAMILYID)
        {// 7 inch touchscreen
            if ((hw_ver == HW_VERSION_V55_A) || (hw_ver == HW_VERSION_V55_B) ||
                (hw_ver == HW_VERSION_V71A_A) || (hw_ver == HW_VERSION_V71A_B) ||
                (hw_ver == HW_VERSION_V9X_A) || (hw_ver == HW_VERSION_V9X_B) ||
                (hw_ver == HW_VERSION_V66_A) ) 
            {// old board not support power control
                if (debug >= DEBUG_TRACE)
                    printk(KERN_INFO "Atmel probe hardware version:0x%x\n", hw_ver);
                error = mxt_write_block(mxt->client, 
                            get_object_address(mxt, MXT_GEN_POWERCONFIG_T7), 
                            get_object_size(mxt, MXT_GEN_POWERCONFIG_T7), 
                            config_T7);
                if (error < 0)
                {
                    printk(KERN_WARNING "mxt_suspend, write T7 fail\n");
                }
            }
            else 
            {
                mxt->power_on(0);
            }
        }
        else // 10 inch touchscreen
        {
            //mxt->power_on(0);
            error = mxt_write_block(mxt->client, 
                        get_object_address(mxt, MXT_GEN_POWERCONFIG_T7), 
                        get_object_size(mxt, MXT_GEN_POWERCONFIG_T7), 
                        config_T7);
            if (error < 0)
            {
                printk(KERN_WARNING "mxt_suspend, write T7 fail\n");
            }
        }
        return 0;
}

static int mxt_resume(struct device *dev)
{
        int8_t config_T7[3] = {15, 255, 30};
        int error;
        struct mxt_data *mxt = dev_get_drvdata(dev);
        //mxt->power_on(1);

        if (mxt->device_info.family_id == MXT224_FAMILYID) 
        {// 7 inch touchscreen
            if ((hw_ver == HW_VERSION_V55_A) || (hw_ver == HW_VERSION_V55_B) ||
                (hw_ver == HW_VERSION_V71A_A) || (hw_ver == HW_VERSION_V71A_B) ||
                (hw_ver == HW_VERSION_V9X_A) || (hw_ver == HW_VERSION_V9X_B)  ||
                (hw_ver == HW_VERSION_V66_A)) 
            {// old board not support power control
                if (debug >= DEBUG_TRACE)
                    printk(KERN_INFO "Atmel probe hardware version:0x%x\n", hw_ver);
                error = mxt_write_block(mxt->client, 
                        get_object_address(mxt, MXT_GEN_POWERCONFIG_T7), 
                        get_object_size(mxt, MXT_GEN_POWERCONFIG_T7), 
                        config_T7);
                if (error < 0)
                {
                    printk(KERN_WARNING "mxt_resume, write T7 fail, and delay 30ms retry\n");
                    msleep(30);
                    error = mxt_write_block(mxt->client, 
                            get_object_address(mxt, MXT_GEN_POWERCONFIG_T7), 
                            get_object_size(mxt, MXT_GEN_POWERCONFIG_T7), 
                            config_T7);
                    if(error < 0)
                    {
                        printk(KERN_WARNING "mxt_resume, write T7 fail\n");
                    }
                }
                calibrate_chip(mxt);
            }
            else
            {
                mxt->power_on(1);
            }
        }
        else // 10 inch touchscreen
        {
            //mxt->power_on(1);
            error = mxt_write_block(mxt->client, 
                        get_object_address(mxt, MXT_GEN_POWERCONFIG_T7), 
                        get_object_size(mxt, MXT_GEN_POWERCONFIG_T7), 
                        config_T7);
            if (error < 0)
            {
                printk(KERN_WARNING "mxt_resume, write T7 fail, and delay 30ms retry\n");
                msleep(30);
                error = mxt_write_block(mxt->client, 
                        get_object_address(mxt, MXT_GEN_POWERCONFIG_T7), 
                        get_object_size(mxt, MXT_GEN_POWERCONFIG_T7), 
                        config_T7);
                if(error < 0)
                {
                    printk(KERN_WARNING "mxt_resume, write T7 fail\n");
                }
            }
            calibrate_chip(mxt);
        }
        
        if (device_may_wakeup(dev))
        {
                if (mxt->client->irq)
                        disable_irq_wake(mxt->client->irq);
        }

        /* re-enable the interrupt prior to wake device */
        if (mxt->client->irq)
                enable_irq(mxt->client->irq);
        
        mod_timer(&mxt->timer, jiffies + TOUCHSCREEN_TIMEOUT);
        
        return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mxt_early_suspend(struct early_suspend *h)
{
	struct mxt_data *ts = container_of(h, struct mxt_data, early_suspend);

	mxt_suspend(&ts->client->dev);
}

static void mxt_late_resume(struct early_suspend *h)
{
	struct mxt_data *ts = container_of(h, struct mxt_data, early_suspend);

	mxt_resume(&ts->client->dev);
}
#endif

#if defined(GET_TOUCHSCREEN_INFORMATION)
static uint8_t  zte_ts_major_version;
static uint8_t  zte_ts_minor_version;
static uint8_t  zte_ts_i2c_address;
struct proc_dir_entry *parent =  NULL;
static int atmel_write_proc_info(char *page, char **start, off_t offset,
					int count, int *eof, void *data)
{
        *eof = 1;

        return sprintf(page, "   name : Atmel\n" 
                "   i2c address : 0x%x\n"
                "   IC type : 2000 series\n"
                "   fireware version : %d.%d\n"
                "   module : Atmel+EELY\n", 
                zte_ts_i2c_address, zte_ts_major_version, zte_ts_minor_version);
}
#endif
static int __devinit mxt_probe(struct i2c_client *client,
			       const struct i2c_device_id *id)
{
        struct mxt_data               *mxt;
        struct mxt_platform_data *pdata;
        struct input_dev            *input;
        u8                                   *id_data;
        int                                    error;

        mxt_debug(DEBUG_INFO, "atmel mxt_probe\n");

        if (client == NULL) {
        	pr_err("maXTouch: client == NULL\n");
        	return	-EINVAL;
        } else if (client->adapter == NULL) {
        	pr_err("maXTouch: client->adapter == NULL\n");
        	return	-EINVAL;
        } else if (&client->dev == NULL) {
        	pr_err("maXTouch: client->dev == NULL\n");
        	return	-EINVAL;
        } else if (&client->adapter->dev == NULL) {
        	pr_err("maXTouch: client->adapter->dev == NULL\n");
        	return	-EINVAL;
        } else if (id == NULL) {
        	pr_err("maXTouch: id == NULL\n");
        	return	-EINVAL;
        }

        /* Check if the I2C bus supports BYTE transfer */
        error = i2c_check_functionality(client->adapter,
        		I2C_FUNC_I2C);
        if (!error) 
        {
        	dev_err(&client->dev, "%s adapter not supported\n",
        			dev_driver_string(&client->adapter->dev));
        	return -ENODEV;
        }

        /* Allocate structure - we need it to identify device */
        mxt = kzalloc(sizeof(struct mxt_data), GFP_KERNEL);
        if (mxt == NULL) 
        {
        	dev_err(&client->dev, "insufficient memory\n");
        	error = -ENOMEM;
        	goto err_mxt_alloc;
        }

        id_data = kmalloc(MXT_ID_BLOCK_SIZE, GFP_KERNEL);
        if (id_data == NULL) 
        {
        	dev_err(&client->dev, "insufficient memory\n");
        	error = -ENOMEM;
        	goto err_id_alloc;
        }

        input = input_allocate_device();
        if (!input) 
        {
        	dev_err(&client->dev, "error allocating input device\n");
        	error = -ENOMEM;
        	goto err_input_dev_alloc;
        }

        /* Initialize Platform data */
        pdata = client->dev.platform_data;
        if (pdata == NULL) 
        {
        	dev_err(&client->dev, "platform data is required!\n");
        	error = -EINVAL;
        	goto err_pdata;
        }

        mxt->finger_pressed = 0;
        mxt->finger_count    = 0;
        
        mxt->read_fail_counter = 0;
        mxt->message_counter = 0;
        mxt->max_x_val          = pdata->max_x;
        mxt->max_y_val          = pdata->max_y;

        /* Get data that is defined in board specific code. */
        mxt->init_hw    = pdata->init_platform_hw;
        mxt->exit_hw   = pdata->exit_platform_hw;
        mxt->read_chg = pdata->read_chg;
        mxt->numtouch = pdata->numtouch;

        mxt->power_on = pdata->power_on;

        mxt->client = client;
        mxt->input  = input;

        mxt->pre_data[0] = 0;  
        
        if (mxt->init_hw != NULL)
        {
        	mxt->init_hw();
        }
        msleep(20);
        
        if (mxt_identify(client, mxt, id_data) < 0) 
        {
        	dev_err(&client->dev, "Chip could not be identified\n");
        	error = -ENODEV;
        	goto err_identify;
        }

        mxt->atmel_wq = create_singlethread_workqueue("atmel_wq");
        if (!mxt->atmel_wq) 
        {
        	printk(KERN_ERR"%s: create workqueue failed\n", __func__);
        	error = -ENOMEM;
        	goto err_cread_wq_failed;
        }

        INIT_WORK(&mxt->work, mxt_work_func);

        input->name = "atmel-touchscreen";
        input->phys = mxt->phys_name;
        input->id.bustype = BUS_I2C;
        input->dev.parent = &client->dev;

        __set_bit(EV_ABS, input->evbit);
 //       __set_bit(EV_SYN, input->evbit);
        __set_bit(EV_KEY, input->evbit);
        __set_bit(EV_MSC, input->evbit);

        __set_bit(BTN_TOUCH, input->keybit);
//        __set_bit(BTN_2, input->keybit);
//        __set_bit(KEY_MENU, input->keybit);
//        __set_bit(KEY_HOME, input->keybit);
//        __set_bit(KEY_BACK, input->keybit);

        //input->mscbit[0] = BIT_MASK(MSC_GESTURE);

#ifdef ABS_MT_TRACKING_ID
        /* Multitouch */
        input_set_abs_params(input, ABS_MT_POSITION_X, 0, mxt->max_x_val, 0, 0);
        input_set_abs_params(input, ABS_MT_POSITION_Y, 0, mxt->max_y_val, 0, 0);
        input_set_abs_params(input, ABS_MT_TOUCH_MAJOR, 0, MXT_MAX_TOUCH_SIZE,
        		     0, 0);
		input_set_abs_params(input, ABS_MT_PRESSURE, 0,255, 0, 0);			 
        input_set_abs_params(input, ABS_MT_WIDTH_MAJOR, 0, MXT_MAX_WIDTH_SIZE,
        		     0, 0);
        input_set_abs_params(input, ABS_MT_TRACKING_ID, 0, mxt->numtouch, 0, 0);
		
#else
        /* Single touch */
        input_set_abs_params(input, ABS_X, 0, mxt->max_x_val, 0, 0);
        input_set_abs_params(input, ABS_Y, 0, mxt->max_y_val, 0, 0);
        input_set_abs_params(input, ABS_PRESSURE, 0, MXT_MAX_REPORTED_PRESSURE,
        		     0, 0);
        input_set_abs_params(input, ABS_TOOL_WIDTH, 0, MXT_MAX_REPORTED_WIDTH,
        		     0, 0);
#endif

        input_set_drvdata(input, mxt);
        i2c_set_clientdata(client, mxt);
        mxt_debug(DEBUG_TRACE, "maXTouch driver input register device\n");
        error = input_register_device(mxt->input);
        if (error < 0) 
        {
                dev_err(&client->dev,
                	"Failed to register input device\n");
                goto err_register_device;
        }
        
        error = mxt_read_object_table(client, mxt, id_data);
        if (error < 0)
        	goto err_read_ot;

#ifdef CHECK_TS_CONFIG_DATA_ZTE
        error = mxt_check_config_data(client, mxt);
        if(error < 0)
        {
                mxt_debug(DEBUG_TRACE, "Error check config data CRC\n");
        }
        
        if(!mxt->configuration_crc)
        {
                mxt_debug(DEBUG_TRACE, "Touchscreen CRC Check Fail!, reload the Config data to chip\n");
                mxt_reload_config_data(client, mxt);
        }
#endif
        
        /* Allocate the interrupt */
        mxt->irq = client->irq;
        if (mxt->irq) 
        {
                /* Try to request IRQ with falling edge first. This is
                * not always supported. If it fails, try with any edge. */
                error = request_irq(mxt->irq,
                                                mxt_irq_handler,
                                                IRQF_TRIGGER_LOW,
                                                client->dev.driver->name,
                                                mxt);
                if (error < 0) 
                {
                        dev_err(&client->dev,
                                        "failed to allocate irq %d\n", mxt->irq);
                        goto err_irq;
                }
        }
        
#ifdef CONFIG_HAS_EARLYSUSPEND
        mxt->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN +1;
        mxt->early_suspend.suspend = mxt_early_suspend;
        mxt->early_suspend.resume = mxt_late_resume;
        register_early_suspend(&mxt->early_suspend);
#endif
#if defined(GET_TOUCHSCREEN_INFORMATION)        
        zte_ts_major_version = mxt->device_info.major;
        zte_ts_minor_version = mxt->device_info.minor;
        zte_ts_i2c_address = mxt->client->addr;

        parent = proc_mkdir("touchscreen", NULL);
        if (create_proc_read_entry("TouchScreen_Info", 0,
        		parent, atmel_write_proc_info, NULL) == NULL) 
        {
        	    printk(KERN_WARNING "Atmel unable to create /proc/touchscreen/ts_information entry");
        	    remove_proc_entry("TouchScreen_Info", 0);
        }
#endif
        setup_timer(&mxt->timer, mxt_timer, (unsigned long) mxt);
        mod_timer(&mxt->timer, jiffies + TOUCHSCREEN_TIMEOUT);

        kfree(id_data);
        return 0;

err_irq:
        kfree(mxt->rid_map);
        kfree(mxt->object_table);
err_read_ot:
err_register_device:
        destroy_workqueue(mxt->atmel_wq);
        
err_cread_wq_failed:
err_identify:
        if (mxt->exit_hw != NULL)
        	mxt->exit_hw();
        
err_pdata:
        if (input)
                input_free_device(input);
        
err_input_dev_alloc:
        kfree(id_data);

err_id_alloc:
        kfree(mxt);
        
err_mxt_alloc:
        
        return error;
}

static int __devexit mxt_remove(struct i2c_client *client)
{
        struct mxt_data *mxt;

        mxt = i2c_get_clientdata(client);

        if (mxt == NULL) 
        {
                return -1;
        }
        
        if (mxt->exit_hw != NULL)
        {       
                mxt->exit_hw();
        }
        if (mxt->irq) 
        {
                free_irq(mxt->irq, mxt);
        }
        del_timer(&mxt->timer);
        
        destroy_workqueue(mxt->atmel_wq);
        input_unregister_device(mxt->input);

        if(mxt->rid_map != NULL)
        {
                kfree(mxt->rid_map);
                mxt->rid_map = NULL;
        }
        if(mxt->object_table != NULL)
        {
                kfree(mxt->object_table);
                mxt->object_table = NULL;
        }
        kfree(mxt);
        mxt = NULL;

        i2c_set_clientdata(client, NULL);
        if (debug >= DEBUG_TRACE)
                dev_info(&client->dev, "Touchscreen unregistered\n");

        return 0;
}

static const struct i2c_device_id mxt_idtable[] = {
        {"AtmelMxt_i2c", 0,}, 
        { }
};

MODULE_DEVICE_TABLE(i2c, mxt_idtable);

static const struct dev_pm_ops mxt_pm_ops = {
	.suspend = mxt_suspend,
	.resume = mxt_resume,
};

static struct i2c_driver mxt_driver = {
        .driver = {
        	.name	= "AtmelMxt_i2c", 
        	.owner  = THIS_MODULE,
#ifndef CONFIG_HAS_EARLYSUSPEND              	
        	.pm = &mxt_pm_ops,
#endif        	
        },

        .id_table	= mxt_idtable,
        .probe	= mxt_probe,
        .remove	= __devexit_p(mxt_remove),
};

extern int zte_ftm_mod_ctl;

static int __init mxt_init(void)
{
        int err;
       if(zte_ftm_mod_ctl)
       {
          printk(KERN_ERR "ftm mode no regesiter atmel ts!\n");
	   return 0;	  
       }
              
        err = i2c_add_driver(&mxt_driver);
        if (err) 
        {
                printk(KERN_WARNING "Adding maXTouch driver failed (errno = %d)\n", err);
        } 
        else 
        {
                mxt_debug(DEBUG_TRACE, "Successfully added driver %s\n", mxt_driver.driver.name);
        }
        return err;
}

static void __exit mxt_cleanup(void)
{
        i2c_del_driver(&mxt_driver);
#if defined(GET_TOUCHSCREEN_INFORMATION)           
        remove_proc_entry("TouchScreen_Info", 0);
#endif
}

module_init(mxt_init);
module_exit(mxt_cleanup);

MODULE_AUTHOR("Iiro Valkonen");
MODULE_DESCRIPTION("Driver for Atmel maXTouch Touchscreen Controller");
MODULE_LICENSE("GPL");
