
#ifndef _DVD_CTRL_H
#define _DVD_CTRL_H

#include "..\inc\main.h"

#define DVD_CTRL_TASK_ID    3


#define ON_DISC_IN_PLACE    0xf001
#define ON_DISC_OUT         0xf002
#define ON_DISC_PWR_ON      0xf003
#define ON_DISC_PWR_OFF     0xf004 

void InitServoCtrl();

// DVD״̬��Ϣ
#define DVD_DISC_NOT_EXIST      0x00
#define DVD_DISC_EXIST          0x01
#define DVD_DISC_UNDER_IN       0x02
#define DVD_DISC_UNDER_OUT      0x03
#define DVD_DISC_WAIT_TAKE_AWAY 0x04

INT8U GetDiscState();       // ��ȡ��Ƭ״̬��Ϣ
void ServoCtrlTask(); // ϵͳ��ʱ���øú���

#endif