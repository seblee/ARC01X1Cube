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
#include "bsp_user_lib.h"
#include "fifo.h"
#include "usart.h"
#include "userData.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/
static osStatus_t Q1Pro(uint8_t ups, char *buff, uint8_t len);
static osStatus_t GOPPro(uint8_t ups, char *buff, uint8_t len);
static osStatus_t GBATPro(uint8_t ups, char *buff, uint8_t len);
static osStatus_t BLPro(uint8_t ups, char *buff, uint8_t len);
static osStatus_t GMODPro(uint8_t ups, char *buff, uint8_t len);
/* Private variables ---------------------------------------------------------*/

const cmd_ups_t commandMap[] = {
    {"Q1\r",   '(', '\r', 46, Q1Pro  }, //  UPS_Q1
    {"GOP\r",  '(', '\r', 34, GOPPro }, //  UPS_GOP
    {"GBAT\r", '(', '\r', 26, GBATPro}, //  UPS_GBAT
    {"BL\r",   'B', '\r', 5,  BLPro  }, //  UPS_BL
    {"GMOD\r", '(', '\r', 2,  GMODPro}, //  UPS_GMOD
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
static int        upsSendBuf(uint8_t ups, uint8_t *_buf, uint16_t _len);
/* Private user code ----------------------------------------------------------*/

/*----------------------------------------------------------------------------
 *      Thread 1 'Thread_Name': Sample thread
 *---------------------------------------------------------------------------*/

void upsTask(void *argument)
{
    uint32_t   ups = (uint32_t)argument;
    osStatus_t rec;

    while (1) {
        static uint8_t errCnt[2] = {0};
        osDelay(1000);
        rec = upsCommand(ups, UPS_Q1);
        if (rec == osOK) {
            osDelay(10);
            errCnt[ups] = 0;
            deviceStatus(ups + 4, 1);
        } else {
            if (errCnt[ups] < 10) {
                errCnt[ups]++;
            } else if (errCnt[ups] == 10) {
                errCnt[ups]++;
                memset(&modbusVar[70 + 15 * ups], 0, 30);
            } else {
                deviceStatus(ups + 4, 0);
            }
            osDelay(500);
            continue;
        }
        rec = upsCommand(ups, UPS_GOP);
        osDelay(10);
        rec = upsCommand(ups, UPS_GBAT);
        osDelay(10);
        rec = upsCommand(ups, UPS_BL);
        osDelay(10);
        rec = upsCommand(ups, UPS_GMOD);
        osDelay(10);
        osThreadYield();  // suspend thread
    }
}
// Q1    收←◆ 51 31 0D
// GOP   收←◆ 47 4F 50 0D
// GBAT  收←◆ 47 42 41 54 0D
// BL    收←◆ 42 4C 0D
// GMOD  收←◆ 47 4D 4F 44 0D

uint32_t upsRXmqClear(void)
{
    return osThreadFlagsWait(1, osFlagsWaitAny, 0);  // wait for message
}
static uint8_t upsRec(uint8_t ups, char *buff, uint32_t timeout)
{
    uint32_t status;
    status = osThreadFlagsWait(1, osFlagsWaitAny, 800);
    if (status == osFlagsErrorTimeout) {
        return 0;
    } else {
        int length = fifo_s_used(&upsRxFifo[ups]);
        fifo_s_gets(&upsRxFifo[ups], buff, length);
        return length;
    }
}
static int upsSend(uint8_t ups, _upsInquiryCmd_t cmd)
{
    // return spiMsgQPush((uint8_t *)commandMap[cmd].cmd, strlen(commandMap[cmd].cmd));
    return upsSendBuf(ups, (uint8_t *)commandMap[cmd].cmd, strlen(commandMap[cmd].cmd));
}

static osStatus_t upsCommand(uint8_t ups, _upsInquiryCmd_t cmd)
{
    uint8_t len;
    char    cache[320] = {0};
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
                if (*(p + commandMap[cmd].bklen) == commandMap[cmd].bkEnd) {
                    commandMap[cmd].process(ups, p, len);
                } else {
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
        rc = HAL_UART_Transmit_IT(&huart5, upsTxbuf[ups], _len);
    }

    if (rc == HAL_OK) {
        return (int)_len;
    } else {
        return -(int)rc;
    }
}

static osStatus_t Q1Pro(uint8_t ups, char *buff, uint8_t len)
{
    float    mmm, nnn, ppp, rrr, sss, ttt;
    int      qqq           = 0;
    char     upsStatus[20] = {0};
    uint8_t  i;
    uint16_t cache = 0;

    //"(220.1 220.2 220.3 090 50.1 1.20 25.5 00001111\r" 51 31 0D
    //"(MMM.M NNN.N PPP.P QQQ RR.R S.SS TT.T 76543210\r"
    sscanf(buff, "(%f %f %f %d %f %f %f %8s\r", &mmm, &nnn, &ppp, &qqq, &rrr, &sss, &ttt, upsStatus);

    cache                    = qqq;
    modbusVar[15 * ups + 70] = BEBufToUint16((uint8_t *)&cache);
    cache                    = 0;
    for (i = 0; i < 8; i++) {
        cache <<= 1;
        cache |= (upsStatus[i] == '1') ? 1 : 0;
    }
    modbusVar[15 * ups + 71] = BEBufToUint16((uint8_t *)&cache);
    cache                    = mmm * 100;
    modbusVar[15 * ups + 72] = BEBufToUint16((uint8_t *)&cache);
    cache                    = ppp * 100;
    modbusVar[15 * ups + 73] = BEBufToUint16((uint8_t *)&cache);
    cache                    = ttt * 10;
    modbusVar[15 * ups + 77] = BEBufToUint16((uint8_t *)&cache);

    return osOK;
}

static osStatus_t GOPPro(uint8_t ups, char *buff, uint8_t len)
{
    float    AAA, BBBB, CCCC;
    int      DDDD = 0, EEEE = 0, FFFF = 0;
    uint16_t cache;
    //"(220.0 50.00 010.0 02200 02200 020\r" 47 4f 50 0d
    //"(AAA.A BB.BB CCC.C DDDDD EEEEE FFF\r"
    sscanf(buff, "(%f %f %f %d %d %d\r", &AAA, &BBBB, &CCCC, &DDDD, &EEEE, &FFFF);
    cache                    = CCCC * 100;
    modbusVar[15 * ups + 74] = BEBufToUint16((uint8_t *)&cache);
    cache                    = BBBB * 10;
    modbusVar[15 * ups + 75] = BEBufToUint16((uint8_t *)&cache);
    return osOK;
}

static osStatus_t GBATPro(uint8_t ups, char *buff, uint8_t len)
{
    float    AAA, BBB, DDD, EEE;
    int      CC = 0;
    uint16_t cache;
    // (220.0 100.00 16 10.0 10.5\r 47 42 41 54 0D
    //"(AAA.A BBB.BB CC DD.D EE.E\r"
    sscanf(buff, "(%f %f %d %f %f\r", &AAA, &BBB, &CC, &DDD, &EEE);
    cache                    = AAA * 100;
    modbusVar[15 * ups + 76] = BEBufToUint16((uint8_t *)&cache);
    return osOK;
}

static osStatus_t BLPro(uint8_t ups, char *buff, uint8_t len)
{
    int      AAA = 0;
    uint16_t cache;
    //" BLAAA\r"   42 4C 0D
    sscanf(buff, "BL%d\r", &AAA);
    cache                    = AAA;
    modbusVar[15 * ups + 78] = BEBufToUint16((uint8_t *)&cache);
    return osOK;
}

static osStatus_t GMODPro(uint8_t ups, char *buff, uint8_t len)
{
    char     M;
    uint16_t cache;
    //"(M\r" 47 4D 4F 44 0D
    sscanf(buff, "(%c\r", &M);
    cache                    = M;
    modbusVar[15 * ups + 80] = BEBufToUint16((uint8_t *)&cache);
    return osOK;
}
