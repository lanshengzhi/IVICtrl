
#ifndef OS_CFG_H
#define OS_CFG_H

#define OS_VERSION              100             // 软件版本号

#define OS_MAX_EVENTS           1               // Max. number of event control blocks

#define OS_MAX_TASKS            6               // Max. number of tasks in you application     

#define OS_STASK_RESUME_EN      0
#define OS_TASK_SUSPEND_EN      0
#define OS_TIME_DLY_EN          1

#define UART_PRINT_NUM_EN       0
#define UART_PRINT_STR_EN       0

#define OS_SEND_MESSAGE_EN      1
#define OS_POST_MESSAGE_EN      1
#define OS_WAIT_MESSAGE_EN      1
#define OS_GET_MESSAGE_EN       1

#if UART_PRINT_NUM_EN
#define UART_PRINT_STR_EN       1
#endif

#define OS_Sem_EN               0
#define OS_Q_EN                 0


/*
******************************************************************** 
* 通信协议 
******************************************************************** 
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

#define VOLUME_UP           21
#define VOLUME_DOWN         22

#endif