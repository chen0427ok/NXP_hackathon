#include "FreeRTOS.h"
#include "task.h"
#include "fsl_debug_console.h"
#include "lvgl_support.h"
#include "pin_mux.h"
#include "board.h"
#include "lvgl.h"
#include "gui_guider.h"
#include "events_init.h"
#include "custom.h"
#include <stdio.h>
#include "widgets_init.h"
#include "lvgl_demo_utils.h"
#include "lvgl_freertos.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_device_registers.h"  // 引入以支持__NOP()

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define DHT11_GPIO GPIO6
#define DHT11_GPIO_PIN 24

/*******************************************************************************
 * Variables
 ******************************************************************************/
static volatile bool s_lvgl_initialized = false;
lv_ui guider_ui;

volatile uint32_t g_systickCounter;
volatile bool g_pinSet = false;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

#if LV_USE_LOG
static void print_cb(const char *buf)
{
    PRINTF("\r%s\n", buf);
}
#endif

static uint32_t get_idle_time_cb(void)
{
    return (getIdleTaskTime() / 1000);
}

static void reset_idle_time_cb(void)
{
    resetIdleTaskTime();
}

/*******************************************************************************
 * 微秒延遲函數基於__NOP()
 ******************************************************************************/
static void delayms(int ms)
{
    volatile uint32_t i = 0;
    for (i = 0; i < ms*70000; ++i)
    {
    __NOP(); /* delay */
    }

}

static void delayus(int us)
{
    volatile uint32_t i = 0;
    for (i = 0; i < us*70; ++i)
    {
    __NOP(); /* delay */
    }

}

/*******************************************************************************
 * Task Definitions
 ******************************************************************************/
static void AppTask(void *param)
{
#if LV_USE_LOG
    lv_log_register_print_cb(print_cb);
#endif

    lv_timer_register_get_idle_cb(get_idle_time_cb);
    lv_timer_register_reset_idle_cb(reset_idle_time_cb);
    lv_port_pre_init();
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();

    s_lvgl_initialized = true;

    setup_ui(&guider_ui);
    events_init(&guider_ui);
    custom_init(&guider_ui);

#if LV_USE_VIDEO
    Video_InitPXP();
#endif

    for (;;)
    {
        lv_task_handler();
        vTaskDelay(1);
    }
}

/*******************************************************************************
 * DHT11 Task
 ******************************************************************************/
uint8_t read_data_bit(void)
{
    uint8_t bit = 0;
    while (GPIO_PinRead(DHT11_GPIO, DHT11_GPIO_PIN) == 0);  // 等待信號線變高
    delayus(35);   // 等待 35µs
    if (GPIO_PinRead(DHT11_GPIO, DHT11_GPIO_PIN) == 1)  // 如果信號線仍為高, 則為 1
    {
        bit = 1;
    }
    while (GPIO_PinRead(DHT11_GPIO, DHT11_GPIO_PIN) == 1);  // 等待信號線變低
    return bit;
}

static void DHT11_Task(void *param)
{
    gpio_pin_config_t USER_DHT_out_config = {
        .direction = kGPIO_DigitalOutput,
        .outputLogic = 1U,
        .interruptMode = kGPIO_NoIntmode
    };

    gpio_pin_config_t USER_DHT_in_config = {
        .direction = kGPIO_DigitalInput,
        .outputLogic = 0U,
        .interruptMode = kGPIO_NoIntmode
    };

    PRINTF("DHT11 Task Started\r\n");

    uint8_t temperature = 0;
    uint8_t humidity = 0;

    for (;;)
    {
        GPIO_PinInit(DHT11_GPIO, DHT11_GPIO_PIN, &USER_DHT_out_config);

        delayms(2000);  // 延遲 2 秒

        GPIO_PinWrite(DHT11_GPIO, DHT11_GPIO_PIN, 0);
        delayms(18);  // 18ms 延遲
        GPIO_PinWrite(DHT11_GPIO, DHT11_GPIO_PIN, 1);

        delayus(40);  // 40us 延遲

        GPIO_PinInit(DHT11_GPIO, DHT11_GPIO_PIN, &USER_DHT_in_config);
        delayus(160);  // 等待 180us
        //PRINTF("111/r/n");
        uint8_t data[5] = {0};
        for (int i = 0; i < 5; i++)
        {
            for (int j = 7; j >= 0; j--)
            {
                data[i] |= (read_data_bit() << j);
            }
        }
        //PRINTF("222/r/n");
        temperature = data[2];
        humidity = data[0];

        PRINTF("Temperature: %d, Humidity: %d\r\n", temperature, humidity);

        lv_arc_set_value(guider_ui.screen_arc_1, temperature);
        lv_label_set_text_fmt(guider_ui.screen_label_1, "%d °C", temperature);

        lv_arc_set_value(guider_ui.screen_arc_2, humidity);
        lv_label_set_text_fmt(guider_ui.screen_label_2, "%d %%", humidity);

        vTaskDelay(pdMS_TO_TICKS(2000));  // 每 2 秒更新一次數據
    }
}

/*!
 * @brief Main function
 */
int main(void)
{
    BaseType_t stat;

    BOARD_ConfigMPU();
    BOARD_InitPins();
    BOARD_InitI2C1Pins();
    BOARD_InitSemcPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();
    DEMO_InitUsTimer();

    /* 創建 LVGL 任務 */
    stat = xTaskCreate(AppTask, "lvgl", configMINIMAL_STACK_SIZE + 12000, NULL, configMAX_PRIORITIES - 1, NULL);
    if (pdPASS != stat)
    {
        PRINTF("Failed to create lvgl task");
        while (1);
    }

    /* 創建 DHT11 任務 */
    stat = xTaskCreate(DHT11_Task, "DHT11", configMINIMAL_STACK_SIZE + 2000, NULL, configMAX_PRIORITIES - 3, NULL);
    if (pdPASS != stat)
    {
        PRINTF("Failed to create DHT11 task");
        while (1);
    }

    vTaskStartScheduler();

    for (;;)
    {
    } /* should never get here */
}

/*!
 * @brief Malloc failed hook.
 */
void vApplicationMallocFailedHook(void)
{
    PRINTF("Malloc failed. Increase the heap size.");

    for (;;)
        ;
}

/*!
 * @brief FreeRTOS tick hook.
 */
void vApplicationTickHook(void)
{
    if (s_lvgl_initialized)
    {
        lv_tick_inc(1);
    }
}

/*!
 * @brief Stack overflow hook.
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)pcTaskName;
    (void)xTask;

    for (;;)
        ;
}

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
{
    /* If the buffers to be provided to the Idle task are declared inside this
    function then they must be declared static - otherwise they will be allocated on
    the stack and so not exists after this function exits. */
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
    state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/*-----------------------------------------------------------*/

/* configUSE_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize)
{
    /* If the buffers to be provided to the Timer task are declared inside this
    function then they must be declared static - otherwise they will be allocated on
    the stack and so not exists after this function exits. */
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

    /* Pass out a pointer to the StaticTask_t structure in which the Timer
    task's state will be stored. */
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

    /* Pass out the array that will be used as the Timer task's stack. */
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;

    /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configTIMER_TASK_STACK_DEPTH is specified in words, not bytes. */
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
