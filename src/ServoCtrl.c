
#include "main.h"
#include "servoctrl.h"
#include "serialport.h"

#define MOTO_CTRL_P             P3_7    // DVD�������������+
#define MOTO_CTRL_N             P3_5    // DVD�������������-

#define DISC_INSERT_DET         P1_1    // ��Ƭ������   
#define DISC_OUT_DET            P1_0    // ������λ���
#define DISC_IN_PLACE_DET       P1_4    // �����λ���

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
    MOTO_CTRL_N = 0;    // ��ֹ���ת��
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
        case DISC_OPEN:         // �û�������������
            if (g_nDiscState == DVD_DISC_WAIT_TAKE_AWAY)
            {
                g_step = 2;   
            }
            else if (g_nDiscState == DVD_DISC_EXIST)
            {
                g_step = 5;  
            }
            break;
        case DISC_FORCE_OUT:    // �û�����ǿ�Ƴ���
            g_step = 9;
            break;
        case DISC_REDO_INIT:    // ʹ���״̬�ص���ʼ״̬����wince�ػ��ٿ���ʱ���ᷢ�������Ϣ
            g_step = 0; 
            break;
        default:
            break;
    }

    switch (g_step)
    {
        case 0: // ��������Ƭ״̬
            MOTO_CTRL_P = 0;
            MOTO_CTRL_N = 0;
            g_nDiscState = DVD_DISC_NOT_EXIST;

            if (DISC_IN_PLACE_DET == IN_PLACE_ACTIVE_LEVEL)     // ��Ƭ���ڣ��ȴ��û������򲥷�
            {
                // ׼�����Ͷ�����Ϣ 
                //UartPrintStr("Disc exist\r\n");    
                g_nDiscState = DVD_DISC_EXIST;
                OSWaitMessage(0);        // ����ֹͣ���ȴ�������ǿ�Ƴ���       
            }
            else if (DISC_INSERT_DET == INSERT_ACTVIE_LEVEL)
            {
                // �е�Ƭ������û��λ����һ�����������ʹ֮��λ
                //UartPrintStr("Disc is detect\r\n");
                g_step = 2;
            }
            else
            {
                // û�е�Ƭ
                //UartPrintStr("Disc not exist\r\n");
                g_step = 1;
            }

            break;
            
        case 1: // �ȴ���Ƭ����
            if (DISC_INSERT_DET == INSERT_ACTVIE_LEVEL) // �е�Ƭ����
            {
                g_step = 2;
            }
            else    // δ��⵽��Ƭ���������
            {
                OSTimeDly(20);
                g_nDiscState = DVD_DISC_NOT_EXIST;
                g_bDiscInsertFlag = TRUE;
            }
            break;

        case 2: // ����ARM��DVD��Դ�� 
            {
                INT8U buf[] = {DISC_CMD, DISC_PWR_ON};
                SendMsgToHost(buf, sizeof(buf));
                //UartPrintStr("Requst arm to turn on power\r\n");
                OSWaitMessage(1000);    // �ȴ�ARM֪ͨ��Դ�Ѿ���
                g_step = 3;
            }
            break;

        case 3: // �ȴ�ARM��DVD��Դ��
            if (msg == DISC_PWR_ON)
            {
                //UartPrintStr("DVD know that Arm has turn on the power\r\n");
                MOTO_CTRL_P = 1;    // ARM�Ѿ��򿪵�Դ�����Կ�ʼ��ת���
                MOTO_CTRL_N = 0;  
                g_step = 4; 
                g_nDiscState = DVD_DISC_UNDER_IN;
            }
            else
            {
                g_step = 1;   // ������ARM�����DVD��Դ
                OSTimeDly(1000);
            }
            break;

        case 4: // �ȴ���Ƭ��λ
            if (DISC_IN_PLACE_DET == IN_PLACE_ACTIVE_LEVEL)
            {
                INT8U buf[] = {DISC_CMD, DISC_IN_IN_PLACE};

                g_nDlyCount = 0;
                MOTO_CTRL_P = 0;    // ֹͣ���
                MOTO_CTRL_N = 0;
                if (g_bDiscInsertFlag)
                {
                    g_bDiscInsertFlag = FALSE;
                    buf[1] = DISC_INSERT_IN_PLACE;
                }    
                g_nDiscState = DVD_DISC_EXIST; 
                SendMsgToHost(buf, sizeof(buf));       // ֪ͨARM����Ƭ��λ�����Կ�ʼ���ŵ�Ƭ
                OSWaitMessage(0);      // ֹͣ���񣬵ȴ����� 
            }
            else
            {
                if (DISC_INSERT_DET != INSERT_ACTVIE_LEVEL) // ������̱��û�ǿ��ȡ��
                {
                    g_nDlyCount = 0;
                    g_step = 0;
                }
                else if (++g_nDlyCount == 200)    // ����5�룬��û�����λ��ֹͣ���
                {
                    MOTO_CTRL_P = 0;    // ֹͣ���
                    MOTO_CTRL_N = 0;
                    g_step = 8;
                    g_nDlyCount = 0;     
                }
                OSTimeDly(20);
            }
            break;

        case 5:     // ���յ���������ʱ��ToggleDiscInOrOut()�ἤ������ʹ֮���е���
            {  
                INT8U buf[] = {DISC_CMD, DISC_PWR_ON};
                //UartPrintStr("Arm request disc out\r\n");
                SendMsgToHost(buf, sizeof(buf));   // ����ARM�򿪵�Դ
                OSWaitMessage(1000);  // �������ȴ�
                g_step = 6;
                break;
            }

        case 6:   // �����������
            if (msg == DISC_PWR_ON)
            {
                MOTO_CTRL_P = 0;
                MOTO_CTRL_N = 1;
                g_step = 7; 
                g_nDiscState = DVD_DISC_UNDER_OUT;
            }
            else
            {
                g_step = 5;         // ���������DVD��Դ
                OSTimeDly(1000);
            }
            break;

        case 7: // �ȴ�������λ
            if (DISC_OUT_DET == OUT_ACTIVE_LEVEL)   // �Ѿ�������λ
            {
                INT8U buf[] = {DISC_CMD, DISC_OUT_IN_PLACE};

                g_nDlyCount = 0;
                MOTO_CTRL_P = 0;    // ֹͣ���
                MOTO_CTRL_N = 0;
                g_step = 8;
                g_nDiscState = DVD_DISC_WAIT_TAKE_AWAY;
                SendMsgToHost(buf, sizeof(buf));   // ֪ͨ�ϲ㣬�Ѿ�������λ
            }
            else if (++g_nDlyCount > 200)   // ����δ��λ����ʱ 
            {
                INT8U buf[] = {DISC_CMD, DISC_OUT_IN_PLACE};

                g_nDlyCount = 0;
                MOTO_CTRL_P = 0;    // ֹͣ���
                MOTO_CTRL_N = 0;
                g_step = 0;
                g_nDiscState = DVD_DISC_WAIT_TAKE_AWAY;
                SendMsgToHost(buf, sizeof(buf));   // ֪ͨ�ϲ㣬�Ѿ�������λ���Ա�UI���л����л�       
            }
            else    // δ������λ�������ȴ�
            {
                OSTimeDly(20);
            }
            break;

        case 8: // �ȴ��û��ѵ�Ƭȡ��
            if (DISC_INSERT_DET != INSERT_ACTVIE_LEVEL)
            {
                g_nDlyCount = 0;
                g_step = 1;   // ��Ƭ�Ѿ���ȡ�� 
            }
            else
            {
                if (++g_nDlyCount > 80)  // ����8�룬�û���δ�ѵ�Ƭ���ߣ����������
                {
                    //UartPrintStr("User not take disc away, roll in again!\r\n");
                    g_nDlyCount = 0;
                    g_step = 1;   // �������
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
                SendMsgToHost(buf, sizeof(buf));   // ֪ͨARMǿ�Ƴ������
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