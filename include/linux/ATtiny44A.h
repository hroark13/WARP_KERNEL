/*
 *  ATtiny44A.h - capacitance ATTINY44A prox driver
 *
 *  Copyright (C) 2011 ZTE
 *  zhaoyang196673 <zhao.yang3@zte.com.cn>
 *  Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __LINUX_ATTINY44A_H
#define __LINUX_ATTINY44A_H

//zhaoyang196673 add for capacitance ATTINY44A prox begin  
struct attiny44a_platform_data {
	unsigned dev1_int_gpio;
	unsigned dev1_pm_gpio;    
	unsigned dev2_int_gpio;
	unsigned dev2_pm_gpio; 

	int (*setup_dev1_int_gpio)(int enable);
	int (*setup_dev1_pm_gpio)(int enable);
    int (*setup_dev2_int_gpio)(int enable);
	int (*setup_dev2_pm_gpio)(int enable);

/** ZTE_MODIFY zhaoyang add enable function of capacitance ATTINY44A prox device zhaoy0020 2011-6-30*/
    int (*enable_dev)(void);
/** ZTE_MODIFY zhaoy0020 end*/
};
//zhaoyang196673 add for capacitance ATTINY44A prox end
#endif /* __LINUX_ATTINY44A_H */
