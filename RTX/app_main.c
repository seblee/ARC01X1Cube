/*----------------------------------------------------------------------------
 * CMSIS-RTOS 'main' function template
 *---------------------------------------------------------------------------*/
#ifdef _RTE_
#include "RTE_Components.h"
#ifdef RTE_RTX_CONFIG_H
#include RTE_RTX_CONFIG_H
#endif
#endif

#include "app_main.h"
#include "cmsis_os2.h"
#include "iwdg.h"
#include "userData.h"

static const osThreadAttr_t ThreadAttr_start_main = {"app_main", NULL, NULL, NULL, NULL, NULL, osPriorityNormal, NULL, NULL};
osThreadId_t                northTaskTid;  // thread id
static const osThreadAttr_t ThreadAttr_northTask = {"northTask", NULL, NULL, NULL, NULL, NULL, osPriorityNormal, NULL, NULL};
osThreadId_t                uartRxTaskTid;  // thread id
static const osThreadAttr_t ThreadAttr_uartRxTask = {"uartRxTask", NULL, NULL, NULL, NULL, NULL, osPriorityNormal, NULL, NULL};
osThreadId_t                ACTaskTid;  // thread id
static const osThreadAttr_t ThreadAttr_ACTask = {"ACTask", NULL, NULL, NULL, NULL, NULL, osPriorityNormal, NULL, NULL};
osThreadId_t                ipmTaskTid;  // thread id
static const osThreadAttr_t ThreadAttr_ipmTask = {"ipmTask", NULL, NULL, NULL, NULL, NULL, osPriorityNormal, NULL, NULL};
osThreadId_t                upsTaskTid[2];  // thread id
static const osThreadAttr_t ThreadAttr_ups1Task = {"ups1Task", NULL, NULL, NULL, NULL, NULL, osPriorityNormal, NULL, NULL};
static const osThreadAttr_t ThreadAttr_ups2Task = {"ups2Task", NULL, NULL, NULL, NULL, NULL, osPriorityNormal, NULL, NULL};

/*----------------------------------------------------------------------------
 * Application main thread
 *---------------------------------------------------------------------------*/
__NO_RETURN static void start_main(void *argument)
{
    (void)argument;
    // ...

    Init_MsgQueue();
    northTaskTid = osThreadNew(northTask, NULL, &ThreadAttr_northTask);
    if (northTaskTid == NULL) {
    }
    uartRxTaskTid = osThreadNew(uartRxTask, NULL, &ThreadAttr_uartRxTask);
    if (uartRxTaskTid == NULL) {
    }
    ACTaskTid = osThreadNew(ACTask, NULL, &ThreadAttr_ACTask);
    if (ACTaskTid == NULL) {
    }
    ipmTaskTid = osThreadNew(ipmTask, NULL, &ThreadAttr_ipmTask);
    if (ipmTaskTid == NULL) {
    }
    upsTaskTid[UPS1] = osThreadNew(upsTask, (void *)UPS1, &ThreadAttr_ups1Task);
    if (upsTaskTid[UPS1] == NULL) {
    }
    upsTaskTid[UPS2] = osThreadNew(upsTask, (void *)UPS2, &ThreadAttr_ups2Task);
    if (upsTaskTid[UPS2] == NULL) {
    }

    for (;;) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        osDelay(500);
        HAL_IWDG_Refresh(&hiwdg);
    }
}

void app_main(void)
{
    // System Initialization
    // SystemCoreClockUpdate();
    // ...

    osKernelInitialize();  // Initialize CMSIS-RTOS

    osThreadNew(start_main, NULL, &ThreadAttr_start_main);  // Create application main thread
    if (osKernelGetState() == osKernelReady) {
        osKernelStart();  // Start thread execution
    }
}
