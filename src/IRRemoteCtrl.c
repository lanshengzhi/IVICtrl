
#include "main.h"
#include "IRRemoteCtrl.h"

//#define DEBUG_IR    1

#define IR_DATA_SIZE    8      // 大小必须是2的指数

static INT8U  g_step;
static INT8U  g_dataCnt;
static INT16U g_irCmdData;
static INT8U  g_irCmd;

static INT8U g_inIndex;   // 指向空的位置
static INT8U g_outIndex;   // 指向待读取的数据
static INT8U g_msgQueue[IR_DATA_SIZE];


static BOOL PushQueue(INT8U irkey)  
{
    INT8U inIndex;

    OS_ENTER_CRITICAL();

    inIndex = (g_inIndex + 1)&(IR_DATA_SIZE-1);

    if (inIndex == g_outIndex)
    {
        OS_EXIT_CRITICAL();
        return FALSE;   // 队列已满
    }

    g_msgQueue[g_inIndex] = irkey;
    g_inIndex = inIndex;

    OS_EXIT_CRITICAL();

    return TRUE;
}

static BOOL PollQueue(INT8U data *pKey)
{
    OS_ENTER_CRITICAL();

    if (g_inIndex == g_outIndex)    // 队列是空的 
    {
        OS_EXIT_CRITICAL();
        return FALSE;
    }

    *pKey = g_msgQueue[g_outIndex];
    g_outIndex = (g_outIndex + 1)&(IR_DATA_SIZE-1);

    OS_EXIT_CRITICAL();

    return TRUE;
}

/*
********************************************************************
  函数名称: InitIRRemoteCtrl()
  函数功能: 初始化IR口，打开中断
  输入参数: 无
  输出参数: 无
  返回值:   无
  Call By: main()
********************************************************************
*/
void InitIRRemoteCtrl()
{
    g_inIndex  = 0;
    g_outIndex = 0;
    g_step     = 0;
    g_dataCnt  = 0;
    g_irCmdData = 0;
    g_irCmd = 0;

	TMOD |= 0x09;	// 设置T0为16位计数器，由INT0控制T0计数
    TR0 = 0;    // 关闭计数器
	IT0 = 1;	// 下降沿产生中断
	EX0 = 1; 	// 打开外部中断0
    TH0 = 0;
    TL0 = 0;                // 清计数器    
}

/*
********************************************************************
  函数名称: IRRemoteISR()
  函数功能: 处理IR中断, 当INT0出现下降沿时，该函数被调用
  输入参数: 无
  输出参数: 无
  返回值:   无
  实现原理：
  0: 检测同步帧，或重复帧
     当IR产生中断时，打开定时器，对IR后续的高电平进行计数，当再来一次下降沿中断时，
     停止计数，如果T = 4.5ms，则表示收到红外同步帧，调整状态机到 1; 如果T = 2.5ms重复
  1: 接收32位数据

********************************************************************
*/
void IRRemoteISR() interrupt 0
{
    INT16U count = 0;

    TR0 = 0;    // 停止计数
    TF0 = 0;    
    count = TH0 << 8 | TL0; // 读取计数
    TH0 = 0;
    TL0 = 0;                // 清计数器

	switch (g_step)
	{
		case 0:         // 判断同步头
            if (4000 < count && count < 4200)   // 判断是否为同步头脉冲
            {
                g_irCmdData = 0;
                g_step = 1;           // 第二步开始读取数据
                g_dataCnt = 0;
                break;
            }
			break;

        case 1:
            if (++g_dataCnt == 16)    // 收到2个字节的地址码
            {
                g_dataCnt = 0;
                g_step = 2;
            }
            break;

        case 2:
            g_irCmdData <<= 1;

            if (1200 < count && count < 1600)   // 收到1
            {
                g_irCmdData |= 1;
            }

            if (++g_dataCnt == 16)    // 收到16个字节的数据码
            {
                g_irCmd = (g_irCmdData>>8)&0xff;
                g_irCmd = g_irCmd^0xff;

                if (g_irCmd == (g_irCmdData & 0xff))
                {
                    PushQueue(g_irCmd);
                    g_dataCnt = 0;
                    g_step = 3;
                }
                else
                {
                    g_step = 0;
                }           
            }
            break;

        case 3:     // 判断是否有重复脉冲
            if (36000 < count && count < 37000)
            {
                g_step = 4;
            }
            else
            {
                g_step = 0;
            }
            break;

        case 4:
            if (22700 < count && count < 22900)
            {
                PushQueue(g_irCmd);
            }
            else if (1900 < count && count < 2200)
            {
                g_step = 4;  
            }
            else
            {
                g_step = 0;  
            }

            break;

		default:
            g_step = 0;
			break;
	}

    TR0 = 1;    // 重新打开计数器
}

void IRRemoteTask()
{
    INT8U irData = 0;

    if (PollQueue(&irData))
    {
        INT8U irBuf[2];
        irBuf[0] = IR_CODE;    // Data type
        irBuf[1] = irData;
        SendMsgToHost(irBuf, 2);
        //UartPrintStr("Key = ");
        //UartPrintNum(irData);
        //UartPrintStr("\r\n");
    }

    OSTimeDly(20);
}

