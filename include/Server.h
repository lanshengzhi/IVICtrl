
#ifndef _SERVER_H
#define _SERVER_H

#include "..\inc\main.h"

/*
 通过通信协议发送数据
*/
void UartSendByProtocol(INT8U *pBuf, INT8U nLen);
void ServerTask();

/* 
 数据类型 
*/
#define RAW_ADC_VALUE       0x01
#define KEY_CODE            0x02
#define DISC_CMD            0x03
#define IR_CODE             0x04

// RAW_ADC_VALUE INFO
#define RAW_KEY_DOWN        0x01
#define RAW_KEY_REPEAT      0x02
#define RAW_KEY_UP          0x04

// KEY_CODE INFO
#define KEY_DOWN            0x01
#define KEY_KEY_REPEAT      0x02
#define KEY_UP              0X04

// DISC_CMD
#define DISC_INFO_REQ       0x00    // ARM请求Disc 状态
#define DISC_PWR_ON         0x01    // 请求打开电源
#define DISC_PWR_OFF        0x02    // 
#define DISC_IN_IN_PLACE    0x03    // 入碟到位
#define DISC_INSERT_IN_PLACE 0x04   // 从无碟到有碟
#define DISC_OUT_IN_PLACE   0x05    // 出碟到位
#define DISC_OPEN           0x06    // ARM->MCU 出碟
#define DISC_FORCE_OUT      0x07    // 强制出碟
#define DISC_REDO_INIT      0x08    // 通知MCU重新初始化Disc状态
//#define DISK_

#endif