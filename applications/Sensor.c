#include <rtthread.h>
#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "Sensor.h"


/**********************ADC宏定义配置 *************************/
#define NUM_SCAN          (4)      // 要做多少次通道扫描取平均
#define VPLUS_CHANNEL_0   (P10_0)
#define VPLUS_CHANNEL_1   (P10_1)
#define VPLUS_CHANNEL_2   (P10_2)
#define MICRO_TO_MILLI_CONV_RATIO   (1000u)  // μV -> mV比例
#define ACQUISITION_TIME_NS         (1000u)  // ADC通道采样保持时间
#define ADC_SCAN_DELAY_MS           (2000u)  // 读取之间线程休眠时间
int32_t moisture_mv=0;
int32_t temperature_mv=0;
int32_t light_mv=0;
// 索引 result_arr 数组里的每一路通道
enum ADC_CHANNELS {
    CHANNEL_0 = 0,
    CHANNEL_1,
    CHANNEL_2,
    NUM_CHANNELS
};

/* 全局变量 */
static cyhal_adc_t            adc_obj;            // ADC外设句柄
static cyhal_adc_channel_t    adc_chan_0_obj;     // 通道句柄
static cyhal_adc_channel_t    adc_chan_1_obj;
static cyhal_adc_channel_t    adc_chan_2_obj;
static int32_t                result_arr[NUM_CHANNELS * NUM_SCAN];
static volatile bool          async_read_complete = false;   // 异步读完成的标志

/**********************ADC外设基础配置 *************************/
/* ADC外设基础工作模式配置 */
static const cyhal_adc_config_t adc_config = {
    .continuous_scanning = false,                   // 连续扫描模式
    .average_count       = 1,                      // PDL 本身不做多次平均
    .vref                = CYHAL_ADC_REF_VDDA,     // 参考电压用 VDDA（模拟电源）
    .vneg                = CYHAL_ADC_VNEG_VSSA,    // 单端测量负端接地
    .resolution          = 12u,
    .ext_vref            = NC,
    .bypass_pin          = NC
};

/* 异步回调 软中断完成一次指定次数扫描，触发回调，将 async_read_complete标志位置1 */
static void adc_event_handler(void* arg, cyhal_adc_event_t event)
{
    if (event & CYHAL_ADC_ASYNC_READ_COMPLETE) {
        async_read_complete = true;
    }
}

/* 多通道初始化函数 */
void adc_multi_channel_init(void)
{
    cy_rslt_t rslt;

    // 1) 初始化 ADC 外设，指定用哪一路硬件（这里给第 0 通道的引脚，HAL 会据此选择底层 ADC block）
    rslt = cyhal_adc_init(&adc_obj, VPLUS_CHANNEL_0, NULL);
    if (rslt != CY_RSLT_SUCCESS) {
        rt_kprintf("adc init failed %ld\n", rslt);
        return;
    }

    // 2) 应用之前配置好的大项
    rslt = cyhal_adc_configure(&adc_obj, &adc_config);
    if (rslt != CY_RSLT_SUCCESS) {
        rt_kprintf("adc init failed %ld\n", rslt);
        return;
    }

    // 3) 每个通道的细节配置，平均，采样时间等
    const cyhal_adc_channel_config_t ch_cfg = {
        .enabled           = true,
        .enable_averaging  = false,
        .min_acquisition_ns = ACQUISITION_TIME_NS
    };

    // 4) 打开三个通道
    rslt = cyhal_adc_channel_init_diff(&adc_chan_0_obj, &adc_obj, VPLUS_CHANNEL_0, CYHAL_ADC_VNEG, &ch_cfg);
    if (rslt != CY_RSLT_SUCCESS)
    {
        rt_kprintf("ADC error %ld at %s:%d\n", rslt, __FILE__, __LINE__);
        return;
    }
    rslt = cyhal_adc_channel_init_diff(&adc_chan_1_obj, &adc_obj, VPLUS_CHANNEL_1,CYHAL_ADC_VNEG, &ch_cfg);
    if (rslt != CY_RSLT_SUCCESS)
    {
        rt_kprintf("ADC error %ld at %s:%d\n", rslt, __FILE__, __LINE__);
        return;
    }
    rslt = cyhal_adc_channel_init_diff(&adc_chan_2_obj, &adc_obj, VPLUS_CHANNEL_2,CYHAL_ADC_VNEG, &ch_cfg);
    if (rslt != CY_RSLT_SUCCESS)
    {
        rt_kprintf("ADC error %ld at %s:%d\n", rslt, __FILE__, __LINE__);
        return;
    }

    // 5) 注册回调：打开异步完成事件
    cyhal_adc_register_callback(&adc_obj, &adc_event_handler, result_arr);
    cyhal_adc_enable_event(&adc_obj,
                          CYHAL_ADC_ASYNC_READ_COMPLETE,
                          CYHAL_ISR_PRIORITY_DEFAULT,
                          true);
}

/* 多通道异步读取与处理 */
void adc_multi_channel_process(void)
{
    // 异步触发 4 次扫描，每次扫 3 路，结果放到 result_arr
    async_read_complete = false;
    cy_rslt_t rslt = cyhal_adc_read_async_uv(&adc_obj, NUM_SCAN, result_arr);
    if (rslt != CY_RSLT_SUCCESS)
    {
        rt_kprintf("ADC error %ld at %s:%d\n", rslt, __FILE__, __LINE__);
        return;
    }
    // 等待回调把 async_read_complete 置位
    /*while (!async_read_complete) {
        printf("ADC transfrom is not complete!");
    }*/

    // 下面把每路 4 次的 μV 值累加平均，再换算成 mV
    {
        int64_t sum0 = 0, sum1 = 0, sum2 = 0;
        for (int i = 0; i < NUM_SCAN; ++i) {
            sum0 += result_arr[i * NUM_CHANNELS + CHANNEL_0];
            sum1 += result_arr[i * NUM_CHANNELS + CHANNEL_1];
            sum2 += result_arr[i * NUM_CHANNELS + CHANNEL_2];
        }
        moisture_mv = (sum0 / NUM_SCAN) / MICRO_TO_MILLI_CONV_RATIO;
        temperature_mv = (sum1 / NUM_SCAN) / MICRO_TO_MILLI_CONV_RATIO;
        light_mv = (sum2 / NUM_SCAN) / MICRO_TO_MILLI_CONV_RATIO;
        // rt_kprintf("moisture_mv = %d,temperature_mv=%d,light_mv=%d\n",moisture_mv,temperature_mv,light_mv);
        rt_thread_delay(50);
    }
}

static void Sensor_thread_entry(void *parameter)
{
    adc_multi_channel_init();

    for (;;)
    {
        adc_multi_channel_process();
        rt_thread_mdelay(ADC_SCAN_DELAY_MS);
    }
}

/* 线程注册 */
int Sensor_thread(void)
{
    rt_thread_t tid = rt_thread_create("Sensor",
                                       Sensor_thread_entry,
                                       RT_NULL,
                                       2048,
                                       20,
                                       10);
    if (tid)
        rt_thread_startup(tid);
    return tid ? RT_EOK : -RT_ERROR;

}
INIT_APP_EXPORT(Sensor_thread);
// MSH_CMD_EXPORT(Sensor_thread, Sensor detect);
