#include "cmsis_os2.h"  // CMSIS RTOS header file
#include "usart.h"

#define BUFFER_SIZE 128
uint8_t rxDMAbuffer[BUFFER_SIZE] = {0};
/*----------------------------------------------------------------------------
 *      Thread 1 'Thread_Name': Sample thread
 *---------------------------------------------------------------------------*/

osThreadId_t tid_Thread;  // thread id

void Thread(void *argument);  // thread function

void u1Thread(void *argument)
{
    osThreadId_t thread_id = osThreadGetId();
    const char  *name      = osThreadGetName(thread_id);
    // printf("threadName:%s\r\n", name);
     HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rxDMAbuffer, BUFFER_SIZE);

    while (1) {
    }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(huart);
    UNUSED(Size);

    /* NOTE : This function should not be modified, when the callback is needed,
              the HAL_UARTEx_RxEventCallback can be implemented in the user file.
     */

    if (huart == &huart1) {
        rx_len = Size;  //计算接收到的数据长度
        if (rx_len > 0) {
            if (rxCount + rx_len > BUFFER_SIZE)
                rxCount = 0;
            memcpy(bleRxBuff + rxCount, rxDMAbuffer, rx_len);
            rxCount += rx_len;
            uartGetflag = 1;
            osThreadFlagsSet(bleTid, 0x01);
        }
        memset(rxDMAbuffer, 0, rx_len);  //清零接收缓冲区
        uart_start_receive();            //重新打开DMA接收
    }
}