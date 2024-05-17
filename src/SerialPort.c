
#include "main.h"
#include "ServoCtrl.h"

#define RX_BUF_SIZE     16      // 大小必须是2的指数

static BOOL g_bTxIsStop;

static INT8U g_serialBuf[5];
static INT8U g_RxInIndex;   // 指向空的位置
static INT8U g_RxOutIndex;   // 指向待读取的数据
static INT8U g_RxQueue[RX_BUF_SIZE];

static BOOL PushRxQueue(INT8U msg)  
{
    INT8U inIndex;

    OS_ENTER_CRITICAL();
    inIndex = (g_RxInIndex + 1)&(RX_BUF_SIZE-1);

    if (inIndex == g_RxOutIndex)
    {
        OS_EXIT_CRITICAL();
        return FALSE;           // 队列已满
    }

    g_RxQueue[g_RxInIndex] = msg;
    g_RxInIndex = inIndex;      // 指向下一个空位  

    OS_EXIT_CRITICAL();

    return TRUE;
}

static BOOL PollRxQueue(INT8U data *pMsg)
{
    OS_ENTER_CRITICAL();

    if (g_RxInIndex == g_RxOutIndex)    // 队列是空的 
    {
        OS_EXIT_CRITICAL();
        return FALSE;
    }

    *pMsg = g_RxQueue[g_RxOutIndex];
    g_RxOutIndex = (g_RxOutIndex + 1)&(RX_BUF_SIZE-1);  // 指向下一个数据

    OS_EXIT_CRITICAL();

    return TRUE;
}

static BOOL UartWrite(INT8U *pBuf, INT8U nLen)
{
    if (pBuf == NULL || nLen == 0)
        return FALSE;

    TI = 0;

    do
    {
        SBUF = *pBuf;
        while (!g_bTxIsStop);   // 等待发送结束
        g_bTxIsStop = FALSE;
        ++pBuf;
    }
    while (--nLen);

    return TRUE;
}

BOOL InitUart()
{
    TMOD  = 0x20 | (TMOD & 0x0f);    // Time1 设为方式2，自动再装入8位计数初值
    SCON  = 0x50;       // 采用串口工作方式1，10异步收发
    PCON |= 0x80; 	    // 双倍波特率
    TH1   = 0xFD;       // 8位重装计数初值，波特率为19200

    g_bTxIsStop = FALSE;
    g_RxInIndex = 0;
    g_RxOutIndex = 0;
    
    TR1 = 1;            // 启动定时器
    ES  = 1;            // 允许串口中断

    return TRUE;
}

#if UART_PRINT_NUM_EN
void UartPrintNum(INT16U num)
{
    INT8U i = 9;
    INT8U vec[10];
      
    vec[i]  = 0;
             
    do
    {
        vec[--i] = num%10 + '0';
        num /= 10;
    }
    while (num!=0);

    UartPrintStr(vec + i);
}
#endif

#if UART_PRINT_STR_EN
void UartPrintStr(INT8U *pStr)
{
    while (*pStr)
    {
        UartWrite(pStr, 1); 
        ++pStr; 
    }   
}
#endif

void SendMsgToHost(INT8U data *pBuf, INT8U nLen)
{
    INT8U syncHead[] = {0xaa, 0x55};        // 同步头

    UartWrite(syncHead, sizeof(syncHead));  // 发送同步头
    UartWrite(&nLen, 1);                    // 发送数据长度
    UartWrite(pBuf, nLen);                  // 发送数据
}

void Uart0ISR() interrupt 4
{
    if (RI)                     // 接收中断
    {
        RI = 0;                 // 清中断标志
        PushRxQueue(SBUF);      // 把数据放入缓冲队列
    }
    else if (TI)
    {
        TI = 0;
        g_bTxIsStop = TRUE;     // 设置标志，UartWrite判断该表示
    }
}

/*
 解析通信协议
*/

static void ExcuteHostCmd()
{
    switch (g_serialBuf[0])
    {
        case DISC_CMD:
            switch (g_serialBuf[1])
            {
                case DISC_PWR_ON:       // ARM已经把电源打开
                    UartPrintStr("Arm has trun on the power\r\n");
                    OSSendMessage(DVD_CTRL_TASK_ID, OS_MSG_USER+DISC_PWR_ON);
                    break;

                case DISC_OPEN:         // 出碟或入碟
                    UartPrintStr("Arm request disc toggle\r\n");
                    OSSendMessage(DVD_CTRL_TASK_ID, OS_MSG_USER+DISC_OPEN); 
                    break;

                case DISC_FORCE_OUT:    // 强制出碟
                    UartPrintStr("Arm request force disc out\r\n");
                    OSSendMessage(DVD_CTRL_TASK_ID, OS_MSG_USER+DISC_FORCE_OUT);
                    break;

                case DISC_INFO_REQ:     // ARM请求碟片状态
                    {
                        INT8U buf[3] = {0x3, DISC_INFO_REQ, 0x00};
                        buf[2] = GetDiscState();
                        UartPrintStr("Arm request disc info\r\n");
                        SendMsgToHost(buf, sizeof(buf));
                    }
                    break;
                case DISC_REDO_INIT:
                    //UartPrintStr("Arm request reinit dvd\r\n");
                    OSSendMessage(DVD_CTRL_TASK_ID, OS_MSG_USER+DISC_REDO_INIT);
                    break;
            }
            break;
            
        default:
            break;  
    }   
}

/*
 查询待处理的数据
*/
void ServerTask()
{    
    INT8U nData;

    static INT8U nRecStep = 0;
    static INT8U nLen = 0;
    static INT8U g_nTotalLen = 0;

    if (!PollRxQueue(&nData))   // Queue is empty
    {
        OSTimeDly(20);
        return;
    }

    switch (nRecStep)
    {
        case 0:     // 同步头1
            if (nData == 0xaa)
            {
                ++nRecStep; 
            }
            break;

        case 1:     // 同步头2
            if (nData == 0x55)
            {
                ++nRecStep;
            }
            else if (nData == 0xaa)
            {
                nRecStep = 1;
            }
            else
            {
                nRecStep = 0;
            }
            break;

        case 2:     // 数据长度
            if (nData < 20 && nData > 0)
            {
                nLen = 0;
                g_nTotalLen = nData; 
                ++nRecStep; 
            }
            else
            {
                nRecStep = 0;
            }
            break;

        case 3:     // 数据
            g_serialBuf[nLen] = nData;

            if (++nLen == g_nTotalLen)
            {
                ExcuteHostCmd();
                nRecStep = 0;
            }
            break;

        default:
            nRecStep = 0;
            break;           
    }
}








