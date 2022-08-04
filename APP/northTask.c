/**
 * @file northTask.c
 * @author  xiaowine (xiaowine@sina.cn)
 * @brief
 * @version 01.00
 * @date    2022-08-04
 *
 * @copyright Copyright (c) {2020}  xiaowine
 *
 * @par 修改日志:
 * <table>
 * <tr><th>Date       <th>Version <th>Author  <th>Description
 * <tr><td>2022-08-04 <td>1.0     <td>wangh     <td>内容
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
#include <string.h>
#include "bsp_modbus.h"
#include "cmsis_os2.h"  // CMSIS RTOS header file
#include "fifo.h"
#include "usart.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
uint32_t northTaskEvent;
MODBUS_T northMod;

uint16_t modbusVar[MOD_VAR_SIZE] = {0X1234};
/* Public variables ----------------------------------------------------------*/
extern osThreadId_t northTaskTid;
extern uint8_t      USART1_Tx_buf[MOD_BUF_SIZE];
/* Private function prototypes -----------------------------------------------*/

static int RS485_SendBuf(uint8_t *_buf, uint16_t _len);
/* Private user code ---------------------------------------------------------*/
/*----------------------------------------------------------------------------
 *      Thread 1 'Thread_Name': Sample thread
 *---------------------------------------------------------------------------*/

void northTask(void *argument)
{
    MODBUS_InitVar(&northMod, 1, 19200, WKM_MODBUS_DEVICE);
    northMod.transmit = RS485_SendBuf;

    while (1) {
        northTaskEvent = osThreadFlagsWait(1, osFlagsWaitAny, 150);
        if (northTaskEvent == osFlagsErrorTimeout) {
        } else {
        }
    }
}

static int RS485_SendBuf(uint8_t *_buf, uint16_t _len)
{
    HAL_StatusTypeDef rc;
    // UART_DIR4_TX;  // 485_DIR4
    USART1_DIR_TX;  // 485_DIR2
    memcpy(USART1_Tx_buf, _buf, _len);
    rc = HAL_UART_Transmit_DMA(&huart1, USART1_Tx_buf, _len);

    if (rc == HAL_OK) {
        return (int)_len;
    } else {
        return -(int)rc;
    }
}
