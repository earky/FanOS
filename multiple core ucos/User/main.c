#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "PWM.h"
#include "os.h"
#include "I2C.h"
#include "GPIO.h"
#include "Serial.h"
#include "SPI.h"

int ID = 0;
char str[512] = "123\n";

int mainttt(){
		Serial_Init();
		Serial_SendString("hello\n");
		
		if(ID == 0){
				GPIO_EXTI_Init();
			
				uint8_t* p = (uint8_t*) 0x200000c0;
				uint32_t addr = (uint32_t) p;
			
				sprintf(str,"%x\n", addr);
				Serial_SendString(str);
				while(1){}
		}else{
				Slave_GPIO_Init();
				
			
				Slave_SendData();
				for(int i=0;i<100000;i++);
				Slave_SendData();
				while(1){
						//GPIO_SetBits(GPIOB, GPIO_Pin_12);
						
						for(int i=0;i<100000;i++);
						//Slave_SendData();
						//GPIO_SetBits(GPIOB, GPIO_Pin_12);	
						//GPIO_ResetBits(GPIOB, GPIO_Pin_12);
						
				}
		}
}


OS_STK Task_PWM_Led_STK[128];
OS_STK Task_Serial_STK[128];


//uint8_t Task_Switch_Buffer[512];


//char sstr[10];

//uint8_t tempData[] = {1,2,3,4,5,6,7,8,9};
uint8_t tempData[] = {9,8,7,6,5,4,3,2,1};
int id = 1;



/* 
							Task switch
		if( max_count -  min_count > diff_count){
							min_id -- > max_id 
		}
*/



void PWM_Led(void * p_arg){
		
		int i = 0;
	
		while(1){	
				for (i = 0; i <= 100; i++)
				{
					PWM_SetCompare1(i);			//依次将定时器的CCR寄存器设置为0~100，PWM占空比逐渐增大，LED逐渐变亮
					OSTimeDly(100u);				//延时10ms
				}
				for (i = 0; i <= 100; i++)
				{
					PWM_SetCompare1(100 - i);	//依次将定时器的CCR寄存器设置为100~0，PWM占空比逐渐减小，LED逐渐变暗
					OSTimeDly(100u);				//延时10ms
				}
			//sprintf(str, "\n> %d\n", i++);
			//Serial_SendString(str);
			for(int i = 0; i<1000000; i++);
					
			OSTimeDly(1000u);
		}
}

void Serial_Send_Count(void* p_arg){
#if OS_CRITICAL_METHOD == 3u                               /* Allocate storage for CPU status register     */
    OS_CPU_SR  cpu_sr = 0u;
#endif
	OS_TCB    *ptcb = OSTCBPrioTbl[5];
	OS_TCB    *p    = OSTCBPrioTbl[4];
	OS_TCB    *idle = OSTCBPrioTbl[OS_TASK_IDLE_PRIO];
	while(1){	
		uint32_t countNow = 0;
		uint32_t countSend = 0;
		uint32_t t = 0;
		
		OS_ENTER_CRITICAL();
			countNow = ptcb->OSTCBCountNow;
			countSend = ptcb->OSTCBCountSend;
			t = total_count;
		OS_EXIT_CRITICAL();
		
		sprintf(str, "5:\nCount_Now:%d \nCount_Send:%d \nTotal_Count:%d \n", 
				countNow, countSend, t);

		Serial_SendString(str);
		
		OS_ENTER_CRITICAL();
			countNow = p->OSTCBCountNow;
			countSend = p->OSTCBCountSend;
			t = total_count;
		OS_EXIT_CRITICAL();
		sprintf(str, "4:\nCount_Now:%d \nCount_Send:%d \nTotal_Count:%d \n", 
				countNow, countSend, t);
		Serial_SendString(str);
		
		
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
				uint8_t status = OS_GetCpuUsage(I2C_SLAVE_ADDRESS, (uint16_t*)&countSend);
				t = total_count;
				OS_EXIT_CRITICAL();
				
				if(status != RES_OK){
					Serial_SendString("get cpu usage error\n");
				}
				sprintf(str, "recv-idle:\nCount_Send:%d \n\n\n", 
						countSend);
				Serial_SendString(str);
		}
		OSTimeDly(100000u);	
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

OS_STK Stack3[] = {
0000,0x2710,0000,0x7070707,0x8080808,0x9090909,0x10101010,
0x11111111,0xfffffffd,0000,0x20000295,0x40013800,0x80,0x12121212,
0x800152d,0x800152c,0x21000000,0x6e,0x5050505,0x6060606,0x800228b,0000
};

OS_STK Stack_len = sizeof(Stack3) / sizeof(OS_STK);

int main(void)
{
	PWM_Init();			//PWM初始化
	Serial_Init();	//Serial初始化
	

	
	Serial_SendString("hello\n");

	OSInit();

	
	OSTaskCreate(PWM_Led,           (void *)0, &Task_PWM_Led_STK[127] , 4, 0 ,SPECIFIC_FALSE);
	
	char s[100];
	OS_STK len = sizeof(Stack3) / sizeof(OS_STK);
	len = len;
	
	OS_TCB* ptcb = OSTCBPrioTbl[5];
//	OS_STK* p_stk = ptcb->OSTCBStkBasePtr + 1u;
//	
//	for(int i = len - 1;i >= 0; i--){
//		*(--p_stk) = Stack[i];
//	}
//	
//	ptcb->OSTCBStkPtr = p_stk;
	//OS_Multiple_Task_SW(5, Stack3, len);
	
	sprintf(s, " len:%d\n ptr:%#x", len, (OS_STK)ptcb->OSTCBStkPtr);
	Serial_SendString(s);
	
	//OSTaskCreate(Serial_Send_Stack, (void *)0, &Task_Serial_STK[127]  , 4);
	OSTaskCreate(Serial_Send_Count, (void *)0, &Task_Serial_STK[127]  , 5, 0, SPECIFIC_TRUE);
	//if(OSCoreID == 0)
	SysTick_Config(2000u);
	OSStart();
	return 0;
}


//int arr[5];
//int main346(){
//	SPI_Master_Init();
//	
////	SPI_Start();
////	for(int i=0;i<1000000;i++);
////	SPI_Stop();
////	
////	while(1){}
//		
//	Serial_Init();
//	Serial_SendString("hello\n");
//	char str[30];
//	uint8_t arr[100];
//	uint8_t j = 0;
//	
//	uint32_t size;
//	
//	
//	uint8_t status = OS_GetStackData(8, arr, &size);
//	
////	OS_CommStart();
////	uint32_t ssize = OS_ReqSize(2);
////	OS_CommStop();
//	
//	sprintf(str, "size:%u\n", size);
//	Serial_SendString(str);
//	sprintf(str, "status:%u\n", status);
//	Serial_SendString(str);
//	
//	Serial_SendString("\n\nhello\n");
//	status = OS_GetStackData(8, arr, &size);
//	sprintf(str, "size:%u\n", size);
//	Serial_SendString(str);
//	sprintf(str, "status:%u\n", status);
//	Serial_SendString(str);
//	//for(uint32_t i = 0; i<size;i++)
//	while(1){}
//		
//		
////	while(1){
////		SPI_Start();
////		uint8_t x = SPI_SwapByte(j);
////		SPI_Stop();
////		sprintf(str, "%d-%d\n",j, x);
////		j ++;
////		Serial_SendString(str);
//////		for(int i=0;i<100000;i++){
//////		}
////	}
//	
//	return 0;
//}

//int main00(){
//	SPI_Slave_Init();
//	Serial_Init();
//	Serial_SendString("hello\n");
//	while(1){
//		
//		for(int i=0;i<1000000;i++){
//		}
//	}
//}


//// 发送size_l 再发送给size_h
//void sendData(uint8_t* buf, uint32_t size)
//{
//		char str[20];
//		for(uint32_t i = 0; i<size; i++){
//				sprintf(str, "%u\n", buf[i]);
//				Serial_SendString(str);
//		}
//}
//int mainTTT(){
//		Serial_Init();
//	
//		uint32_t size = 4578;
//		uint16_t tsize = (uint16_t)size;
//		sendData( (uint8_t*)&tsize, 2);
//	
//		uint8_t size_h = 17;
//		uint8_t size_l = 226;
//		uint32_t Size = (size_h) << 8 | size_l;
//		sprintf(str, "%d\n", Size);
//		Serial_SendString(str);
//		while(1){}

//			
//}

//char str[128];
//int mainGetStackMaster(void)
//{
//    // 系统初始化
//    SystemInit();    
//    // 初始化I2C主机
//    I2C2_Master_Init();
//    Serial_Init();
//	
//		Serial_SendString("master\n");
//		uint8_t buf[128];
//		uint16_t size;
//		INT8U prio;
//	
//		uint8_t x = OS_GetStackData(&prio, I2C_SLAVE_ADDRESS, buf, &size);
//		sprintf(str, "%d %d %d\n", prio, x, size);
//		Serial_SendString(str);
//		for(uint8_t i = 0;i<size;i++){
//			sprintf(str,"%x ", buf[i]);
//			Serial_SendString(str);
//		}
//		
//		
//	
//		while(1){}
//}


//int mainGetStackSlave(void)
//{
//		// 系统初始化
//    SystemInit();  
//    // 初始化I2C从机
//    I2C2_Slave_Init(I2C_SLAVE_ADDRESS);
//		Serial_Init();
//		char str[200];
//	
//		Serial_SendString("Hello\n");
//		
//		
//		while(1){
//				//sprintf(str, "%d\n", iflag.size_recv);
//				for(int i = 0; i<10000000; i++);
//		}
//}

//int mainSendStackMaster(void)
//{
//		// 系统初始化
//    SystemInit();    
//    // 初始化I2C主机
//    I2C2_Master_Init();
//    Serial_Init();
//	
//		Serial_SendString("master\n");
//		
//		uint8_t x = OS_SendStackData(0, I2C_SLAVE_ADDRESS, (uint8_t*) Stack3, sizeof(Stack3));
//		sprintf(str, "%d", x);
//		Serial_SendString(str);
//	
//		while(1);
//}

//int mainSendStackSlave(void)
//{
//		// 系统初始化
//    SystemInit();  
//    // 初始化I2C从机
//    I2C2_Slave_Init(I2C_SLAVE_ADDRESS);
//		Serial_Init();
//		char str[200];
//	
//		Serial_SendString("Hello\n");
//		
//		
//		while(1){
//			sprintf(str, "\nrcv:%d\n", iflag.size_recv);
//				Serial_SendString(str);
//			
//				for(uint8_t i = 0;i<iflag.size_recv;i++){
//					sprintf(str,"%x ", iflag.ptr_recv[i]);
//					Serial_SendString(str);
//				}
//				for(int i = 0; i<1000000; i++);
//		}
//}



//uint8_t getVdata[20] = {1,2,3,4,5,6,7,8,9,1,2,3,4,5,6};
//int mainGetVariableMaster(void)
//{
//		// 系统初始化
//    SystemInit();    
//    // 初始化I2C主机
//    I2C2_Master_Init();
//    Serial_Init();
//	
//		Serial_SendString("master\n");
//		uint8_t buf[128];
//		uint16_t size;
//		uint32_t address;
//	
//		INT8U target_prio;		
//	
//		uint8_t x = OS_GetVariableData(I2C_SLAVE_ADDRESS, &target_prio, buf, &size, &address);
//		
//		sprintf(str, "prio:%d \nsize:%d \nts_address:%x \nmy_address:%x\n", target_prio, size, address, getVdata);
//		Serial_SendString(str);
//		for(uint8_t i = 0;i<size;i++){
//			sprintf(str,"%x ", buf[i]);
//			Serial_SendString(str);
//		}
//		
//		
//	
//		while(1){}

//}

//int mainGetVariableSlave(void)
//{
//		// 系统初始化
//    SystemInit();  
//    // 初始化I2C从机
//    I2C2_Slave_Init(I2C_SLAVE_ADDRESS);
//	
//		iflag.ptr_send = getVdata;
//		iflag.prio_send = 10;
//		iflag.size_send = sizeof(getVdata);
//		
//		Serial_Init();
//		Serial_SendString("Hello\n");
//		
//		while(1){
//				//sprintf(str, "%d\n", iflag.size_recv);
//				for(int i = 0; i<10000000; i++);
//		}
//}


////uint8_t sendVData[20] = {1,2,3,4,5,6,1,2,3,4};
//uint8_t sendVData[20] = {7,6,5,4,3,2,1,4,3,2};
//int mainSendVariableMaster(void)
//{
//		// 系统初始化
//    SystemInit();    
//    // 初始化I2C主机
//    I2C2_Master_Init();
//    Serial_Init();
//	
//		Serial_SendString("master\n");
//		uint16_t size = sizeof(sendVData);
//	
//		uint8_t x = OS_SendVariableData(3, sendVData, size, (uint32_t)sendVData);
//		//uint8_t OS_SendVariableData(INT8U prio, uint8_t* buf, uint16_t size, uint32_t address)
//		sprintf(str, "x = %d\n", x);
//	
//		Serial_SendString(str);
//		for(uint8_t i = 0;i<size;i++){
//	
//		}
//	
//		while(1){}
//}

//int mainSendVariableSlave(void)
//{
//		// 系统初始化
//    SystemInit();  
//    // 初始化I2C从机
//    I2C2_Slave_Init(I2C_SLAVE_ADDRESS);
//		Serial_Init();
//		char str[200];
//	
//		Serial_SendString("Slave\n");
//		while(1){
//				sprintf(str, "\nprio:%d \nsize:%d \nts_address:%x \nmy_address:%x\n", 
//					iflag.prio_recv, iflag.size_recv, (uint32_t)iflag.ptr_recv, (uint32_t)sendVData);
//				Serial_SendString(str);
////				for(uint8_t i = 0;i<iflag.size_recv;i++){
////					sprintf(str,"%x ", iflag.ptr_recv[i]);
////					Serial_SendString(str);
////				}
//			
////				for(uint8_t i = 0;i<iflag.size_recv;i++){
////					sprintf(str,"%x ", sendVData[i]);
////					Serial_SendString(str);
////				}
//				
//				for(uint8_t i = 0;i<sizeof(sendVData);i++){
//					sprintf(str,"%x ", sendVData[i]);
//					Serial_SendString(str);
//				}
//				for(int i = 0; i<1000000; i++);
//		}
//}
