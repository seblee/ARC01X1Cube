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

static const osThreadAttr_t ThreadAttr_start_main = {"app_main", NULL, NULL, NULL, NULL, NULL, osPriorityNormal, NULL, NULL};
osThreadId_t                northTaskTid;  // thread id
static const osThreadAttr_t ThreadAttr_northTask = {"northTask", NULL, NULL, NULL, NULL, NULL, osPriorityNormal, NULL, NULL};
osThreadId_t                uartRxTaskTid;  // thread id
static const osThreadAttr_t ThreadAttr_uartRxTask = {"uartRxTask", NULL, NULL, NULL, NULL, NULL, osPriorityNormal, NULL, NULL};

/*----------------------------------------------------------------------------
 * Application main thread
 *---------------------------------------------------------------------------*/
__NO_RETURN static void start_main(void *argument)
{
    (void)argument;
    // ...

    northTaskTid = osThreadNew(northTask, NULL, &ThreadAttr_northTask);
    if (northTaskTid == NULL) {
    }
    uartRxTaskTid = osThreadNew(uartRxTask, NULL, &ThreadAttr_uartRxTask);
    if (uartRxTaskTid == NULL) {
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
