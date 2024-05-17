#ifndef OS_CPU_H
#define OS_CPU_H
/*
************************************************************************
*                           DATA TYPES
*                       (Compiler Specific)
************************************************************************
*/
typedef bit                 BOOL;          
typedef unsigned char       INT8U;         //�޷���8λ��
typedef signed   char       INT8S;         //�з���8λ��
typedef unsigned int        INT16U;        //�޷���16λ��
typedef signed   int        INT16S;        //�з���16λ��
typedef unsigned long       INT32U;        //�޷���32λ��
typedef signed   long       INT32S;        //�з���32λ��
typedef float               FP32;          //�����ȸ�����
typedef double              FP64;          //˫���ȸ�����

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
* Description: ���ڳ�ʼ�����ض�CPU��ص�����
* Arguments  : none
* return     : none
* Calls      : none
* Called By  : OSInit()  
* Note       : ֻ�����ڲ�����  
******************************************************************** 
*/
void OSInitMisc();

/*
********************************************************************
* Description: �ڽ������֮ǰ������ض���CPU��һЩ���ã�������˿�
               �Ź��������ܵ��жϿ���
* Arguments  : none
* return     : none
* Calls      : none
* Called By  : OSStart()  
* Note       : ֻ�����ڲ�����    
******************************************************************** 
*/
#define OSPrevSched()           \
            WDTCTRL = 0x80;     \
            EA = 1

/*
********************************************************************
* Description: �ڵ��ȵĹ��̵��У�����ض���CPU��һЩ���ã������忴
               �Ź������⿴�Ź���λ
* Arguments  : none
* return     : none
* Calls      : none
* Called By  : OSStart()  
* Note       : ֻ�����ڲ�����    
******************************************************************** 
*/
#define OSUnderSched()        \
            WDTCTRL |= 0x40


#endif