/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-06-12     28101       the first version
 */
// bsp_pwm.h
#ifndef APPLICATIONS_PWM_CONTROL_H_
#define APPLICATIONS_BSP_PWM_H_
#include <rtthread.h>
#include <rtdevice.h>

/* 线程示例 */
void pwm_open(void);
void pwm_close(void);
void auto_watering(void);
#endif /* APPLICATIONS_PWM_CONTROL_H_ */

