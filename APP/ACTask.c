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

const modbusTable_t ACTable03H[] = {
  /**id  code   Num reg offset buf**/
    {1, MOD_03H, 5, 0,  0, 0},
    {1, MOD_03H, 2, 7,  3, 0},
    {1, MOD_03H, 7, 12, 4, 0},
    {1, MOD_03H, 1, 40, 6, 0},
    {1, MOD_03H, 4, 45, 7, 0},
};
#define ACTable03H_LENGTH sizeof(ACTable03H) / sizeof(modbusTable_t)

const MOD_VARTab_t AC_VARTab[] = {
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
#define AC_VARTab_LENGTH sizeof(AC_VARTab) / sizeof(MOD_VARTab_t)

/* Public variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static int U2SendBuf(uint8_t *_buf, uint16_t _len);
/* Private user code ---------------------------------------------------------*/

/*----------------------------------------------------------------------------
 *      Thread 1 'ACTask_Name': Sample thread
 *---------------------------------------------------------------------------*/

void ACTask(void *argument)
{
    uint8_t i, index, count = 0;
    MODBUS_InitVar(&ACMod, 1, 19200, WKM_MODBUS_HOST);
    ACMod.transmit = U2SendBuf;

    while (1) {
        osDelay(999);  // Insert thread code here...
        for (i = 0; i < ACTable03H_LENGTH; i++) {
            if (MODH_ReadParam_03H(&ACMod, ACTable03H[i].id, ACTable03H[i].Reg, ACTable03H[i].Num) == 1) {
                uint16_t *p = (uint16_t *)(&ACMod.RxBuf[3]);
                index       = ACTable03H[i].offset;
                count       = 0;
                do {
                    memcpy(&modbusVar[AC_VARTab[index].Dec], p, AC_VARTab[index].len * 2);
                    count += AC_VARTab[index].len;
                    p += AC_VARTab[index].len;
                    index++;
                } while ((index >= AC_VARTab_LENGTH) || (count < ACTable03H[i].Num));
                osDelay(50);
            } else {
                // time out
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
