/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-06-12     28101       the first version
 */
#ifndef APPLICATIONS_UDP_CLIENT_H_
#define APPLICATIONS_UDP_CLIENT_H_

#include <time.h>

extern char timestamp[32];
extern char llm_result[32];
extern char detect_result[32];
extern char weather_city[32];
extern char weather_temp[16];
extern char weather_wind_dir[16];
extern char weather_wind_power[16];
extern char weather_humidity[16];
extern char weather_rain[16];
extern int year;
extern int month;
extern int day;
extern int hour;
extern int minute;

#endif /* APPLICATIONS_UDP_CLIENT_H_ */
