
#include "main.h"
#include <string.h>

/*
********************************************************************
本文件全局变量
******************************************************************** 
*/ 
static INT8U          OSRdyGrp;                     // Ready list group 
static INT8U    idata OSRdyTbl[OS_RDY_TBL_SIZE];    // Table of tasks which are ready to run
#if OS_MAX_EVENTS
static OS_EVENT idata OSEventTbl[OS_MAX_EVENTS];
#endif
static OS_TCB   idata *OSTCBCur;                    // 指向当前任务块
static OS_TCB   idata OSTCBTbl[OS_LOWEST_PRIO + 1]; // 任务控制块数组

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
  函数功能: 通知等待该事件的任务进入就绪状态
  输入参数: 无
  输出参数: 无
  返回值:   无
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

    y    = OSUnMapTbl[pevent->OSEventGrp];      // 找出等待该事件优先级最高的任务    
    bity = OSMapTbl[y];
    x    = OSUnMapTbl[pevent->OSEventTbl[y]];
    bitx = OSMapTbl[x];
    prio = (y << 3) + x;               

    if ((pevent->OSEventTbl[y] &= ~bitx) == 0)  // 把任务从等待列表中清除
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
  函数功能: 使任务进入事件的等待列表，同时挂起任务
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
  函数功能: 初始化事件的等待列表
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
  函数功能: 获取事件控制快
******************************************************************** 
*/
OS_EVENT idata *OSMutexCreate(INT8U index)
{
    OS_EVENT idata *pevent = OSEventTbl+index;

    pevent->OSEventCnt = 0xff;      // 标志资源可用
    OSEventWaitListInit(pevent);

    return pevent;  
}

BOOL OSMutexPend(OS_EVENT idata *pevent, INT16U timeout)
{
    OS_ENTER_CRITICAL();

    if (pevent->OSEventCnt == 0xff)     // 如果为0xff，说明资源可用
    {
        pevent->OSEventOwner = OSTCBCur->OSTCBPrio;  // 设置拥有该互斥量的任务
        pevent->OSEventCnt = 0;         // 把互斥量标志为不可用
        OS_EXIT_CRITICAL();     
        return TRUE;
    }

    OSTCBCur->OSTCBStat |= OS_STAT_MUTEX;   // 资源不可用，任务进入休眠
    OSTCBCur->OSTCBDly   = timeout;         // 超时
    OSEventTaskWait(pevent);                // 进入等待列表
    OS_EXIT_CRITICAL();

    return FALSE;
}

/*
 应该是拥有该互斥量的任务，才能调用MutexPost
*/
BOOL OSMutexPost(OS_EVENT idata *pevent)
{
    OS_ENTER_CRITICAL();

    if (pevent->OSEventOwner != OSTCBCur->OSTCBPrio)    // 这里确保只有拥有Mutex的任务才能
    {
        OS_EXIT_CRITICAL();
        return FALSE;
    }

    if (pevent->OSEventGrp)         // 还有任务在等待这个事件
    {
        OSEventTaskRdy(pevent, OS_STAT_MUTEX);  // 使正在等待该事件的任务进入就绪状态
    }
    else
    {
        pevent->OSEventCnt = 0xff;      // 设置资源可用
    }

    OS_EXIT_CRITICAL();

    return TRUE;
}

/*
********************************************************************
  函数名称: void OSInit()
  函数功能: 操作系统初始化
  输入参数: 无
  输出参数: 无
  返回值:   无
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

    OSInitMisc();                           // 初始化定时器中断,在文件os_cpu.c中定义 
    OSTaskCreate(OSTaskIdle, OS_IDLE_PRIO); // 初始化空闲任务，其优先级最低，且永远处于就绪状态
}

/*
********************************************************************
* 函数名称: void OSTimeTick()
* 函数功能: 把标志位gTickFlag置1, 表明1ms时间到， 由定时中断函数调用
* 输入参数: 无
* 输出参数: 无    
* 返回值:   无
* 其他:    
******************************************************************** 
*/ 
void OSTimeTick() interrupt 5
{
    INT8U prio;
    OS_TCB idata *ptcb = OSTCBTbl; 

    OS_CLEAR_TICK_INT;              // 清定时中断标志 

    OS_ENTER_CRITICAL();
    for (prio = 0; prio < OS_LOWEST_PRIO; ++prio)      // 遍历所有任务块
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
  函数名称: BOOL OSTaskCreate(void (*pfnTask)(void), INT8U prio)
  函数功能: 创建任务 
  输入参数: *pfnTask 指向要创建的任务(函数); prio-->任务优先级
  输出参数: 无  
  返回值:   1-->创建成功；0-->创建失败
  其他:     
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
  函数名称: void OSStart(void)
  函数功能: 启动操作系统，并进行调度
  输入参数: 无
  输出参数: 无
  返回值:   无
  其他:     
*********************************************************************/
void OSStart(void)
{  
    INT8U nPrioHighRdy;
    INT8U y;
    INT8U x;

    OSPrevSched();      // 调度前，做特定MCU相关的设置，如：打开中断，打开看门狗等

    while (1)
    {
        y = OSUnMapTbl[OSRdyGrp];           // 找出优先级最高且就绪的任务
        x = OSUnMapTbl[OSRdyTbl[y]];
        nPrioHighRdy = (y << 3) + x;

        OSTCBCur = &OSTCBTbl[nPrioHighRdy]; // OSTCBCur指向当前任务块

        if (OSTCBCur->pfnTask)
        {
            OSTCBCur->pfnTask();            // 运行任务
        }

        OSUnderSched();     // 调度过程，做特定MCU相关的设置，如：看门狗
    } 
}

/*********************************************************************
  函数名称: void OSTaskIdle()
  函数功能: 当没有优先级更高的任务时，就运行OSTaskIdle()
  输入参数: 无
  输出参数: 无
  返回值:   无
  其他: 注意！空闲任务永远处于就绪状态    
*********************************************************************/
void OSTaskIdle()
{
    //OSEnterIdle();      // 使CPU进入空闲模式，当有中断时，程序将往下执行   
}  

/*********************************************************************
  函数名称: void OSTimeDly(INT16U ticks)
  函数功能: 延时当前任务
  输入参数: ticks -->延时个数，单位为ms
  输出参数: 无
  返回值:   无
  其他: 注意！该函数只能被任务调用    
*********************************************************************/
#if OS_TIME_DLY_EN
void OSTimeDly(INT16U ticks)
{
    OS_ENTER_CRITICAL();
    // 把当前任务从就绪表中移除
    if ((OSRdyTbl[OSTCBCur->OSTCBY] &= ~OSTCBCur->OSTCBBitX) == 0)  
    {
        OSRdyGrp &= ~OSTCBCur->OSTCBBitY;   
    }
    
    OSTCBCur->OSTCBDly  = ticks;
    OS_EXIT_CRITICAL();
}
#endif

/*********************************************************************
  函数名称: INT8U OSPostMessage(INT8U prio, INT16U msg)
  函数功能: 发送消息给优先级为prio的任务, 
            然后任务通过OSGetMessage可以查询这个msg
  输入参数: prio, msg
  输出参数: 无
  返回值:   
  其他:     
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
    // 使任务进入就绪状态
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
    // 使任务进入就绪状态
    OSRdyGrp               |= ptcb->OSTCBBitY;
    OSRdyTbl[ptcb->OSTCBY] |= ptcb->OSTCBBitX; 

    OS_EXIT_CRITICAL();

    return OS_NO_ERR;        
}
#endif
/*********************************************************************
  函数名称: INT8U OSMsgPend(INT16U timeout)
  函数功能: 发送消息给优先级为prio的任务
  输入参数: prio, msg
  输出参数: 无
  返回值:   
  其他:     该函数只能被任务调用    
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
