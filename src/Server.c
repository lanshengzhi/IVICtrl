
#include "..\inc\main.h"

#define COM_IN_BUF_SIZE     16
#define COM_OUT_BUF_SIZE    16

typedef struct 
{
    INT8U inIndex;
    INT8U outIndex;
    INT8U size;
    INT8U idata *pBuf;    
}Queue;

static Queue idata g_comInQueue;
static INT8U idata g_comInBuf[COM_IN_BUF_SIZE];

static void UartISR();
static void AnalyseData(INT8U nData);
static void DealArmMsg(INT8U idata *pBuf, INT8U len);

static BOOL PushQueue(Queue idata *pQueue, INT8U msg) //reentrant  
{
    INT8U inIndex = 0;

    OS_ENTER_CRITICAL();
    inIndex = (pQueue->inIndex + 1)%pQueue->size;

    if (inIndex == pQueue->outIndex)
    {
        OS_EXIT_CRITICAL();
        return FALSE;   // 队列已满
    }
    
    pQueue->pBuf[pQueue->inIndex] = msg;
    pQueue->inIndex = inIndex;
    OS_EXIT_CRITICAL();

    return TRUE;
}

static BOOL PollQueue(Queue idata *pQueue, INT8U *pMsg) //reentrant
{
    OS_ENTER_CRITICAL();
    if (pQueue->inIndex == pQueue->outIndex)    // 队列是空的 
    {
        OS_EXIT_CRITICAL();
        return FALSE;
    }

    *pMsg = pQueue->pBuf[pQueue->outIndex];
    pQueue->outIndex = (pQueue->outIndex + 1)%pQueue->size;
    OS_EXIT_CRITICAL();

    return TRUE;
}