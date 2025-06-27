/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-05-23     28101       the first version
 */
#ifndef APPLICATIONS_SENSOR_H_
#define APPLICATIONS_SENSOR_H_

extern int32_t moisture_mv;
extern int32_t temperature_mv;
extern int32_t light_mv;

int Sensor_thread_Init(void);

#endif /* APPLICATIONS_SENSOR_H_ */
