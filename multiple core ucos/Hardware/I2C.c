#include "stm32f10x.h"
#include "stm32f10x_i2c.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "I2C.h" 
//#include "ucos_ii.h"

uint8_t I2C_Address[3] = {I2C_MASTER_ADDRESS ,I2C_SLAVE_ADDRESS1, I2C_SLAVE_ADDRESS2};
//OS_STK* OSStackPtrTbl[64];
I2C_Flag iflag;

void I2C_Flag_Init(I2C_Flag* iflag)
{
		iflag->idx_recv = iflag->idx_send = 0;
		iflag->ptr_recv = iflag->ptr_send = NULL;
		iflag->size_recv = iflag->size_send = 0;
		iflag->is_first_data = 1;
		iflag->is_busy = 0;
		iflag->mode = 0;
}

// 全局变量
uint8_t i2c_ram[256];           // 从机数据存储器
volatile uint8_t i2c_ram_index; // 当前RAM索引
volatile uint8_t i2c_rx_data;   // 接收到的数据
volatile uint8_t i2c_tx_data;   // 要发送的数据
volatile uint8_t i2c_state;     // I2C状态机状态

// 状态定义
#define I2C_STATE_IDLE         0
#define I2C_STATE_ADDR_RECEIVED 1
#define I2C_STATE_DATA_RECEIVED 2
#define I2C_STATE_DATA_REQUEST  3

// I2C2 从机初始化
void I2C2_Slave_Init(uint8_t address)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    I2C_InitTypeDef I2C_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    // 1. 开启时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    
    // 2. 配置I2C引脚：PB10(SCL), PB11(SDA) - 确保使用开漏模式
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;      // 复用开漏
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // 3. 复位I2C外设以确保干净的状态
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C2, ENABLE);
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C2, DISABLE);
    
    // 4. 配置I2C
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = address << 1;  // 标准库需要左移1位
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = 100000;           // 100kHz
    
    I2C_Init(I2C2, &I2C_InitStructure);
    
    // 5. 配置I2C中断
    NVIC_InitStructure.NVIC_IRQChannel = I2C2_EV_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    NVIC_InitStructure.NVIC_IRQChannel = I2C2_ER_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_Init(&NVIC_InitStructure);
    
    // 6. 使能I2C中断
    I2C_ITConfig(I2C2, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, ENABLE);
    
    // 7. 使能I2C
    I2C_Cmd(I2C2, ENABLE);
    
    i2c_state = I2C_STATE_IDLE;
		
		I2C_Flag_Init(&iflag);
}

// I2C2 事件中断处理
OS_STK Stack[] = {
0000,0x2710,0000,0x7070707,0x8080808,0x9090909,0x10101010,
0x11111111,0xfffffffd,0000,0x20000295,0x40013800,0x80,0x12121212,
0x800152d,0x800152c,0x21000000,0x6e,0x5050505,0x6060606,0x800228b,0000
};
uint16_t j = 0;


//uint16_t idx = 0;
//uint8_t Mode = 0;
//uint8_t Rxne_First_Data = 1;
//uint8_t* ptr = NULL;
//uint16_t size = 0;
char sstr[30];
void  Clear_Flag(void)
{
		i2c_state = I2C_STATE_IDLE; // 重置状态机
		iflag.is_first_data = 1;
		iflag.idx_recv = iflag.idx_send = 0;
}
char ssstr[30];
// GET_STACK_DATA
char temp[30] = "abcababasdgasdgasdg";
void  Get_Stack_Data_RXNE_Handler(uint32_t data)
{
#if OS_CRITICAL_METHOD == 3u                               /* Allocate storage for CPU status register     */
    OS_CPU_SR  cpu_sr = 0u;
#endif
		INT8U MinPrio;
		OS_TCB* ptcb;
		INT8U IsOSIntNesting;
		
		// MinPrio = OSSuspendTaskPrio;
		//IsOSIntNesting = OSIntNesting;
		
		
		ptcb = OSTCBPrioTbl[OSSuspendTaskPrio];
		iflag.ptr_send = (uint8_t *)ptcb->OSTCBStkPtr; 
		iflag.size_send = (ptcb->OSTCBStkBasePtr - ptcb->OSTCBStkPtr + 1u) * 4;
		iflag.prio_send = OSSuspendTaskPrio;
	
//		/* 任务如果在中断过程中是不允许调度的 */
//		if(OSIntNesting){
//				iflag.ptr_send = (uint8_t *)temp; 
//				iflag.size_send = 8;
//				iflag.prio_send = 0;
//		}else{	/* 不是中断过程才允许调度 */
//				/* 中断过程不允许进行任务挂起操作 */
//				OSTaskSuspend(MinPrio);
//				
//				iflag.prio_send = MinPrio;
//		}
}

void  Get_Stack_Data_TXE_Handler(void)
{
		switch(iflag.idx_send){
			case 0:
				I2C2->DR =(iflag.size_send >> 8) & 0x00FF;
				break;
			case 1:
				I2C2->DR = iflag.size_send & 0x00FF ;
				break;
			case 2:
				I2C2->DR = iflag.prio_send;
				break;
			default:
				I2C2->DR  = iflag.ptr_send[iflag.idx_send - 3];
				break;
		}
		
		iflag.idx_send ++;
}

// SEND_STACK_DATA

uint8_t sendstack[128];
void  Send_Stack_Data_RXNE_Handler(uint32_t data)
{
		static uint8_t size_h, size_l;
		switch(iflag.idx_recv){
			case 0:
				
				break;
			case 1:
				size_h = data;
				break;
			case 2:
				size_l = data;
				iflag.size_recv = (size_h << 8) | size_l;
				//sprintf(ssstr, ">> rcv:%d\n", iflag.size_recv);
				//Serial_SendString(ssstr);
				break;
			case 3:
				iflag.prio_recv = (INT8U) data;
			  iflag.ptr_recv  = Task_Switch_Buffer;
				break;
			default:
//				sprintf(ssstr, "index:%d\n", iflag.idx_recv - 4);
//				Serial_SendString(ssstr);
				iflag.ptr_recv[iflag.idx_recv - 4] = (uint8_t)data;
				if(iflag.idx_recv - 3 == iflag.size_recv){
						OSSemPost(GetStackSem);
				}
				//sprintf(ssstr, "%#x ",data);
				//Serial_SendString(ssstr);
				
				break;
		}
		iflag.idx_recv ++;
}

void  Send_Stack_Data_TXE_Handler(void)
{

}

// GET_VARIABLE_DATA
void  Get_Variable_Data_RXNE_Handler(uint32_t data)
{	
		
}

void  Get_Variable_Data_TXE_Handler(void)
{
		static uint32_t address;
		switch(iflag.idx_send){
			case 0:
				I2C2->DR = (iflag.size_send >> 8) & 0x00FF;
				address = (uint32_t)iflag.ptr_send;
				break;
			case 1:
				I2C2->DR = iflag.size_send & 0x00FF;
				break;
			case 2:
			case 3:
			case 4:
			case 5:
				I2C2->DR = address & 0x000000FF;
				address = address >> 8;
				break;
			case 6:
				I2C2->DR = iflag.prio_send;
				break;
			default:
				I2C2->DR = iflag.ptr_send[iflag.idx_send - 7];
				break;
		}
		iflag.idx_send ++;
}

// SEND_VARIABLE_DATA
void Send_Variable_Data_RXNE_Handler(uint32_t data)
{	
		char str[20];
		static uint8_t size_h, size_l;
		static uint32_t address;
		switch(iflag.idx_recv){
			case 0:
				break;
			case 1:
				size_h = data;
				break;
			case 2:
				size_l = data;
				iflag.size_recv = (size_h << 8) | size_l;
				break;
			case 3:
			case 4:
			case 5:
				address |= (data << 24);
				address = address >> 8;
				break;
			case 6:
				address |= (data << 24);
				break;
			case 7:
				iflag.prio_recv = data;
				iflag.ptr_recv = (uint8_t*) address;
				break;
			default:
				iflag.ptr_recv[iflag.idx_recv - 8] = data;
				break;
		}
		
		iflag.idx_recv ++;
}

void Send_Variable_Data_TXE_Handler(void)
{

}

// GET_CPU_USAGE
void Get_CPU_Usage_RXNE_Handler(uint32_t data)
{
		
}

void Get_CPU_Usage_TXE_Handler(void)
{
		static uint16_t count_idle;
		switch(iflag.idx_send){
			case 0:
				// 获取idle任务的计数
				count_idle = OSTCBPrioTbl[OS_TASK_IDLE_PRIO]->OSTCBCountSend;
				I2C2->DR = 0u;
				break;
			case 1:
				I2C2->DR = 2u;
				break;
			case 2:
			case 3:
				I2C2->DR = count_idle & 0x000000FF;
				count_idle = count_idle >> 8;
				break;
			default:
				break;
		}
		iflag.idx_send ++;
}

// TASK_SWITCH_REQUEST
void Task_Switch_Request_RXNE_Handler(uint32_t data)
{
#if OS_CRITICAL_METHOD == 3u                               /* Allocate storage for CPU status register     */
    OS_CPU_SR  cpu_sr = 0u;
#endif
		INT8U MinPrio;
		OS_TCB* ptcb;
	
		OS_ENTER_CRITICAL();
		MinPrio = OSMinCountPrio;
		OS_EXIT_CRITICAL();
		
		ptcb = OSTCBPrioTbl[MinPrio];
		OSSuspendTaskPrio = MinPrio;
	
		iflag.ptr_send = (uint8_t *)ptcb->OSTCBStkPtr; 
		iflag.prio_send = MinPrio;
		iflag.size_send = (ptcb->OSTCBStkBasePtr - ptcb->OSTCBStkPtr + 1u) * 4;
}

void Task_Switch_Request_TXE_Handler(void)
{
		switch(iflag.idx_send){
			case 0:
				I2C2->DR = 0u;
				break;
			case 1:
				I2C2->DR = 1u;
				break;
			case 2:
				I2C2->DR = iflag.prio_send;
				iflag.is_busy = 1;
				OSSemPost(TaskSuspendSem);
				break;
			default:
				I2C2->DR = 0u;
				break;
		}
		iflag.idx_send ++;
}

void Is_Busy_RXNE_Handler(uint32_t data)
{

}

void Is_Busy_TXE_Handler(void)
{
		switch(iflag.idx_send){
			case 0:
				I2C2->DR = 0u;
				break;
			case 1:
				I2C2->DR = 1u;
				break;
			case 2:
				I2C2->DR = iflag.is_busy;
				break;
			default:
				I2C2->DR = 1u;	//如果出错则发送忙
				break;
		}
		iflag.idx_send ++;
}
// TASK_ACTIVATE
void Task_Activate_RXNE_Handler(uint32_t data)
{
		
}

void Task_Activate_TXE_Handler(void)
{

}

// TASK_DISACTIVATE
void Task_Disactivate_RXNE_Handler(uint32_t data)
{
		
}

void Task_Disactivate_TXE_Handler(void)
{

}

void I2C2_EV_IRQHandler(void)
{	
		uint32_t sr1 = I2C2->SR1;
    // 使用状态标志而不是直接检查事件寄存器
    if(sr1 & I2C_SR1_ADDR)
    {		
				uint32_t sr2 = I2C2->SR2; // 清除 ADDR 标志

        if (sr2 & I2C_SR2_TRA) // TRA = 1 → 主机要读（从机发送）
        {
            i2c_state = I2C_STATE_DATA_REQUEST;
        }
        else // TRA = 0 → 主机要写（从机接收）
        {
            i2c_state = I2C_STATE_DATA_RECEIVED;
        }
				
				iflag.idx_recv = iflag.idx_send = 0;
    }
    else if(sr1 & I2C_SR1_TXE)
    {
				if (i2c_state == I2C_STATE_DATA_REQUEST)
        {
							switch(iflag.mode){
								case GET_STACK_DATA:
									Get_Stack_Data_TXE_Handler();
									break;
								case SEND_STACK_DATA:
									Send_Stack_Data_TXE_Handler();
									break;
								case GET_VARIABLE_DATA:
									Get_Variable_Data_TXE_Handler();
									break;
								case SEND_VARIABLE_DATA:
									Send_Variable_Data_TXE_Handler();
									break;
								case GET_CPU_USAGE:
									Get_CPU_Usage_TXE_Handler();
									break;
								case TASK_ACTIVATE:
									Task_Activate_TXE_Handler();
									break;
								case TASK_DISACTIVATE:
									Task_Disactivate_TXE_Handler();
									break;
								case TASK_SWITCH_REQUEST:
									Task_Switch_Request_TXE_Handler();
									break;
								case IS_BUSY:
									Is_Busy_TXE_Handler();
									break;
								default:
									break;
							}
        }
    }
    else if(sr1 & I2C_SR1_RXNE)
    {	
				if (i2c_state == I2C_STATE_DATA_RECEIVED)
        {		
						uint32_t tmp = I2C2->DR;
						if(iflag.is_first_data)
						{
								iflag.is_first_data = 0;
								iflag.mode = tmp;
						}
						
						switch (iflag.mode) {
							case GET_STACK_DATA:
								Get_Stack_Data_RXNE_Handler(tmp);
								break;
							case SEND_STACK_DATA:
								Send_Stack_Data_RXNE_Handler(tmp);
								break;
							case GET_VARIABLE_DATA:
								Get_Variable_Data_RXNE_Handler(tmp);
								break;
							case SEND_VARIABLE_DATA:
								Send_Variable_Data_RXNE_Handler(tmp);
								break;
							case GET_CPU_USAGE:
								Get_CPU_Usage_RXNE_Handler(tmp);
								break;
							case TASK_ACTIVATE:
								Task_Activate_RXNE_Handler(tmp);
								break;
							case TASK_DISACTIVATE:
								Task_Disactivate_RXNE_Handler(tmp);
								break;
							case TASK_SWITCH_REQUEST:
								Task_Switch_Request_RXNE_Handler(tmp);
								break;
							case IS_BUSY:
								Is_Busy_RXNE_Handler(tmp);
								break;
							default:
								break;
						}
        }
    }
    else if(sr1 & I2C_SR1_STOPF)
    {
				// 正确清除STOPF标志的序列
				uint32_t tmp = I2C2->SR1; // 读取SR1寄存器（必须操作）
				I2C2->CR1 |= 0x0000;      // 写入CR1（任何值均可，保持操作序列）
//				switch(iflag.mode){
//						case SEND_STACK_DATA:
//							Send_Stack_Data_STOPF_Handler();
//							break;
//						default:
//							break;
//				}
				//Serial_SendString("stopf\n");
				Clear_Flag();
    }
    else
    {	
        // 清除其他可能的事件标志
        I2C_ClearITPendingBit(I2C2, I2C_IT_ADD10);
        I2C_ClearITPendingBit(I2C2, I2C_IT_SB);
        I2C_ClearITPendingBit(I2C2, I2C_IT_BTF);
    }
}

// I2C2 错误中断处理
void I2C2_ER_IRQHandler(void)
{
    // 总线错误处理
    if(I2C_GetITStatus(I2C2, I2C_IT_BERR))
    {
        I2C_ClearITPendingBit(I2C2, I2C_IT_BERR);
        Serial_SendString("Bus error detected\n");
        
        // 执行更彻底的总线恢复
        I2C_Cmd(I2C2, DISABLE);
        // 短暂延迟
        for(volatile uint32_t i = 0; i < 1000; i++);
        // 重新初始化I2C
        //I2C2_Slave_Init();
    }
    
    // 应答错误处理
    if(I2C_GetITStatus(I2C2, I2C_IT_AF))
    {		
				char str[20];
				uint32_t sr1 = I2C2->SR1;
				//sprintf(str, "%x\n", sr1);
				//Serial_SendString(str);
        I2C_ClearITPendingBit(I2C2, I2C_IT_AF);
				Clear_Flag();
        //Serial_SendString("Acknowledge failure\n");
    }
    
    // 过载/下溢错误处理
    if(I2C_GetITStatus(I2C2, I2C_IT_OVR))
    {
        I2C_ClearITPendingBit(I2C2, I2C_IT_OVR);
        Serial_SendString("Overrun/Underrun error\n");
    }
}

// I2C2 主机初始化
void I2C2_Master_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    I2C_InitTypeDef I2C_InitStructure;
    
    // 开启时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    // 配置I2C引脚：PB10(SCL), PB11(SDA)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // 配置I2C
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x00;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = 100000;  // 100kHz
    
    I2C_Init(I2C2, &I2C_InitStructure);
    I2C_Cmd(I2C2, ENABLE);
}

// I2C主机写入数据
uint8_t I2C2_Write(uint8_t devAddr, uint8_t mode, uint8_t *data, uint16_t len)
{
    uint32_t timeout = 100000;  // 超时计数器
    
    // 1. 发送起始条件
    I2C_GenerateSTART(I2C2, ENABLE);
    while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT)) {
        if(--timeout == 0) {
					I2C_GenerateSTOP(I2C2, ENABLE);
					return 1;
				}
    }
    
    // 2. 发送设备地址（写模式）
    I2C_Send7bitAddress(I2C2, devAddr << 1, I2C_Direction_Transmitter);
    timeout = 100000;
    while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
        if(--timeout == 0) {
					I2C_GenerateSTOP(I2C2, ENABLE);
					return 2;
				}
    }
    
    // 3. 发送模式
    I2C_SendData(I2C2, mode);
    timeout = 100000;
    while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
        if(--timeout == 0) {
					I2C_GenerateSTOP(I2C2, ENABLE);
					return 3;
				}
    }
				
    // 4. 发送数据
    for(uint16_t i = 0; i < len; i++) {
        I2C_SendData(I2C2, data[i]);
        timeout = 100000;
        while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
            if(--timeout == 0) return 4;
        }
    }
    
    // 5. 发送停止条件
    I2C_GenerateSTOP(I2C2, ENABLE);
    return 0;  // 成功
}

// I2C主机读取数据
uint8_t I2C2_Read(uint8_t devAddr, uint8_t *data, uint16_t* len)
{
    uint32_t timeout = 100000;
		
    // 1. 发送起始条件
    I2C_GenerateSTART(I2C2, ENABLE);
    timeout = 100000;
    while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT)) {
        if(--timeout == 0) {
					I2C_GenerateSTOP(I2C2, ENABLE);
					return 1;
				}
    }
    
    // 2. 发送设备地址（读模式）
    I2C_Send7bitAddress(I2C2, (devAddr << 1) | 0x01 , I2C_Direction_Receiver);
    timeout = 100000;
    while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) {
        if(--timeout == 0) {
					I2C_GenerateSTOP(I2C2, ENABLE);
					return 2;
				}
    }
    
		// 3. 接收长度
		uint16_t size;
		
		timeout = 100000;
		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_RECEIVED)) {
				if(--timeout == 0) {
						I2C_GenerateSTOP(I2C2, ENABLE);
						return 3;
				}
		}
		uint8_t size_h = I2C_ReceiveData(I2C2);
		
		timeout = 100000;
		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_RECEIVED)) {
				if(--timeout == 0) {
						I2C_GenerateSTOP(I2C2, ENABLE);
						return 3;
				}
		}
		uint8_t size_l = I2C_ReceiveData(I2C2);
		
		size = (size_h << 8) | size_l;
		*len = size;
//		sprintf(sstr, "%d %d %d\n", size_h, size_l, size);
//		Serial_SendString(sstr);
		
    // 3. 接收数据
    for(uint16_t i = 0; i < size; i++) {
				// 在接收最后一个字节前禁用ACK并发送STOP
				if(i == size - 1) {
						I2C_AcknowledgeConfig(I2C2, DISABLE); // 最后一个字节，不发送ACK
						I2C_GenerateSTOP(I2C2, ENABLE);        // 接收完最后一个字节后发送STOP
				}

				timeout = 100000;
				while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_RECEIVED)) {
						if(--timeout == 0) {
								I2C_GenerateSTOP(I2C2, ENABLE);
								return 3;
						}
				}
				data[i] = I2C_ReceiveData(I2C2);
				
		}
		I2C_AcknowledgeConfig(I2C2, ENABLE); // 恢复ACK使能
    
    return 0;  // 成功
}