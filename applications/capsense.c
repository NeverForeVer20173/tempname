/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-06-19     28101       the first version
 */
#include <rtthread.h>
#include "cycfg_capsense.h"
#include "capsense.h"
#include "cycfg_peripherals.h"
#include "cy_capsense_structure.h"

/***********************************全局变量 *************************************/
#define KEY_NONE    0xFF
#define KEY_DOWN    1
#define KEY_UP      2

#define MOVEMENT_THRESHOLD   5
static rt_sem_t   capsense_sem = RT_NULL;
uint8_t   slider_touch_prev = 0;
uint16_t  slider_pos_prev_x = 0;
volatile uint8_t key_event = KEY_NONE;

rt_mailbox_t msg_cs = RT_NULL;
/*********************************capsense ***************************************/
static void capsense_isr(void)
{
    rt_interrupt_enter();
    Cy_CapSense_InterruptHandler(CYBSP_CSD_HW, &cy_capsense_context);
    rt_interrupt_leave();
}

void capsense_callback(cy_stc_active_scan_sns_t *ptr)
{
    rt_sem_release(capsense_sem);
}

static void initialize_capsense(void)
{
    cy_stc_sysint_t intcfg = {
        .intrSrc      = csd_interrupt_IRQn,
        .intrPriority = 7,
    };

    RT_ASSERT(Cy_CapSense_Init(&cy_capsense_context) == CYRET_SUCCESS);

    // 中断注册时要确保参数顺序正确
    cyhal_system_set_isr(csd_interrupt_IRQn, csd_interrupt_IRQn, 7, &capsense_isr);
    NVIC_ClearPendingIRQ(intcfg.intrSrc);
    NVIC_EnableIRQ(intcfg.intrSrc);

    RT_ASSERT(Cy_CapSense_Enable(&cy_capsense_context) == CYRET_SUCCESS);
    RT_ASSERT(Cy_CapSense_RegisterCallback(CY_CAPSENSE_END_OF_SCAN_E,
                                           capsense_callback,
                                           &cy_capsense_context) == CYRET_SUCCESS);
}

int CapsenseKey_Init(void)
{
    msg_cs = rt_mb_create("cs_mb",16,RT_IPC_FLAG_FIFO);
    if (!msg_cs)
    {
        rt_kprintf("msg_cs create failed\n");
        return -RT_ERROR;
    }

    // 先创建信号量，避免首个回调丢信号
    capsense_sem = rt_sem_create("capsense", 0, RT_IPC_FLAG_FIFO);
    if (!capsense_sem)
    {
        rt_kprintf("capsem create failed\n");
        return -RT_ERROR;
    }

    initialize_capsense();

    // 发起第一次扫描
    Cy_CapSense_ScanAllWidgets(&cy_capsense_context);

    return RT_EOK;
}

void CapsenseKey_Process(void)
{
    cy_stc_capsense_touch_t *touch;
    uint8_t touch_status;
    uint16_t pos_x = 0;
    static uint8_t last_sent = KEY_NONE;
    // 阻塞等待本次扫描完成
    if (rt_sem_take(capsense_sem, RT_WAITING_FOREVER) != RT_EOK)
        return;

    // 处理所有 Widget 数据
    Cy_CapSense_ProcessAllWidgets(&cy_capsense_context);

    // 读取滑条信息
    touch = Cy_CapSense_GetTouchInfo(CY_CAPSENSE_LINEARSLIDER0_WDGT_ID,
                                     &cy_capsense_context);
    touch_status = touch->numPosition;

    if (touch_status >= 1)
    {
        // 单指或多指滑条，只取第一个触点的 X 坐标
        pos_x = touch->ptrPosition->x;
    }

    // 初始状态先置空
    key_event = KEY_NONE;

    // 区分“持续滑动”和“新触摸第一帧”
    if (touch_status >= 1 && slider_touch_prev >= 1)
    {
        // ——持续触摸，做阈值判定
        int16_t xdiff = (int16_t)pos_x - (int16_t)slider_pos_prev_x;
        if (xdiff > MOVEMENT_THRESHOLD)
        {
            key_event = KEY_DOWN;
        }
        else if (xdiff < -MOVEMENT_THRESHOLD)
        {
            key_event = KEY_UP;
        }
        // 更新上一帧基准
        slider_pos_prev_x = pos_x;
    }
    else if (touch_status >= 1 && slider_touch_prev == 0)
    {
        // ——新触摸第一帧，只记录基准位置
        slider_pos_prev_x = pos_x;
    }
    // 如果 touch_status == 0，则保持 slider_pos_prev_x 不动，让下一次触摸走“新触摸”分支
    slider_touch_prev = touch_status;

    if (key_event != KEY_NONE && key_event != last_sent)
    {
        rt_err_t send_ret = rt_mb_send(msg_cs, key_event);
        /*if (send_ret == RT_EOK)
            rt_kprintf("[BT] sent event %d\n", key_event);
        else
            rt_kprintf("[BT] send failed, code=%d\n", send_ret);*/
        last_sent = key_event;
    }
    else if (key_event == KEY_NONE)
    {
        /* 手指松开后，允许下次再发任何事件 */
        last_sent = KEY_NONE;
    }

    // 同步到 Tuner
    Cy_CapSense_RunTuner(&cy_capsense_context);
    // 发起下一次扫描
    Cy_CapSense_ScanAllWidgets(&cy_capsense_context);
}

static void capsense_thread_entry(void *param)
{
    CapsenseKey_Init();

    while (1)
    {
        CapsenseKey_Process();
        rt_thread_mdelay(50);
    }
}

int Capkey_thread(void)
{
    rt_thread_t tid = rt_thread_create("capsense_msg",
                                       capsense_thread_entry,
                                       RT_NULL,
                                       1024,
                                       15,
                                       10);
    if (tid)
        rt_thread_startup(tid);
    rt_kprintf("capsense thread started\n");
    return tid ? RT_EOK : -RT_ERROR;
}
//MSH_CMD_EXPORT(Capkey_thread, Initialize CapSense key input);
INIT_APP_EXPORT(Capkey_thread);
