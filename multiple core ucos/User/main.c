#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "PWM.h"
#include "os.h"
#include "I2C.h"
#include "Serial.h"
#include "SPI.h"

//OS_STK Task_PWM_Led_STK[128];
//OS_STK Task_Serial_STK[128];
//char sstr[10];

//void PWM_Led(void * p_arg){
//	
//		int i = 0;
//	
//		while(1){	
////				for (i = 0; i <= 100; i++)
////				{
////					PWM_SetCompare1(i);			//依次将定时器的CCR寄存器设置为0~100，PWM占空比逐渐增大，LED逐渐变亮
////					OSTimeDly(1000u);				//延时10ms
////				}
////				for (i = 0; i <= 100; i++)
////				{
////					PWM_SetCompare1(100 - i);	//依次将定时器的CCR寄存器设置为100~0，PWM占空比逐渐减小，LED逐渐变暗
////					OSTimeDly(1000u);				//延时10ms
////				}
//			sprintf(sstr, "\n> %d\n", i++);
//			Serial_SendString(sstr);
//			OSTimeDly(10000u);
//		}
//}

//char str[512] = "123\n";
//void Serial_Send_Stack(void* p_arg){
//	
//	OS_TCB    *ptcb = OSTCBPrioTbl[5];
//	while(1){
//		sprintf(str, ">\n%X\n%X\n", (unsigned int)ptcb->OSTCBStkBasePtr, (unsigned int)ptcb->OSTCBStkPtr);
//		Serial_SendString(str);
//		
//		//typedef unsigned int   OS_STK;  
//		//sprintf(str, "%#x,%#x,%#x,%#x"

//		unsigned int sz = (unsigned int)ptcb->OSTCBStkBasePtr - (unsigned int)ptcb->OSTCBStkPtr;
//		sprintf(str, "\n sz:%d\n", sz);
//		Serial_SendString(str);
//		//str[0] = '\0';
//		sprintf(str, "");
//		
//		OS_STK* ptr = ptcb->OSTCBStkPtr;
//		for(;ptr<=ptcb->OSTCBStkBasePtr;ptr++){
//			//sprintf(str, "> %#04x\n", *(OS_STK*)((unsigned int)ptcb->OSTCBStkPtr + i) & 0x0000FFFF);
//			//sprintf(str, "%#04x,", *(OS_STK*)((unsigned int)ptcb->OSTCBStkPtr + i) & 0x0000FFFF);
//			sprintf(str, "%#04x,", *ptr);
//			Serial_SendString(str);
//		}
//		Serial_SendByte('\n');
//		//Serial_SendString(str);
//		OSTimeDly(10000u);	
//	}
//	
//}

OS_STK Stack3[] = {
0000,0x2710,0000,0x7070707,0x8080808,0x9090909,0x10101010,
0x11111111,0xfffffffd,0000,0x20000295,0x40013800,0x80,0x12121212,
0x800152d,0x800152c,0x21000000,0x6e,0x5050505,0x6060606,0x800228b,0000
};

OS_STK Stack_len = sizeof(Stack3) / sizeof(OS_STK);

//int main2(void)
//{
//	PWM_Init();			//PWM初始化
//	Serial_Init();	//Serial初始化
//	
//	OSInit();
//	OSTaskCreate(PWM_Led,           (void *)0, &Task_PWM_Led_STK[127] , 5);
//	
//	char s[100];
//	OS_STK len = sizeof(Stack3) / sizeof(OS_STK);
//	len = len;
//	
//	OS_TCB* ptcb = OSTCBPrioTbl[5];
////	OS_STK* p_stk = ptcb->OSTCBStkBasePtr + 1u;
////	
////	for(int i = len - 1;i >= 0; i--){
////		*(--p_stk) = Stack[i];
////	}
////	
////	ptcb->OSTCBStkPtr = p_stk;
//	OS_Multiple_Task_SW(5, Stack3, len);
//	
//	sprintf(s, " len:%d\n ptr:%#x", len, (OS_STK)ptcb->OSTCBStkPtr);
//	Serial_SendString(s);
//	
//	OSTaskCreate(Serial_Send_Stack, (void *)0, &Task_Serial_STK[127]  , 4);
//	
//	SysTick_Config(1000u);
//	OSStart();
//	return 0;
//}


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

char str[128];
int main(void)
{
    // 系统初始化
    SystemInit();    
    // 初始化I2C主机
    I2C2_Master_Init();
    Serial_Init();
	
		Serial_SendString("master\n");
		uint8_t buf[128];
		uint16_t size;
		uint8_t x = OS_GetStackData(0, buf, (&size));
		sprintf(str, "%d %d\n", x, size);
		Serial_SendString(str);
		for(uint8_t i = 0;i<size;i++){
			sprintf(str,"%x ", buf[i]);
			Serial_SendString(str);
		}
		
		
	
		while(1){}
}


int mainGets(void)
{
		// 系统初始化
    SystemInit();  
    // 初始化I2C从机
    I2C2_Slave_Init(I2C_SLAVE_ADDRESS);
		Serial_Init();
		char str[200];
	
		Serial_SendString("Hello\n");
		
		
		while(1){
				//sprintf(str, "%d\n", iflag.size_recv);
				for(int i = 0; i<10000000; i++);
		}
}

int main11(void)
{
		// 系统初始化
    SystemInit();    
    // 初始化I2C主机
    I2C2_Master_Init();
    Serial_Init();
	
		Serial_SendString("master\n");
		
		uint8_t x = OS_SendStackData(0, (uint8_t*) Stack3, sizeof(Stack3));
		sprintf(str, "%d", x);
		Serial_SendString(str);
	
		while(1);
}

int main1(void)
{
		// 系统初始化
    SystemInit();  
    // 初始化I2C从机
    I2C2_Slave_Init(I2C_SLAVE_ADDRESS);
		Serial_Init();
		char str[200];
	
		Serial_SendString("Hello\n");
		
		
		while(1){
			sprintf(str, "\nrcv:%d\n", iflag.size_recv);
				Serial_SendString(str);
			
				for(uint8_t i = 0;i<iflag.size_recv;i++){
					sprintf(str,"%x ", iflag.ptr_recv[i]);
					Serial_SendString(str);
				}
				for(int i = 0; i<1000000; i++);
		}
}