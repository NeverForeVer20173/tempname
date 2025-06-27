#ifndef APPLICATIONS_BUTTON_H_
#define APPLICATIONS_BUTTON_H_

#include <rtthread.h>
#include <rtdevice.h>



/* 多击检测时间窗 */
#define MULTI_CLICK_MS  100

/* 消息队列，用于把 key_count 发给其他线程 */
extern rt_mailbox_t mailbox1;

/**
 * @brief  初始化按键驱动和 mailbox，并启动按键线程
 *
 * 该函数由 INIT_APP_EXPORT 自动调用。
 *
 * @return RT_EOK 成功
 *         -RT_ERROR 失败
 */
int key_sample(void);

/**
 * @brief  按键轮询线程入口
 *
 * 不要手动调用，由 key_sample() 创建并启动。
 */
void get_key_thread_entry(void *parameter);

/**
 * @brief  简单的去抖初始化，只设置 GPIO
 */
void KEY_Init(void);

#endif /* APPLICATIONS_BUTTON_H_ */
