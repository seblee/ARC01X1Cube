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
#define U3RXFLAG 0x00000003U

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

uint8_t  USART3_Rx_buf[MOD_BUF_SIZE];
uint8_t  USART3_Tx_buf[MOD_BUF_SIZE];
fifo_s_t uart3RxFifo;
uint8_t  uart3RxFifoBuf[MOD_BUF_SIZE];

/* Public variables ----------------------------------------------------------*/
extern MODBUS_T northMod;
// extern MODBUS_T ACMod;
extern MODBUS_T ipmMod;

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
    USART3_DIR_RX;
    fifo_s_init(&uart3RxFifo, uart3RxFifoBuf, MOD_BUF_SIZE);
    USART4_DIR_RX;
    fifo_s_init(&upsRxFifo[UPS1], upsRxFifoBuf[UPS1], MOD_BUF_SIZE);
    USART5_DIR_RX;
    fifo_s_init(&upsRxFifo[UPS2], upsRxFifoBuf[UPS2], MOD_BUF_SIZE);

    uartDMAStart(&hdma_usart1_rx, &huart1, USART1_Rx_buf, MOD_BUF_SIZE);
    uartDMAStart(&hdma_usart2_rx, &huart2, USART2_Rx_buf, MOD_BUF_SIZE);
    uartDMAStart(&hdma_usart3_rx, &huart3, USART3_Rx_buf, MOD_BUF_SIZE);
    uartDMAStart(&hdma_uart4_rx, &huart4, upsRxbuf[UPS1], MOD_BUF_SIZE);
    HAL_UARTEx_ReceiveToIdle_IT(&huart5, upsRxbuf[UPS2], MOD_BUF_SIZE);
    while (1) {
        osStatus_t status;
        uint32_t   dataOut;
        status = osMessageQueueGet(mid_MsgRx, &dataOut, NULL, 10U);  // wait for message
        if (status == osOK) {
            int length;
            if (dataOut == U1RXFLAG) {
                length = fifo_s_used(&uart1RxFifo);
                fifo_s_gets(&uart1RxFifo, (char *)northMod.RxBuf, length);
                MODBUS_RxData(&northMod, northMod.RxBuf, length);
            } else if (dataOut == U2RXFLAG) {
                length = fifo_s_used(&uart2RxFifo);
                fifo_s_gets(&uart2RxFifo, (char *)ipmMod.RxBuf, length);
                MODBUS_RxData(&ipmMod, ipmMod.RxBuf, length);
            } else if (dataOut == U3RXFLAG) {
                length = fifo_s_used(&uart3RxFifo);
                // fifo_s_gets(&uart3RxFifo, (char *)ACMod.RxBuf, length);
                // MODBUS_RxData(&ACMod, ACMod.RxBuf, length);
            }
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
    } else if (huart == &huart2) {
        fifo_s_puts(&uart2RxFifo, (char *)USART2_Rx_buf, Size);  //数据填入 FIFO
        dataIn = U2RXFLAG;
        osMessageQueuePut(mid_MsgRx, &dataIn, 0U, 0U);
        uartDMAStart(&hdma_usart2_rx, &huart2, USART2_Rx_buf, MOD_BUF_SIZE);
    } else if (huart == &huart3) {
        //fifo_s_puts(&uart3RxFifo, (char *)USART3_Rx_buf, Size);  //数据填入 FIFO
        dataIn = U3RXFLAG;
        osMessageQueuePut(mid_MsgRx, &dataIn, 0U, 0U);
        uartDMAStart(&hdma_usart3_rx, &huart3, USART3_Rx_buf, MOD_BUF_SIZE);
    } else if (huart == &huart4) {
        fifo_s_puts(&upsRxFifo[UPS1], (char *)upsRxbuf[UPS1], Size);  //数据填入 FIFO
        osThreadFlagsSet(upsTaskTid[UPS1], 1);
        uartDMAStart(&hdma_uart4_rx, &huart4, upsRxbuf[UPS1], MOD_BUF_SIZE);
    } else if (huart == &huart5) {
        fifo_s_puts(&upsRxFifo[UPS2], (char *)upsRxbuf[UPS2], Size);  //数据填入 FIFO
        osThreadFlagsSet(upsTaskTid[UPS2], 1);
        HAL_UARTEx_ReceiveToIdle_IT(&huart5, upsRxbuf[UPS2], MOD_BUF_SIZE);
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
    } else if (huart == &huart2) {
        USART2_DIR_RX;
    } else if (huart == &huart3) {
        USART3_DIR_RX;
    } else if (huart == &huart4) {
        USART4_DIR_RX;
    } else if (huart == &huart5) {
        USART5_DIR_RX;
    }
    /* NOTE: This function should not be modified, when the callback is needed,
             the HAL_UART_TxCpltCallback could be implemented in the user file
     */
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(huart);
    /* NOTE: This function should not be modified, when the callback is needed,
             the HAL_UART_ErrorCallback could be implemented in the user file
     */
    if (HAL_UART_GetError(huart) & HAL_UART_ERROR_ORE) {
        __HAL_UART_FLUSH_DRREGISTER(huart);  //读DR寄存器，就可以清除ORE错误标志位
    }
    if (huart == &huart1) {
        uartDMAStart(&hdma_usart1_rx, &huart1, USART1_Rx_buf, MOD_BUF_SIZE);
        USART1_DIR_RX;
    } else if (huart == &huart2) {
        uartDMAStart(&hdma_usart2_rx, &huart2, USART2_Rx_buf, MOD_BUF_SIZE);
        USART2_DIR_RX;
    } else if (huart == &huart3) {
        uartDMAStart(&hdma_usart3_rx, &huart3, USART3_Rx_buf, MOD_BUF_SIZE);
        USART3_DIR_RX;
    } else if (huart == &huart4) {
        uartDMAStart(&hdma_uart4_rx, &huart4, upsRxbuf[UPS1], MOD_BUF_SIZE);
        USART4_DIR_RX;
    } else if (huart == &huart5) {
        HAL_UARTEx_ReceiveToIdle_IT(&huart5, upsRxbuf[UPS2], MOD_BUF_SIZE);
        USART5_DIR_RX;
    }
}
