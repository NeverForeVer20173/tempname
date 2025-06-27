/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-06-12     28101       the first version
 */
#include <rtthread.h>
#include <board.h>
#include <rtdevice.h>
#include <sys/socket.h>
#include "netdb.h"
#include <cJSON.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

/* 本机地址 */
#define LOCAL_IP   "172.20.10.7"  /* 本地 IP */
#define LOCAL_PORT 43521          /* 本地端口 */
int year, month, day, hour, minute;
char timestamp[32] = { 0 };
char llm_result[32] = { 0 };
char detect_result[32] = { 0 };
char weather_city[32], weather_temp[16],
     weather_wind_dir[16], weather_wind_power[16],
     weather_humidity[16], weather_rain[16];

static void get_str_from_json(cJSON *obj, const char *key, char *buf, size_t buf_size)
{
    cJSON *item = cJSON_GetObjectItem(obj, key);
    if (cJSON_IsString(item))
    {
        rt_strncpy(buf, item->valuestring, buf_size-1);
        buf[buf_size - 1] = '\0';
    }
    else {
        buf[0] = '\0';    // 如果键不存在、不是字符串或值为 NULL，则清空缓冲区
    }

}

static void udp_clienter(void *parameter)
{
    int ret;
    /* 创建一个socket，协议簇为AT Socket 协议栈，类型是SOCK_DGRAM，UDP类型 */
    int sock_fd = socket(AF_INET,SOCK_DGRAM,0);
    if (sock_fd  == -1)
    {
        rt_kprintf("Socket error, thread exit.\n");
        return;
    }

    // 绑定地址和端口号
    struct sockaddr_in local_addr = {0};
    local_addr.sin_family = AF_INET;                                //使用IPv4地址
    local_addr.sin_addr.s_addr = inet_addr(LOCAL_IP);               //本机IP地址
    local_addr.sin_port = htons(LOCAL_PORT);                        //端口
    if(bind(sock_fd, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0)
    {
        rt_kprintf("bind error!\n");
        closesocket(sock_fd);
        return;
    }
    struct timeval tv = {5, 0};   /* 5 秒超时 */
    setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));


    while (1)
    {
           struct sockaddr_in from_addr;
           socklen_t addrlen = sizeof(from_addr);
           char recvbuf[1024];
           rt_memset(recvbuf, 0, sizeof(recvbuf));  // 清空 recvbuf，防止打印之前的内容或垃圾数据
           int len = recvfrom(sock_fd,
                              recvbuf, sizeof(recvbuf) - 1,
                              0,
                              (struct sockaddr*)&from_addr,
                              &addrlen);
           if (len <= 0)
           {
               rt_kprintf("recvfrom() failed: %d, errno=%d\n", len, errno);
               continue;
           }

           recvbuf[len] = '\0';
           rt_kprintf("Received from %s:%d\n",inet_ntoa(from_addr.sin_addr),ntohs(from_addr.sin_port));

           /* JSON 解析 */
           cJSON *root = cJSON_Parse(recvbuf);
           if (root)
           {
               get_str_from_json(root, "timestamp",    timestamp,     sizeof(timestamp));

               if (sscanf(timestamp, "%4d%2d%2d-%2d%2d",
                          &year, &month, &day, &hour, &minute) == 5)
               {
                   /*rt_kprintf("year=%d, month=%d, day=%d, hour=%d, minute=%d\n",
                              year, month, day, hour, minute);*/
               }
               else
               {
                   rt_kprintf("timestamp 格式错误\n");
               }

               get_str_from_json(root, "llm_result",   llm_result,    sizeof(llm_result));
               get_str_from_json(root, "detectd_result",detect_result, sizeof(detect_result));
               cJSON *detect_item = cJSON_GetObjectItem(root, "detect_result");
               if (cJSON_IsString(detect_item))
               {
                   // 这里用 detect_item 而不是 detect_result
                   rt_kprintf("DEBUG: got detect_result = %s\n", detect_item->valuestring);
                   // 再把它拷贝到全局 buf
                   rt_strncpy(detect_result, detect_item->valuestring, sizeof(detect_result) - 1);
                   detect_result[sizeof(detect_result)-1] = '\0';
               }
               else
               {
                   // detect_item 可能为 NULL，要先检查
                   if (detect_item)
                       rt_kprintf("DEBUG: detect_result is not a string (type=%d)\n", detect_item->type);
                   else
                       rt_kprintf("DEBUG: detect_result item is missing\n");
               }

               rt_kprintf("  time: %s\n",    timestamp);
               rt_kprintf("  llm:  %s\n",    llm_result);
               rt_kprintf("  yolo: %s\n",    detect_result);
               /* 天气 */
               cJSON *weather = cJSON_GetObjectItem(root, "weather");
               if (cJSON_IsObject(weather) && weather)
               {
                   get_str_from_json(weather, "city",       weather_city,      sizeof(weather_city));
                   get_str_from_json(weather, "temp",       weather_temp,      sizeof(weather_temp));
                   get_str_from_json(weather, "wind_dir",   weather_wind_dir,  sizeof(weather_wind_dir));
                   get_str_from_json(weather, "wind_power", weather_wind_power,sizeof(weather_wind_power));
                   get_str_from_json(weather, "humidity",   weather_humidity,  sizeof(weather_humidity));
                   get_str_from_json(weather, "rain",       weather_rain,      sizeof(weather_rain));

                   /*rt_kprintf("  weather:\n");
                   rt_kprintf("    city=%s, temp=%s, wind_dir=%s\n",
                              weather_city, weather_temp, weather_wind_dir);
                   rt_kprintf("    wind_power=%s, humidity=%s, rain=%s\n",
                              weather_wind_power, weather_humidity, weather_rain);*/
               }
               else
               {
                   rt_kprintf("Warning: no weather object\n");
               }

               cJSON_Delete(root);
           }
           else
           {
               rt_kprintf("  Warning: invalid JSON\n");
           }
    }
    // 退出循环后关闭socket
    closesocket(sock_fd);
}

static int udp_thread(void)
{
    rt_thread_t tid1 = rt_thread_create("UDP_RECIEVE", udp_clienter, RT_NULL, 4096, 14, 10);
    if (!tid1)
    {
        rt_kprintf("create udp thread failed\n");
        return -RT_ERROR;
    }
    rt_thread_startup(tid1);
    rt_kprintf("udp thread started\n");

    return RT_EOK;
}
INIT_APP_EXPORT(udp_thread);
//MSH_CMD_EXPORT(udp_thread,upd recieve);
