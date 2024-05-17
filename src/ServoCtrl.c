
#include "main.h"
#include "servoctrl.h"
#include "serialport.h"

#define MOTO_CTRL_P             P3_7    // DVD电机进出碟控制+
#define MOTO_CTRL_N             P3_5    // DVD电机进出碟控制-

#define DISC_INSERT_DET         P1_1    // 碟片插入检测   
#define DISC_OUT_DET            P1_0    // 出碟到位检测
#define DISC_IN_PLACE_DET       P1_4    // 入碟到位检测

#define INSERT_ACTVIE_LEVEL     1       
#define OUT_ACTIVE_LEVEL        0
#define IN_PLACE_ACTIVE_LEVEL   0

static BOOL  g_bDiscInsertFlag;
static INT8U g_nDiscState;
static INT8U g_step;
static INT8U g_nDlyCount;

void InitServoCtrl()
{
    MOTO_CTRL_P = 0;
    MOTO_CTRL_N = 0;    // 禁止电机转动
    g_nDiscState = 0;
    g_step = 0;
    g_nDlyCount = 0;
    g_bDiscInsertFlag = 0;
}

INT8U GetDiscState()
{
    return g_nDiscState;
}

void ServoCtrlTask()
{
    INT16U msg = 0xfff & OSGetMessage();

    switch (msg)
    {
        case DISC_OPEN:         // 用户请求入碟或出碟
            if (g_nDiscState == DVD_DISC_WAIT_TAKE_AWAY)
            {
                g_step = 2;   
            }
            else if (g_nDiscState == DVD_DISC_EXIST)
            {
                g_step = 5;  
            }
            break;
        case DISC_FORCE_OUT:    // 用户请求强制出碟
            g_step = 9;
            break;
        case DISC_REDO_INIT:    // 使检测状态回到初始状态，当wince关机再开机时，会发送这个消息
            g_step = 0; 
            break;
        default:
            break;
    }

    switch (g_step)
    {
        case 0: // 开机检测碟片状态
            MOTO_CTRL_P = 0;
            MOTO_CTRL_N = 0;
            g_nDiscState = DVD_DISC_NOT_EXIST;

            if (DISC_IN_PLACE_DET == IN_PLACE_ACTIVE_LEVEL)     // 碟片存在，等待用户出碟或播放
            {
                // 准备发送读碟信息 
                //UartPrintStr("Disc exist\r\n");    
                g_nDiscState = DVD_DISC_EXIST;
                OSWaitMessage(0);        // 任务停止，等待出碟或强制出碟       
            }
            else if (DISC_INSERT_DET == INSERT_ACTVIE_LEVEL)
            {
                // 有碟片，但是没到位，下一步运作电机，使之到位
                //UartPrintStr("Disc is detect\r\n");
                g_step = 2;
            }
            else
            {
                // 没有碟片
                //UartPrintStr("Disc not exist\r\n");
                g_step = 1;
            }

            break;
            
        case 1: // 等待碟片插入
            if (DISC_INSERT_DET == INSERT_ACTVIE_LEVEL) // 有碟片插入
            {
                g_step = 2;
            }
            else    // 未检测到碟片，继续检测
            {
                OSTimeDly(20);
                g_nDiscState = DVD_DISC_NOT_EXIST;
                g_bDiscInsertFlag = TRUE;
            }
            break;

        case 2: // 请求ARM把DVD电源打开 
            {
                INT8U buf[] = {DISC_CMD, DISC_PWR_ON};
                SendMsgToHost(buf, sizeof(buf));
                //UartPrintStr("Requst arm to turn on power\r\n");
                OSWaitMessage(1000);    // 等待ARM通知电源已经打开
                g_step = 3;
            }
            break;

        case 3: // 等待ARM把DVD电源打开
            if (msg == DISC_PWR_ON)
            {
                //UartPrintStr("DVD know that Arm has turn on the power\r\n");
                MOTO_CTRL_P = 1;    // ARM已经打开电源，可以开始运转电机
                MOTO_CTRL_N = 0;  
                g_step = 4; 
                g_nDiscState = DVD_DISC_UNDER_IN;
            }
            else
            {
                g_step = 1;   // 重新向ARM请求打开DVD电源
                OSTimeDly(1000);
            }
            break;

        case 4: // 等待碟片到位
            if (DISC_IN_PLACE_DET == IN_PLACE_ACTIVE_LEVEL)
            {
                INT8U buf[] = {DISC_CMD, DISC_IN_IN_PLACE};

                g_nDlyCount = 0;
                MOTO_CTRL_P = 0;    // 停止电机
                MOTO_CTRL_N = 0;
                if (g_bDiscInsertFlag)
                {
                    g_bDiscInsertFlag = FALSE;
                    buf[1] = DISC_INSERT_IN_PLACE;
                }    
                g_nDiscState = DVD_DISC_EXIST; 
                SendMsgToHost(buf, sizeof(buf));       // 通知ARM，碟片到位，可以开始播放碟片
                OSWaitMessage(0);      // 停止任务，等待出碟 
            }
            else
            {
                if (DISC_INSERT_DET != INSERT_ACTVIE_LEVEL) // 入碟过程被用户强行取走
                {
                    g_nDlyCount = 0;
                    g_step = 0;
                }
                else if (++g_nDlyCount == 200)    // 过了5秒，还没入碟到位，停止入碟
                {
                    MOTO_CTRL_P = 0;    // 停止电机
                    MOTO_CTRL_N = 0;
                    g_step = 8;
                    g_nDlyCount = 0;     
                }
                OSTimeDly(20);
            }
            break;

        case 5:     // 当收到出碟命令时，ToggleDiscInOrOut()会激活任务，使之运行到这
            {  
                INT8U buf[] = {DISC_CMD, DISC_PWR_ON};
                //UartPrintStr("Arm request disc out\r\n");
                SendMsgToHost(buf, sizeof(buf));   // 请求ARM打开电源
                OSWaitMessage(1000);  // 任务进入等待
                g_step = 6;
                break;
            }

        case 6:   // 启动电机出碟
            if (msg == DISC_PWR_ON)
            {
                MOTO_CTRL_P = 0;
                MOTO_CTRL_N = 1;
                g_step = 7; 
                g_nDiscState = DVD_DISC_UNDER_OUT;
            }
            else
            {
                g_step = 5;         // 重新请求打开DVD电源
                OSTimeDly(1000);
            }
            break;

        case 7: // 等待出碟到位
            if (DISC_OUT_DET == OUT_ACTIVE_LEVEL)   // 已经出碟到位
            {
                INT8U buf[] = {DISC_CMD, DISC_OUT_IN_PLACE};

                g_nDlyCount = 0;
                MOTO_CTRL_P = 0;    // 停止电机
                MOTO_CTRL_N = 0;
                g_step = 8;
                g_nDiscState = DVD_DISC_WAIT_TAKE_AWAY;
                SendMsgToHost(buf, sizeof(buf));   // 通知上层，已经出碟到位
            }
            else if (++g_nDlyCount > 200)   // 出碟未到位，超时 
            {
                INT8U buf[] = {DISC_CMD, DISC_OUT_IN_PLACE};

                g_nDlyCount = 0;
                MOTO_CTRL_P = 0;    // 停止电机
                MOTO_CTRL_N = 0;
                g_step = 0;
                g_nDiscState = DVD_DISC_WAIT_TAKE_AWAY;
                SendMsgToHost(buf, sizeof(buf));   // 通知上层，已经出碟到位，以便UI进行画面切换       
            }
            else    // 未出碟到位，继续等待
            {
                OSTimeDly(20);
            }
            break;

        case 8: // 等待用户把碟片取走
            if (DISC_INSERT_DET != INSERT_ACTVIE_LEVEL)
            {
                g_nDlyCount = 0;
                g_step = 1;   // 碟片已经被取走 
            }
            else
            {
                if (++g_nDlyCount > 80)  // 超过8秒，用户还未把碟片拿走，则重新入碟
                {
                    //UartPrintStr("User not take disc away, roll in again!\r\n");
                    g_nDlyCount = 0;
                    g_step = 1;   // 重新入碟
                }
                OSTimeDly(100);
            }
            break;

        case 9:
            {  
                INT8U buf[] = {DISC_CMD, DISC_PWR_ON};
                SendMsgToHost(buf, sizeof(buf));
                OSWaitMessage(1000);
                g_step = 10;
                break;
            }
            break;
        case 10:
            if (msg == DISC_PWR_ON)
            {
                MOTO_CTRL_P = 0;
                MOTO_CTRL_N = 1;
                g_step = 11;
                OSTimeDly(5000);
            }
            else
            {
                g_step = 9;
                OSTimeDly(1000);
            }
                
            break;

        case 11:
            {
                INT8U buf[] = {DISC_CMD, DISC_FORCE_OUT};
                SendMsgToHost(buf, sizeof(buf));   // 通知ARM强制出碟完成
                MOTO_CTRL_P = 0;
                MOTO_CTRL_N = 0;
                g_step = 0;
                break;
            }

        default:
            g_step = 0;
            break;
    }
}