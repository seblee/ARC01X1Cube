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
#include <string.h>
#include "bsp_modbus.h"
#include "bsp_user_lib.h"
#include "cmsis_os2.h"  // CMSIS RTOS header file
#include "usart.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
MODBUS_T            ipmMod        = {0};
const modbusTable_t ipmTable03H[] = {
  /**id  code    Num     reg offset buf**/
    {1, MOD_03H, 0x03, 0x0025, 0, 0}, //  IPM_Ux
    {1, MOD_03H, 0x07, 0x002b, 1, 0}, //  IPM_Ux
    {1, MOD_03H, 0x04, 0x0036, 2, 0}, //  IPM_Ux
    {1, MOD_03H, 0x03, 0x003e, 3, 0}, //  IPM_Ux
};
#define ipmTable03H_LENGTH sizeof(ipmTable03H) / sizeof(modbusTable_t)

const MOD_VARTab_t ipm_VARTab[] = {
  /*   S     D  Len*/
    {0x0025, 50, 3}, //  0
    {0x002b, 53, 7}, //  1
    {0x0036, 60, 4}, //  2
    {0x003e, 66, 1}, //  3
    {0x003f, 65, 1}, //  4
    {0x0040, 64, 1}, //  5
};
#define ipm_VARTab_LENGTH sizeof(ipm_VARTab) / sizeof(MOD_VARTab_t)
/* Public variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static int U2SendBuf(uint8_t *_buf, uint16_t _len);
/* Private user code ---------------------------------------------------------*/

/*----------------------------------------------------------------------------
 *      Thread 1 'Thread_Name': Sample thread
 *---------------------------------------------------------------------------*/

void ipmTask(void *argument)
{
    uint8_t i, index, count = 0;
    MODBUS_InitVar(&ipmMod, 1, 19200, WKM_MODBUS_HOST);
    ipmMod.transmit = U2SendBuf;

    while (1) {
        static uint8_t errCnt = 0;
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
                } while ((index < ipm_VARTab_LENGTH) && (count < ipmTable03H[i].Num));
                osDelay(50);
                errCnt = 0;
                deviceStatus(3, 1);
            } else {
                osDelay(500);
                if (errCnt < 10) {
                    errCnt++;
                } else if (errCnt == 10) {
                    memset(&modbusVar[50], 0, 40);
                    errCnt++;
                } else {
                    deviceStatus(3, 0);
                }

                break;
            }
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
