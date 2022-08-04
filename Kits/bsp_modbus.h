/*
*********************************************************************************************************
*
*	模块名称 : MODEBUS 通信模块 (主机程序）
*	文件名称 : modbus_host.h
*	版    本 : V1.4
*	说    明 : 头文件
*
*	Copyright (C), 2015-2016, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/
#ifndef __MOSBUS_HOST_H
#define __MOSBUS_HOST_H

#include "userData.h"

/* 01H 读强制单线圈 */
/* 05H 写强制单线圈 */
#define REG_D01 0x0101
#define REG_D02 0x0102
#define REG_D03 0x0103
#define REG_D04 0x0104
#define REG_DXX REG_D04

/* 02H 读取输入状态 */
#define REG_T01 0x0201
#define REG_T02 0x0202
#define REG_T03 0x0203
#define REG_TXX REG_T03

/* 03H 读保持寄存器 */
/* 06H 写保持寄存器 */
/* 10H 写多个保存寄存器 */
#define REG_P01 0x0301
#define REG_P02 0x0302

/* 04H 读取输入寄存器(模拟信号) */
#define REG_A01 0x0401
#define REG_AXX REG_A01

/* 接收中断程序的工作模式 -- ModbusInitVar() 函数的形参 */
#define WKM_NO_CRC        0 /* 不校验CRC和数据长度。比如ASCII协议。超时后直接给应用层处理 */
#define WKM_MODBUS_HOST   1 /* 校验CRC,不校验地址。 供Modbus主机应用 */
#define WKM_MODBUS_DEVICE 2 /* 校验CRC，并校验本机地址。 供Modbus设备（从机）应用 */

/* RTU 应答代码 */
#define RSP_OK           0    /* 成功 */
#define RSP_ERR_CMD      0x01 /* 不支持的功能码 */
#define RSP_ERR_REG_ADDR 0x02 /* 寄存器地址错误 */
#define RSP_ERR_VALUE    0x03 /* 数据值域错误 */
#define RSP_ERR_WRITE    0x04 /* 写入失败 */

#define MOD_01H 0x01  //读线圈
#define MOD_02H 0x02  //读离散输入
#define MOD_03H 0x03  //读保持寄存器
#define MOD_04H 0x04  //读输入寄存器
#define MOD_05H 0x05  //写线圈
#define MOD_06H 0x06  //写单个保持寄存器
#define MOD_0FH 0x0F  //写多个线圈
#define MOD_10H 0x10  //写多个保持寄存器

typedef struct {
    uint8_t RxBuf[MOD_BUF_SIZE];
    uint8_t RxCount;
    uint8_t RxStatus;
    uint8_t RxNewFlag;

    uint32_t Baud;
    uint8_t  RspCode;

    uint8_t TxBuf[MOD_BUF_SIZE];
    uint8_t TxCount;

    uint16_t Reg01H; /* 保存主机发送的寄存器首地址 */
    uint16_t Reg02H;
    uint16_t Reg03H;
    uint16_t Reg04H;

    uint8_t RegNum; /* 寄存器个数 */

    uint8_t fAck01H; /* 应答命令标志 0 表示执行失败 1表示执行成功 */
    uint8_t fAck02H;
    uint8_t fAck03H;
    uint8_t fAck04H;
    uint8_t fAck05H;
    uint8_t fAck06H;
    uint8_t fAck10H;

    uint8_t      WorkMode; /* 接收中断的工作模式： ASCII, MODBUS主机或 MODBUS 从机 */
    uint8_t      id;
    uint8_t      g_rtu_timeout;
    osThreadId_t threadId;
    int (*transmit)(uint8_t *_buf, uint16_t _len);

} MODBUS_T;

typedef struct {
    /* 03H 06H 读写保持寄存器 */
    uint16_t P01[8];
    /* 02H 读写离散输入寄存器 */
    uint16_t T01;
    uint16_t T02;
    uint16_t T03;

    /* 04H 读取模拟量寄存器 */
    uint16_t A01;

    /* 01H 05H 读写单个强制线圈 */
    uint16_t D01;
    uint16_t D02;
    uint16_t D03;
    uint16_t D04;

} VAR_T;

void MODBUS_InitVar(MODBUS_T *_tmod, uint8_t _id, uint32_t _Baud, uint8_t _WorkMode);

void    MODBUS_Poll(MODBUS_T *tmod);
uint8_t MODH_ReadParam_01H(MODBUS_T *_tmod, uint8_t _id, uint16_t _reg, uint16_t _num);
uint8_t MODH_ReadParam_02H(MODBUS_T *_tmod, uint8_t _id, uint16_t _reg, uint16_t _num);
uint8_t MODH_ReadParam_03H(MODBUS_T *_tmod, uint8_t _id, uint16_t _reg, uint16_t _num);
uint8_t MODH_ReadParam_04H(MODBUS_T *_tmod, uint8_t _id, uint16_t _reg, uint16_t _num);
uint8_t MODH_WriteParam_05H(MODBUS_T *_tmod, uint8_t _id, uint16_t _reg, uint16_t _value);
uint8_t MODH_WriteParam_06H(MODBUS_T *_tmod, uint8_t _id, uint16_t _reg, uint16_t _value);
uint8_t MODH_WriteParam_10H(MODBUS_T *_tmod, uint8_t _id, uint16_t _reg, uint8_t _num, uint8_t *_buf);

void MODBUS_RxData(MODBUS_T *tmod, uint8_t *src, uint8_t len);
void MODBUS_RxTimeOut(MODBUS_T *tmod);

extern VAR_T g_tVar[2];

#endif

/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
