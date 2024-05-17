#ifndef OS_CPU_H
#define OS_CPU_H
/*
************************************************************************
*                           DATA TYPES
*                       (Compiler Specific)
************************************************************************
*/
typedef bit                 BOOL;          
typedef unsigned char       INT8U;         //无符号8位数
typedef signed   char       INT8S;         //有符号8位数
typedef unsigned int        INT16U;        //无符号16位数
typedef signed   int        INT16S;        //有符号16位数
typedef unsigned long       INT32U;        //无符号32位数
typedef signed   long       INT32S;        //有符号32位数
typedef float               FP32;          //单精度浮点数
typedef double              FP64;          //双精度浮点数

typedef unsigned char       BYTE;        

#ifndef NULL
#define NULL                0
#endif

#define OSEnterIdle()       PCON |= 0x01

#define OS_CLEAR_TICK_INT   TF2 = 0         // Clear T2 interrupt flag 

#define OS_ENTER_CRITICAL() EA = 0          // Disable interrupts                                   
#define OS_EXIT_CRITICAL()  EA = 1          // Enable interrupts

/*
********************************************************************
* Description: 用于初始化与特定CPU相关的设置
* Arguments  : none
* return     : none
* Calls      : none
* Called By  : OSInit()  
* Note       : 只用于内部调用  
******************************************************************** 
*/
void OSInitMisc();

/*
********************************************************************
* Description: 在进入调度之前，针对特定的CPU做一些设置，这里打开了看
               门狗，并打开总的中断控制
* Arguments  : none
* return     : none
* Calls      : none
* Called By  : OSStart()  
* Note       : 只用于内部调用    
******************************************************************** 
*/
#define OSPrevSched()           \
            WDTCTRL = 0x80;     \
            EA = 1

/*
********************************************************************
* Description: 在调度的过程当中，针对特定的CPU做一些设置，这里清看
               门狗，避免看门狗复位
* Arguments  : none
* return     : none
* Calls      : none
* Called By  : OSStart()  
* Note       : 只用于内部调用    
******************************************************************** 
*/
#define OSUnderSched()        \
            WDTCTRL |= 0x40


#endif