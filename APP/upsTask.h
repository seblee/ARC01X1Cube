/**
 * @file upsTask.h
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
#ifndef __UPSTASK_H_
#define __UPSTASK_H_

/* Private includes ----------------------------------------------------------*/
#include "cmsis_os2.h"  // CMSIS RTOS header file
/* Private typedef -----------------------------------------------------------*/
typedef struct {
    char   *cmd;
    char    bkHead;
    char    bkEnd;
    uint8_t bklen;
    osStatus_t (*process)(uint8_t, char *, uint8_t);
} cmd_ups_t;

typedef enum {
    UPS_Q1,
    UPS_GOP,
    UPS_GBAT,
    UPS_BL,
    UPS_GMOD,
} _upsInquiryCmd_t;
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
/* Public variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private user code ---------------------------------------------------------*/

#endif