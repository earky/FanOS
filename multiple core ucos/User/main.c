#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "PWM.h"
#include "os.h"
#include "Serial.h"

OS_STK Task_PWM_Led_STK[128];
OS_STK Task_Serial_STK[128];
char sstr[10];

void PWM_Led(void * p_arg){
	
		int i = 0;
	
		while(1){	
//				for (i = 0; i <= 100; i++)
//				{
//					PWM_SetCompare1(i);			//依次将定时器的CCR寄存器设置为0~100，PWM占空比逐渐增大，LED逐渐变亮
//					OSTimeDly(1000u);				//延时10ms
//				}
//				for (i = 0; i <= 100; i++)
//				{
//					PWM_SetCompare1(100 - i);	//依次将定时器的CCR寄存器设置为100~0，PWM占空比逐渐减小，LED逐渐变暗
//					OSTimeDly(1000u);				//延时10ms
//				}
			sprintf(sstr, "\n> %d\n", i++);
			Serial_SendString(sstr);
			OSTimeDly(10000u);
		}
}

char str[512] = "123\n";
void Serial_Send_Stack(void* p_arg){
	
	OS_TCB    *ptcb = OSTCBPrioTbl[5];
	while(1){
		sprintf(str, ">\n%X\n%X\n", (unsigned int)ptcb->OSTCBStkBasePtr, (unsigned int)ptcb->OSTCBStkPtr);
		Serial_SendString(str);
		
		//typedef unsigned int   OS_STK;  
		//sprintf(str, "%#x,%#x,%#x,%#x"

		unsigned int sz = (unsigned int)ptcb->OSTCBStkBasePtr - (unsigned int)ptcb->OSTCBStkPtr;
		sprintf(str, "\n sz:%d\n", sz);
		Serial_SendString(str);
		//str[0] = '\0';
		sprintf(str, "");
		
		OS_STK* ptr = ptcb->OSTCBStkPtr;
		for(;ptr<=ptcb->OSTCBStkBasePtr;ptr++){
			//sprintf(str, "> %#04x\n", *(OS_STK*)((unsigned int)ptcb->OSTCBStkPtr + i) & 0x0000FFFF);
			//sprintf(str, "%#04x,", *(OS_STK*)((unsigned int)ptcb->OSTCBStkPtr + i) & 0x0000FFFF);
			sprintf(str, "%#04x,", *ptr);
			Serial_SendString(str);
		}
		Serial_SendByte('\n');
		//Serial_SendString(str);
		OSTimeDly(10000u);	
	}
	
}

OS_STK Stack[] = {
0000,0x2710,0000,0x7070707,0x8080808,0x9090909,0x10101010,0x11111111,
0xfffffffd,0000,0x200002e9,0x40013800,0x80,0x12121212,0x800152d,
0x800152c,0x21000000,0x8a,0x5050505,0x6060606,0x800224f,
};

int main(void)
{
	PWM_Init();			//PWM初始化
	Serial_Init();	//Serial初始化
	
	OSInit();
	OSTaskCreate(PWM_Led,           (void *)0, &Task_PWM_Led_STK[127] , 5);
	
	char s[100];
	OS_STK len = sizeof(Stack) / sizeof(OS_STK);
	
	OS_TCB* ptcb = OSTCBPrioTbl[5];
	OS_STK* p_stk = ptcb->OSTCBStkBasePtr + 1u;
	
	for(int i = len - 1;i >= 0; i--){
		*(--p_stk) = Stack[i];
	}
	
	ptcb->OSTCBStkPtr = p_stk;
//	for(OS_STK i=0;i<len;i++){
//		Task_PWM_Led_STK[127 - i] = Stack[len - i - 1];
//	}
//	
//	ptcb->OSTCBStkPtr = (OS_STK*) (((OS_STK)ptcb->OSTCBStkBasePtr + 4) - len * 2);
	
	sprintf(s, " len:%d\n ptr:%#x", len, (OS_STK)ptcb->OSTCBStkPtr);
	Serial_SendString(s);
	
	OSTaskCreate(Serial_Send_Stack, (void *)0, &Task_Serial_STK[127]  , 4);
	
	SysTick_Config(1000u);
	OSStart();
	return 0;
}
