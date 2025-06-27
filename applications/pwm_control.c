/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-06-12     28101       the first version
 */
#include "pwm_control.h"
#include <rtthread.h>
#include <rtdevice.h>
#include "Sensor.h"

#define PWM_DEV_NAME "pwm0"
#define PWM_DEV_CHANNEL 7
#define THREAD_PRIORITY         28
#define THREAD_STACK_SIZE       2048
#define THREAD_TIMESLICE        5
static rt_thread_t tid1 = RT_NULL;
struct rt_device_pwm *pwm_dev;

// 初始化
int pwm_init(void)
{
    rt_uint32_t period, pulse;

    period = 1 * 1000 * 1000;
    pulse = 0;

    pwm_dev = (struct rt_device_pwm *)rt_device_find(PWM_DEV_NAME);

    if (pwm_dev == RT_NULL)
    {
        rt_kprintf("pwm sample run failed! can't find %s device!\n", PWM_DEV_NAME);
        return RT_ERROR;
    }

    rt_pwm_set(pwm_dev, PWM_DEV_CHANNEL, period, pulse);
    rt_pwm_enable(pwm_dev, PWM_DEV_CHANNEL);

    // rt_kprintf("Now PWM[%s] Channel[%d] Period[%d] Pulse[%d]\n", PWM_DEV_NAME, PWM_DEV_CHANNEL, period, pulse);

    /* 在打开设备之后，设置周期 1ms，占空 0%（保险关断） */
    rt_pwm_set(pwm_dev, PWM_DEV_CHANNEL, period, pulse);
    rt_pwm_enable(pwm_dev, PWM_DEV_CHANNEL);

}

void pwm_open(void)
{
    rt_uint32_t period, pulse, dir;
    period = 1 * 1000 * 1000;
    pulse = 1 * 1000 * 1000;

    rt_pwm_set(pwm_dev, PWM_DEV_CHANNEL, period, pulse); /* pwm_dev, PWM_DEV_CHANNEL ,PWM 周期（period）,高电平时长（pulse） */
    rt_pwm_enable(pwm_dev, PWM_DEV_CHANNEL);
    rt_kprintf("pwm open\n");
}

void pwm_close(void)
{
    rt_pwm_disable(pwm_dev, PWM_DEV_CHANNEL);
    rt_kprintf("pwm close\n");
}


void auto_watering(void)
{
    rt_kprintf("pwm auto\n");
    if(moisture_mv < 1000 && moisture_mv > 0)
    {
        pwm_open();
    }
    else
    {
        pwm_close();
    }
}

static void pwm_entry(void *parameter)
{
    pwm_init();
    auto_watering();
}

int pwm_thread(void)
{
    tid1 = rt_thread_create("pwm_thread",
                            pwm_entry, RT_NULL,
                            THREAD_STACK_SIZE,
                            THREAD_PRIORITY, THREAD_TIMESLICE);

    if (tid1 != RT_NULL)
        rt_thread_startup(tid1);

    // rt_kprintf("thread start!\n");
    return 0;
}
//MSH_CMD_EXPORT (pwm_thread, pwm wet control);
INIT_APP_EXPORT(pwm_init);
