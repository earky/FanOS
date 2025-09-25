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

// ȫ�ֱ���
uint8_t i2c_ram[256];           // �ӻ����ݴ洢��
volatile uint8_t i2c_ram_index; // ��ǰRAM����
volatile uint8_t i2c_rx_data;   // ���յ�������
volatile uint8_t i2c_tx_data;   // Ҫ���͵�����
volatile uint8_t i2c_state;     // I2C״̬��״̬

// ״̬����
#define I2C_STATE_IDLE         0
#define I2C_STATE_ADDR_RECEIVED 1
#define I2C_STATE_DATA_RECEIVED 2
#define I2C_STATE_DATA_REQUEST  3

// I2C2 �ӻ���ʼ��
void I2C2_Slave_Init(uint8_t address)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    I2C_InitTypeDef I2C_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    // 1. ����ʱ��
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    
    // 2. ����I2C���ţ�PB10(SCL), PB11(SDA) - ȷ��ʹ�ÿ�©ģʽ
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;      // ���ÿ�©
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // 3. ��λI2C������ȷ���ɾ���״̬
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C2, ENABLE);
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C2, DISABLE);
    
    // 4. ����I2C
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = address << 1;  // ��׼����Ҫ����1λ
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = 100000;           // 100kHz
    
    I2C_Init(I2C2, &I2C_InitStructure);
    
    // 5. ����I2C�ж�
    NVIC_InitStructure.NVIC_IRQChannel = I2C2_EV_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    NVIC_InitStructure.NVIC_IRQChannel = I2C2_ER_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_Init(&NVIC_InitStructure);
    
    // 6. ʹ��I2C�ж�
    I2C_ITConfig(I2C2, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, ENABLE);
    
    // 7. ʹ��I2C
    I2C_Cmd(I2C2, ENABLE);
    
    i2c_state = I2C_STATE_IDLE;
		
		I2C_Flag_Init(&iflag);
}

// I2C2 �¼��жϴ���
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
		i2c_state = I2C_STATE_IDLE; // ����״̬��
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
	
//		/* ����������жϹ������ǲ�������ȵ� */
//		if(OSIntNesting){
//				iflag.ptr_send = (uint8_t *)temp; 
//				iflag.size_send = 8;
//				iflag.prio_send = 0;
//		}else{	/* �����жϹ��̲�������� */
//				/* �жϹ��̲������������������ */
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
				// ��ȡidle����ļ���
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
				I2C2->DR = 1u;	//�����������æ
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
    // ʹ��״̬��־������ֱ�Ӽ���¼��Ĵ���
    if(sr1 & I2C_SR1_ADDR)
    {		
				uint32_t sr2 = I2C2->SR2; // ��� ADDR ��־

        if (sr2 & I2C_SR2_TRA) // TRA = 1 �� ����Ҫ�����ӻ����ͣ�
        {
            i2c_state = I2C_STATE_DATA_REQUEST;
        }
        else // TRA = 0 �� ����Ҫд���ӻ����գ�
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
				// ��ȷ���STOPF��־������
				uint32_t tmp = I2C2->SR1; // ��ȡSR1�Ĵ��������������
				I2C2->CR1 |= 0x0000;      // д��CR1���κ�ֵ���ɣ����ֲ������У�
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
        // ����������ܵ��¼���־
        I2C_ClearITPendingBit(I2C2, I2C_IT_ADD10);
        I2C_ClearITPendingBit(I2C2, I2C_IT_SB);
        I2C_ClearITPendingBit(I2C2, I2C_IT_BTF);
    }
}

// I2C2 �����жϴ���
void I2C2_ER_IRQHandler(void)
{
    // ���ߴ�����
    if(I2C_GetITStatus(I2C2, I2C_IT_BERR))
    {
        I2C_ClearITPendingBit(I2C2, I2C_IT_BERR);
        Serial_SendString("Bus error detected\n");
        
        // ִ�и����׵����߻ָ�
        I2C_Cmd(I2C2, DISABLE);
        // �����ӳ�
        for(volatile uint32_t i = 0; i < 1000; i++);
        // ���³�ʼ��I2C
        //I2C2_Slave_Init();
    }
    
    // Ӧ�������
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
    
    // ����/���������
    if(I2C_GetITStatus(I2C2, I2C_IT_OVR))
    {
        I2C_ClearITPendingBit(I2C2, I2C_IT_OVR);
        Serial_SendString("Overrun/Underrun error\n");
    }
}

// I2C2 ������ʼ��
void I2C2_Master_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    I2C_InitTypeDef I2C_InitStructure;
    
    // ����ʱ��
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    // ����I2C���ţ�PB10(SCL), PB11(SDA)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // ����I2C
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x00;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = 100000;  // 100kHz
    
    I2C_Init(I2C2, &I2C_InitStructure);
    I2C_Cmd(I2C2, ENABLE);
}

// I2C����д������
uint8_t I2C2_Write(uint8_t devAddr, uint8_t mode, uint8_t *data, uint16_t len)
{
    uint32_t timeout = 100000;  // ��ʱ������
    
    // 1. ������ʼ����
    I2C_GenerateSTART(I2C2, ENABLE);
    while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT)) {
        if(--timeout == 0) {
					I2C_GenerateSTOP(I2C2, ENABLE);
					return 1;
				}
    }
    
    // 2. �����豸��ַ��дģʽ��
    I2C_Send7bitAddress(I2C2, devAddr << 1, I2C_Direction_Transmitter);
    timeout = 100000;
    while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
        if(--timeout == 0) {
					I2C_GenerateSTOP(I2C2, ENABLE);
					return 2;
				}
    }
    
    // 3. ����ģʽ
    I2C_SendData(I2C2, mode);
    timeout = 100000;
    while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
        if(--timeout == 0) {
					I2C_GenerateSTOP(I2C2, ENABLE);
					return 3;
				}
    }
				
    // 4. ��������
    for(uint16_t i = 0; i < len; i++) {
        I2C_SendData(I2C2, data[i]);
        timeout = 100000;
        while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
            if(--timeout == 0) return 4;
        }
    }
    
    // 5. ����ֹͣ����
    I2C_GenerateSTOP(I2C2, ENABLE);
    return 0;  // �ɹ�
}

// I2C������ȡ����
uint8_t I2C2_Read(uint8_t devAddr, uint8_t *data, uint16_t* len)
{
    uint32_t timeout = 100000;
		
    // 1. ������ʼ����
    I2C_GenerateSTART(I2C2, ENABLE);
    timeout = 100000;
    while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT)) {
        if(--timeout == 0) {
					I2C_GenerateSTOP(I2C2, ENABLE);
					return 1;
				}
    }
    
    // 2. �����豸��ַ����ģʽ��
    I2C_Send7bitAddress(I2C2, (devAddr << 1) | 0x01 , I2C_Direction_Receiver);
    timeout = 100000;
    while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) {
        if(--timeout == 0) {
					I2C_GenerateSTOP(I2C2, ENABLE);
					return 2;
				}
    }
    
		// 3. ���ճ���
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
		
    // 3. ��������
    for(uint16_t i = 0; i < size; i++) {
				// �ڽ������һ���ֽ�ǰ����ACK������STOP
				if(i == size - 1) {
						I2C_AcknowledgeConfig(I2C2, DISABLE); // ���һ���ֽڣ�������ACK
						I2C_GenerateSTOP(I2C2, ENABLE);        // ���������һ���ֽں���STOP
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
		I2C_AcknowledgeConfig(I2C2, ENABLE); // �ָ�ACKʹ��
    
    return 0;  // �ɹ�
}