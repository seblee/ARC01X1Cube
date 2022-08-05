/**
 * @file ACTask.c
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
#include "usart.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
MODBUS_T ACMod = {0};
/* Public variables ----------------------------------------------------------*/
extern osThreadId_t ACTaskTid;  // thread id
/* Private function prototypes -----------------------------------------------*/
static int U2SendBuf(uint8_t *_buf, uint16_t _len);
/* Private user code ---------------------------------------------------------*/

/*----------------------------------------------------------------------------
 *      Thread 1 'ACTask_Name': Sample thread
 *---------------------------------------------------------------------------*/

void ACTask(void *argument)
{
    MODBUS_InitVar(&ACMod, 1, 19200, WKM_MODBUS_HOST);
    ACMod.transmit = U2SendBuf;
    USART2_DIR_RX;

    while (1) {
        osDelay(500);  // Insert thread code here...
        if (MODH_ReadParam_03H(&ACMod, 1, 100, 2) == 1) { 
        } else {
            // time out
        }

        osThreadYield();  // suspend thread
    }
}

static int U2SendBuf(uint8_t *_buf, uint16_t _len)
{
    HAL_StatusTypeDef rc;
    USART2_DIR_TX;  // 485_DIR2
    memcpy(USART2_Tx_buf, _buf, _len);
    rc = HAL_UART_Transmit_DMA(&huart2, USART2_Tx_buf, _len);

    if (rc == HAL_OK) {
        return (int)_len;
    } else {
        return -(int)rc;
    }
}
