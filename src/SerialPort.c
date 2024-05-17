
#include "main.h"
#include "ServoCtrl.h"

#define RX_BUF_SIZE     16      // ��С������2��ָ��

static BOOL g_bTxIsStop;

static INT8U g_serialBuf[5];
static INT8U g_RxInIndex;   // ָ��յ�λ��
static INT8U g_RxOutIndex;   // ָ�����ȡ������
static INT8U g_RxQueue[RX_BUF_SIZE];

static BOOL PushRxQueue(INT8U msg)  
{
    INT8U inIndex;

    OS_ENTER_CRITICAL();
    inIndex = (g_RxInIndex + 1)&(RX_BUF_SIZE-1);

    if (inIndex == g_RxOutIndex)
    {
        OS_EXIT_CRITICAL();
        return FALSE;           // ��������
    }

    g_RxQueue[g_RxInIndex] = msg;
    g_RxInIndex = inIndex;      // ָ����һ����λ  

    OS_EXIT_CRITICAL();

    return TRUE;
}

static BOOL PollRxQueue(INT8U data *pMsg)
{
    OS_ENTER_CRITICAL();

    if (g_RxInIndex == g_RxOutIndex)    // �����ǿյ� 
    {
        OS_EXIT_CRITICAL();
        return FALSE;
    }

    *pMsg = g_RxQueue[g_RxOutIndex];
    g_RxOutIndex = (g_RxOutIndex + 1)&(RX_BUF_SIZE-1);  // ָ����һ������

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
        while (!g_bTxIsStop);   // �ȴ����ͽ���
        g_bTxIsStop = FALSE;
        ++pBuf;
    }
    while (--nLen);

    return TRUE;
}

BOOL InitUart()
{
    TMOD  = 0x20 | (TMOD & 0x0f);    // Time1 ��Ϊ��ʽ2���Զ���װ��8λ������ֵ
    SCON  = 0x50;       // ���ô��ڹ�����ʽ1��10�첽�շ�
    PCON |= 0x80; 	    // ˫��������
    TH1   = 0xFD;       // 8λ��װ������ֵ��������Ϊ19200

    g_bTxIsStop = FALSE;
    g_RxInIndex = 0;
    g_RxOutIndex = 0;
    
    TR1 = 1;            // ������ʱ��
    ES  = 1;            // �������ж�

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
    INT8U syncHead[] = {0xaa, 0x55};        // ͬ��ͷ

    UartWrite(syncHead, sizeof(syncHead));  // ����ͬ��ͷ
    UartWrite(&nLen, 1);                    // �������ݳ���
    UartWrite(pBuf, nLen);                  // ��������
}

void Uart0ISR() interrupt 4
{
    if (RI)                     // �����ж�
    {
        RI = 0;                 // ���жϱ�־
        PushRxQueue(SBUF);      // �����ݷ��뻺�����
    }
    else if (TI)
    {
        TI = 0;
        g_bTxIsStop = TRUE;     // ���ñ�־��UartWrite�жϸñ�ʾ
    }
}

/*
 ����ͨ��Э��
*/

static void ExcuteHostCmd()
{
    switch (g_serialBuf[0])
    {
        case DISC_CMD:
            switch (g_serialBuf[1])
            {
                case DISC_PWR_ON:       // ARM�Ѿ��ѵ�Դ��
                    UartPrintStr("Arm has trun on the power\r\n");
                    OSSendMessage(DVD_CTRL_TASK_ID, OS_MSG_USER+DISC_PWR_ON);
                    break;

                case DISC_OPEN:         // ���������
                    UartPrintStr("Arm request disc toggle\r\n");
                    OSSendMessage(DVD_CTRL_TASK_ID, OS_MSG_USER+DISC_OPEN); 
                    break;

                case DISC_FORCE_OUT:    // ǿ�Ƴ���
                    UartPrintStr("Arm request force disc out\r\n");
                    OSSendMessage(DVD_CTRL_TASK_ID, OS_MSG_USER+DISC_FORCE_OUT);
                    break;

                case DISC_INFO_REQ:     // ARM�����Ƭ״̬
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
 ��ѯ�����������
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
        case 0:     // ͬ��ͷ1
            if (nData == 0xaa)
            {
                ++nRecStep; 
            }
            break;

        case 1:     // ͬ��ͷ2
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

        case 2:     // ���ݳ���
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

        case 3:     // ����
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








