/**
 * @file upsTask.c
 *
 * @author  xiaowine (xiaowine@sina.cn)
 * @brief
 * @version 01.00
 * @date    2022-08-09
 *
 * @copyright Copyright (c) {2020}  xiaowine
 *
 * @par 修改日志:
 * <table>
 * <tr><th>Date       <th>Version <th>Author  <th>Description
 * <tr><td>2022-08-09 <td>1.0     <td>wangh     <td>内容
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
#include "upsTask.h"
#include <stdio.h>
#include <string.h>
#include "fifo.h"
#include "usart.h"
#include "userData.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/
static osStatus_t Q1Pro(uint8_t ups, char *buff, uint8_t len);
/* Private variables ---------------------------------------------------------*/

const cmd_ups_t commandMap[] = {
    {"Q1\r", '(', '\r', 1, Q1Pro}, // UPS_Q1
};

/* Private variables ----------------------------------------------------------*/
uint8_t  upsRxbuf[2][MOD_BUF_SIZE];
uint8_t  upsTxbuf[2][MOD_BUF_SIZE];
fifo_s_t upsRxFifo[2];
uint8_t  upsRxFifoBuf[2][MOD_BUF_SIZE];
/* Public variables -----------------------------------------------------------*/

/* Public function prototypes -------------------------------------------------*/

/* Private function prototypes ------------------------------------------------*/
static osStatus_t upsCommand(uint8_t ups, _upsInquiryCmd_t cmd);
/* Private user code ----------------------------------------------------------*/

/*----------------------------------------------------------------------------
 *      Thread 1 'Thread_Name': Sample thread
 *---------------------------------------------------------------------------*/

void upsTask(void *argument)
{
    uint32_t ups = (uint32_t)argument;

    while (1) {
        osStatus_t rec;
        osDelay(1000);
        rec = upsCommand(ups, UPS_Q1);
        if (rec == osOK) {
        } else {
        }
        osThreadYield();  // suspend thread
    }
}

uint32_t upsRXmqClear(void)
{
    return osThreadFlagsWait(1, osFlagsWaitAny, 0);  // wait for message
}
static uint8_t upsRec(uint8_t ups, char *buff, uint32_t timeout)
{
    uint32_t status;
    status = osThreadFlagsWait(1, osFlagsWaitAny, 0);
    if (status == osFlagsErrorTimeout) {
        return 0;
    } else {
        int length = fifo_s_used(&upsRxFifo[ups]);
        fifo_s_gets(&upsRxFifo[ups], buff, length);
        return length;
    }
}
static osStatus_t upsSend(uint8_t ups, _upsInquiryCmd_t cmd)
{
    if (ups == UPS1) {
    } else if (ups == UPS2) {
    } else {
        return osErrorParameter;
    }
    return osOK;
    // return spiMsgQPush((uint8_t *)commandMap[cmd].cmd, strlen(commandMap[cmd].cmd));
}

static osStatus_t upsCommand(uint8_t ups, _upsInquiryCmd_t cmd)
{
    uint8_t len;
    char    cache[128] = {0};
    // rt_kprintf("ups%d,cmd%d**\r\n", ups, cmd, len);
    upsRXmqClear();
    upsSend(ups, cmd);
    len = upsRec(ups, cache, 800);
    if ((commandMap[cmd].process != NULL) && (len >= commandMap[cmd].bklen)) {
        char *p;
        p = strchr((void *)cache, commandMap[cmd].bkHead);
        if (p != NULL) {
            len -= (p - (char *)cache);
            if (len >= commandMap[cmd].bklen) {
                if (*(p + commandMap[cmd].bklen) == commandMap[cmd].bkEnd)
                    commandMap[cmd].process(ups, p, len);
                else {
                    // rt_kprintf("ups%d,cmd%d,errLen:%d bklen:%d end:%02x,temp:%s **\r\n", ups, cmd, len, commandMap[cmd].bklen, *(p + commandMap[cmd].bklen), temp);
                    return osError;
                }
            } else {
                // rt_kprintf("ups:%d errLen:%d,temp:%s **\r\n", ups, len, temp);
                return osError;
            }
        } else {
            // rt_kprintf("ups:%d err head:%s **\r\n", ups, temp);
            return osError;
        }
        return osOK;
    }
    if (len > 0) {
        // rt_kprintf("ups%d,cmd%d len:%d temp:%s **\r\n", ups, cmd, len, temp);
    }
    if (len == 0) {
        return osErrorTimeout;
    }
    return osOK;
}

static osStatus_t Q1Pro(uint8_t ups, char *buff, uint8_t len)
{
    float   mmm, nnn, ppp, rrr, sss, ttt;
    int     qqq           = 0;
    char    upsStatus[20] = {0};
    uint8_t i;

    //"(MMM.M NNN.N PPP.P QQQ RR.R S.SS TT.T 76543210\r"
    sscanf(buff, "(%f %f %f %d %f %f %f %s\r", &mmm, &nnn, &ppp, &qqq, &rrr, &sss, &ttt, upsStatus);
    modbusVar[15 * ups + 70] = qqq;
    for (i = 0; i < 8; i++) {
        modbusVar[15 * ups + 71] <<= 1;
        modbusVar[15 * ups + 71] |= (upsStatus[i] == '1') ? 1 : 0;
    }
    modbusVar[15 * ups + 72] = mmm * 100;
    modbusVar[15 * ups + 77] = ttt * 10;
    return osOK;
}

static int upsSendBuf(uint8_t ups, uint8_t *_buf, uint16_t _len)
{
    HAL_StatusTypeDef rc = HAL_OK;
    // UART_DIR4_TX;  // 485_DIR4
    memcpy(upsTxbuf[ups], _buf, _len);
    if (ups == UPS1) {
        USART4_DIR_TX;  // 485_DIR2
        rc = HAL_UART_Transmit_DMA(&huart4, upsTxbuf[ups], _len);
    } else if (ups == UPS2) {
        USART5_DIR_TX;  //
        // rc = HAL_UART_Transmit_DMA(&huart4, upsTxbuf[ups], _len);
    }

    if (rc == HAL_OK) {
        return (int)_len;
    } else {
        return -(int)rc;
    }
}
