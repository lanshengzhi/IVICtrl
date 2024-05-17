
#include "main.h"

#define T2_RELOAD_H         (65536-922)/256
#define T2_RELOAD_L         (65536-922)%256

/*
********************************************************************
* Description: ���ڳ�ʼ�����ض�CPU��ص�����
* Arguments  : none
* return     : none
* Calls      : none
* Called By  : OSInit()  
* Note       :   
******************************************************************** 
*/
void OSInitMisc()
{
    CHIPCON &= ~0x10;       // Disable AUX memory
    IE = 0;                 // Disable all interrupt
    CKCON &= ~0x38;         // Timer = CPU CLK/12
    T2CON = 0;

    RCAP2H = T2_RELOAD_H;   // Set Timer 2 reload 
    RCAP2L = T2_RELOAD_L;   
    ET2 = 1;                
    TR2 = 1;                 
}

