#ifndef OS_CORE_H
#define OS_CORE_H

#define OS_PRIO_SELF            0xFF

/*
******************************************************
* 系统消息
******************************************************
*/
#define OS_MSG_NONE     0x0000
#define OS_MSG_TIMEOUT  0x0001
#define OS_MSG_USER     0x1000

/*
******************************************************
* ERROR CODES
******************************************************
*/
#define OS_NO_ERR                 0
#define OS_ERR_EVENT_TYPE         1
#define OS_ERR_PEND_ISR           2

#define OS_TIMEOUT               10
#define OS_TASK_NOT_EXIST        11


#define OS_PRIO_EXIST            40
#define OS_PRIO_ERR              41
#define OS_PRIO_INVALID          42

#define OS_TASK_DEL_ERR          60
#define OS_TASK_DEL_IDLE         61
#define OS_TASK_DEL_REQ          62
#define OS_TASK_DEL_ISR          63

#define OS_TIME_NOT_DLY          80
#define OS_TIME_INVALID_MINUTES  81
#define OS_TIME_INVALID_SECONDS  82
#define OS_TIME_INVALID_MILLI    83
#define OS_TIME_ZERO_DLY         84

#define OS_TASK_SUSPEND_PRIO     90
#define OS_TASK_SUSPEND_IDLE     91

#define OS_TASK_RESUME_PRIO     100
#define OS_TASK_NOT_SUSPENDED   101

#define OS_TASK_OPT_ERR         130


#define OS_LOWEST_PRIO          OS_MAX_TASKS    
#define OS_IDLE_PRIO            OS_LOWEST_PRIO

#define OS_EVENT_TBL_SIZE       ((OS_LOWEST_PRIO)/8+1)
#define OS_RDY_TBL_SIZE         ((OS_LOWEST_PRIO)/8+1)

/*
******************************************************************** 
任务状态 
******************************************************************** 
*/
#define  OS_STAT_RDY            0x00        /* Ready to run                                            */
#define  OS_STAT_SEM            0x01        /* Pending on semaphore                                    */
#define  OS_STAT_MBOX           0x02        /* Pending on mailbox                                      */
#define  OS_STAT_Q              0x04        /* Pending on queue                                        */
#define  OS_STAT_SUSPEND        0x08        /* Task is suspended                                       */
#define  OS_STAT_MUTEX          0x10        /* Pending on mutual exclusion semaphore                   */
#define  OS_STAT_FLAG           0x20        /* Pending on event flag gro*/
#define  OS_STAT_MSG            0x40
/*
********************************************************************
事件控制块:
********************************************************************
*/
#if (OS_MAX_EVENTS >= 1)
typedef struct {         
    INT8U   OSEventTbl[OS_EVENT_TBL_SIZE]; /* List of tasks waiting for event to occur                 */
    INT8U   OSEventGrp;                    /* Group corresponding to tasks waiting for event to occur  */
    INT8U   OSEventCnt;
    INT8U   OSEventOwner;
} OS_EVENT;
#endif



/*
********************************************************************
任务控制块:
********************************************************************
*/
typedef struct
{
    void     (* pfnTask)();   // 指向任务

    INT16U    OSTCBMsg;        // 任务消息
    INT16U    OSTCBDly;        // 任务延时
    INT8U     OSTCBStat;       // 任务状态
    INT8U     OSTCBPrio;       // 任务优先级

    INT8U     OSTCBX;          // 任务
    INT8U     OSTCBY;          // 任务所在的组
    INT8U     OSTCBBitX;
    INT8U     OSTCBBitY;
}OS_TCB;


/*
********************************************************************
函数接口
********************************************************************
*/
void   OSInit(void); 
INT8U  OSTaskCreate(void (*pfnTask)(), INT8U prio);
void   OSStart(void);
void   OSTaskIdle();

INT16U OSVersion();
void   OSTimeDly(INT16U ticks);
INT8U  OSSendMessage(INT8U prio, INT16U msg);
INT8U  OSPostMessage(INT8U prio, INT16U msg);
void   OSWaitMessage(INT16U timeout);
INT16U OSGetMessage();

OS_EVENT idata *OSMutexCreate(INT8U index);
BOOL OSMutexPend(OS_EVENT idata *pevent, INT16U timeout);
BOOL OSMutexPost(OS_EVENT idata *pevent);


#endif
