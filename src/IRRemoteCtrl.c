
#include "main.h"
#include "IRRemoteCtrl.h"

//#define DEBUG_IR    1

#define IR_DATA_SIZE    8      // ��С������2��ָ��

static INT8U  g_step;
static INT8U  g_dataCnt;
static INT16U g_irCmdData;
static INT8U  g_irCmd;

static INT8U g_inIndex;   // ָ��յ�λ��
static INT8U g_outIndex;   // ָ�����ȡ������
static INT8U g_msgQueue[IR_DATA_SIZE];


static BOOL PushQueue(INT8U irkey)  
{
    INT8U inIndex;

    OS_ENTER_CRITICAL();

    inIndex = (g_inIndex + 1)&(IR_DATA_SIZE-1);

    if (inIndex == g_outIndex)
    {
        OS_EXIT_CRITICAL();
        return FALSE;   // ��������
    }

    g_msgQueue[g_inIndex] = irkey;
    g_inIndex = inIndex;

    OS_EXIT_CRITICAL();

    return TRUE;
}

static BOOL PollQueue(INT8U data *pKey)
{
    OS_ENTER_CRITICAL();

    if (g_inIndex == g_outIndex)    // �����ǿյ� 
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
  ��������: InitIRRemoteCtrl()
  ��������: ��ʼ��IR�ڣ����ж�
  �������: ��
  �������: ��
  ����ֵ:   ��
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

	TMOD |= 0x09;	// ����T0Ϊ16λ����������INT0����T0����
    TR0 = 0;    // �رռ�����
	IT0 = 1;	// �½��ز����ж�
	EX0 = 1; 	// ���ⲿ�ж�0
    TH0 = 0;
    TL0 = 0;                // �������    
}

/*
********************************************************************
  ��������: IRRemoteISR()
  ��������: ����IR�ж�, ��INT0�����½���ʱ���ú���������
  �������: ��
  �������: ��
  ����ֵ:   ��
  ʵ��ԭ��
  0: ���ͬ��֡�����ظ�֡
     ��IR�����ж�ʱ���򿪶�ʱ������IR�����ĸߵ�ƽ���м�����������һ���½����ж�ʱ��
     ֹͣ���������T = 4.5ms�����ʾ�յ�����ͬ��֡������״̬���� 1; ���T = 2.5ms�ظ�
  1: ����32λ����

********************************************************************
*/
void IRRemoteISR() interrupt 0
{
    INT16U count = 0;

    TR0 = 0;    // ֹͣ����
    TF0 = 0;    
    count = TH0 << 8 | TL0; // ��ȡ����
    TH0 = 0;
    TL0 = 0;                // �������

	switch (g_step)
	{
		case 0:         // �ж�ͬ��ͷ
            if (4000 < count && count < 4200)   // �ж��Ƿ�Ϊͬ��ͷ����
            {
                g_irCmdData = 0;
                g_step = 1;           // �ڶ�����ʼ��ȡ����
                g_dataCnt = 0;
                break;
            }
			break;

        case 1:
            if (++g_dataCnt == 16)    // �յ�2���ֽڵĵ�ַ��
            {
                g_dataCnt = 0;
                g_step = 2;
            }
            break;

        case 2:
            g_irCmdData <<= 1;

            if (1200 < count && count < 1600)   // �յ�1
            {
                g_irCmdData |= 1;
            }

            if (++g_dataCnt == 16)    // �յ�16���ֽڵ�������
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

        case 3:     // �ж��Ƿ����ظ�����
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

    TR0 = 1;    // ���´򿪼�����
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

