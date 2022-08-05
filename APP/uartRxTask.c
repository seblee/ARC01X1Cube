/**
 * @file uartRxTask.c
 * @author  xiaowine (xiaowine@sina.cn)
 * @brief
 * @version 01.00
 * @date    2022-08-03
 *
 * @copyright Copyright (c) {2020}  xiaowine
 *
 * @par 修改日志:
 * <table>
 * <tr><th>Date       <th>Version <th>Author  <th>Description
 * <tr><td>2022-08-03 <td>1.0     <td>wangh     <td>内容
 * </table>
 * ******************************************************************
 * *                   .::::
 * *                 .::::::::
 * *                ::::::::::
 * *             ..:::::::::::
 * *          '::::::::::::
 * *            .:::::::::
 * *       '::::::::::::::..        女神助攻,流量冲天
 * *            ..::::::::::::.     永不宕机,代码无bug
 * *          ``:::::::::::::::
 * *           ::::``:::::::::'        .:::
 * *          ::::'   ':::::'       .::::::::
 * *        .::::'      ::::     .:::::::'::::
 * *       .:::'       :::::  .:::::::::' ':::::
 * *      .::'        :::::.:::::::::'      ':::::
 * *     .::'         ::::::::::::::'         ``::::
 * * ...:::           ::::::::::::'              ``::
 * *```` ':.          ':::::::::'                  ::::.
 * *                   '.:::::'                    ':'````.
 * ******************************************************************
 */

/* Private includes ----------------------------------------------------------*/
#include "bsp_modbus.h"
#include "cmsis_os2.h"  // CMSIS RTOS header file
#include "fifo.h"
#include "usart.h"
#include "userData.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define U1RXFLAG 0x00000001U
#define U2RXFLAG 0x00000002U

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
uint8_t  USART1_Rx_buf[MOD_BUF_SIZE];
uint8_t  USART1_Tx_buf[MOD_BUF_SIZE];
fifo_s_t uart1RxFifo;
uint8_t  uart1RxFifoBuf[MOD_BUF_SIZE];
uint8_t  USART2_Rx_buf[MOD_BUF_SIZE];
uint8_t  USART2_Tx_buf[MOD_BUF_SIZE];
fifo_s_t uart2RxFifo;
uint8_t  uart2RxFifoBuf[MOD_BUF_SIZE];

/* Public variables ----------------------------------------------------------*/
extern MODBUS_T northMod;
extern MODBUS_T ACMod;

extern osMessageQueueId_t mid_MsgRx;
/* Private function prototypes -----------------------------------------------*/
static void uartDMAStart(DMA_HandleTypeDef *hdma, UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);

/* Private user code ---------------------------------------------------------*/

/*----------------------------------------------------------------------------
 *      uartRxTask 1 'Thread_Name': Sample thread
 *---------------------------------------------------------------------------*/

void uartRxTask(void *argument)
{

    USART1_DIR_RX;
    fifo_s_init(&uart1RxFifo, uart1RxFifoBuf, MOD_BUF_SIZE);
    USART2_DIR_RX;
    fifo_s_init(&uart2RxFifo, uart2RxFifoBuf, MOD_BUF_SIZE);

    uartDMAStart(&hdma_usart1_rx, &huart1, USART1_Rx_buf, MOD_BUF_SIZE);
    uartDMAStart(&hdma_usart2_rx, &huart2, USART2_Rx_buf, MOD_BUF_SIZE);
    while (1) {
        osStatus_t status;
        uint32_t   dataOut;
        status = osMessageQueueGet(mid_MsgRx, &dataOut, NULL, 0U);  // wait for message
        if (status == osOK) {
            uint8_t cache[MOD_BUF_SIZE];
            int     length;
            if (dataOut == U1RXFLAG) {
                length = fifo_s_used(&uart1RxFifo);
                fifo_s_gets(&uart1RxFifo, (char *)cache, length);
                MODBUS_RxData(&northMod, cache, length);
            } else if (dataOut == U2RXFLAG) {
                length = fifo_s_used(&uart2RxFifo);
                fifo_s_gets(&uart2RxFifo, (char *)cache, length);
                MODBUS_RxData(&ACMod, cache, length);
            }
            // osEventFlagsClear (evt_id, U1RXFLAG|U2RXFLAG);
        }
    }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    uint32_t dataIn;
    if (huart == &huart1) {
        // static uint8_t Rx_buf_pos;  //本次回调接收的数据在缓冲区的起点
        // static uint8_t Rx_length;   //本次回调接收数据的长度
        // Rx_length = Size - Rx_buf_pos;
        // fifo_s_puts(&uart1RxFifo, &USART1_Rx_buf[Rx_buf_pos], Rx_length);  //数据填入 FIFO
        // Rx_buf_pos += Rx_length;
        // if (Rx_buf_pos >= MOD_BUF_SIZE)
        //     Rx_buf_pos = 0;  //缓冲区用完后，返回 0 处重新开始
        // osThreadFlagsSet(tidUartRxTAsk, 0x01);
        fifo_s_puts(&uart1RxFifo, (char *)USART1_Rx_buf, Size);  //数据填入 FIFO
        dataIn = U1RXFLAG;
        osMessageQueuePut(mid_MsgRx, &dataIn, 0U, 0U);
        uartDMAStart(&hdma_usart1_rx, &huart1, USART1_Rx_buf, MOD_BUF_SIZE);
    }
    if (huart == &huart2) {
        fifo_s_puts(&uart2RxFifo, (char *)USART2_Rx_buf, Size);  //数据填入 FIFO
        dataIn = U2RXFLAG;
        osMessageQueuePut(mid_MsgRx, &dataIn, 0U, 0U);
        uartDMAStart(&hdma_usart2_rx, &huart2, USART2_Rx_buf, MOD_BUF_SIZE);
    }
}

static void uartDMAStart(DMA_HandleTypeDef *hdma, UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{
    HAL_UARTEx_ReceiveToIdle_DMA(huart, pData, Size);
    __HAL_DMA_DISABLE_IT(hdma, DMA_IT_HT);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(huart);
    if (huart == &huart1) {
        USART1_DIR_RX;
    }
    if (huart == &huart2) {
        USART2_DIR_RX;
    }
    /* NOTE: This function should not be modified, when the callback is needed,
             the HAL_UART_TxCpltCallback could be implemented in the user file
     */
}
