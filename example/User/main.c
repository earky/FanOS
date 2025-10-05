#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "PWM.h"
#include "os.h"
#include "I2C.h"
#include "GPIO.h"
#include "Serial.h"

char str[512];

OS_STK Task_PWM_Led_STK1[128];
OS_STK Task_PWM_Led_STK2[128];
OS_STK Task_PWM_Led_STK3[128];
OS_STK Task_PWM_Led_STK4[128];

OS_STK Task_Serial_STK[128];

//uint8_t tempData[] = {1,2,3,4,5,6,7,8,9};
uint8_t tempData[] = {9,8,7,6,5,4,3,2,1};

void PWM_Led1(void * p_arg){
		int i = 0;
		while(1){	
				for (i = 0; i <= 100; i++)
				{
					PWM_SetCompare1(i);			//依次将定时器的CCR寄存器设置为0~100，PWM占空比逐渐增大，LED逐渐变亮
					OSTimeDly(10u);				//延时10ms
				}
				for (i = 0; i <= 100; i++)
				{
					PWM_SetCompare1(100 - i);	//依次将定时器的CCR寄存器设置为100~0，PWM占空比逐渐减小，LED逐渐变暗
					OSTimeDly(10u);				//延时10ms
				}
			for(int i = 0; i<1000000; i++);		
		}
}

void PWM_Led2(void * p_arg){
		int i = 0;
		while(1){	
				for (i = 0; i <= 100; i++)
				{
					PWM_SetCompare2(i);			//依次将定时器的CCR寄存器设置为0~100，PWM占空比逐渐增大，LED逐渐变亮
					OSTimeDly(10u);				//延时10ms
				}
				for (i = 0; i <= 100; i++)
				{
					PWM_SetCompare2(100 - i);	//依次将定时器的CCR寄存器设置为100~0，PWM占空比逐渐减小，LED逐渐变暗
					OSTimeDly(10u);				//延时10ms
				}
			for(int i = 0; i<1000000; i++);		
		}
}

void PWM_Led3(void * p_arg){
		int i = 0;
		while(1){	
				for (i = 0; i <= 100; i++)
				{
					PWM_SetCompare3(i);			//依次将定时器的CCR寄存器设置为0~100，PWM占空比逐渐增大，LED逐渐变亮
					OSTimeDly(10u);				//延时10ms
				}
				for (i = 0; i <= 100; i++)
				{
					PWM_SetCompare3(100 - i);	//依次将定时器的CCR寄存器设置为100~0，PWM占空比逐渐减小，LED逐渐变暗
					OSTimeDly(10u);				//延时10ms
				}
			for(int i = 0; i<1000000; i++);		
		}
}

void PWM_Led4(void * p_arg){
		int i = 0;
		while(1){	
				for (i = 0; i <= 100; i++)
				{
					PWM_SetCompare4(i);			//依次将定时器的CCR寄存器设置为0~100，PWM占空比逐渐增大，LED逐渐变亮
					OSTimeDly(10u);				//延时10ms
				}
				for (i = 0; i <= 100; i++)
				{
					PWM_SetCompare4(100 - i);	//依次将定时器的CCR寄存器设置为100~0，PWM占空比逐渐减小，LED逐渐变暗
					OSTimeDly(10u);				//延时10ms
				}
			for(int i = 0; i<1000000; i++);		
		}
}

void Serial_Send_Count(void* p_arg){
#if OS_CRITICAL_METHOD == 3u                               /* Allocate storage for CPU status register     */
    OS_CPU_SR  cpu_sr = 0u;
#endif
	OS_TCB    *idle = OSTCBPrioTbl[OS_TASK_IDLE_PRIO];
	while(1){	
		uint32_t countNow = 0;
		uint32_t countSend = 0;
		uint32_t t = 0;
//		
//		OS_ENTER_CRITICAL();
//			countNow = ptcb->OSTCBCountNow;
//			countSend = ptcb->OSTCBCountSend;
//			t = total_count;
//		OS_EXIT_CRITICAL();
//		
//		sprintf(str, "5:\nCount_Now:%d \nCount_Send:%d \nTotal_Count:%d \n", 
//				countNow, countSend, t);

//		Serial_SendString(str);
//		
//		OS_ENTER_CRITICAL();
//			countNow = p->OSTCBCountNow;
//			countSend = p->OSTCBCountSend;
//			t = total_count;
//		OS_EXIT_CRITICAL();
//		sprintf(str, "4:\nCount_Now:%d \nCount_Send:%d \nTotal_Count:%d \n", 
//				countNow, countSend, t);
//		Serial_SendString(str);
//		
		
		OS_ENTER_CRITICAL();
			countNow = idle->OSTCBCountNow;
			countSend = idle->OSTCBCountSend;
			t = total_count;
		OS_EXIT_CRITICAL();
		sprintf(str, "idle:\nCount_Now:%d \nCount_Send:%d \nTotal_Count:%d \n", 
				countNow, countSend, t);
		Serial_SendString(str);
		
		if(OSCoreID == 0)
		{
				OS_ENTER_CRITICAL();
				uint8_t status = OS_GetCpuUsage(I2C_SLAVE_ADDRESS1, (uint16_t*)&countSend);
				t = total_count;
				OS_EXIT_CRITICAL();
				
				if(status != RES_OK){
					Serial_SendString("get cpu usage error1\n");
				}
				sprintf(str, "recv-idle1:\nCount_Send:%d \n", 
						countSend);
				Serial_SendString(str);
				
				OS_ENTER_CRITICAL();
				status = OS_GetCpuUsage(I2C_SLAVE_ADDRESS2, (uint16_t*)&countSend);
				t = total_count;
				OS_EXIT_CRITICAL();
				
				if(status != RES_OK){
					Serial_SendString("get cpu usage error2\n");
				}
				sprintf(str, "recv-idle2:\nCount_Send:%d \n", 
						countSend);
				Serial_SendString(str);
		}
		OSTimeDly(2000u);	
	}
	
}

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
OS_STK T[128];
uint8_t Data[] = {1,2,3,4,5,6,7,8,9};
void OS_Data_Transfer_Switch_Callback(uint8_t type){
		switch(type){
			case 0:
				Serial_SendString("OS_Data_Transfer_Switch_Callback 0\n");
				break;
			case 1:
				//sprintf(str, " real_addr:%x\n", Data);
				//Serial_SendString(str);
				Serial_SendString("OS_Data_Transfer_Switch_Callback 1\n");
				for(int i=0;i<9;i++){
						sprintf(str, "%u ", Data[i]);
						Serial_SendString(str);
				}
				Serial_SendString("\n");
				break;
			default:
			
				break;
		}
}

void Send_Shared_Memory_data(void* p_arg){
		OSTimeDly(1000u);
		while(1){
			
			for(int i=0;i<9;i++)
				Data[i]++;
			
			Serial_SendString("Send_Shared_Memory_data\n");
			OS_SendData(Data, 9, 1);
			OSTimeDly(1000u);
		}	
}

int main(void)
{
	
	PWM_Init();			//PWM初始化
	Serial_Init();	//Serial初始化
	Serial_SendString("hello\n");

	OSInit();

	//OSTaskCreate(PWM_Led1,           (void *)0, &Task_PWM_Led_STK1[127] , 7,  0 ,SPECIFIC_FALSE);
	//OSTaskCreate(PWM_Led2,           (void *)0, &Task_PWM_Led_STK2[127] , 8,  0 ,SPECIFIC_FALSE);
	//OSTaskCreate(PWM_Led3,           (void *)0, &Task_PWM_Led_STK3[127] , 9,  0 ,SPECIFIC_FALSE);
	//OSTaskCreate(PWM_Led4,           (void *)0, &Task_PWM_Led_STK4[127] , 10, 0 ,SPECIFIC_FALSE);
	//OSTaskCreate(Serial_Send_Count,  (void *)0, &Task_Serial_STK[127]  , 5, 0, SPECIFIC_TRUE);
	//OSTaskCreate(Send_Shared_Memory_data, (void *)0, &T[127]  , 6, 1, SPECIFIC_TRUE);
	SysTick_Config(40000u);
	OSStart();
	return 0;
}

