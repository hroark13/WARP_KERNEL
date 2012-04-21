/* include/asm/mach-msm/htc_pwrsink.h
 *
 * Copyright (C) 2008 HTC Corporation.
 * Copyright (C) 2007 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/hrtimer.h>
#include <../../../drivers/staging/android/timed_output.h>
#include <linux/sched.h>

#include <mach/msm_rpcrouter.h>

#define PM_LIBPROG      0x30000061
//change rpc ver number
#if (CONFIG_ZTE_PLATFORM)
#define PM_LIBVERS      0x00030005  
#define HTC_PROCEDURE_SET_VIB_ON_OFF	22
#else
#if (CONFIG_MSM_AMSS_VERSION == 6220) || (CONFIG_MSM_AMSS_VERSION == 6225)
#define PM_LIBVERS      0xfb837d0b
#else
#define PM_LIBVERS      0x10001
#endif

#define HTC_PROCEDURE_SET_VIB_ON_OFF	21
#endif
#define PMIC_VIBRATOR_LEVEL	(3000)

static struct work_struct work_vibrator_on;
static struct work_struct work_vibrator_off;
static struct hrtimer vibe_timer;

static volatile int g_vibrator_status=0;//represent the vibrator's real on off status; 1:on  0:off
struct timespec volatile g_ts_start;
struct timespec volatile g_ts_end;
static void set_pmic_vibrator(int on)
{
    static struct msm_rpc_endpoint *vib_endpoint;
    struct set_vib_on_off_req
    {
        struct rpc_request_hdr hdr;
        uint32_t data;
    }
    req;

    if (!vib_endpoint)
    {
        vib_endpoint = msm_rpc_connect(PM_LIBPROG, PM_LIBVERS, 0);
        if (IS_ERR(vib_endpoint))
        {
            printk(KERN_ERR "init vib rpc failed!\n");
            vib_endpoint = 0;
            return;
        }
    }


    if (on)
        req.data = cpu_to_be32(PMIC_VIBRATOR_LEVEL);
    else
        req.data = cpu_to_be32(0);

    msm_rpc_call(vib_endpoint, HTC_PROCEDURE_SET_VIB_ON_OFF, &req,
                 sizeof(req), 5 * HZ);
	
    if(on)
    {
        g_vibrator_status=1;
        g_ts_start = current_kernel_time();
    }
    else
    {
        if(g_vibrator_status==1)
        {
            g_ts_end = current_kernel_time();
            pr_info("vibrator vibrated %ld ms.\n",
                    (g_ts_end.tv_sec-g_ts_start.tv_sec)*1000+g_ts_end.tv_nsec/1000000-g_ts_start.tv_nsec/1000000
                   );
        }
        g_vibrator_status=0;
    }
}



static void pmic_vibrator_on(struct work_struct *work)
{
    if( g_vibrator_status==0)//if vibrator is on now
    {
        set_pmic_vibrator(1);
        pr_info("Q:pmic_vibrator_on,done\n");
    }
    else
    {
        pr_info("Q:pmic_vibrator_on, already on, do nothing.\n");
    }
}

static void pmic_vibrator_off(struct work_struct *work)
{
    if( g_vibrator_status==1)//if vibrator is on now
    {
        set_pmic_vibrator(0);
        pr_info("Q:pmic_vibrator_off,done\n"); 
    }
    else
    {
        pr_info("Q:pmic_vibrator_off, already off, do nothing.\n");
    }
}

static int timed_vibrator_on(struct timed_output_dev *sdev)
{
    if(!schedule_work(&work_vibrator_on))
    {
        pr_info("vibrator schedule on work failed !\n");
        return 0;
    }
    return 1;
}

static int timed_vibrator_off(struct timed_output_dev *sdev)
{
    if(!schedule_work(&work_vibrator_off))
    {
        pr_info("vibrator schedule off work failed !\n");
        return 0;
    }
    return 1;
}

static void vibrator_enable(struct timed_output_dev *dev, int value)
{
    pr_info("vibrator_enable,%d ms,vibrator:%s now.\n",value,g_vibrator_status?"on":"off");
    hrtimer_cancel(&vibe_timer);
    cancel_work_sync(&work_vibrator_on);
    cancel_work_sync(&work_vibrator_off);

    if (value == 0)
    {
        if(!timed_vibrator_off(dev))//if queue failed, delay 10ms try again by timer
        {
            value=10;
            hrtimer_start(&vibe_timer,
                          ktime_set(value / 1000, (value % 1000) * 1000000),
                          HRTIMER_MODE_REL);
            value=0;
        }
    }
    else
    {
        value = (value > 15000 ? 15000 : value);
        timed_vibrator_on(dev);

        hrtimer_start(&vibe_timer,
                      ktime_set(value / 1000, (value % 1000) * 1000000),
                      HRTIMER_MODE_REL);
    }
}

static int vibrator_get_time(struct timed_output_dev *dev)
{
    if (hrtimer_active(&vibe_timer))
    {
        ktime_t r = hrtimer_get_remaining(&vibe_timer);
        return r.tv.sec * 1000 + r.tv.nsec / 1000000;
    }
    else
        return 0;
}

static enum hrtimer_restart vibrator_timer_func(struct hrtimer *timer)
{
    int value=0;
    pr_info("timer:vibrator timeout!\n");
    if(!timed_vibrator_off(NULL))
    {
        value=500;
        hrtimer_start(&vibe_timer,
                      ktime_set(value / 1000, (value % 1000) * 1000000),
                      HRTIMER_MODE_REL);
        value=0;
    }
    return HRTIMER_NORESTART;
}

static struct timed_output_dev pmic_vibrator =
    {
        .name = "vibrator",
                .get_time = vibrator_get_time,
                            .enable = vibrator_enable,
                                  };

void __init msm_init_pmic_vibrator(void)
{

    pr_info("msm_init_pmic_vibrator\n");
    INIT_WORK(&work_vibrator_on, pmic_vibrator_on);
    INIT_WORK(&work_vibrator_off, pmic_vibrator_off);

    hrtimer_init(&vibe_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    vibe_timer.function = vibrator_timer_func;

    timed_output_dev_register(&pmic_vibrator);
}

MODULE_DESCRIPTION("timed output pmic vibrator device");
MODULE_LICENSE("GPL");

