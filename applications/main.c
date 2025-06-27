#include <rtthread.h>
#include <rtdevice.h>
#include <wlan_mgnt.h>
#include "drv_gpio.h"

#define AUTO_SSID    "HiwonderESP"
#define AUTO_PASSWD  "hiwonder"
#define LED_PIN      GET_PIN(0, 1)

/* 失败时重试的回调 */
static void _reconnect_cb(int event, struct rt_wlan_buff *buff, void *param)
{
    if (event == RT_WLAN_DEV_EVT_CONNECT_FAIL)
    {
        rt_kprintf("Wi-Fi connect failed, retrying...\n");
        rt_wlan_connect(AUTO_SSID, AUTO_PASSWD);
    }
}

static void _connect_ok_cb(int event, struct rt_wlan_buff *buff, void *param)
{
    if (event == RT_WLAN_DEV_EVT_CONNECT)
    {
        rt_kprintf("Wi-Fi connected: \"%s\"\n", AUTO_SSID);
        /* 例如把 LED 拉低表示“已连上” */
        rt_pin_write(LED_PIN, PIN_LOW);
    }
}

int main(void)
{
    /* 初始化 LED 引脚 */
    rt_pin_mode(LED_PIN, PIN_MODE_OUTPUT);

    rt_pin_write(LED_PIN, PIN_HIGH);

    rt_wlan_config_autoreconnect(RT_TRUE);

    /* 注册“连接失败”和“连接成功”的事件处理器 */
    rt_wlan_register_event_handler(RT_WLAN_DEV_EVT_CONNECT_FAIL,
                                   _reconnect_cb, RT_NULL);
    rt_wlan_register_event_handler(RT_WLAN_DEV_EVT_CONNECT,
                                   _connect_ok_cb, RT_NULL);

    /* 第一次发起连接尝试 */
    rt_wlan_connect(AUTO_SSID, AUTO_PASSWD);
    rt_kprintf("Auto-connect launched: \"%s\"\n", AUTO_SSID);

    /* 主循环：只要没连上就闪，连上后回调会熄灭 */
    while (1)
    {
        rt_thread_mdelay(500);
        rt_pin_write(LED_PIN, PIN_HIGH);
        rt_thread_mdelay(500);
        rt_pin_write(LED_PIN, PIN_LOW);
    }

    return 0;
}
