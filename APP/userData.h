/**
 * @file userData.h
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

#ifndef __USERDATA_H
#define __USERDATA_H

/* Private includes ----------------------------------------------------------*/
#include "sys.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define MOD_BUF_SIZE 300
#define MOD_BUF_MAX  256
#define MOD_VAR_SIZE 256

#define USART1_DIR_TX HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET)
#define USART1_DIR_RX HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET)
/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
/* Public variables ----------------------------------------------------------*/
extern DMA_HandleTypeDef hdma_usart1_rx;

extern uint8_t  USART1_Rx_buf[MOD_BUF_SIZE];
extern uint8_t  USART1_Tx_buf[MOD_BUF_SIZE];
extern uint16_t modbusVar[MOD_VAR_SIZE];
/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

#endif
