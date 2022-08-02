#include "bsp_modbus.h"
#include "cmsis_os2.h"  // CMSIS RTOS header file
#include "fifo.h"
#include "usart.h"
/*----------------------------------------------------------------------------
 *      Thread 1 'Thread_Name': Sample thread
 *---------------------------------------------------------------------------*/
#define RX_BUF_SIZE 300
uint8_t  USART1_Rx_buf[RX_BUF_SIZE];
uint8_t  USART1_Rx_buf[RX_BUF_SIZE];
fifo_s_t uart_rx_fifo;
fifo_s_t uart_tx_fifo;

extern osThreadId_t      northTaskTid;
uint32_t                 northTaskEvent;
#define U1RXFLAG (uint32_t)0X8000
MODBUS_T                 modDEVICE;
extern DMA_HandleTypeDef hdma_usart1_rx;


static void uart_start_receive(void);

void northTask(void *argument)
{
    fifo_s_init(&uart_rx_fifo, USART1_Rx_buf, RX_BUF_SIZE);
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, USART1_Rx_buf, RX_BUF_SIZE);
    while (1) {
        northTaskEvent = osThreadFlagsWait(U1RXFLAG, osFlagsWaitAny, 150);
        if (northTaskEvent == osFlagsErrorTimeout) {
            modDEVICE.g_rtu_timeout = 1;
            MODBUS_Poll(&modDEVICE);
        } else {
            uint8_t cache[RX_BUF_SIZE];
            int     length = fifo_s_used(&uart_rx_fifo);
            fifo_s_gets(&uart_rx_fifo, (char*)cache, length);
            MODBUS_RxData(&modDEVICE, cache, length);
        }
    }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart == &huart1) {
        // static uint8_t Rx_buf_pos;  //本次回调接收的数据在缓冲区的起点
        // static uint8_t Rx_length;   //本次回调接收数据的长度
        // Rx_length = Size - Rx_buf_pos;
        // fifo_s_puts(&uart_rx_fifo, &USART1_Rx_buf[Rx_buf_pos], Rx_length);  //数据填入 FIFO
        // Rx_buf_pos += Rx_length;
        // if (Rx_buf_pos >= RX_BUF_SIZE)
        //     Rx_buf_pos = 0;  //缓冲区用完后，返回 0 处重新开始
        // osThreadFlagsSet(northTaskTid, 0x01);
        fifo_s_puts(&uart_rx_fifo, (char*)USART1_Rx_buf, Size);  //数据填入 FIFO
        osThreadFlagsSet(northTaskTid, U1RXFLAG);
        uart_start_receive();
    }
}
static void uart_start_receive(void)
{
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, USART1_Rx_buf, RX_BUF_SIZE);
    __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);
}
