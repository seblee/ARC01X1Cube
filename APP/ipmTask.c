/**
 * @file ipmTask.c
 * @author  xiaowine (xiaowine@sina.cn)
 * @brief
 * @version 01.00
 * @date    2022-08-05
 *
 * @copyright Copyright (c) {2020}  xiaowine
 *
 * @par 修改日志:
 * <table>
 * <tr><th>Date       <th>Version <th>Author  <th>Description
 * <tr><td>2022-08-05 <td>1.0     <td>wangh     <td>内容
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
#include "usart.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
MODBUS_T            ipmMod        = {0};
const modbusTable_t ipmTable03H[] = {
  /**id  code   Num reg offset buf**/
    {1, MOD_03H, 5, 0,  0, 0},
    {1, MOD_03H, 2, 7,  3, 0},
    {1, MOD_03H, 7, 12, 4, 0},
    {1, MOD_03H, 1, 40, 6, 0},
    {1, MOD_03H, 4, 45, 7, 0},
};
#define ipmTable03H_LENGTH sizeof(ipmTable03H) / sizeof(modbusTable_t)

const MOD_VARTab_t ipm_VARTab[] = {
  /* S   D  Len*/
    {0,  33, 2}, //  0
    {2,  37, 1}, //  1
    {3,  17, 2}, //  2
    {7,  19, 2}, //  3
    {12, 16, 1}, //  4
    {13, 24, 6}, //  5
    {40, 21, 1}, //  6
    {45, 40, 4}, //  7
};
#define ipm_VARTab_LENGTH sizeof(ipm_VARTab) / sizeof(MOD_VARTab_t)
/* Public variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

/*----------------------------------------------------------------------------
 *      Thread 1 'Thread_Name': Sample thread
 *---------------------------------------------------------------------------*/

void ipmTask(void *argument)
{
    uint8_t i, index, count = 0;
    MODBUS_InitVar(&ipmMod, 1, 19200, WKM_MODBUS_HOST);
    ipmMod.transmit = U3SendBuf;
    USART3_DIR_RX;

    while (1) {
        osDelay(999);  // Insert thread code here...
        for (i = 0; i < ipmTable03H_LENGTH; i++) {
            if (MODH_ReadParam_03H(&ipmMod, ipmTable03H[i].id, ipmTable03H[i].Reg, ipmTable03H[i].Num) == 1) {
                uint16_t *p = (uint16_t *)(&ipmMod.RxBuf[3]);
                index       = ipmTable03H[i].offset;
                count       = 0;
                do {
                    memcpy(&modbusVar[ipm_VARTab[index].Dec], p, ipm_VARTab[index].len * 2);
                    count += ipm_VARTab[index].len;
                    p += ipm_VARTab[index].len;
                    index++;
                } while ((index >= ipm_VARTab_LENGTH) || (count < ipmTable03H[i].Num));
                osDelay(50);
            } else {
                // time out
            }
        }
        osThreadYield();  // suspend thread
    }
}

static int U3SendBuf(uint8_t *_buf, uint16_t _len)
{
    HAL_StatusTypeDef rc;
    USART3_DIR_TX;  // 485_DIR3
    memcpy(USART3_Tx_buf, _buf, _len);
    rc = HAL_UART_Transmit_DMA(&huart3, USART3_Tx_buf, _len);

    if (rc == HAL_OK) {
        return (int)_len;
    } else {
        return -(int)rc;
    }
}
