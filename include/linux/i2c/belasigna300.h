// Generated file. Do not modify.
#ifndef BELASIGNA300_H
#define BELASIGNA300_H
#include <linux/workqueue.h>
#include <linux/i2c.h>

struct bs300_platform_data {
	int (*wake_up_chip)(int);
	struct work_struct	 init_work;
	struct i2c_client *client;
	
};



#endif