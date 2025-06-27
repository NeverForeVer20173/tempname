#include <rtthread.h>
#include <rtdevice.h>
#include <drv_gpio.h>

#define KEY_PIN             GET_PIN(6, 2)
#define DOUBLE_CLICK_MS     300   /* 双击判定窗口 */


/* 事件定义 */
#define KEY_NONE        0xFF
#define KEY_CLICK       3
#define KEY_DOUBLECLICK 4


rt_mailbox_t mailbox1 = RT_NULL;

/* 状态变量和定时器句柄 */
static rt_timer_t  dbl_tmr     = RT_NULL;
static rt_bool_t   waiting_2nd = RT_FALSE;

/* 定时器到期回调：视为单击 */
static void dbl_timeout(void *param)
{
    waiting_2nd = RT_FALSE;
    rt_mb_send(mailbox1, (rt_ubase_t)KEY_CLICK);
    //rt_kprintf("[KEY] Single Click\n");
}

/* GPIO 引脚初始化 */
void KEY_Init(void)
{
    rt_pin_mode(KEY_PIN, PIN_MODE_INPUT_PULLUP);
}

/* 轮询按键线程 */
static void get_key_thread_entry(void *parameter)
{
    KEY_Init();

    while (1)
    {
        if (rt_pin_read(KEY_PIN) == PIN_LOW)
        {
            /* 简易按键按下检测 & 消抖 */
            rt_thread_mdelay(50);
            if (rt_pin_read(KEY_PIN) != PIN_LOW)
            {
                rt_thread_mdelay(10);
                continue;
            }

            /* 如果还在等待第二次按下，就是双击 */
            if (waiting_2nd)
            {
                waiting_2nd = RT_FALSE;
                /* 停掉定时器，取消单击回调 */
                rt_timer_stop(dbl_tmr);
                /* 发双击事件 */
                rt_mb_send(mailbox1, (rt_ubase_t)KEY_DOUBLECLICK);
                //rt_kprintf("[KEY] Double Click\n");
            }
            else
            {
                /* 第一次按下，启动定时器，等待双击窗口 */
                waiting_2nd = RT_TRUE;
                rt_timer_start(dbl_tmr);
            }

            /* 等待按键释放，再开始下一轮检测 */
            while (rt_pin_read(KEY_PIN) == PIN_LOW)
            {
                rt_thread_mdelay(10);
            }
        }
        rt_thread_mdelay(10);
    }
}

/* 应用初始化：先建 mailbox，然后建定时器，再启动线程 */
static int key_detect(void)
{
    /* 1) Mailbox 用来发 EVT_CLICK/EVT_DOUBLE_CLICK */
    mailbox1 = rt_mb_create("mb_key", 4, RT_IPC_FLAG_FIFO);
    if (!mailbox1)
    {
        rt_kprintf("mailbox1 create failed\n");
        return -RT_ERROR;
    }

    /* 2) 双击定时器 */
    dbl_tmr = rt_timer_create("dbl_tmr",
                              dbl_timeout,
                              RT_NULL,
                              rt_tick_from_millisecond(DOUBLE_CLICK_MS),
                              RT_TIMER_FLAG_SOFT_TIMER | RT_TIMER_FLAG_ONE_SHOT);
    if (!dbl_tmr)
    {
        rt_kprintf("dbl timer create failed\n");
        return -RT_ERROR;
    }

    /* 3) 启动按键线程 */
    rt_thread_t tid = rt_thread_create("key_thr",
                                       get_key_thread_entry,
                                       RT_NULL,
                                       512,  /* 栈 512 bytes */
                                       15,   /* 优先级 */
                                       10);  /* 时间片 */
    if (!tid)
    {
        rt_kprintf("create key thread failed\n");
        return -RT_ERROR;
    }
    rt_thread_startup(tid);
    rt_kprintf("key thread started\n");

    return RT_EOK;
}
INIT_APP_EXPORT(key_detect);
