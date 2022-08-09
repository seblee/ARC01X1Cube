/**
 * @file bsp_modbus.c
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
#include "bsp_modbus.h"
#include <string.h>
#include "bsp_user_lib.h"
#include "usart.h"
#define TIMEOUT 100 /* 接收命令超时时间, 单位ms */
#define NUM     1   /* 循环发送次数 */

/* 保存每个从机的计数器值 */

//
static void MODH_AnalyzeApp(MODBUS_T *_tmod);

static void   MODH_01Handle(MODBUS_T *_tmod);
static void   MODH_02Handle(MODBUS_T *_tmod);
static void   MODH_03Handle(MODBUS_T *_tmod);
static void   MODH_04Handle(MODBUS_T *_tmod);
static void   MODH_05Handle(MODBUS_T *_tmod);
static void   MODH_06Handle(MODBUS_T *_tmod);
static void   MODH_10Handle(MODBUS_T *_tmod);
static int8_t waitResponse(uint8_t ModCmd);

static void MODSendAckWithCRC(MODBUS_T *_tmod);

static void MODDevice_01H(MODBUS_T *_tmod);
static void MODDevice_02H(MODBUS_T *_tmod);
static void MODDevice_03H(MODBUS_T *_tmod);
static void MODDevice_04H(MODBUS_T *_tmod);
static void MODDevice_05H(MODBUS_T *_tmod);
static void MODDevice_06H(MODBUS_T *_tmod);
static void MODDevice_0FH(MODBUS_T *_tmod);
static void MODDevice_10H(MODBUS_T *_tmod);

/*
*********************************************************************************************************
*	函 数 名: MODBUS_InitVar
*	功能说明: 初始化Modbus结构变量
*	形    参: _Baud 通信波特率，改参数决定了RTU协议包间的超时时间。3.5个字符。
*			  _WorkMode 接收中断处理模式1. RXM_NO_CRC   RXM_MODBUS_HOST   RXM_MODBUS_DEVICE
*
*	返 回 值: 无
*********************************************************************************************************
*/
void MODBUS_InitVar(MODBUS_T *_tmod, uint8_t _id, uint32_t _Baud, uint8_t _WorkMode)
{
    _tmod->g_rtu_timeout = 0;
    _tmod->RxCount       = 0;
    _tmod->id            = _id;
    _tmod->Baud          = _Baud;
    _tmod->WorkMode      = _WorkMode; /* 接收数据帧不进行CRC校验 */

    _tmod->threadId = osThreadGetId();
}

/*
*********************************************************************************************************
*	函 数 名: MODSendAckWithCRC
*	功能说明: 发送应答,自动加CRC.
*	形    参: 无。发送数据在   _tmod->TxBuf[], [  _tmod->TxCount
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODSendAckWithCRC(MODBUS_T *_tmod)
{
    uint16_t crc;
    crc                            = CRC16_Modbus(_tmod->TxBuf, _tmod->TxCount);
    _tmod->TxBuf[_tmod->TxCount++] = crc >> 8;
    _tmod->TxBuf[_tmod->TxCount++] = crc;
    if (_tmod->transmit) {
        _tmod->transmit(_tmod->TxBuf, _tmod->TxCount);
    }

#if 1 /* 此部分为了串口打印结果,实际运用中可不要 */
    // g_tPrint.Txlen =   _tmod->TxCount;
    // memcpy(g_tPrint.TxBuf,   _tmod->TxBuf,   _tmod->TxCount);
#endif
}

/*
*********************************************************************************************************
*	函 数 名: MODH_AnalyzeApp
*	功能说明: 分析应用层协议。处理应答。
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODH_AnalyzeApp(MODBUS_T *_tmod)
{
    if (_tmod->WorkMode == WKM_MODBUS_HOST) {
        switch (_tmod->RxBuf[1]) /* 第2个字节 功能码 */
        {
            case MOD_01H: /* 读取线圈状态 */
                MODH_01Handle(_tmod);
                break;

            case MOD_02H: /* 读取输入状态 */
                MODH_02Handle(_tmod);
                break;

            case MOD_03H: /* 读取保持寄存器 在一个或多个保持寄存器中取得当前的二进制值 */
                MODH_03Handle(_tmod);
                break;

            case MOD_04H: /* 读取输入寄存器 */
                MODH_04Handle(_tmod);
                break;

            case MOD_05H: /* 强制单线圈 */
                MODH_05Handle(_tmod);
                break;

            case MOD_06H: /* 写单个寄存器 */
                MODH_06Handle(_tmod);
                break;

            case MOD_10H: /* 写多个寄存器 */
                MODH_10Handle(_tmod);
                break;

            default:
                _tmod->RspCode = RSP_ERR_CMD;
                break;
        }
        osThreadFlagsSet(_tmod->threadId, _tmod->RxBuf[1]);
    } else if (_tmod->WorkMode == WKM_MODBUS_DEVICE) {
        switch (_tmod->RxBuf[1]) /* 第2个字节 功能码 */
        {
            case MOD_01H:  //读线圈
                MODDevice_01H(_tmod);
                break;
            case MOD_02H:  //读离散输入
                MODDevice_02H(_tmod);
                break;
            case MOD_03H:  //读保持寄存器
                MODDevice_03H(_tmod);
                break;
            case MOD_04H:  //读输入寄存器
                MODDevice_04H(_tmod);
                break;
            case MOD_05H:  //写线圈
                MODDevice_05H(_tmod);
                break;
            case MOD_06H:  //写单个保持寄存器
                MODDevice_06H(_tmod);
                break;
            case MOD_0FH:  //写多个线圈
                MODDevice_0FH(_tmod);
                break;
            case MOD_10H:  //写多个保持寄存器
                MODDevice_10H(_tmod);
                break;
            default:
                break;
        }
    }
}

/*
*********************************************************************************************************
*	函 数 名: MODH_Send01H
*	功能说明: 发送01H指令，查询1个或多个保持寄存器
*	形    参: _id : 从站地址
*			  _reg : 寄存器编号
*			  _num : 寄存器个数
*	返 回 值: 无
*********************************************************************************************************
*/
void MODH_Send01H(MODBUS_T *_tmod, uint8_t _id, uint16_t _reg, uint16_t _num)
{
    _tmod->TxCount                 = 0;
    _tmod->TxBuf[_tmod->TxCount++] = _id;       /* 从站地址 */
    _tmod->TxBuf[_tmod->TxCount++] = 0x01;      /* 功能码 */
    _tmod->TxBuf[_tmod->TxCount++] = _reg >> 8; /* 寄存器编号 高字节 */
    _tmod->TxBuf[_tmod->TxCount++] = _reg;      /* 寄存器编号 低字节 */
    _tmod->TxBuf[_tmod->TxCount++] = _num >> 8; /* 寄存器个数 高字节 */
    _tmod->TxBuf[_tmod->TxCount++] = _num;      /* 寄存器个数 低字节 */

    MODSendAckWithCRC(_tmod); /* 发送数据，自动加CRC */
    _tmod->fAck01H = 0;       /* 清接收标志 */
    _tmod->RegNum  = _num;    /* 寄存器个数 */
    _tmod->Reg01H  = _reg;    /* 保存03H指令中的寄存器地址，方便对应答数据进行分类 */
}

/*
*********************************************************************************************************
*	函 数 名: MODH_Send02H
*	功能说明: 发送02H指令，读离散输入寄存器
*	形    参: _id : 从站地址
*			  _reg : 寄存器编号
*			  _num : 寄存器个数
*	返 回 值: 无
*********************************************************************************************************
*/
void MODH_Send02H(MODBUS_T *_tmod, uint8_t _id, uint16_t _reg, uint16_t _num)
{
    _tmod->TxCount                 = 0;
    _tmod->TxBuf[_tmod->TxCount++] = _id;       /* 从站地址 */
    _tmod->TxBuf[_tmod->TxCount++] = 0x02;      /* 功能码 */
    _tmod->TxBuf[_tmod->TxCount++] = _reg >> 8; /* 寄存器编号 高字节 */
    _tmod->TxBuf[_tmod->TxCount++] = _reg;      /* 寄存器编号 低字节 */
    _tmod->TxBuf[_tmod->TxCount++] = _num >> 8; /* 寄存器个数 高字节 */
    _tmod->TxBuf[_tmod->TxCount++] = _num;      /* 寄存器个数 低字节 */

    MODSendAckWithCRC(_tmod); /* 发送数据，自动加CRC */
    _tmod->fAck02H = 0;       /* 清接收标志 */
    _tmod->RegNum  = _num;    /* 寄存器个数 */
    _tmod->Reg02H  = _reg;    /* 保存03H指令中的寄存器地址，方便对应答数据进行分类 */
}

/*
*********************************************************************************************************
*	函 数 名: MODH_Send03H
*	功能说明: 发送03H指令，查询1个或多个保持寄存器
*	形    参: _id : 从站地址
*			  _reg : 寄存器编号
*			  _num : 寄存器个数
*	返 回 值: 无
*********************************************************************************************************
*/
void MODH_Send03H(MODBUS_T *_tmod, uint8_t _id, uint16_t _reg, uint16_t _num)
{
    _tmod->TxCount                 = 0;
    _tmod->TxBuf[_tmod->TxCount++] = _id;       /* 从站地址 */
    _tmod->TxBuf[_tmod->TxCount++] = 0x03;      /* 功能码 */
    _tmod->TxBuf[_tmod->TxCount++] = _reg >> 8; /* 寄存器编号 高字节 */
    _tmod->TxBuf[_tmod->TxCount++] = _reg;      /* 寄存器编号 低字节 */
    _tmod->TxBuf[_tmod->TxCount++] = _num >> 8; /* 寄存器个数 高字节 */
    _tmod->TxBuf[_tmod->TxCount++] = _num;      /* 寄存器个数 低字节 */

    MODSendAckWithCRC(_tmod); /* 发送数据，自动加CRC */
    _tmod->fAck03H = 0;       /* 清接收标志 */
    _tmod->RegNum  = _num;    /* 寄存器个数 */
    _tmod->Reg03H  = _reg;    /* 保存03H指令中的寄存器地址，方便对应答数据进行分类 */
}

/*
*********************************************************************************************************
*	函 数 名: MODH_Send04H
*	功能说明: 发送04H指令，读输入寄存器
*	形    参: _id : 从站地址
*			  _reg : 寄存器编号
*			  _num : 寄存器个数
*	返 回 值: 无
*********************************************************************************************************
*/
void MODH_Send04H(MODBUS_T *_tmod, uint8_t _id, uint16_t _reg, uint16_t _num)
{
    _tmod->TxCount                 = 0;
    _tmod->TxBuf[_tmod->TxCount++] = _id;       /* 从站地址 */
    _tmod->TxBuf[_tmod->TxCount++] = 0x04;      /* 功能码 */
    _tmod->TxBuf[_tmod->TxCount++] = _reg >> 8; /* 寄存器编号 高字节 */
    _tmod->TxBuf[_tmod->TxCount++] = _reg;      /* 寄存器编号 低字节 */
    _tmod->TxBuf[_tmod->TxCount++] = _num >> 8; /* 寄存器个数 高字节 */
    _tmod->TxBuf[_tmod->TxCount++] = _num;      /* 寄存器个数 低字节 */

    MODSendAckWithCRC(_tmod); /* 发送数据，自动加CRC */
    _tmod->fAck04H = 0;       /* 清接收标志 */
    _tmod->RegNum  = _num;    /* 寄存器个数 */
    _tmod->Reg04H  = _reg;    /* 保存03H指令中的寄存器地址，方便对应答数据进行分类 */
}

/*
*********************************************************************************************************
*	函 数 名: MODH_Send05H
*	功能说明: 发送05H指令，写强置单线圈
*	形    参: _id : 从站地址
*			  _reg : 寄存器编号
*			  _value : 寄存器值,2字节
*	返 回 值: 无
*********************************************************************************************************
*/
void MODH_Send05H(MODBUS_T *_tmod, uint8_t _id, uint16_t _reg, uint16_t _value)
{
    _tmod->TxCount                 = 0;
    _tmod->TxBuf[_tmod->TxCount++] = _id;       /* 从站地址 */
    _tmod->TxBuf[_tmod->TxCount++] = 0x05;      /* 功能码 */
    _tmod->TxBuf[_tmod->TxCount++] = _reg >> 8; /* 寄存器编号 高字节 */
    _tmod->TxBuf[_tmod->TxCount++] = _reg;      /* 寄存器编号 低字节 */
    //	  _tmod->TxBuf[  _tmod->TxCount++] = _value >> 8;		/* 寄存器值 高字节 */
    //	  _tmod->TxBuf[  _tmod->TxCount++] = _value;			/* 寄存器值 低字节 */
    if (_value == 0)
        _tmod->TxBuf[_tmod->TxCount++] = 0; /* 寄存器值 高字节 */
    else
        _tmod->TxBuf[_tmod->TxCount++] = 0xff; /* 寄存器值 高字节 */
    _tmod->TxBuf[_tmod->TxCount++] = 0;        /* 寄存器值 低字节 */

    MODSendAckWithCRC(_tmod); /* 发送数据，自动加CRC */

    _tmod->fAck05H = 0; /* 如果收到从机的应答，则这个标志会设为1 */
}

/*
*********************************************************************************************************
*	函 数 名: MODH_Send06H
*	功能说明: 发送06H指令，写1个保持寄存器
*	形    参: _id : 从站地址
*			  _reg : 寄存器编号
*			  _value : 寄存器值,2字节
*	返 回 值: 无
*********************************************************************************************************
*/
void MODH_Send06H(MODBUS_T *_tmod, uint8_t _id, uint16_t _reg, uint16_t _value)
{
    _tmod->TxCount                 = 0;
    _tmod->TxBuf[_tmod->TxCount++] = _id;         /* 从站地址 */
    _tmod->TxBuf[_tmod->TxCount++] = 0x06;        /* 功能码 */
    _tmod->TxBuf[_tmod->TxCount++] = _reg >> 8;   /* 寄存器编号 高字节 */
    _tmod->TxBuf[_tmod->TxCount++] = _reg;        /* 寄存器编号 低字节 */
    _tmod->TxBuf[_tmod->TxCount++] = _value >> 8; /* 寄存器值 高字节 */
    _tmod->TxBuf[_tmod->TxCount++] = _value;      /* 寄存器值 低字节 */

    MODSendAckWithCRC(_tmod); /* 发送数据，自动加CRC */

    _tmod->fAck06H = 0; /* 如果收到从机的应答，则这个标志会设为1 */
}

/*
*********************************************************************************************************
*	函 数 名: MODH_Send10H
*	功能说明: 发送10H指令，连续写多个保持寄存器. 最多一次支持23个寄存器。
*	形    参: _id : 从站地址
*			  _reg : 寄存器编号
*			  _num : 寄存器个数n (每个寄存器2个字节) 值域
*			  _buf : n个寄存器的数据。长度 = 2 * n
*	返 回 值: 无
*********************************************************************************************************
*/
void MODH_Send10H(MODBUS_T *_tmod, uint8_t _id, uint16_t _reg, uint8_t _num, uint8_t *_buf)
{
    uint16_t i;

    _tmod->TxCount                 = 0;
    _tmod->TxBuf[_tmod->TxCount++] = _id;       /* 从站地址 */
    _tmod->TxBuf[_tmod->TxCount++] = 0x10;      /* 从站地址 */
    _tmod->TxBuf[_tmod->TxCount++] = _reg >> 8; /* 寄存器编号 高字节 */
    _tmod->TxBuf[_tmod->TxCount++] = _reg;      /* 寄存器编号 低字节 */
    _tmod->TxBuf[_tmod->TxCount++] = _num >> 8; /* 寄存器个数 高字节 */
    _tmod->TxBuf[_tmod->TxCount++] = _num;      /* 寄存器个数 低字节 */
    _tmod->TxBuf[_tmod->TxCount++] = 2 * _num;  /* 数据字节数 */

    for (i = 0; i < 2 * _num; i++) {
        if (_tmod->TxCount > MOD_BUF_MAX - 3) {
            return; /* 数据超过缓冲区超度，直接丢弃不发送 */
        }
        _tmod->TxBuf[_tmod->TxCount++] = _buf[i]; /* 后面的数据长度 */
    }

    MODSendAckWithCRC(_tmod); /* 发送数据，自动加CRC */
    _tmod->fAck10H = 0;       /* 清接收标志 */
}

/*
*********************************************************************************************************
*	函 数 名: MODH_ReciveNew
*	功能说明: 串口接收中断服务程序会调用本函数。当收到一个字节时，执行一次本函数。
*	形    参:
*	返 回 值: 1 表示有数据
*********************************************************************************************************
*/
void MODH_ReciveNew(uint8_t _data)
{
    /*
        3.5个字符的时间间隔，只是用在RTU模式下面，因为RTU模式没有开始符和结束符，
        两个数据包之间只能靠时间间隔来区分，Modbus定义在不同的波特率下，间隔时间是不一样的，
        所以就是3.5个字符的时间，波特率高，这个时间间隔就小，波特率低，这个时间间隔相应就大

        4800  = 7.297ms
        9600  = 3.646ms
        19200  = 1.771ms
        38400  = 0.885ms
    */
    // uint32_t timeout;

    // g_modh_timeout = 0;

    // timeout = 35000000 / HBAUD485; /* 计算超时时间，单位us 35000000*/

    // /* 硬件定时中断，定时精度us 硬件定时器2用于MODBUS从机, 定时器3用于MODBUS从机主机*/
    // bsp_StartHardTimer(3, timeout, (void *)MODBUS_RxTimeOut);

    // if (  _tmod->RxCount < H_RX_BUF_SIZE)
    // {
    //       _tmod->RxBuf[  _tmod->RxCount++] = _data;
    // }
}

/*
*********************************************************************************************************
*	函 数 名: MODBUS_RxTimeOut
*	功能说明: 超过3.5个字符时间后执行本函数。 设置全局变量 g_rtu_timeout = 1; 通知主程序开始解码。
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void MODBUS_RxTimeOut(MODBUS_T *_tmod)
{
    _tmod->g_rtu_timeout = 1;
    MODBUS_Poll(_tmod);
}

/**
 * @brief
 * @param  tmodh:data handle
 */
void MODBUS_RxData(MODBUS_T *_tmod, uint8_t *src, uint8_t len)
{
    //    if ((_tmod->RxCount + len) > MOD_BUF_SIZE)
    //        _tmod->RxCount = 0;
    // memcpy(_tmod->RxBuf + _tmod->RxCount, src, len);
    _tmod->RxCount       = len;
    _tmod->g_rtu_timeout = 0;
    MODBUS_Poll(_tmod);
}

/*
*********************************************************************************************************
*	函 数 名: MODBUS_Poll
*	功能说明: 接收控制器指令. 1ms 响应时间。
*	形    参: 无
*	返 回 值: 0 表示无数据 1表示收到正确命令
*********************************************************************************************************
*/
void MODBUS_Poll(MODBUS_T *_tmod)
{
    uint16_t crc1;
    uint8_t  i = 0;
    // if (g_modh_timeout == 0) /* 超过3.5个字符时间后执行MODH_RxTimeOut()函数。全局变量 g_rtu_timeout = 1 */
    // {
    //     /* 没有超时，继续接收。不要清零   _tmod->RxCount */
    //     return;
    // }

    /* 收到命令
        05 06 00 88 04 57 3B70 (8 字节)
            05    :  数码管屏的号站，
            06    :  指令
            00 88 :  数码管屏的显示寄存器
            04 57 :  数据,,,转换成 10 进制是 1111.高位在前,
            3B70  :  二个字节 CRC 码	从05到 57的校验
    */
    if (_tmod->RxCount < 4) {
        goto err_ret;
    }

    for (i = 0; i < _tmod->RxCount; i++) {
        if (_tmod->RxBuf[i] == _tmod->id) {
            memmove(&_tmod->RxBuf[0], &_tmod->RxBuf[i], _tmod->RxCount - i);
            _tmod->RxCount -= i;
            break;
        }
    }
    if (_tmod->RxCount < 4) {
        goto err_ret;
    }
    /* 计算CRC校验和 */
    crc1 = CRC16_Modbus(_tmod->RxBuf, _tmod->RxCount);
    if (crc1 != 0) {
        goto err_ret;
    }

    /* 分析应用层协议 */
    MODH_AnalyzeApp(_tmod);

err_ret:
#if 1 /* 此部分为了串口打印结果,实际运用中可不要 */
      // g_tPrint.Rxlen =   _tmod->RxCount;
      // memcpy(g_tPrint.RxBuf,   _tmod->RxBuf,   _tmod->RxCount);
#endif

    _tmod->RxCount = 0; /* 必须清零计数器，方便下次帧同步 */
}

/*
*********************************************************************************************************
*	函 数 名: MODH_01Handle
*	功能说明: 分析01H指令的应答数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODH_01Handle(MODBUS_T *_tmod)
{
    if (_tmod->RxCount > 0) {
        _tmod->fAck01H = 1;
    }
}

/*
*********************************************************************************************************
*	函 数 名: MODH_02Handle
*	功能说明: 分析02H指令的应答数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODH_02Handle(MODBUS_T *_tmod)
{
    if (_tmod->RxCount > 0) {
        _tmod->fAck02H = 1;
    }
}

/*
*********************************************************************************************************
*	函 数 名: MODH_04Handle
*	功能说明: 分析04H指令的应答数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODH_04Handle(MODBUS_T *_tmod)
{
    if (_tmod->RxCount > 0) {
        _tmod->fAck04H = 1;
    }
}

/*
*********************************************************************************************************
*	函 数 名: MODH_05Handle
*	功能说明: 分析05H指令的应答数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODH_05Handle(MODBUS_T *_tmod)
{
    if (_tmod->RxCount > 0) {
        if (_tmod->RxBuf[0] == _tmod->id) {
            _tmod->fAck05H = 1; /* 接收到应答 */
        }
    };
}

/*
*********************************************************************************************************
*	函 数 名: MODH_06Handle
*	功能说明: 分析06H指令的应答数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODH_06Handle(MODBUS_T *_tmod)
{
    if (_tmod->RxCount > 0) {
        if (_tmod->RxBuf[0] == _tmod->id) {
            _tmod->fAck06H = 1; /* 接收到应答 */
        }
    }
}

/*
*********************************************************************************************************
*	函 数 名: MODH_03Handle
*	功能说明: 分析03H指令的应答数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void MODH_03Handle(MODBUS_T *_tmod)
{
    if (_tmod->RxCount > 0) {
        _tmod->fAck03H = 1;
        // memcpy(&modbusVar[_tmod->Reg03H], &_tmod->RxBuf[3], _tmod->RxBuf[2]);
    }
}

// void MODH_03Handle(MODBUS_T *_tmod)
//{
//	uint8_t bytes;
//	uint8_t *p;
//
//	if (_tmod->RxCount > 0)
//	{
//		bytes = _tmod->RxBuf[2];	/* 数据长度 字节数 */
//		switch (_tmod->Reg03H)
//		{
//			case REG_P01:
//				if (bytes == 32)
//				{
//					p = &_tmod->RxBuf[3];
//
//					g_tVar.P01 = BEBufToUint16(p); p += 2;	/* 寄存器 */
//					g_tVar.P02 = BEBufToUint16(p); p += 2;	/* 寄存器 */
//					_tmod->fAck03H = 1;
//				}
//				break;
//		}
//	}
//}

/*
*********************************************************************************************************
*	函 数 名: MODH_10Handle
*	功能说明: 分析10H指令的应答数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void MODH_10Handle(MODBUS_T *_tmod)
{
    /*
        10H指令的应答:
            从机地址                11
            功能码                  10
            寄存器起始地址高字节	00
            寄存器起始地址低字节    01
            寄存器数量高字节        00
            寄存器数量低字节        02
            CRC校验高字节           12
            CRC校验低字节           98
    */
    if (_tmod->RxCount > 0) {
        if (_tmod->RxBuf[0] == _tmod->id) {
            _tmod->fAck10H = 1; /* 接收到应答 */
        }
    }
}

/*
*********************************************************************************************************
*	函 数 名: MODH_ReadParam_01H
*	功能说明: 单个参数. 通过发送01H指令实现，发送之后，等待从机应答。
*	形    参: 无
*	返 回 值: 1 表示成功。0 表示失败（通信超时或被拒绝）
*********************************************************************************************************
*/
uint8_t MODH_ReadParam_01H(MODBUS_T *_tmod, uint8_t _id, uint16_t _reg, uint16_t _num)
{
    // int32_t time1;
    uint8_t i;
    _tmod->id = _id;
    for (i = 0; i < NUM; i++) {
        MODH_Send01H(_tmod, _id, _reg, _num); /* 发送命令 */

        /**
         * wait response
         * @brief
         */
        if (waitResponse(MOD_01H) == 0) {
            break;
        }

        // time1 = bsp_GetRunTime(); /* 记录命令发送的时刻 */
        // while (1)                 /* 等待应答,超时或接收到应答则break  */
        // {
        //     bsp_Idle();

        //     if (bsp_CheckRunTime(time1) > TIMEOUT)
        //     {
        //         break; /* 通信超时了 */
        //     }

        //     if (  _tmod->fAck01H > 0)
        //     {
        //         break; /* 接收到应答 */
        //     }
        // }

        // if (  _tmod->fAck01H > 0)
        // {
        //     break; /* 循环NUM次，如果接收到命令则break循环 */
        // }
    }

    if (_tmod->fAck01H == 0) {
        return 0;
    } else {
        return 1; /* 01H 读成功 */
    }
}

/*
*********************************************************************************************************
*	函 数 名: MODH_ReadParam_02H
*	功能说明: 单个参数. 通过发送02H指令实现，发送之后，等待从机应答。
*	形    参: 无
*	返 回 值: 1 表示成功。0 表示失败（通信超时或被拒绝）
*********************************************************************************************************
*/
uint8_t MODH_ReadParam_02H(MODBUS_T *_tmod, uint8_t _id, uint16_t _reg, uint16_t _num)
{
    // int32_t time1;
    uint8_t i;
    _tmod->id = _id;
    for (i = 0; i < NUM; i++) {
        MODH_Send02H(_tmod, _id, _reg, _num);
        /**
         * wait response
         * @brief
         */
        if (waitResponse(MOD_02H) == 0) {
            break;
        }
        // time1 = bsp_GetRunTime(); /* 记录命令发送的时刻 */

        // while (1)
        // {
        //     bsp_Idle();

        //     if (bsp_CheckRunTime(time1) > TIMEOUT)
        //     {
        //         break; /* 通信超时了 */
        //     }

        //     if (  _tmod->fAck02H > 0)
        //     {
        //         break;
        //     }
        // }

        // if (  _tmod->fAck02H > 0)
        // {
        //     break;
        // }
    }

    if (_tmod->fAck02H == 0) {
        return 0;
    } else {
        return 1; /* 02H 读成功 */
    }
}
/*
*********************************************************************************************************
*	函 数 名: MODH_ReadParam_03H
*	功能说明: 单个参数. 通过发送03H指令实现，发送之后，等待从机应答。
*	形    参: 无
*	返 回 值: 1 表示成功。0 表示失败（通信超时或被拒绝）
*********************************************************************************************************
*/
uint8_t MODH_ReadParam_03H(MODBUS_T *_tmod, uint8_t _id, uint16_t _reg, uint16_t _num)
{
    // int32_t time1;
    uint8_t i;
    _tmod->id = _id;
    for (i = 0; i < NUM; i++) {
        MODH_Send03H(_tmod, _id, _reg, _num);
        /**
         * wait response
         * @brief
         */
        if (waitResponse(MOD_03H) == 0) {
            break;
        }
        // time1 = bsp_GetRunTime(); /* 记录命令发送的时刻 */

        // while (1)
        // {
        //     bsp_Idle();

        //     if (bsp_CheckRunTime(time1) > TIMEOUT)
        //     {
        //         break; /* 通信超时了 */
        //     }

        //     if ( _tmod->fAck03H > 0)
        //     {
        //         break;
        //     }
        // }

        // if ( _tmod->fAck03H > 0)
        // {
        //     break;
        // }
    }

    if (_tmod->fAck03H == 0) {
        return 0; /* 通信超时了 */
    } else {
        return 1; /* 写入03H参数成功 */
    }
}

/*
*********************************************************************************************************
*	函 数 名: MODH_ReadParam_04H
*	功能说明: 单个参数. 通过发送04H指令实现，发送之后，等待从机应答。
*	形    参: 无
*	返 回 值: 1 表示成功。0 表示失败（通信超时或被拒绝）
*********************************************************************************************************
*/
uint8_t MODH_ReadParam_04H(MODBUS_T *_tmod, uint8_t _id, uint16_t _reg, uint16_t _num)
{
    // int32_t time1;
    uint8_t i;

    _tmod->id = _id;
    for (i = 0; i < NUM; i++) {
        MODH_Send04H(_tmod, _id, _reg, _num);
        /**
         * wait response
         * @brief
         */
        if (waitResponse(MOD_04H) == 0) {
            break;
        }
        // time1 = bsp_GetRunTime(); /* 记录命令发送的时刻 */

        // while (1)
        // {
        //     bsp_Idle();

        //     if (bsp_CheckRunTime(time1) > TIMEOUT)
        //     {
        //         break; /* 通信超时了 */
        //     }

        //     if (_tmod->fAck04H > 0)
        //     {
        //         break;
        //     }
        // }

        // if (_tmod->fAck04H > 0)
        // {
        //     break;
        // }
    }

    if (_tmod->fAck04H == 0) {
        return 0; /* 通信超时了 */
    } else {
        return 1; /* 04H 读成功 */
    }
}
/*
*********************************************************************************************************
*	函 数 名: MODH_WriteParam_05H
*	功能说明: 单个参数. 通过发送05H指令实现，发送之后，等待从机应答。
*	形    参: 无
*	返 回 值: 1 表示成功。0 表示失败（通信超时或被拒绝）
*********************************************************************************************************
*/
uint8_t MODH_WriteParam_05H(MODBUS_T *_tmod, uint8_t _id, uint16_t _reg, uint16_t _value)
{
    // int32_t time1;
    uint8_t i;

    _tmod->id = _id;
    for (i = 0; i < NUM; i++) {
        MODH_Send05H(_tmod, _id, _reg, _value);
        /**
         * wait response
         * @brief
         */
        if (waitResponse(MOD_05H) == 0) {
            break;
        }
        // time1 = bsp_GetRunTime(); /* 记录命令发送的时刻 */

        // while (1)
        // {
        //     bsp_Idle();

        //     /* 超时大于 TIMEOUT，则认为异常 */
        //     if (bsp_CheckRunTime(time1) > TIMEOUT)
        //     {
        //         break; /* 通信超时了 */
        //     }

        //     if (_tmod->fAck05H > 0)
        //     {
        //         break;
        //     }
        // }

        // if (_tmod->fAck05H > 0)
        // {
        //     break;
        // }
    }

    if (_tmod->fAck05H == 0) {
        return 0; /* 通信超时了 */
    } else {
        return 1; /* 05H 写成功 */
    }
}

/*
*********************************************************************************************************
*	函 数 名: MODH_WriteParam_06H
*	功能说明: 单个参数. 通过发送06H指令实现，发送之后，等待从机应答。循环NUM次写命令
*	形    参: 无
*	返 回 值: 1 表示成功。0 表示失败（通信超时或被拒绝）
*********************************************************************************************************
*/
uint8_t MODH_WriteParam_06H(MODBUS_T *_tmod, uint8_t _id, uint16_t _reg, uint16_t _value)
{
    // int32_t time1;
    uint8_t i;
    _tmod->id = _id;
    for (i = 0; i < NUM; i++) {
        MODH_Send06H(_tmod, _id, _reg, _value);
        /**
         * wait response
         * @brief
         */
        if (waitResponse(0x06) == 0) {
            break;
        }
        // time1 = bsp_GetRunTime(); /* 记录命令发送的时刻 */

        // while (1)
        // {
        //     bsp_Idle();

        //     if (bsp_CheckRunTime(time1) > TIMEOUT)
        //     {
        //         break;
        //     }

        //     if (  _tmod->fAck06H > 0)
        //     {
        //         break;
        //     }
        // }

        // if (  _tmod->fAck06H > 0)
        // {
        //     break;
        // }
    }

    if (_tmod->fAck06H == 0) {
        return 0; /* 通信超时了 */
    } else {
        return 1; /* 写入06H参数成功 */
    }
}

/*
*********************************************************************************************************
*	函 数 名: MODH_WriteParam_10H
*	功能说明: 单个参数. 通过发送10H指令实现，发送之后，等待从机应答。循环NUM次写命令
*	形    参: 无
*	返 回 值: 1 表示成功。0 表示失败（通信超时或被拒绝）
*********************************************************************************************************
*/

/**
 * @brief   MODH_WriteParam_10H
 * @param  _id           slave address
 * @param  _reg            reg address
 * @param  _num            number of reg
 * @param  _buf            data buff for write
 * @return uint8_t
 */
uint8_t MODH_WriteParam_10H(MODBUS_T *_tmod, uint8_t _id, uint16_t _reg, uint8_t _num, uint8_t *_buf)
{
    // int32_t time1;
    uint8_t i;
    _tmod->id = _id;

    for (i = 0; i < NUM; i++) {
        MODH_Send10H(_tmod, _id, _reg, _num, _buf);
        /**
         * wait response
         * @brief
         */
        if (waitResponse(0x10) == 0) {
            break;
        }
        // time1 = bsp_GetRunTime(); /* 记录命令发送的时刻 */

        // while (1)
        // {
        //     bsp_Idle();

        //     if (bsp_CheckRunTime(time1) > TIMEOUT)
        //     {
        //         break;
        //     }

        //     if (  _tmod->fAck10H > 0)
        //     {
        //         break;
        //     }
        // }

        // if (  _tmod->fAck10H > 0)
        // {
        //     break;
        // }
    }

    if (_tmod->fAck10H == 0) {
        return 0; /* 通信超时了 */
    } else {
        return 1; /* 写入10H参数成功 */
    }
}

/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
static int8_t waitResponse(uint8_t ModCmd)
{
    uint32_t Event;
    // osSignalClear(tid_testThread, ModCmd);
    Event = osThreadFlagsWait(ModCmd, osFlagsWaitAny, 200);  // wait for message
    if (Event == osFlagsErrorTimeout) {
        return -1;
    } else {
        return 0;
    }
}

static void MODDevice_01H(MODBUS_T *_tmod)
{
    _tmod->TxCount                 = 0;
    _tmod->TxBuf[_tmod->TxCount++] = _tmod->RxBuf[0];
    _tmod->TxBuf[_tmod->TxCount++] = 0x81;
    _tmod->TxBuf[_tmod->TxCount++] = 0x01;
    MODSendAckWithCRC(_tmod); /* 发送数据，自动加CRC */
}
static void MODDevice_02H(MODBUS_T *_tmod)
{
    _tmod->TxCount                 = 0;
    _tmod->TxBuf[_tmod->TxCount++] = _tmod->RxBuf[0];
    _tmod->TxBuf[_tmod->TxCount++] = 0x82;
    _tmod->TxBuf[_tmod->TxCount++] = 0x01;
    MODSendAckWithCRC(_tmod); /* 发送数据，自动加CRC */
}
static void MODDevice_03H(MODBUS_T *_tmod)
{
    _tmod->Reg03H  = (_tmod->RxBuf[2] << 8) | _tmod->RxBuf[3]; /* 保存03H指令中的寄存器地址，方便对应答数据进行分类 */
    _tmod->RegNum  = (_tmod->RxBuf[4] << 8) | _tmod->RxBuf[5]; /* 寄存器个数 */
    _tmod->TxCount = 0;
    if ((_tmod->Reg03H + _tmod->RegNum) > MOD_VAR_SIZE) {
        _tmod->TxBuf[_tmod->TxCount++] = _tmod->RxBuf[0];
        _tmod->TxBuf[_tmod->TxCount++] = 0x83;
        _tmod->TxBuf[_tmod->TxCount++] = 0x02;
    } else {
        _tmod->TxBuf[_tmod->TxCount++] = _tmod->RxBuf[0];
        _tmod->TxBuf[_tmod->TxCount++] = 0x03;
        _tmod->TxBuf[_tmod->TxCount++] = _tmod->RegNum * 2;
        memcpy(&_tmod->TxBuf[_tmod->TxCount], &modbusVar[_tmod->Reg03H], _tmod->TxBuf[2]);
        _tmod->TxCount += _tmod->TxBuf[2];
    }
    MODSendAckWithCRC(_tmod); /* 发送数据，自动加CRC */
}
static void MODDevice_04H(MODBUS_T *_tmod)
{
    _tmod->TxCount                 = 0;
    _tmod->TxBuf[_tmod->TxCount++] = _tmod->RxBuf[0];
    _tmod->TxBuf[_tmod->TxCount++] = 0x84;
    _tmod->TxBuf[_tmod->TxCount++] = 0x01;
    MODSendAckWithCRC(_tmod); /* 发送数据，自动加CRC */
}
static void MODDevice_05H(MODBUS_T *_tmod)
{
    _tmod->TxCount                 = 0;
    _tmod->TxBuf[_tmod->TxCount++] = _tmod->RxBuf[0];
    _tmod->TxBuf[_tmod->TxCount++] = 0x85;
    _tmod->TxBuf[_tmod->TxCount++] = 0x01;
    MODSendAckWithCRC(_tmod); /* 发送数据，自动加CRC */
}
static void MODDevice_06H(MODBUS_T *_tmod)
{
    _tmod->TxCount                 = 0;
    _tmod->TxBuf[_tmod->TxCount++] = _tmod->RxBuf[0];
    _tmod->TxBuf[_tmod->TxCount++] = 0x86;
    _tmod->TxBuf[_tmod->TxCount++] = 0x01;
    MODSendAckWithCRC(_tmod); /* 发送数据，自动加CRC */
}
static void MODDevice_0FH(MODBUS_T *_tmod)
{
    _tmod->TxCount                 = 0;
    _tmod->TxBuf[_tmod->TxCount++] = _tmod->RxBuf[0];
    _tmod->TxBuf[_tmod->TxCount++] = 0x8F;
    _tmod->TxBuf[_tmod->TxCount++] = 0x01;
    MODSendAckWithCRC(_tmod); /* 发送数据，自动加CRC */
}
static void MODDevice_10H(MODBUS_T *_tmod)
{
    _tmod->TxCount                 = 0;
    _tmod->TxBuf[_tmod->TxCount++] = _tmod->RxBuf[0];
    _tmod->TxBuf[_tmod->TxCount++] = 0x90;
    _tmod->TxBuf[_tmod->TxCount++] = 0x01;
    MODSendAckWithCRC(_tmod); /* 发送数据，自动加CRC */
}
