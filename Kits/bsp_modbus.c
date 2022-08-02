/*
*********************************************************************************************************
*
*	模块名称 : MODSBUS通信程序 （主机）
*	文件名称 : modbus_host.c
*	版    本 : V1.4
*	说    明 : 无线通信程序。通信协议基于MODBUS
*	修改记录 :
*		版本号  日期        作者    说明
*       V1.4   2015-11-28 修改协议
*
*	Copyright (C), 2015-2016, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/
#include "bsp_modbus.h"
#include <string.h>
#include "bsp_user_lib.h"
#define TIMEOUT 100 /* 接收命令超时时间, 单位ms */
#define NUM     1   /* 循环发送次数 */

/* 保存每个从机的计数器值 */

MODBUS_T g_tModH;

//
static void MODH_AnalyzeApp(MODBUS_T *tmod);

static void   MODH_Read_01H(MODBUS_T *tmod);
static void   MODH_Read_02H(MODBUS_T *tmod);
static void   MODH_Read_03H(MODBUS_T *tmod);
static void   MODH_Read_04H(MODBUS_T *tmod);
static void   MODH_Read_05H(MODBUS_T *tmod);
static void   MODH_Read_06H(MODBUS_T *tmod);
static void   MODH_Read_10H(MODBUS_T *tmod);
static void   RS485_SendBuf(uint8_t *_buf, uint16_t _len);
static int8_t waitResponse(uint8_t ModCmd);

VAR_T g_tVar[2];

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
void MODBUS_InitVar(MODBUS_T *tmod, uint8_t ID, uint32_t _Baud, uint8_t _WorkMode)
{
    tmod->g_rtu_timeout = 0;
    tmod->RxCount       = 0;
    tmod->id      = ID;
    tmod->Baud          = _Baud;
    tmod->WorkMode      = _WorkMode; /* 接收数据帧不进行CRC校验 */
}

/*
*********************************************************************************************************
*	函 数 名: MODH_SendPacket
*	功能说明: 发送数据包 COM1口
*	形    参: _buf : 数据缓冲区
*			  _len : 数据长度
*	返 回 值: 无
*********************************************************************************************************
*/
void MODH_SendPacket(uint8_t *_buf, uint16_t _len)
{
    RS485_SendBuf(_buf, _len);
}

/*
*********************************************************************************************************
*	函 数 名: MODH_SendAckWithCRC
*	功能说明: 发送应答,自动加CRC.
*	形    参: 无。发送数据在 g_tModH.TxBuf[], [g_tModH.TxCount
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODH_SendAckWithCRC(void)
{
    uint16_t crc;
    crc                              = CRC16_Modbus(g_tModH.TxBuf, g_tModH.TxCount);
    g_tModH.TxBuf[g_tModH.TxCount++] = crc >> 8;
    g_tModH.TxBuf[g_tModH.TxCount++] = crc;
    MODH_SendPacket(g_tModH.TxBuf, g_tModH.TxCount);

#if 1 /* 此部分为了串口打印结果,实际运用中可不要 */
    // g_tPrint.Txlen = g_tModH.TxCount;
    // memcpy(g_tPrint.TxBuf, g_tModH.TxBuf, g_tModH.TxCount);
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
static void MODH_AnalyzeApp(MODBUS_T *tmod)
{
    switch (tmod->RxBuf[1]) /* 第2个字节 功能码 */
    {
        case 0x01: /* 读取线圈状态 */
            MODH_Read_01H(tmod);
            break;

        case 0x02: /* 读取输入状态 */
            MODH_Read_02H(tmod);
            break;

        case 0x03: /* 读取保持寄存器 在一个或多个保持寄存器中取得当前的二进制值 */
            MODH_Read_03H(tmod);
            break;

        case 0x04: /* 读取输入寄存器 */
            MODH_Read_04H(tmod);
            break;

        case 0x05: /* 强制单线圈 */
            MODH_Read_05H(tmod);
            break;

        case 0x06: /* 写单个寄存器 */
            MODH_Read_06H(tmod);
            break;

        case 0x10: /* 写多个寄存器 */
            MODH_Read_10H(tmod);
            break;

        default:
            g_tModH.RspCode = RSP_ERR_CMD;
            break;
    }
    osSignalSet(osThreadGetId(), g_tModH.RxBuf[1]);
}

/*
*********************************************************************************************************
*	函 数 名: MODH_Send01H
*	功能说明: 发送01H指令，查询1个或多个保持寄存器
*	形    参: _addr : 从站地址
*			  _reg : 寄存器编号
*			  _num : 寄存器个数
*	返 回 值: 无
*********************************************************************************************************
*/
void MODH_Send01H(uint8_t _addr, uint16_t _reg, uint16_t _num)
{
    g_tModH.TxCount                  = 0;
    g_tModH.TxBuf[g_tModH.TxCount++] = _addr;     /* 从站地址 */
    g_tModH.TxBuf[g_tModH.TxCount++] = 0x01;      /* 功能码 */
    g_tModH.TxBuf[g_tModH.TxCount++] = _reg >> 8; /* 寄存器编号 高字节 */
    g_tModH.TxBuf[g_tModH.TxCount++] = _reg;      /* 寄存器编号 低字节 */
    g_tModH.TxBuf[g_tModH.TxCount++] = _num >> 8; /* 寄存器个数 高字节 */
    g_tModH.TxBuf[g_tModH.TxCount++] = _num;      /* 寄存器个数 低字节 */

    MODH_SendAckWithCRC();  /* 发送数据，自动加CRC */
    g_tModH.fAck01H = 0;    /* 清接收标志 */
    g_tModH.RegNum  = _num; /* 寄存器个数 */
    g_tModH.Reg01H  = _reg; /* 保存03H指令中的寄存器地址，方便对应答数据进行分类 */
}

/*
*********************************************************************************************************
*	函 数 名: MODH_Send02H
*	功能说明: 发送02H指令，读离散输入寄存器
*	形    参: _addr : 从站地址
*			  _reg : 寄存器编号
*			  _num : 寄存器个数
*	返 回 值: 无
*********************************************************************************************************
*/
void MODH_Send02H(uint8_t _addr, uint16_t _reg, uint16_t _num)
{
    g_tModH.TxCount                  = 0;
    g_tModH.TxBuf[g_tModH.TxCount++] = _addr;     /* 从站地址 */
    g_tModH.TxBuf[g_tModH.TxCount++] = 0x02;      /* 功能码 */
    g_tModH.TxBuf[g_tModH.TxCount++] = _reg >> 8; /* 寄存器编号 高字节 */
    g_tModH.TxBuf[g_tModH.TxCount++] = _reg;      /* 寄存器编号 低字节 */
    g_tModH.TxBuf[g_tModH.TxCount++] = _num >> 8; /* 寄存器个数 高字节 */
    g_tModH.TxBuf[g_tModH.TxCount++] = _num;      /* 寄存器个数 低字节 */

    MODH_SendAckWithCRC();  /* 发送数据，自动加CRC */
    g_tModH.fAck02H = 0;    /* 清接收标志 */
    g_tModH.RegNum  = _num; /* 寄存器个数 */
    g_tModH.Reg02H  = _reg; /* 保存03H指令中的寄存器地址，方便对应答数据进行分类 */
}

/*
*********************************************************************************************************
*	函 数 名: MODH_Send03H
*	功能说明: 发送03H指令，查询1个或多个保持寄存器
*	形    参: _addr : 从站地址
*			  _reg : 寄存器编号
*			  _num : 寄存器个数
*	返 回 值: 无
*********************************************************************************************************
*/
void MODH_Send03H(uint8_t _addr, uint16_t _reg, uint16_t _num)
{
    g_tModH.TxCount                  = 0;
    g_tModH.TxBuf[g_tModH.TxCount++] = _addr;     /* 从站地址 */
    g_tModH.TxBuf[g_tModH.TxCount++] = 0x03;      /* 功能码 */
    g_tModH.TxBuf[g_tModH.TxCount++] = _reg >> 8; /* 寄存器编号 高字节 */
    g_tModH.TxBuf[g_tModH.TxCount++] = _reg;      /* 寄存器编号 低字节 */
    g_tModH.TxBuf[g_tModH.TxCount++] = _num >> 8; /* 寄存器个数 高字节 */
    g_tModH.TxBuf[g_tModH.TxCount++] = _num;      /* 寄存器个数 低字节 */

    MODH_SendAckWithCRC();  /* 发送数据，自动加CRC */
    g_tModH.fAck03H = 0;    /* 清接收标志 */
    g_tModH.RegNum  = _num; /* 寄存器个数 */
    g_tModH.Reg03H  = _reg; /* 保存03H指令中的寄存器地址，方便对应答数据进行分类 */
}

/*
*********************************************************************************************************
*	函 数 名: MODH_Send04H
*	功能说明: 发送04H指令，读输入寄存器
*	形    参: _addr : 从站地址
*			  _reg : 寄存器编号
*			  _num : 寄存器个数
*	返 回 值: 无
*********************************************************************************************************
*/
void MODH_Send04H(uint8_t _addr, uint16_t _reg, uint16_t _num)
{
    g_tModH.TxCount                  = 0;
    g_tModH.TxBuf[g_tModH.TxCount++] = _addr;     /* 从站地址 */
    g_tModH.TxBuf[g_tModH.TxCount++] = 0x04;      /* 功能码 */
    g_tModH.TxBuf[g_tModH.TxCount++] = _reg >> 8; /* 寄存器编号 高字节 */
    g_tModH.TxBuf[g_tModH.TxCount++] = _reg;      /* 寄存器编号 低字节 */
    g_tModH.TxBuf[g_tModH.TxCount++] = _num >> 8; /* 寄存器个数 高字节 */
    g_tModH.TxBuf[g_tModH.TxCount++] = _num;      /* 寄存器个数 低字节 */

    MODH_SendAckWithCRC();  /* 发送数据，自动加CRC */
    g_tModH.fAck04H = 0;    /* 清接收标志 */
    g_tModH.RegNum  = _num; /* 寄存器个数 */
    g_tModH.Reg04H  = _reg; /* 保存03H指令中的寄存器地址，方便对应答数据进行分类 */
}

/*
*********************************************************************************************************
*	函 数 名: MODH_Send05H
*	功能说明: 发送05H指令，写强置单线圈
*	形    参: _addr : 从站地址
*			  _reg : 寄存器编号
*			  _value : 寄存器值,2字节
*	返 回 值: 无
*********************************************************************************************************
*/
void MODH_Send05H(uint8_t _addr, uint16_t _reg, uint16_t _value)
{
    g_tModH.TxCount                  = 0;
    g_tModH.TxBuf[g_tModH.TxCount++] = _addr;     /* 从站地址 */
    g_tModH.TxBuf[g_tModH.TxCount++] = 0x05;      /* 功能码 */
    g_tModH.TxBuf[g_tModH.TxCount++] = _reg >> 8; /* 寄存器编号 高字节 */
    g_tModH.TxBuf[g_tModH.TxCount++] = _reg;      /* 寄存器编号 低字节 */
    //	g_tModH.TxBuf[g_tModH.TxCount++] = _value >> 8;		/* 寄存器值 高字节 */
    //	g_tModH.TxBuf[g_tModH.TxCount++] = _value;			/* 寄存器值 低字节 */
    if (_value == 0)
        g_tModH.TxBuf[g_tModH.TxCount++] = 0; /* 寄存器值 高字节 */
    else
        g_tModH.TxBuf[g_tModH.TxCount++] = 0xff; /* 寄存器值 高字节 */
    g_tModH.TxBuf[g_tModH.TxCount++] = 0;        /* 寄存器值 低字节 */

    MODH_SendAckWithCRC(); /* 发送数据，自动加CRC */

    g_tModH.fAck05H = 0; /* 如果收到从机的应答，则这个标志会设为1 */
}

/*
*********************************************************************************************************
*	函 数 名: MODH_Send06H
*	功能说明: 发送06H指令，写1个保持寄存器
*	形    参: _addr : 从站地址
*			  _reg : 寄存器编号
*			  _value : 寄存器值,2字节
*	返 回 值: 无
*********************************************************************************************************
*/
void MODH_Send06H(uint8_t _addr, uint16_t _reg, uint16_t _value)
{
    g_tModH.TxCount                  = 0;
    g_tModH.TxBuf[g_tModH.TxCount++] = _addr;       /* 从站地址 */
    g_tModH.TxBuf[g_tModH.TxCount++] = 0x06;        /* 功能码 */
    g_tModH.TxBuf[g_tModH.TxCount++] = _reg >> 8;   /* 寄存器编号 高字节 */
    g_tModH.TxBuf[g_tModH.TxCount++] = _reg;        /* 寄存器编号 低字节 */
    g_tModH.TxBuf[g_tModH.TxCount++] = _value >> 8; /* 寄存器值 高字节 */
    g_tModH.TxBuf[g_tModH.TxCount++] = _value;      /* 寄存器值 低字节 */

    MODH_SendAckWithCRC(); /* 发送数据，自动加CRC */

    g_tModH.fAck06H = 0; /* 如果收到从机的应答，则这个标志会设为1 */
}

/*
*********************************************************************************************************
*	函 数 名: MODH_Send10H
*	功能说明: 发送10H指令，连续写多个保持寄存器. 最多一次支持23个寄存器。
*	形    参: _addr : 从站地址
*			  _reg : 寄存器编号
*			  _num : 寄存器个数n (每个寄存器2个字节) 值域
*			  _buf : n个寄存器的数据。长度 = 2 * n
*	返 回 值: 无
*********************************************************************************************************
*/
void MODH_Send10H(uint8_t _addr, uint16_t _reg, uint8_t _num, uint8_t *_buf)
{
    uint16_t i;

    g_tModH.TxCount                  = 0;
    g_tModH.TxBuf[g_tModH.TxCount++] = _addr;     /* 从站地址 */
    g_tModH.TxBuf[g_tModH.TxCount++] = 0x10;      /* 从站地址 */
    g_tModH.TxBuf[g_tModH.TxCount++] = _reg >> 8; /* 寄存器编号 高字节 */
    g_tModH.TxBuf[g_tModH.TxCount++] = _reg;      /* 寄存器编号 低字节 */
    g_tModH.TxBuf[g_tModH.TxCount++] = _num >> 8; /* 寄存器个数 高字节 */
    g_tModH.TxBuf[g_tModH.TxCount++] = _num;      /* 寄存器个数 低字节 */
    g_tModH.TxBuf[g_tModH.TxCount++] = 2 * _num;  /* 数据字节数 */

    for (i = 0; i < 2 * _num; i++) {
        if (g_tModH.TxCount > H_RX_BUF_SIZE - 3) {
            return; /* 数据超过缓冲区超度，直接丢弃不发送 */
        }
        g_tModH.TxBuf[g_tModH.TxCount++] = _buf[i]; /* 后面的数据长度 */
    }

    MODH_SendAckWithCRC(); /* 发送数据，自动加CRC */
    g_tModH.fAck10H = 0;   /* 清接收标志 */
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

    // if (g_tModH.RxCount < H_RX_BUF_SIZE)
    // {
    //     g_tModH.RxBuf[g_tModH.RxCount++] = _data;
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
void MODBUS_RxTimeOut(MODBUS_T *tmod)
{
    tmod->g_rtu_timeout = 1;
    MODBUS_Poll(tmod);
}

/**
 * @brief
 * @param  tmodh:data handle
 */
void MODBUS_RxData(MODBUS_T *tmod, uint8_t *src, uint8_t len)
{
    if ((tmod->RxCount + len) > H_RX_BUF_SIZE)
        tmod->RxCount = 0;
    memcpy(tmod->RxBuf + tmod->RxCount, src, len);
    tmod->RxCount += len;
    tmod->g_rtu_timeout = 0;
    MODBUS_Poll(tmod);
}

/*
*********************************************************************************************************
*	函 数 名: MODBUS_Poll
*	功能说明: 接收控制器指令. 1ms 响应时间。
*	形    参: 无
*	返 回 值: 0 表示无数据 1表示收到正确命令
*********************************************************************************************************
*/
void MODBUS_Poll(MODBUS_T *tmod)
{
    uint16_t crc1;
    uint8_t  i = 0;
    // if (g_modh_timeout == 0) /* 超过3.5个字符时间后执行MODH_RxTimeOut()函数。全局变量 g_rtu_timeout = 1 */
    // {
    //     /* 没有超时，继续接收。不要清零 g_tModH.RxCount */
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
    if (tmod->WorkMode == WKM_MODBUS_HOST) {
        if (tmod->RxCount < 4) {
            goto err_ret;
        }
    } else if (tmod->WorkMode == WKM_MODBUS_DEVICE) {
        if (tmod->RxCount < 8) {
            goto err_ret;
        }
    }

    for (i = 0; i < tmod->RxCount; i++) {
        if (tmod->RxBuf[i] == tmod->id) {
            memmove(&tmod->RxBuf[0], &tmod->RxBuf[i], tmod->RxCount - i);
            tmod->RxCount -= i;
            break;
        }
    }
    if (tmod->WorkMode == WKM_MODBUS_HOST) {
        if (tmod->RxCount < 4) {
            goto err_ret;
        }
    } else if (tmod->WorkMode == WKM_MODBUS_DEVICE) {
        if (tmod->RxCount < 8) {
            goto err_ret;
        }
    }
    /* 计算CRC校验和 */
    crc1 = CRC16_Modbus(tmod->RxBuf, tmod->RxCount);
    if (crc1 != 0) {
        goto err_ret;
    }

    /* 分析应用层协议 */
    MODH_AnalyzeApp(tmod);

err_ret:
#if 1 /* 此部分为了串口打印结果,实际运用中可不要 */
      // g_tPrint.Rxlen = g_tModH.RxCount;
      // memcpy(g_tPrint.RxBuf, g_tModH.RxBuf, g_tModH.RxCount);
#endif

    g_tModH.RxCount = 0; /* 必须清零计数器，方便下次帧同步 */
}

/*
*********************************************************************************************************
*	函 数 名: MODH_Read_01H
*	功能说明: 分析01H指令的应答数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODH_Read_01H(MODBUS_T *tmod)
{
    uint8_t  bytes;
    uint8_t *p;

    if (tmod->RxCount > 0) {
        bytes = tmod->RxBuf[2]; /* 数据长度 字节数 */
        switch (tmod->Reg01H) {
            case REG_D01:
                if (bytes >= 1) {
                    p = &tmod->RxBuf[3];

                    g_tVar[tmod->RxBuf[0] - 1].D01 = BEBufToUint16(p);
                    p += 2; /* 寄存器 */
                    g_tVar[tmod->RxBuf[0] - 1].D02 = BEBufToUint16(p);
                    p += 2; /* 寄存器 */
                    g_tVar[tmod->RxBuf[0] - 1].D03 = BEBufToUint16(p);
                    p += 2; /* 寄存器 */
                    g_tVar[tmod->RxBuf[0] - 1].D04 = BEBufToUint16(p);
                    p += 2; /* 寄存器 */

                    tmod->fAck01H = 1;
                }
                break;
        }
    }
}

/*
*********************************************************************************************************
*	函 数 名: MODH_Read_02H
*	功能说明: 分析02H指令的应答数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODH_Read_02H(MODBUS_T *tmod)
{
    uint8_t  bytes;
    uint8_t *p;

    if (tmod->RxCount > 0) {
        bytes = tmod->RxBuf[2]; /* 数据长度 字节数 */
        switch (tmod->Reg02H) {
            case REG_T01:
                if (bytes == 6) {
                    p = &tmod->RxBuf[3];

                    g_tVar[tmod->RxBuf[0] - 1].T01 = BEBufToUint16(p);
                    p += 2; /* 寄存器 */
                    g_tVar[tmod->RxBuf[0] - 1].T02 = BEBufToUint16(p);
                    p += 2; /* 寄存器 */
                    g_tVar[tmod->RxBuf[0] - 1].T03 = BEBufToUint16(p);
                    p += 2; /* 寄存器 */

                    tmod->fAck02H = 1;
                }
                break;
        }
    }
}

/*
*********************************************************************************************************
*	函 数 名: MODH_Read_04H
*	功能说明: 分析04H指令的应答数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODH_Read_04H(MODBUS_T *tmod)
{
    uint8_t  bytes;
    uint8_t *p;

    if (tmod->RxCount > 0) {
        bytes = tmod->RxBuf[2]; /* 数据长度 字节数 */
        switch (tmod->Reg04H) {
            case REG_T01:
                if (bytes == 2) {
                    p = &tmod->RxBuf[3];

                    g_tVar[tmod->RxBuf[0] - 1].A01 = BEBufToUint16(p);
                    p += 2; /* 寄存器 */

                    tmod->fAck04H = 1;
                }
                break;
        }
    }
}

/*
*********************************************************************************************************
*	函 数 名: MODH_Read_05H
*	功能说明: 分析05H指令的应答数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODH_Read_05H(MODBUS_T *tmod)
{
    if (tmod->RxCount > 0) {
        if (tmod->RxBuf[0] == SlaveHMIAddr) {
            tmod->fAck05H = 1; /* 接收到应答 */
        }
    };
}

/*
*********************************************************************************************************
*	函 数 名: MODH_Read_06H
*	功能说明: 分析06H指令的应答数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODH_Read_06H(MODBUS_T *tmod)
{
    if (tmod->RxCount > 0) {
        if ((tmod->RxBuf[0] == SlaveHMIAddr) || (tmod->RxBuf[0] == SlaveBoardAddr)) {
            tmod->fAck06H = 1; /* 接收到应答 */
        }
    }
}

/*
*********************************************************************************************************
*	函 数 名: MODH_Read_03H
*	功能说明: 分析03H指令的应答数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void MODH_Read_03H(MODBUS_T *tmod)
{
    uint8_t *p, bytes, i;

    if (tmod->RxCount > 0) {
        bytes = tmod->RxBuf[2]; /* 数据长度 字节数 */
        p     = &tmod->RxBuf[3];
        for (i = 0; i < (bytes / 2); i++) {
            g_tVar[tmod->RxBuf[0] - 1].P01[i] = BEBufToUint16(p);
            p += 2; /* 寄存器 */
        }
        tmod->fAck03H = 1;
    }
}

// void MODH_Read_03H(MODBUS_T *tmod)
//{
//	uint8_t bytes;
//	uint8_t *p;
//
//	if (tmod->RxCount > 0)
//	{
//		bytes = tmod->RxBuf[2];	/* 数据长度 字节数 */
//		switch (tmod->Reg03H)
//		{
//			case REG_P01:
//				if (bytes == 32)
//				{
//					p = &tmod->RxBuf[3];
//
//					g_tVar.P01 = BEBufToUint16(p); p += 2;	/* 寄存器 */
//					g_tVar.P02 = BEBufToUint16(p); p += 2;	/* 寄存器 */
//					tmod->fAck03H = 1;
//				}
//				break;
//		}
//	}
//}

/*
*********************************************************************************************************
*	函 数 名: MODH_Read_10H
*	功能说明: 分析10H指令的应答数据
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void MODH_Read_10H(MODBUS_T *tmod)
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
    if (tmod->RxCount > 0) {
        if (tmod->RxBuf[0] == SlaveHMIAddr) {
            tmod->fAck10H = 1; /* 接收到应答 */
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
uint8_t MODH_ReadParam_01H(uint16_t _reg, uint16_t _num)
{
    // int32_t time1;
    uint8_t i;

    for (i = 0; i < NUM; i++) {
        MODH_Send01H(SlaveHMIAddr, _reg, _num); /* 发送命令 */

        /**
         * wait response
         * @brief
         */
        if (waitResponse(0x01) == 0) {
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

        //     if (g_tModH.fAck01H > 0)
        //     {
        //         break; /* 接收到应答 */
        //     }
        // }

        // if (g_tModH.fAck01H > 0)
        // {
        //     break; /* 循环NUM次，如果接收到命令则break循环 */
        // }
    }

    if (g_tModH.fAck01H == 0) {
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
uint8_t MODH_ReadParam_02H(uint16_t _reg, uint16_t _num)
{
    // int32_t time1;
    uint8_t i;

    for (i = 0; i < NUM; i++) {
        MODH_Send02H(SlaveHMIAddr, _reg, _num);
        /**
         * wait response
         * @brief
         */
        if (waitResponse(0x02) == 0) {
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

        //     if (g_tModH.fAck02H > 0)
        //     {
        //         break;
        //     }
        // }

        // if (g_tModH.fAck02H > 0)
        // {
        //     break;
        // }
    }

    if (g_tModH.fAck02H == 0) {
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
uint8_t MODH_ReadParam_03H(uint8_t _addr, uint16_t _reg, uint16_t _num)
{
    // int32_t time1;
    uint8_t i;

    for (i = 0; i < NUM; i++) {
        MODH_Send03H(_addr, _reg, _num);
        /**
         * wait response
         * @brief
         */
        if (waitResponse(0x03) == 0) {
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

        //     if (g_tModH.fAck03H > 0)
        //     {
        //         break;
        //     }
        // }

        // if (g_tModH.fAck03H > 0)
        // {
        //     break;
        // }
    }

    if (g_tModH.fAck03H == 0) {
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
uint8_t MODH_ReadParam_04H(uint16_t _reg, uint16_t _num)
{
    // int32_t time1;
    uint8_t i;

    for (i = 0; i < NUM; i++) {
        MODH_Send04H(SlaveHMIAddr, _reg, _num);
        /**
         * wait response
         * @brief
         */
        if (waitResponse(0x04) == 0) {
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

        //     if (g_tModH.fAck04H > 0)
        //     {
        //         break;
        //     }
        // }

        // if (g_tModH.fAck04H > 0)
        // {
        //     break;
        // }
    }

    if (g_tModH.fAck04H == 0) {
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
uint8_t MODH_WriteParam_05H(uint16_t _reg, uint16_t _value)
{
    // int32_t time1;
    uint8_t i;

    for (i = 0; i < NUM; i++) {
        MODH_Send05H(SlaveHMIAddr, _reg, _value);
        /**
         * wait response
         * @brief
         */
        if (waitResponse(0x05) == 0) {
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

        //     if (g_tModH.fAck05H > 0)
        //     {
        //         break;
        //     }
        // }

        // if (g_tModH.fAck05H > 0)
        // {
        //     break;
        // }
    }

    if (g_tModH.fAck05H == 0) {
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
uint8_t MODH_WriteParam_06H(uint8_t _addr, uint16_t _reg, uint16_t _value)
{
    // int32_t time1;
    uint8_t i;

    for (i = 0; i < NUM; i++) {
        MODH_Send06H(_addr, _reg, _value);
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

        //     if (g_tModH.fAck06H > 0)
        //     {
        //         break;
        //     }
        // }

        // if (g_tModH.fAck06H > 0)
        // {
        //     break;
        // }
    }

    if (g_tModH.fAck06H == 0) {
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
 * @param  _addr           slave address
 * @param  _reg            reg address
 * @param  _num            number of reg
 * @param  _buf            data buff for write
 * @return uint8_t
 */
uint8_t MODH_WriteParam_10H(uint8_t _addr, uint16_t _reg, uint8_t _num, uint8_t *_buf)
{
    // int32_t time1;
    uint8_t i;

    for (i = 0; i < NUM; i++) {
        MODH_Send10H(_addr, _reg, _num, _buf);
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

        //     if (g_tModH.fAck10H > 0)
        //     {
        //         break;
        //     }
        // }

        // if (g_tModH.fAck10H > 0)
        // {
        //     break;
        // }
    }

    if (g_tModH.fAck10H == 0) {
        return 0; /* 通信超时了 */
    } else {
        return 1; /* 写入10H参数成功 */
    }
}

/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/

static void RS485_SendBuf(uint8_t *_buf, uint16_t _len)
{
    // UART_DIR4_TX;  // 485_DIR4
    USART_DIR2_TX;  // 485_DIR2
    memcpy(tx2DMAbuffer, _buf, _len);
    HAL_UART_Transmit_DMA(&huart2, tx2DMAbuffer, _len);
}

static int8_t waitResponse(uint8_t ModCmd)
{
    osEvent tid_ThreadEvent;
    // osSignalClear(tid_testThread, ModCmd);
    tid_ThreadEvent = osSignalWait(ModCmd, 200);  // wait for message
    if (tid_ThreadEvent.status == osFlagsErrorTimeout) {
        return -1;
    } else {
        return 0;
    }
}
