#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <u8g2_port.h>
#include "cycfg_capsense.h"
#include "cycfg_peripherals.h"
#include "cy_capsense_structure.h"
#include <drv_gpio.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "button.h"
#include "capsense.h"
#include "picture.h"
#include "oled_menu.h"
#include "Sensor.h"
#include "udp_client.h"
#include "pwm_control.h"
/***************************************** 宏定义  ******************************************/
#define OLED_I2C_PIN_SCL  88  // SCL
#define OLED_I2C_PIN_SDA  89  // SDA

u8g2_t u8g2;                                            // u8g2结构体
static ui_anim_state_t ui_anim_state = UI_IDLE;
int anim_step = 0;                                      // 主菜单图标横向滑动的积累步数
const int anim_max = 48;                                // 主菜单图标横向滑动的最大值
int display = 48;                                       // 当前主菜单图标X轴偏移
int current_menu = 0;                                   // 主菜单当前选中的页签
int Picture_Flag = 0;
int trans_y = 0;                                        // 上下滑动过度动画的Y轴积累位移
const int trans_y_max = 64;                             // 上下滑动过度动画的Y轴最大位移
const int trans_step = 4;                               // 上下滑动过度动画的Y轴每帧步长
UIState_ENUM current_ui = UI_STATE_WATCH;               // 开机状态
#define IDLE_TIMEOUT_SECONDS    10                      // 超时睡眠时间设定
#define MOTOR_TIMEOUT_SECONDS    5                     // motor三级菜单超时睡眠时间设定
/* 主菜单页签对应的文字数组 */
const char* words[] = {
    "weather",
    "sensor",
    "note",
    "pwm"
};
extern rt_mailbox_t msg_cs;
extern rt_mailbox_t mailbox1;

#define KEY_NONE            0xFF
#define KEY_DOWN            1
#define KEY_UP              2
#define KEY_CLICK           3
#define KEY_DOUBLECLICK     4

rt_tick_t last_motor_act;
static uint8_t key_event = 0;
/*************************************** 消息接受线程 ****************************************/
rt_err_t read_cap_msg(uint8_t *event, rt_int32_t timeout)
{
    rt_ubase_t rev;
    rt_err_t   ret;

    /* 阻塞或超时接收一条 mailbox 消息 */
    ret = rt_mb_recv(msg_cs, &rev, timeout);
    if (ret == RT_EOK)
    {
        *event = (uint8_t)rev;  /* 转成你的 event 码 */
        //rt_kprintf("[UI] read_cap_msg: got event %d\n", *event);
    }
    else if (ret == -RT_ETIMEOUT)
    {
        /* 超时未收到消息，不算错误，可以不打印或者只打印一次 */
        //rt_kprintf("[UI] read_cap_msg: timeout\n");
    }
    else
    {
        /* 其它错误（比如 mailbox == RT_NULL） */
        rt_kprintf("[UI] read_cap_msg: error %d\n", ret);
    }
    return ret;
}
rt_err_t read_button_msg(uint8_t *event, rt_int32_t timeout)
{
    rt_ubase_t rev;
    rt_err_t   ret;

    /* 阻塞或超时接收一条 mailbox 消息 */
    ret = rt_mb_recv(mailbox1, &rev, timeout);
    if (ret == RT_EOK)
    {
        *event = (uint8_t)rev;  /* 转成你的 event 码 */
        //rt_kprintf("[UI] read_but_msg: got event %d\n", *event);
    }
    else if (ret == -RT_ETIMEOUT)
    {
        /* 超时未收到消息，不算错误，可以不打印或者只打印一次 */
        //rt_kprintf("[UI] read_but_msg: timeout\n");
    }
    else
    {
        /* 其它错误（比如 mailbox == RT_NULL） */
        rt_kprintf("[UI] read_but_msg: error %d\n", ret);
    }
    return ret;
}
/***************************************** 绘制函数  ******************************************/
/* 动画平滑过度 */
void ui_run(char* a, char* a_trg, int b)
{
    if (*a < *a_trg)
    {
        *a += b;
        if (*a > *a_trg) *a = *a_trg;
    }
    else if (*a > *a_trg)
    {
        *a -= b;
        if (*a < *a_trg) *a = *a_trg;
    }
}

/* MENU→Weather转换 */
void To_Weather_Menu_Display(void)
{
    trans_y = 0;   // 清零y轴的偏移量
    current_ui = UI_STATE_MENU_TO_WEATHER;
}
/* Weather→MENU转换 */
void Weather_To_Menu_Display(void)
{
    trans_y = 0;   // 清零y轴的偏移量
    current_ui = UI_STATE_WEATHER_TO_MENU;
}

/* MENU→Plant转换 */
void To_Plant_Menu_Display(void)
{
    trans_y = 0;   // 清零y轴的偏移量
    current_ui = UI_STATE_MENU_TO_PLANT;
}
/* Plant→MENU转换 */
void Plant_To_Menu_Display(void)
{
    trans_y = 0;   // 清零y轴的偏移量
    current_ui = UI_STATE_PLANT_TO_MENU;
}
void To_Note_Menu_Display(void)
{
    trans_y = 0;   // 清零y轴的偏移量
    current_ui = UI_STATE_MENU_TO_NOTE;
}

void Note_To_Menu_Display(void)
{
    trans_y = 0;   // 清零y轴的偏移量
    current_ui = UI_STATE_NOTE_TO_MENU;
}

void To_Motor_Menu_Display(void)
{
    trans_y = 0;   // 清零y轴的偏移量
    current_ui = UI_STATE_MENU_TO_MOTOR;
}

void Motor_To_Menu_Display(void)
{
    trans_y = 0;
    current_ui = UI_STATE_MOTOR_TO_MENU;
}
/********************************** 屏幕绘制 *******************************/
/* 开机屏幕表盘 */
void watch_screen(void)
{
    char date_buf[32] = {0};
    char hm_buf[16]   = {0};

    /* 1) 用全局变量填 tm，算出星期几 */
    struct tm tm_parsed = {0};
    tm_parsed.tm_year = year  - 1900;
    tm_parsed.tm_mon  = month - 1;
    tm_parsed.tm_mday = day;
    tm_parsed.tm_hour = hour;
    tm_parsed.tm_min  = minute;
    tm_parsed.tm_sec  = 0;
    mktime(&tm_parsed);  // 填充 tm_parsed.tm_wday

    /* 2) 格式化“Wed 16 Jun 2025” */
    static const char *week_str[] = { "Sun","Mon","Tue","Wed","Thu","Fri","Sat" };
    static const char *month_str[] = {
        "Jan","Feb","Mar","Apr","May","Jun",
        "Jul","Aug","Sep","Oct","Nov","Dec"
    };
    snprintf(date_buf, sizeof(date_buf),
             "%s %02d %s %04d",
             week_str[tm_parsed.tm_wday],
             tm_parsed.tm_mday,
             month_str[tm_parsed.tm_mon],
             tm_parsed.tm_year + 1900);

    /* 3) 格式化时:分 */
    snprintf(hm_buf, sizeof(hm_buf), "%02d:%02d",
             tm_parsed.tm_hour, tm_parsed.tm_min);

    /* 4) 开始绘制 */
    u8g2_ClearBuffer(&u8g2);

    /* 日期居中，Y=13 */
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
    {
        int w = u8g2_GetStrWidth(&u8g2, date_buf);
        u8g2_DrawStr(&u8g2, (128 - w) / 2, 13, date_buf);
    }

    /* 时分大字体，Y=50 */
    u8g2_SetFont(&u8g2, u8g2_font_ncenB24_tr);
    {
        int w = u8g2_GetStrWidth(&u8g2, hm_buf);
        u8g2_DrawStr(&u8g2, (128 - w) / 2, 50, hm_buf);
    }

    /* 绘制一个 Unicode 符号示例 */
    u8g2_SetFont(&u8g2, u8g2_font_unifont_t_symbols);
    u8g2_DrawGlyph(&u8g2, 112, 68, 0x2603);

    u8g2_SendBuffer(&u8g2);
}


/* 主菜单绘制，处理左右切换 */
void Main_Menu(Speed_ENUM speed)
{
    // 当不在做滑动动画时，滑动才能触发事件，防止动画过程触发新的动作
    if (ui_anim_state == UI_IDLE)
    {
        // 上滑检测
        if (key_event == KEY_UP && current_menu < 3)
        {
            current_menu++;   // 选中下一项
            anim_step = 0;    // 重置动画进度
            ui_anim_state = UI_ANIM_LEFT;   // 切换到左滑动画状态
        }
        // 下滑检测
        else if (key_event == KEY_DOWN && current_menu > 0)
        {
            current_menu--;
            anim_step = 0;
            ui_anim_state = UI_ANIM_RIGHT;
        }
        // 按键检测且当前是第0项，按下按键，调用To_Plant_Menu_Display()切换到植株状态动画
        else if (key_event == KEY_CLICK && current_menu == 0)
        {
            To_Weather_Menu_Display();
            return;   // 结束本帧绘，不再进行主菜单渲染
        }
        else if (key_event == KEY_CLICK && current_menu == 1)
        {
            To_Plant_Menu_Display();
            return;   // 结束本帧绘，不再进行主菜单渲染
        }
        else if (key_event == KEY_CLICK && current_menu == 2)
        {
            To_Note_Menu_Display();
            return;
        }

        else if (key_event == KEY_CLICK && current_menu == 3)
        {
            To_Motor_Menu_Display();
            return;
        }
    }


    /* 动画左右滑动控制 */
    // 如果当前动画状态是向左滑
    if (ui_anim_state == UI_ANIM_LEFT)
    {
        anim_step += speed;   // 累积滑动步数，通过oled_ui.h中的Speed_ENUM来控制一次移动的像素
        display -= speed;     // 将图标群往左移动 Speed_choose 像素
        // 当累积步数 anim_step 达到或超过最大值 anim_max
        if (anim_step >= anim_max)
        {
            anim_step = 0;           // 重置 anim_step = 0
            display = 48 - current_menu * 48;  // 重新计算 display = 48 - current_menu * 48;，把图标群准确定位到当前选项
            ui_anim_state = UI_IDLE;    // 把状态改回 UI_IDLE，允许下次按键响应
        }
    }
    else if (ui_anim_state == UI_ANIM_RIGHT)
    {
        anim_step += speed;
        display += speed;
        if (anim_step >= anim_max)
        {
            anim_step = 0;
            display = 48 - current_menu * 48;
            ui_anim_state = UI_IDLE;
        }
    }

    /* 图像绘制 */
    u8g2_ClearBuffer(&u8g2);
    u8g2_DrawXBM(&u8g2, 44, 36, 40, 40, arrowhead);       // 绘制箭头图，表示选中当前位置
    // 四个图标的位置为，48,96,144,192，第三个和第四个在屏幕之外，当坐标变动的时候，就会有一个被移进屏幕，一个被移动到屏幕外
    u8g2_DrawXBM(&u8g2, display, 16, 32, 32, weather); // 以display为基准，绘制四个图标
    u8g2_DrawXBM(&u8g2, display+48,    16, 32, 32, tree);    // 以display为基准，绘制四个图标
    u8g2_DrawXBM(&u8g2, display+96, 16, 32, 32, message);  // 以display为基准，绘制四个图标
    u8g2_DrawXBM(&u8g2, display+144, 16, 32, 32, motor); // 以display为基准，绘制四个图标

    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);            // 设置字体
    u8g2_DrawStr(&u8g2, 52, 10, words[current_menu]);     // 画当前标签页名称

    u8g2_SendBuffer(&u8g2);
}
/***************************************** 三级菜单 ************************************/
void draw_open()
{
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
    int txt_w = u8g2_GetStrWidth(&u8g2, "motor is already open");
    int x = (128 - txt_w) / 2;

    int fh = u8g2_GetMaxCharHeight(&u8g2);
    int y  = (64 + fh) / 2;
    u8g2_DrawStr(&u8g2, x, y, "motor is already open");
    u8g2_SendBuffer(&u8g2);
    if ((rt_tick_get() - last_motor_act) > MOTOR_TIMEOUT_SECONDS * RT_TICK_PER_SECOND)
    {
        current_ui = UI_STATE_MOTOR_MENU;
    }

}
void draw_close()
{
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
    int txt_w = u8g2_GetStrWidth(&u8g2, "motor is already close");
    int x = (128 - txt_w) / 2;

    int fh = u8g2_GetMaxCharHeight(&u8g2);
    int y  = (64 + fh) / 2;
    u8g2_DrawStr(&u8g2, x, y, "motor is already close");
    u8g2_SendBuffer(&u8g2);
    if ((rt_tick_get() - last_motor_act) > MOTOR_TIMEOUT_SECONDS * RT_TICK_PER_SECOND)
    {
        current_ui = UI_STATE_MOTOR_MENU;
    }
}
void draw_auto()
{
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);

    int txt_w = u8g2_GetStrWidth(&u8g2, "auto watering is open");
    int x = (128 - txt_w) / 2;

    int fh = u8g2_GetMaxCharHeight(&u8g2);
    int y  = (64 + fh) / 2;
    u8g2_DrawStr(&u8g2, x, y, "auto watering is open");
    u8g2_SendBuffer(&u8g2);
    if ((rt_tick_get() - last_motor_act) > MOTOR_TIMEOUT_SECONDS * RT_TICK_PER_SECOND)
    {
        current_ui = UI_STATE_MOTOR_MENU;
    }
}

/****************************************** 二级菜单 ********************************/
/* 天气二级菜单 */
void Weather_Status_Menu()
{
    if (key_event == KEY_DOUBLECLICK)
    {
        Weather_To_Menu_Display();
        return;
    }

    u8g2_ClearBuffer(&u8g2);

    // 标题，居中
    const char *title = "Weather Status";
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
    {
        int w = u8g2_GetStrWidth(&u8g2, title);
        u8g2_DrawStr(&u8g2, (128 - w) / 2, 12, title);
    }

    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);

    // 左侧 X 坐标
    const int left_x = 0;

    // 四行内容
    char buf[32];

    snprintf(buf, sizeof(buf), "City: %s", weather_city);
    u8g2_DrawStr(&u8g2, left_x, 28, buf);

    snprintf(buf, sizeof(buf), "Temp: %s  Hum: %s", weather_temp, weather_humidity);
    u8g2_DrawStr(&u8g2, left_x, 28, buf);

    snprintf(buf, sizeof(buf), "Wind: %s  WindPower%s", weather_wind_dir, weather_wind_power);
    u8g2_DrawStr(&u8g2, left_x, 42, buf);

    snprintf(buf, sizeof(buf), "Rain: %s", weather_rain);
    u8g2_DrawStr(&u8g2, left_x, 56, buf);

    // 图标右下角：16×16 大小
    const int icon_x = 128 - 16;  // 112
    const int icon_y = 64  - 16;  // 48

    if (strcmp(weather_rain, "0") == 0)
    {
        u8g2_DrawXBM(&u8g2, icon_x, icon_y, 16, 16, sun_icon);
    }
    else
    {
        u8g2_DrawXBM(&u8g2, icon_x, icon_y, 16, 16, rain_icon);
    }

    u8g2_SendBuffer(&u8g2);
}

/* 植株菜单二级菜单 */
void Plant_Status_Menu()
{
    char buf[32];

    // 如果按下返回键就退回上一级菜单
    if (key_event == KEY_DOUBLECLICK)
    {
        Plant_To_Menu_Display();
        return;
    }

    // 清屏并设置字体
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);

    // 第一行：固定标题
    u8g2_DrawStr(&u8g2, 40, 10, "Plant Status");

    // 用 sprintf 把浮点数格式化进 buf，然后绘制
    sprintf(buf, "shidu: %.1f%%", moisture_mv);
    u8g2_DrawStr(&u8g2, 3, 26, buf);

    sprintf(buf, "wendu: %.1fC", temperature_mv);
    u8g2_DrawStr(&u8g2, 3, 42, buf);

    sprintf(buf, "guangzhao: %u lx", light_mv);
    u8g2_DrawStr(&u8g2, 3, 58, buf);

    u8g2_SendBuffer(&u8g2);
}

void Note_Menu()
{
    if (key_event == KEY_DOUBLECLICK)
    {
        Note_To_Menu_Display();
        ui_anim_state = UI_IDLE;
        return;
    }
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);

    u8g2_DrawStr(&u8g2,  0, 12, "Note");

    char buf[64];

    snprintf(buf, sizeof(buf), "Detect: %s", detect_result);
    u8g2_DrawStr(&u8g2, 0, 28, buf);

    snprintf(buf, sizeof(buf), "LLM   : %s", llm_result);
    u8g2_DrawStr(&u8g2, 0, 44, buf);

    u8g2_SendBuffer(&u8g2);
}

void Motor_Menu()
{
    static int motor_idx = 0;            // 0=Start,1=Stop,2=Auto
    const int motor_count = 3;
    const char *motor_items[] = {
        "Start watering",
        "Stop watering",
        "Auto watering"
    };

    /* 1. 按键处理 */
    if (key_event == KEY_DOUBLECLICK)
    {
        /* 双击返回上一级 */
        current_ui = UI_STATE_MENU;
        return;
    }
    else if (key_event == KEY_UP && motor_idx > 0)
    {
        motor_idx--;
    }
    else if (key_event == KEY_DOWN && motor_idx < motor_count - 1)
    {
        motor_idx++;
    }
    else if (key_event == KEY_CLICK)
    {
        /* 执行对应动作 */
        switch (motor_idx)
        {
            case 0:
                pwm_open();
                current_ui = UI_STATE_MOTOR_OPENED;
                last_motor_act = rt_tick_get();
                break;
            case 1:
                pwm_close();
                current_ui = UI_STATE_MOTOR_CLOSED;
                last_motor_act = rt_tick_get();
                break;
            case 2:
                auto_watering();
                current_ui = UI_STATE_MOTOR_AUTO;
                last_motor_act = rt_tick_get();
                break;
        }
        return;  /* 本帧无需重绘菜单 */
    }

    /* 2. 绘制菜单 */
    u8g2_ClearBuffer(&u8g2);

    /* 标题 */
    const char *title = "Motor Control";
    u8g2_SetFont(&u8g2, u8g2_font_ncenB10_tr);
    {
        int w = u8g2_GetStrWidth(&u8g2, title);
        u8g2_DrawStr(&u8g2, (128 - w) / 2, 12, title);
    }

    /* 菜单项列表 */
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
    for (int i = 0; i < motor_count; i++)
    {
        int y = 28 + i * 16;
        /* 画文字 */
        u8g2_DrawStr(&u8g2, 10, y, motor_items[i]);

        /* 如果是选中项，就在右侧画一个符号箭头 */
        if (i == motor_idx)
        {
            /* 切到符号字体 */
            u8g2_SetFont(&u8g2, u8g2_font_unifont_t_symbols);
            /* 在最右侧用“→”表示 */
            u8g2_DrawGlyph(&u8g2, 128 - 10, y, 0x2190);
            /* 切回常规字体，以便下一次循环继续绘制文字 */
            u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
        }
    }

    /* 3. 刷新到屏幕 */
    u8g2_SendBuffer(&u8g2);
}
/************************************** 动画转场 ******************************/
/*Weather-MENU */
void update_menu_to_weather_anim(void)
{
    // 检查当前 Y 偏移量 trans_y 是否还没达到最大值 trans_y_max，还没到就进行动画的绘制
    if (trans_y < trans_y_max)
    {
        trans_y += trans_step;
        // 避免过冲
        if (trans_y > trans_y_max) trans_y = trans_y_max;

        u8g2_ClearBuffer(&u8g2);
        u8g2_DrawXBM(&u8g2, 44, trans_y + 26, 40, 40, arrowhead);
        u8g2_DrawXBM(&u8g2, display, trans_y + 6, 32, 32, weather);
        u8g2_DrawXBM(&u8g2, display + 48, trans_y + 6, 32, 32, tree);
        u8g2_DrawXBM(&u8g2, display + 96, trans_y + 6, 32, 32, message);
        u8g2_DrawXBM(&u8g2, display + 144, trans_y + 6, 32, 32, motor);
        u8g2_SendBuffer(&u8g2);
    }
    // 到了最大偏移量，就切换状态进入二级菜单的绘制
    else
    {
        current_ui = UI_STATE_WEATHER_MENU;
    }
}

void update_weather_to_menu_anim(void)
{
    if (trans_y < trans_y_max)
    {
        trans_y += trans_step;
        if (trans_y > trans_y_max) trans_y = trans_y_max;
    }

    char buf[32];
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);

    // base Y positions for each line
    int y0 = 10  - trans_y;
    int y1 = 24  - trans_y;
    int y2 = 36  - trans_y;
    int y3 = 48  - trans_y;


    // Title
    u8g2_DrawStr(&u8g2, (128 - u8g2_GetStrWidth(&u8g2, "Weather Status"))/2, y0, "Weather Status");

    // Temp & Humidity
    snprintf(buf, sizeof(buf), "Temp: %s  Hum: %s", weather_temp, weather_humidity);
    u8g2_DrawStr(&u8g2, 3, y1, buf);

    // Wind direction & power
    snprintf(buf, sizeof(buf), "Wind: %s  WindPower: %s", weather_wind_dir, weather_wind_power);
    u8g2_DrawStr(&u8g2, 3, y2, buf);

    // Rain
    snprintf(buf, sizeof(buf), "Rain: %s", weather_rain);
    u8g2_DrawStr(&u8g2, 3, y3, buf);

    // Draw 16×16 icon in bottom-right corner
    const int icon_w = 16, icon_h = 16;
    const int icon_x = 128 - icon_w;
    const int icon_y = 64  - icon_h;
    if (strcmp(weather_rain, "0") == 0)
    {
        u8g2_DrawXBM(&u8g2, icon_x, icon_y, icon_w, icon_h, sun_icon);
    }
    else
    {
        u8g2_DrawXBM(&u8g2, icon_x, icon_y, icon_w, icon_h, rain_icon);
    }

    u8g2_SendBuffer(&u8g2);

    if (trans_y >= trans_y_max)
    {
        current_ui    = UI_STATE_MENU;
        ui_anim_state = UI_IDLE;
    }
}

/*PLANT-MENU */
void update_menu_to_plant_anim(void)
{
    // 检查当前 Y 偏移量 trans_y 是否还没达到最大值 trans_y_max，还没到就进行动画的绘制
    if (trans_y < trans_y_max)
    {
        trans_y += trans_step;
        // 避免过冲
        if (trans_y > trans_y_max) trans_y = trans_y_max;

        //
        u8g2_ClearBuffer(&u8g2);
        u8g2_DrawXBM(&u8g2, 44, trans_y + 26, 40, 40, arrowhead);
        u8g2_DrawXBM(&u8g2, display, trans_y + 6, 32, 32, weather);
        u8g2_DrawXBM(&u8g2, display + 48, trans_y + 6, 32, 32, tree);
        u8g2_DrawXBM(&u8g2, display + 96, trans_y + 6, 32, 32, message);
        u8g2_DrawXBM(&u8g2, display + 144, trans_y + 6, 32, 32, motor);
        u8g2_SendBuffer(&u8g2);
    }
    // 到了最大偏移量，就切换状态进入二级菜单的绘制
    else
    {
        current_ui = UI_STATE_PLANT_MENU;
    }
}

void update_plant_to_menu_anim(void)
{
    if (trans_y < trans_y_max)
    {
        trans_y += trans_step;
        if (trans_y > trans_y_max) trans_y = trans_y_max;

        char buf[32];
        u8g2_ClearBuffer(&u8g2);
        u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);

        int y0 = 10  - trans_y;
        u8g2_DrawStr(&u8g2, 40, y0, "PLANT STATUS");

        int y1 = 26  - trans_y;
        sprintf(buf, "shidu: %.1f%%", moisture_mv);
        u8g2_DrawStr(&u8g2, 3, y1, buf);

        int y2 = 42  - trans_y;
        sprintf(buf, "wendu: %.1fC", temperature_mv);
        u8g2_DrawStr(&u8g2, 3, y2, buf);

        int y3 = 58  - trans_y;
        sprintf(buf, "guangzhao: %u lx", light_mv);
        u8g2_DrawStr(&u8g2, 3, y3, buf);

        u8g2_SendBuffer(&u8g2);
    }
    else
    {
        current_ui = UI_STATE_MENU;
        ui_anim_state = UI_IDLE;
    }
}

/* NOTE-MENU */
void update_menu_to_note_anim(void)
{
    // 检查当前 Y 偏移量 trans_y 是否还没达到最大值 trans_y_max，还没到就进行动画的绘制
    if (trans_y < trans_y_max)
    {
        trans_y += trans_step;
        // 避免过冲
        if (trans_y > trans_y_max) trans_y = trans_y_max;

        //
        u8g2_ClearBuffer(&u8g2);
        u8g2_DrawXBM(&u8g2, 44, trans_y + 26, 40, 40, arrowhead);
        u8g2_DrawXBM(&u8g2, display, trans_y + 6, 32, 32, weather);
        u8g2_DrawXBM(&u8g2, display + 48, trans_y + 6, 32, 32, tree);
        u8g2_DrawXBM(&u8g2, display + 96, trans_y + 6, 32, 32, message);
        u8g2_DrawXBM(&u8g2, display + 144, trans_y + 6, 32, 32, motor);
        u8g2_SendBuffer(&u8g2);
    }
    // 到了最大偏移量，就切换状态进入二级菜单的绘制
    else
    {
        current_ui = UI_STATE_NOTE_MENU;
    }
}

void update_note_to_menu_anim(void)
{
    if (trans_y < trans_y_max)
    {
        trans_y += trans_step;
        if (trans_y > trans_y_max) trans_y = trans_y_max;

        char buf[32];
        u8g2_ClearBuffer(&u8g2);
        u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);

        int y0 = 10  - trans_y;
        u8g2_DrawStr(&u8g2, 40, y0, "Note");

        int y1 = 26  - trans_y;
        sprintf(buf, "detect: %s", detect_result);
        u8g2_DrawStr(&u8g2, 3, y1, buf);

        int y2 = 42  - trans_y;
        sprintf(buf, "llm: %s", llm_result);
        u8g2_DrawStr(&u8g2, 3, y2, buf);

        u8g2_SendBuffer(&u8g2);
    }
    else
    {
        current_ui = UI_STATE_MENU;
        ui_anim_state = UI_IDLE;
    }
}

/* MOTOR - MENU */
void update_motor_to_menu_anim(void)
{
    if (trans_y < trans_y_max)
    {
        trans_y += trans_step;
        if (trans_y > trans_y_max) trans_y = trans_y_max;

        u8g2_ClearBuffer(&u8g2);
        u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);

        int y0 = 10  - trans_y;
        u8g2_DrawStr(&u8g2, 40, y0, "Watering Control");

        int y1 = 26  - trans_y;
        u8g2_DrawStr(&u8g2, 3, y1, "Start watering");

        int y2 = 42  - trans_y;
        u8g2_DrawStr(&u8g2, 3, y2, "Stop watering");

        int y3 = 58  - trans_y;
        u8g2_DrawStr(&u8g2, 3, y3, "Auto watering");

        u8g2_SendBuffer(&u8g2);
    }
    else
    {
        current_ui = UI_STATE_MENU;
        ui_anim_state = UI_IDLE;
    }
}

void update_menu_to_motor_anim(void)
{
    // 检查当前 Y 偏移量 trans_y 是否还没达到最大值 trans_y_max，还没到就进行动画的绘制
    if (trans_y < trans_y_max)
    {
        trans_y += trans_step;
        // 避免过冲
        if (trans_y > trans_y_max) trans_y = trans_y_max;

        //
        u8g2_ClearBuffer(&u8g2);
        u8g2_DrawXBM(&u8g2, 44, trans_y + 26, 40, 40, arrowhead);
        u8g2_DrawXBM(&u8g2, display, trans_y + 6, 32, 32, weather);
        u8g2_DrawXBM(&u8g2, display + 48, trans_y + 6, 32, 32, tree);
        u8g2_DrawXBM(&u8g2, display + 96, trans_y + 6, 32, 32, message);
        u8g2_DrawXBM(&u8g2, display + 144, trans_y + 6, 32, 32, motor);
        u8g2_SendBuffer(&u8g2);
    }
    // 到了最大偏移量，就切换状态进入二级菜单的绘制
    else
    {
        current_ui = UI_STATE_MOTOR_MENU;
    }
}
/************************************** u8g2 **************************************/
static void oled_init(int argc, char *argv[])
{
    // u8x8_SetI2CAddress(u8g2_GetU8x8(&u8g2), 0x3C << 1);  // 左移1位是因为 u8x8 要求7位地址左移

    // SH1106 I2C 128×64 非品牌全缓冲模式 (_f)，R0 表示不旋转屏幕
    u8g2_Setup_sh1106_i2c_128x64_noname_f(
        &u8g2,
        U8G2_R0,                          // 旋转角度0度
        u8x8_byte_sw_i2c,                 // u8x8_byte_sw_i2c：使用软件 I2C 底层函数
        u8x8_rt_gpio_and_delay            // u8x8_rt_gpio_and_delay：使用 RT-Thread 的 GPIO 和延时实现
    );
    // 配置软件 I2C 的时钟和数据管脚
    u8x8_SetPin(u8g2_GetU8x8(&u8g2),
              U8X8_PIN_I2C_CLOCK, OLED_I2C_PIN_SCL);
    u8x8_SetPin(u8g2_GetU8x8(&u8g2),
              U8X8_PIN_I2C_DATA, OLED_I2C_PIN_SDA);

    // 初始化显示器硬件
    u8g2_InitDisplay(&u8g2);
    // 退出省电模式，开启显示
    u8g2_SetPowerSave(&u8g2, 0);

    u8g2_SetFontMode(&u8g2, 1);    // 启用非透明模式，写 1 即覆盖式绘字
    u8g2_SetFontDirection(&u8g2, 0);// 正常方向

    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
}



void ui_thread_entry(void *parameter)
{
    oled_init(0, RT_NULL);

    rt_tick_t last_act = rt_tick_get();  // 当前系统节拍数

    while (1)
    {
        uint8_t ev;

        if (read_cap_msg(&ev, RT_WAITING_NO) == RT_EOK)
        {
            key_event = ev;
        }
        /* —— 再从 Button mailbox 试读 —— */
        else if (read_button_msg(&ev, RT_WAITING_NO) == RT_EOK)
        {
            key_event = ev;
        }
        else
        {
            key_event = KEY_NONE;
        }
        // printf("current_ui is %d,key_event is %d\n",current_ui,key_event);
        /* 状态机分支不变，只要用全局 key_event 就行 */
        switch (current_ui)
        {
            case UI_STATE_WATCH:
                watch_screen();
                if (key_event == KEY_CLICK)
                {
                    current_ui = UI_STATE_MENU;
                    last_act = rt_tick_get();
                }
                break;
            case UI_STATE_MENU:
                Main_Menu(so_fast_speed);
                if (key_event == KEY_CLICK
                 || key_event == KEY_DOUBLECLICK
                 || key_event == KEY_DOWN
                 || key_event == KEY_UP)
                {
                    last_act = rt_tick_get();
                }
                break;
            case UI_STATE_WEATHER_MENU:
                Weather_Status_Menu();
                break;
            case UI_STATE_PLANT_MENU:
                Plant_Status_Menu();
                break;
            case UI_STATE_NOTE_MENU:
                Note_Menu();
                break;
            case UI_STATE_MOTOR_MENU:
                Motor_Menu();
                break;
            case UI_STATE_MENU_TO_WEATHER:
                update_menu_to_weather_anim();
                break;
            case UI_STATE_WEATHER_TO_MENU:
                update_weather_to_menu_anim();
                break;
            case UI_STATE_MENU_TO_PLANT:
                update_menu_to_plant_anim();
                break;
            case UI_STATE_PLANT_TO_MENU:
                update_plant_to_menu_anim();
                break;
            case UI_STATE_MENU_TO_NOTE:
                update_menu_to_note_anim();
                break;
            case UI_STATE_NOTE_TO_MENU:
                update_note_to_menu_anim();
                break;
            case UI_STATE_MOTOR_TO_MENU:
                update_motor_to_menu_anim();
                break;
            case UI_STATE_MENU_TO_MOTOR:
                update_menu_to_motor_anim();
                break;
            case UI_STATE_MOTOR_OPENED:
                draw_open();
                break;
            case UI_STATE_MOTOR_CLOSED:
                draw_close();
                break;
            case UI_STATE_MOTOR_AUTO:
                draw_auto();
                break;
        }


        /* 超时回到表盘 */
        if (current_ui == UI_STATE_MENU)
        {
            if ((rt_tick_get() - last_act) > IDLE_TIMEOUT_SECONDS * RT_TICK_PER_SECOND)
            {
                current_ui = UI_STATE_WATCH;
            }
        }
        rt_thread_mdelay(10);
    }
}

int oled_ui_thread(void)
{
    rt_thread_t tid = rt_thread_create(
        "oled_ui", ui_thread_entry, RT_NULL, 4096, 20, 10);
    if (tid) rt_thread_startup(tid);
    return RT_EOK;
}
INIT_APP_EXPORT(oled_ui_thread);
// MSH_CMD_EXPORT(oled_ui_thread, Start OLED UI thread);
