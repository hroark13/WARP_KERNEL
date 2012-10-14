/*
 * ON Semiconductor bs3 device driver
*/
/*===========================================================================

                        EDIT HISTORY 

when              comment tag        who                  what, where, why                           
----------    ------------     -----------      --------------------------      
2011/06/20   gouyajun0012    gouyajun           Optimizing code and add error mechanism

===========================================================================*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>

#include <linux/string.h>
#include <linux/list.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/i2c/belasigna300.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>

#define CMD_WRITE_MEMORY 'W'
#include <linux/i2c/belasigna300_data.h>

#define I2C_M_WR 			0
//#define I2C_M_RD 			1


#define BS300_POWER_MODE_DOWN 0
#define BS300_POWER_MODE_ACTIVE 1

#define BS300_DEVICE_ADDRESS  					96
#define BS300_CMD_GET_STATUS     				'S'
#define BS300_CMD_STOP_CORE					'P'
#define BS300_CMD_WRITE_SPECIAL_REGISTERS 	'2'
#define BS300_SPECIAL_REGISTER_LP				0x0C
#define BS300_CMD_WRITE_NORMAL_REGISTERS	'F'
#define BS300_NORMAL_REGISTER_SR 				0x32
#define BS300_CMD_READ_AND_RESET_CRC 		'M'
#define BS300_CMD_EXECUTE_INSTRUCTION 		'O'
#define BS300_CMD_START_CORE 					'G'
#define BS300_CMD_SET_MODE_ADDR				0x0A

#define STATUS_SECURITY_MODE        			1<<3
#define STATUS_SECURITY_UNRESTRICTED		0<<3



static unsigned char Cmd_get_status[] = {BS300_CMD_GET_STATUS/* 'S' */};
static unsigned char Cmd_stop[] = {BS300_CMD_STOP_CORE /* 'P' */};
static unsigned char Cmd_reset_lp[] = {BS300_CMD_WRITE_SPECIAL_REGISTERS/* '2' */, BS300_SPECIAL_REGISTER_LP/*0x0c*/, 0x00, 0x00};
static unsigned char Cmd_reset_sr[] = {BS300_CMD_WRITE_NORMAL_REGISTERS/* 'F' */, BS300_NORMAL_REGISTER_SR/*0x32*/, 0x00, 0x00, 0x00};
static unsigned char Cmd_read_reset_CRC[] = {BS300_CMD_READ_AND_RESET_CRC /* 'M' */};
static unsigned char Cmd_Execute[] = {BS300_CMD_EXECUTE_INSTRUCTION/* 'O' */, 0x3B, 0x20, 0x10, 0x0};
static unsigned char Cmd_start[] = {BS300_CMD_START_CORE/* 'G' */};
static unsigned char Cmd_Reset_Monitor[] = {0x43 /* 'C' */};
static unsigned char Cmd_set_active_mode[] = { 0x07, 0x00};
static unsigned char Cmd_set_bypass_mode[] = { 0x07, 0x01};
static unsigned char Cmd_set_high_sampling_rate_mode[] = {0x07, 0x02}; 
static unsigned char Cmd_get_current_mode[] = { 0x06};
static unsigned char Cmd_set_27db_mode[] = { 0x13, 0x00, 0x00, 0x06};//set gain to 27 db
static unsigned char Cmd_set_30db_mode[] = { 0x13, 0x00, 0x00, 0x07};//set gain to 30 db
static unsigned char Cmd_get_db_mode[] = { 0x12};//get gain

static unsigned char Cmd_set_digital_12db_mode[] = {0xD, 0x1, 0x0, 0x0, 0x2, 0x7F, 0xFF, 0xFF };//set digital gain to 12 db
//static unsigned char Cmd_set_digital_6db_mode[] =   {0xD, 0x1, 0x0, 0x0, 0x1, 0x7F, 0xFF, 0xFF };//set digital gain to 6 db

static unsigned char Cmd_set_digital_0db_mode[] = {0xD, 0x1, 0x0, 0x0, 0x0, 0x7F, 0xFF, 0xFF };//set digital gain to 0 db
static unsigned char Cmd_set_24db_mode[] = { 0x13, 0x00, 0x00, 0x05};//set gain to 24 db
static struct workqueue_struct *bela300_init_work_queue;

static int
bela300_data_write(struct i2c_client *client, unsigned short addr,
				  u16 alength, u8 *val)
{
	int err = 0;
	struct i2c_msg msg;

	if (!client->adapter) {
		dev_err(&client->dev, "<%s> ERROR: No bela300 Device\n", __func__);
		return -ENODEV;
	}

	msg.addr = addr;
	msg.flags = I2C_M_WR;
	msg.len = alength;
	msg.buf = val;

	/* high byte goes out first */

	
	//printk(KERN_ERR "msg->len = %d\n",msg->len);
	err = i2c_transfer(client->adapter, &msg, 1);

	if (err < 0) {
		dev_err(&client->dev, "<%s> ERROR:  i2c Block Write at , "
				      "*val=%d flags=%d "
				      "err=%d\n",
			__func__, val[0], msg.flags, err);
		return err;
	}
	return 0;
}

static int bela300_data_read(struct i2c_client *client, unsigned short addr,
				  u16 alength, u8 *val)
{
	int err = 0;
	struct i2c_msg msg;

	if (!client->adapter) {
		dev_err(&client->dev, "<%s> ERROR: No bela300 Device\n", __func__);
		return -ENODEV;
	}

	msg.addr = addr;
	msg.flags = I2C_M_RD;
	msg.len = alength;
	msg.buf = val;

	/* high byte goes out first */



	err = i2c_transfer(client->adapter, &msg, 1);

	if (err < 0) {
		dev_err(&client->dev, "<%s> ERROR:  i2c bela300_data read, "
				      "*val=%d flags=%d"
				      "err=%d\n",
			__func__, val[0], msg.flags, err);
		return err;
	}
	return 0;
}
int BS300_send_receive(struct i2c_client *client,int Innum, int OutNum, unsigned char* pInbuf, unsigned char* pOutbuf)
{
	int err = 0;

	err = bela300_data_write(client,client->addr,Innum,pInbuf);
	if(err < 0) {
		
		goto fail_err;
	}
	
       if ((OutNum == 0) || (NULL == pOutbuf)){
	   	
           return 0;
       }

	err = bela300_data_read(client, client->addr,OutNum,pOutbuf);
	if(err < 0) {
		
		goto fail_err;
	}

	return 0;
fail_err:

	return err;

}
static int  belasigna300_init_store(struct i2c_client *client)
{
	
	unsigned char status_bytes[2]; 
   	unsigned int cnt; 
	int err = 0;

	for (cnt = 0; cnt < 200; cnt ++) 
	{ 

		if (BS300_send_receive(client,1, 2, Cmd_get_status, status_bytes) == 0) 
		{

			if (((status_bytes[0] & 0x80) == 0x80) && ((status_bytes[1] & 0x80) == 0)) 
			{
	
   				if (STATUS_SECURITY_UNRESTRICTED == 
					(STATUS_SECURITY_MODE & status_bytes[0])) 
				{ 

					dev_err(&client->dev, 
					"<%s> OK: bela300 Device status good\n", __func__);
      				 	break; 
   				} 
			} 
		}
		else
			goto  fail_err;  

		msleep(20);
	} 
	   
	if (cnt == 200) 
	{ 

		dev_err(&client->dev, "<%s> ERROR: bela300 Device status fail\n", __func__);
		err = -1;
		goto  fail_err;  
	} 
	
	err = BS300_send_receive(client, 1, 0, Cmd_stop, NULL);
	if(err < 0) 
	{
		printk("send cmd stop err\n");
		goto fail_err;
	}
	
	err = BS300_send_receive(client, 1, 0, Cmd_Reset_Monitor, NULL) ;
	if(err < 0) 
	{
		printk("send Cmd_Reset_Monitorerr err \n");
		goto fail_err;
	}

	err = BS300_send_receive(client, sizeof(Cmd_reset_lp), 0, Cmd_reset_lp, NULL);
	if(err < 0) 
	{
		printk("send Cmd_reset_lp err \n");
		goto fail_err;
	}

	err = BS300_send_receive(client, sizeof(Cmd_reset_sr), 0, Cmd_reset_sr, NULL) ;
	if(err < 0) 
	{
		printk("send Cmd_reset_sr err \n");
		goto fail_err;
	}


	for (cnt = 0; cnt < DOWNLOAD_BLOCK_COUNT; cnt++) 
	{ 

		err = BS300_send_receive(client, 1, 0, Cmd_read_reset_CRC, NULL); 
		if(err < 0) 
		{
			printk("send Cmd_read_reset_CRC err\n");
			goto fail_err;
		}

		err = BS300_send_receive(client, downloadBlocks[cnt].byteCount, 0, 
				downloadBlocks[cnt].formattedData, NULL);
		if(err < 0) 
		{
			printk("send blocks err\n");
			goto fail_err;
		}

		err = BS300_send_receive(client, sizeof(Cmd_read_reset_CRC), 2, 
				Cmd_read_reset_CRC, status_bytes); 

		if(err < 0) 
		{
			printk("send Cmd_read_reset_CRC err\n");
			goto fail_err;
		}

		if (downloadBlocks[cnt].crc != ((status_bytes[0] << 8) | status_bytes[1])) 
		{ 
			dev_err(&client->dev, 
			"<%s> ERROR: bela300 download block crc error\n", __func__);
			err = -1;
			goto fail_err;

		} 
	} 

	err = BS300_send_receive(client, sizeof(Cmd_reset_lp), 0, Cmd_reset_lp, NULL); 
	if(err < 0) 
	{
		printk("send Cmd_reset_lp err \n");
		goto fail_err;
	}


	err = BS300_send_receive(client, sizeof(Cmd_reset_sr), 0, Cmd_reset_sr, NULL) ;
	if(err < 0) 
	{
		printk("send Cmd_reset_sr err \n");
		goto fail_err;
	}

	err = BS300_send_receive(client,sizeof(Cmd_Execute), 0, Cmd_Execute, NULL);
	if(err < 0) 
	{
		printk("send Cmd_Execute err \n");
		goto fail_err;
	}

	err = BS300_send_receive(client,sizeof(Cmd_start), 0, Cmd_start, NULL);
	if(err < 0) 
	{
		printk("send Cmd_start err \n");
		goto fail_err;
	}		
	printk("chenfei echo suppresstion start successfully!!\n");

	return 0;
fail_err:
	printk("chenfei echo suppresstion start fail!!\n");
	return err;
	
}


static void belasigna300_init_work(struct work_struct *work)
{
	struct bs300_platform_data *data;
	int rc;
	printk(KERN_ERR "belasigna300_init_work\n");
	data = container_of(work, struct bs300_platform_data, init_work);
	printk(KERN_ERR "belasigna300_init_work data = %x 2\n", (unsigned int)data);
	rc = belasigna300_init_store(data->client);
	if(rc < 0)
	{	
		printk(KERN_ERR "belasigna300 init store error\n");
	}
	

}
/*
 * Called when a bs300 device is matched with this driver
 */
struct bs300_platform_data *pdata;

static int belasigna300_proc_show(struct seq_file *m, void *v)
{
	int err = 0;
	unsigned char ret = 0;

	err = bela300_data_write(pdata->client,BS300_CMD_SET_MODE_ADDR,sizeof(Cmd_get_current_mode),Cmd_get_current_mode);
	if(err < 0) 
	{
		printk("send Cmd_get_current_mode err \n");
		return 0;
	}
	err = bela300_data_read(pdata->client,BS300_CMD_SET_MODE_ADDR,1,&ret);
	if(err < 0) 
	{
		printk("send Cmd_get_current_mode err \n");
		return 0;
	}

	printk("send Cmd_get_current_mode ret = %d \n", ret);
	if (ret == 0)
		seq_printf(m, "%d", 1); //active mode
	else if (ret == 1)
		seq_printf(m, "%d", 0); //bypass mode
	else
		seq_printf(m, "%d", ret);//other mode
	
	return 0;
}
static int belasigna300_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, belasigna300_proc_show, NULL);
}

static ssize_t belasigna300_proc_write(struct file *file, const char __user *buffer,
				    size_t count, loff_t *pos)
{	
	char c;
	int rc;
	int err = 0;
	unsigned char ret[4] = {0};
	
	rc = get_user(c, buffer);
	if (rc)
		return rc;

	if (c == '1' )
	{
		err = bela300_data_write(pdata->client,BS300_CMD_SET_MODE_ADDR, sizeof(Cmd_set_active_mode), Cmd_set_active_mode) ;
		if(err < 0) 
		{
			printk("send Cmd_set_active_mode err \n");
			return 1;
		}

		printk("send Cmd_set_active_mode \n");
		
		err = bela300_data_write(pdata->client,BS300_CMD_SET_MODE_ADDR, sizeof(Cmd_set_24db_mode), Cmd_set_24db_mode) ;
		if(err < 0) 
		{
			printk("send Cmd_set_24db_mode err \n");
			return 1;
		}
		
		printk("send Cmd_set_24db_mode \n");

		err = bela300_data_write(pdata->client,BS300_CMD_SET_MODE_ADDR, sizeof(Cmd_set_digital_0db_mode), Cmd_set_digital_0db_mode) ;
		if(err < 0) 
		{
			printk("send Cmd_set_digital_0db_mode err \n");
			return 1;
		}
		
		printk("send Cmd_set_digital_0db_mode \n");
		
		return 1;
	}
	else if (c == '0')
	{
		err = bela300_data_write(pdata->client,BS300_CMD_SET_MODE_ADDR, sizeof(Cmd_set_bypass_mode), Cmd_set_bypass_mode) ;
		if(err < 0) 
		{
			printk("send Cmd_set_bypass_mode err \n");
			return 1;
		}

		printk("send Cmd_set_bypass_mode \n");
		
		err = bela300_data_write(pdata->client,BS300_CMD_SET_MODE_ADDR, sizeof(Cmd_set_27db_mode), Cmd_set_27db_mode) ;
		if(err < 0) 
		{
			printk("send Cmd_set_27db_mode err \n");
			return 1;
		}
		
		printk("send Cmd_set_27db_mode \n");
		//return 1;
	}
	else if (c == '2')
	{
		err = bela300_data_write(pdata->client,BS300_CMD_SET_MODE_ADDR, sizeof(Cmd_set_high_sampling_rate_mode), Cmd_set_high_sampling_rate_mode) ;
		if(err < 0) 
		{
			printk("send Cmd_set_high_sampling_rate_mode err \n");
			return 1;
		}

		printk("send Cmd_set_high_sampling_rate_mode \n");
		
		err = bela300_data_write(pdata->client,BS300_CMD_SET_MODE_ADDR, sizeof(Cmd_set_30db_mode), Cmd_set_30db_mode) ;
		if(err < 0) 
		{
			printk("send Cmd_set_30db_mode err \n");
			return 1;
		}
		
		printk("send Cmd_set_30db_mode \n");

		err = bela300_data_write(pdata->client,BS300_CMD_SET_MODE_ADDR, sizeof(Cmd_set_digital_12db_mode), Cmd_set_digital_12db_mode) ;
		if(err < 0) 
		{
			printk("send Cmd_set_digital_12db_mode err \n");
			return 1;
		}
		
		printk("send Cmd_set_digital_12db_mode \n");
		
		//return 1;
	}
	if (c == '2' ||c == '0' || c == '1')
	{
		err = bela300_data_write(pdata->client,BS300_CMD_SET_MODE_ADDR, sizeof(Cmd_get_db_mode), Cmd_get_db_mode) ;
		if(err < 0) 
		{
			printk("send Cmd_get_db_mode err \n");
			return 1;
		}
		
		err = bela300_data_read(pdata->client,BS300_CMD_SET_MODE_ADDR,3,ret);
		if(err < 0) 
		{
			printk("send Cmd_get_db_mode 2 err \n");
			return 1;
		}	
		
		printk("Cmd_get_db_mode %02x, %02x, %02x \n", ret[0], ret[1], ret[2]);
}
	return 1;
}
static const struct file_operations belasigna300_proc_fops = {
	.open		= belasigna300_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.write		= belasigna300_proc_write,
};

static int belasigna300_proc_init(void)
{
 	struct proc_dir_entry *res;
	res = proc_create("belasigna300_switch", S_IWUGO | S_IRUGO, NULL,
			  &belasigna300_proc_fops);
	if (!res)
	{
		printk(KERN_INFO "failed to create /proc/belasigna300_switch\n");
		return -ENOMEM;
	}

	printk(KERN_INFO "created /proc/belasigna300_switch\n");
	return 0;
}

static int  __devexit belasigna300_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int rc;
    

	printk(KERN_ERR "belasigna300_probe\n");
	  pdata = client->dev.platform_data;
        if (pdata == NULL) 
        {
        	dev_err(&client->dev, "platform data is required!\n");
        	rc = -EINVAL;
        	goto exit;
        }

	
	if (!i2c_check_functionality(client->adapter,
				     I2C_FUNC_I2C)) {
		dev_err(&client->dev, "i2c bus does not support the belasigna300\n");
		rc = -ENODEV;
		printk(KERN_ERR "belasigna300_probe error\n");
		goto exit;
	}
	if (pdata->wake_up_chip != NULL){
		pdata->wake_up_chip(1);
	}
	pdata->client = i2c_use_client(client);
	
	if(pdata->client == NULL)
	{	
		printk(KERN_ERR "i2c_use_client error\n");		
	}
	INIT_WORK(&pdata->init_work, belasigna300_init_work);
	bela300_init_work_queue = create_workqueue("bela300_init");
	
	queue_work(bela300_init_work_queue, &pdata->init_work);
	printk(KERN_ERR "belasigna300_probe end\n");

	if ((rc = belasigna300_proc_init() )!= 0)
		goto exit;
		
	return 0;

 exit:
	return rc;
}
static int __devexit bs300_remove(struct i2c_client *client)
{
	if (pdata->wake_up_chip != NULL){
		pdata->wake_up_chip(0);
	}
	i2c_release_client(pdata->client);
	kfree(i2c_get_clientdata(client));
	printk(KERN_ERR "belasigna300 destroy_workqueue\n");
	
	destroy_workqueue(bela300_init_work_queue);
	return 0;
}
static const struct i2c_device_id belasigna300_id[] = {
	{ "bs300_i2c", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, belasigna300_id);

static int bs300_suspend(struct device *dev)
{
	struct bs300_platform_data * data = (struct bs300_platform_data*)dev->platform_data;
	if (data && data->wake_up_chip != NULL){
		data->wake_up_chip(0);
		printk("belasigna300 suspend\n");
	}
	
	return 0;
}

static int bs300_resume(struct device *dev)
{
	struct bs300_platform_data * data = (struct bs300_platform_data*)dev->platform_data;
	if (data && data->wake_up_chip != NULL){
		data->wake_up_chip(1);
		printk("belasigna300 resume\n");
	}
	return 0;
}


static const struct dev_pm_ops bs300_pm_ops = {
	.suspend = bs300_suspend,
	.resume = bs300_resume,
};



static struct i2c_driver belasigna300_driver = {
	.driver = {
		.name = "bs300_i2c",
		.owner  = THIS_MODULE,
		.pm = &bs300_pm_ops,   
	},
	.probe = belasigna300_probe,
	.remove = bs300_remove,
	.id_table = belasigna300_id,
	
};

extern int zte_ftm_mod_ctl;

static int __init belasigna300_init(void)
{
       if(zte_ftm_mod_ctl)
       {
          printk(KERN_ERR "ftm mode no regesiter belasigna300!\n");
	   return 0;	  
       }
	printk(KERN_ERR "belasigna300_init\n");
	return i2c_add_driver(&belasigna300_driver);
}

static void __exit belasigna300_exit(void)
{
	i2c_del_driver(&belasigna300_driver);
}

MODULE_AUTHOR("Kylin Gou <gou.yajun@zte.com.cn>");
MODULE_DESCRIPTION("belasigna300 audio processor driver");
MODULE_LICENSE("GPL");

late_initcall(belasigna300_init);
module_exit(belasigna300_exit);
