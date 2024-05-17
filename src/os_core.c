
#include "main.h"
#include <string.h>

/*
********************************************************************
���ļ�ȫ�ֱ���
******************************************************************** 
*/ 
static INT8U          OSRdyGrp;                     // Ready list group 
static INT8U    idata OSRdyTbl[OS_RDY_TBL_SIZE];    // Table of tasks which are ready to run
#if OS_MAX_EVENTS
static OS_EVENT idata OSEventTbl[OS_MAX_EVENTS];
#endif
static OS_TCB   idata *OSTCBCur;                    // ָ��ǰ�����
static OS_TCB   idata OSTCBTbl[OS_LOWEST_PRIO + 1]; // ������ƿ�����

/*
*********************************************************************************************************
*                              MAPPING TABLE TO MAP BIT POSITION TO BIT MASK
*
* Note: Index into table is desired bit position, 0..7
*       Indexed value corresponds to bit mask
*********************************************************************************************************
*/
static INT8U const code OSMapTbl[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

/*
*********************************************************************************************************
*                                       PRIORITY RESOLUTION TABLE
*
* Note: Index into table is bit pattern to resolve highest priority
*       Indexed value corresponds to highest priority bit position (i.e. 0..7)
*********************************************************************************************************
*/
static INT8U const code OSUnMapTbl[] = {
    0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0
};

/*
********************************************************************
  ��������: ֪ͨ�ȴ����¼�������������״̬
  �������: ��
  �������: ��
  ����ֵ:   ��
******************************************************************** 
*/
#if OS_MAX_EVENTS
void OSEventTaskRdy(OS_EVENT idata *pevent, INT8U msk)
{
    OS_TCB idata *ptcb;
    INT8U   x;
    INT8U   y;
    INT8U   bitx;
    INT8U   bity;
    INT8U   prio;

    y    = OSUnMapTbl[pevent->OSEventGrp];      // �ҳ��ȴ����¼����ȼ���ߵ�����    
    bity = OSMapTbl[y];
    x    = OSUnMapTbl[pevent->OSEventTbl[y]];
    bitx = OSMapTbl[x];
    prio = (y << 3) + x;               

    if ((pevent->OSEventTbl[y] &= ~bitx) == 0)  // ������ӵȴ��б������
    {
        pevent->OSEventGrp &= ~bity;
    }

    ptcb            =  &OSTCBTbl[prio];         // Point to this task's OS_TCB
    ptcb->OSTCBDly  =  0;                       // 
    ptcb->OSTCBStat &= ~msk;
    ptcb->OSTCBMsg  = 0; 

    pevent->OSEventOwner = ptcb->OSTCBPrio;
     
    if (ptcb->OSTCBStat == OS_STAT_RDY) 
    {
        OSRdyGrp    |=  bity;                   // Put task in the ready to run list
        OSRdyTbl[y] |=  bitx;
    }             
}
#endif
/*
********************************************************************
  ��������: ʹ��������¼��ĵȴ��б�ͬʱ��������
******************************************************************** 
*/
#if OS_MAX_EVENTS
void OSEventTaskWait(OS_EVENT idata *pevent)
{
    if ((OSRdyTbl[OSTCBCur->OSTCBY] &= ~OSTCBCur->OSTCBBitX) == 0)      /* Task no longer ready      */
    {
        OSRdyGrp &= ~OSTCBCur->OSTCBBitY;
    }

    pevent->OSEventTbl[OSTCBCur->OSTCBY] |= OSTCBCur->OSTCBBitX;          /* Put task in waiting list  */
    pevent->OSEventGrp                   |= OSTCBCur->OSTCBBitY;
}
#endif
/*
********************************************************************
  ��������: ��ʼ���¼��ĵȴ��б�
******************************************************************** 
*/
#if OS_MAX_EVENTS
void OSEventWaitListInit(OS_EVENT idata *pevent)
{
    INT8U i;

    pevent->OSEventGrp = 0x00;                   // No task waiting on event

    for (i = 0; i < OS_EVENT_TBL_SIZE; i++)
    {
        pevent->OSEventTbl[i] = 0x00;
    }
}
#endif
/*
********************************************************************
  ��������: ��ȡ�¼����ƿ�
******************************************************************** 
*/
OS_EVENT idata *OSMutexCreate(INT8U index)
{
    OS_EVENT idata *pevent = OSEventTbl+index;

    pevent->OSEventCnt = 0xff;      // ��־��Դ����
    OSEventWaitListInit(pevent);

    return pevent;  
}

BOOL OSMutexPend(OS_EVENT idata *pevent, INT16U timeout)
{
    OS_ENTER_CRITICAL();

    if (pevent->OSEventCnt == 0xff)     // ���Ϊ0xff��˵����Դ����
    {
        pevent->OSEventOwner = OSTCBCur->OSTCBPrio;  // ����ӵ�иû�����������
        pevent->OSEventCnt = 0;         // �ѻ�������־Ϊ������
        OS_EXIT_CRITICAL();     
        return TRUE;
    }

    OSTCBCur->OSTCBStat |= OS_STAT_MUTEX;   // ��Դ�����ã������������
    OSTCBCur->OSTCBDly   = timeout;         // ��ʱ
    OSEventTaskWait(pevent);                // ����ȴ��б�
    OS_EXIT_CRITICAL();

    return FALSE;
}

/*
 Ӧ����ӵ�иû����������񣬲��ܵ���MutexPost
*/
BOOL OSMutexPost(OS_EVENT idata *pevent)
{
    OS_ENTER_CRITICAL();

    if (pevent->OSEventOwner != OSTCBCur->OSTCBPrio)    // ����ȷ��ֻ��ӵ��Mutex���������
    {
        OS_EXIT_CRITICAL();
        return FALSE;
    }

    if (pevent->OSEventGrp)         // ���������ڵȴ�����¼�
    {
        OSEventTaskRdy(pevent, OS_STAT_MUTEX);  // ʹ���ڵȴ����¼�������������״̬
    }
    else
    {
        pevent->OSEventCnt = 0xff;      // ������Դ����
    }

    OS_EXIT_CRITICAL();

    return TRUE;
}

/*
********************************************************************
  ��������: void OSInit()
  ��������: ����ϵͳ��ʼ��
  �������: ��
  �������: ��
  ����ֵ:   ��
******************************************************************** 
*/ 
void OSInit()
{
    INT8U i;
    OS_TCB idata *ptcb = OSTCBTbl;

    OSRdyGrp = 0;       

    for (i = 0; i < OS_RDY_TBL_SIZE; i++)
    {
        OSRdyTbl[i] = 0;
    }

    for (i = 0; i < OS_LOWEST_PRIO + 1; ++i)
    {
        ptcb->pfnTask   = NULL;
        ptcb->OSTCBMsg  = 0;        
        ptcb->OSTCBDly  = 0;        
        ptcb->OSTCBStat = 0;
        ptcb->OSTCBPrio = 0;             
        
        ptcb->OSTCBX    = 0;
        ptcb->OSTCBY    = 0;

        ++ptcb;
    }

    OSInitMisc();                           // ��ʼ����ʱ���ж�,���ļ�os_cpu.c�ж��� 
    OSTaskCreate(OSTaskIdle, OS_IDLE_PRIO); // ��ʼ���������������ȼ���ͣ�����Զ���ھ���״̬
}

/*
********************************************************************
* ��������: void OSTimeTick()
* ��������: �ѱ�־λgTickFlag��1, ����1msʱ�䵽�� �ɶ�ʱ�жϺ�������
* �������: ��
* �������: ��    
* ����ֵ:   ��
* ����:    
******************************************************************** 
*/ 
void OSTimeTick() interrupt 5
{
    INT8U prio;
    OS_TCB idata *ptcb = OSTCBTbl; 

    OS_CLEAR_TICK_INT;              // �嶨ʱ�жϱ�־ 

    OS_ENTER_CRITICAL();
    for (prio = 0; prio < OS_LOWEST_PRIO; ++prio)      // �������������
    {
        if (ptcb->OSTCBDly != 0)
        {
            if (--ptcb->OSTCBDly == 0)
            {
                OSRdyGrp               |= ptcb->OSTCBBitY; //  Make task Rdy to Run (timed out)
                OSRdyTbl[ptcb->OSTCBY] |= ptcb->OSTCBBitX;         
            }
        }

        ++ptcb;
    } 
    OS_EXIT_CRITICAL();    
}

/*********************************************************************
  ��������: BOOL OSTaskCreate(void (*pfnTask)(void), INT8U prio)
  ��������: �������� 
  �������: *pfnTask ָ��Ҫ����������(����); prio-->�������ȼ�
  �������: ��  
  ����ֵ:   1-->�����ɹ���0-->����ʧ��
  ����:     
*********************************************************************/
INT8U OSTaskCreate(void (*pfnTask)(), INT8U prio)
{ 
    OS_TCB idata *ptcb = OSTCBTbl + prio;

    if (prio > OS_LOWEST_PRIO)
    {
        return OS_PRIO_INVALID;
    }

    ptcb->pfnTask   = pfnTask;
    ptcb->OSTCBMsg  = 0;        
    ptcb->OSTCBDly  = 0;        
    ptcb->OSTCBStat = OS_STAT_RDY;
    ptcb->OSTCBPrio = prio;             
    ptcb->OSTCBX    = prio & 0x07;
    ptcb->OSTCBY    = prio >> 3;
    ptcb->OSTCBBitX = OSMapTbl[ptcb->OSTCBX];
    ptcb->OSTCBBitY = OSMapTbl[ptcb->OSTCBY];

    OSRdyGrp               |= ptcb->OSTCBBitY;
    OSRdyTbl[ptcb->OSTCBY] |= ptcb->OSTCBBitX;

    return OS_NO_ERR;
}

/*********************************************************************
  ��������: void OSStart(void)
  ��������: ��������ϵͳ�������е���
  �������: ��
  �������: ��
  ����ֵ:   ��
  ����:     
*********************************************************************/
void OSStart(void)
{  
    INT8U nPrioHighRdy;
    INT8U y;
    INT8U x;

    OSPrevSched();      // ����ǰ�����ض�MCU��ص����ã��磺���жϣ��򿪿��Ź���

    while (1)
    {
        y = OSUnMapTbl[OSRdyGrp];           // �ҳ����ȼ�����Ҿ���������
        x = OSUnMapTbl[OSRdyTbl[y]];
        nPrioHighRdy = (y << 3) + x;

        OSTCBCur = &OSTCBTbl[nPrioHighRdy]; // OSTCBCurָ��ǰ�����

        if (OSTCBCur->pfnTask)
        {
            OSTCBCur->pfnTask();            // ��������
        }

        OSUnderSched();     // ���ȹ��̣����ض�MCU��ص����ã��磺���Ź�
    } 
}

/*********************************************************************
  ��������: void OSTaskIdle()
  ��������: ��û�����ȼ����ߵ�����ʱ��������OSTaskIdle()
  �������: ��
  �������: ��
  ����ֵ:   ��
  ����: ע�⣡����������Զ���ھ���״̬    
*********************************************************************/
void OSTaskIdle()
{
    //OSEnterIdle();      // ʹCPU�������ģʽ�������ж�ʱ����������ִ��   
}  

/*********************************************************************
  ��������: void OSTimeDly(INT16U ticks)
  ��������: ��ʱ��ǰ����
  �������: ticks -->��ʱ��������λΪms
  �������: ��
  ����ֵ:   ��
  ����: ע�⣡�ú���ֻ�ܱ��������    
*********************************************************************/
#if OS_TIME_DLY_EN
void OSTimeDly(INT16U ticks)
{
    OS_ENTER_CRITICAL();
    // �ѵ�ǰ����Ӿ��������Ƴ�
    if ((OSRdyTbl[OSTCBCur->OSTCBY] &= ~OSTCBCur->OSTCBBitX) == 0)  
    {
        OSRdyGrp &= ~OSTCBCur->OSTCBBitY;   
    }
    
    OSTCBCur->OSTCBDly  = ticks;
    OS_EXIT_CRITICAL();
}
#endif

/*********************************************************************
  ��������: INT8U OSPostMessage(INT8U prio, INT16U msg)
  ��������: ������Ϣ�����ȼ�Ϊprio������, 
            Ȼ������ͨ��OSGetMessage���Բ�ѯ���msg
  �������: prio, msg
  �������: ��
  ����ֵ:   
  ����:     
*********************************************************************/
#if OS_POST_MESSAGE_EN
INT8U OSPostMessage(INT8U prio, INT16U msg)
{
    OS_TCB idata *ptcb = OSTCBTbl + prio;

    if (prio >= OS_LOWEST_PRIO && prio != OS_PRIO_SELF)
    {
        return OS_PRIO_INVALID;
    }

    OS_ENTER_CRITICAL();

    ptcb->OSTCBStat &= ~OS_STAT_MSG;
    // ʹ����������״̬
    if (ptcb->OSTCBStat == OS_STAT_RDY)
    {
        OSRdyGrp               |= ptcb->OSTCBBitY;
        OSRdyTbl[ptcb->OSTCBY] |= ptcb->OSTCBBitX;
    } 

    ptcb->OSTCBMsg = msg;

    OS_EXIT_CRITICAL();

    return OS_NO_ERR;        
}
#endif

#if OS_SEND_MESSAGE_EN
INT8U OSSendMessage(INT8U prio, INT16U msg)
{
    OS_TCB idata *ptcb = OSTCBTbl + prio;

    if (prio >= OS_LOWEST_PRIO && prio != OS_PRIO_SELF)
    {
        return OS_PRIO_INVALID;
    }

    OS_ENTER_CRITICAL();

    ptcb->OSTCBStat = OS_STAT_RDY;
    ptcb->OSTCBMsg  = msg;
    ptcb->OSTCBDly  = 0;
    // ʹ����������״̬
    OSRdyGrp               |= ptcb->OSTCBBitY;
    OSRdyTbl[ptcb->OSTCBY] |= ptcb->OSTCBBitX; 

    OS_EXIT_CRITICAL();

    return OS_NO_ERR;        
}
#endif
/*********************************************************************
  ��������: INT8U OSMsgPend(INT16U timeout)
  ��������: ������Ϣ�����ȼ�Ϊprio������
  �������: prio, msg
  �������: ��
  ����ֵ:   
  ����:     �ú���ֻ�ܱ��������    
*********************************************************************/
#if OS_POST_MESSAGE_EN
void OSWaitMessage(INT16U timeout)
{
    OS_ENTER_CRITICAL();
    OSTCBCur->OSTCBDly   = timeout;
    OSTCBCur->OSTCBStat |= OS_STAT_MSG;
    OSTCBCur->OSTCBMsg   = 0; 
    
    if ((OSRdyTbl[OSTCBCur->OSTCBY] &= ~OSTCBCur->OSTCBBitX) == 0)     /* Make task not ready            */
    {
        OSRdyGrp &= ~OSTCBCur->OSTCBBitY;
    }
    OS_EXIT_CRITICAL();      
}
#endif

#if OS_GET_MESSAGE_EN
INT16U OSGetMessage()
{
    INT16U msg;
    OS_ENTER_CRITICAL();
    msg = OSTCBCur->OSTCBMsg;
    OSTCBCur->OSTCBMsg = 0;
    OS_EXIT_CRITICAL(); 
    return msg;
}
#endif
