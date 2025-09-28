#include "os_cfg.h"

#include "I2C.h"
#include "ucos_ii.h"

#if OS_MULTIPLE_CORE > 0u

OS_EVENT* GetStackSem;
OS_EVENT* SendDataSem;
OS_EVENT* GetDataSem;
OS_EVENT* TaskSuspendSem;
OS_EVENT* DataTransferSem;

OSQueue os_queue;

/* ��ĳһ���жϷ���ʱ������ID��Ϊ��Ӧ�˷����жϵ�ID */
uint8_t SendDataCoreID = 1;

uint8_t OSCoreID = 0;

//uint8_t OSDevAddrs[] = {I2C_MASTER_ADDRESS, I2C_SLAVE_ADDRESS};
uint8_t OSDevAddrs[] = {I2C_MASTER_ADDRESS, I2C_SLAVE_ADDRESS};
uint8_t OSDevNums = sizeof(OSDevAddrs) / sizeof(uint8_t);
uint8_t DataGetDevAddr = I2C_SLAVE_ADDRESS;

uint8_t  OSMinCountPrio = OS_TASK_IDLE_PRIO;
uint16_t OSMinCount     = USAGE_MAX_COUNT;

uint8_t OSSuspendTaskPrio = 0;

/* 0:All ok    1:there is no task for main core to schdule   2:no response(maybe the core is broken)*/
uint8_t  OSCoreStatus[MAX_CORE_NUMS] = {0};

uint8_t OSDevAddrTbl[64] = {I2C_SLAVE_ADDRESS, I2C_SLAVE_ADDRESS, I2C_SLAVE_ADDRESS, I2C_SLAVE_ADDRESS, I2C_SLAVE_ADDRESS, I2C_SLAVE_ADDRESS, I2C_SLAVE_ADDRESS, I2C_SLAVE_ADDRESS,
	I2C_SLAVE_ADDRESS, I2C_SLAVE_ADDRESS,I2C_SLAVE_ADDRESS,I2C_SLAVE_ADDRESS,I2C_SLAVE_ADDRESS,I2C_SLAVE_ADDRESS,I2C_SLAVE_ADDRESS};
OS_STK x[128];
OS_STK* OSStackPtrTbl[64] = {x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x};
uint16_t OSCountTbl[64] = {0};
uint16_t OSUsageCount[64] = {0};

uint8_t buffer[128];
uint16_t buffer_len;

uint32_t total_count;
/*
*   ���ucos��Ӳ����ĳ�����
*   �����Ҫʹ�÷�SPI������Ӳ��ʵ��fanos
*   �����޸ĸò�
*/
uint8_t OS_Read(uint8_t devAddr, uint8_t *data, uint16_t* len)
{
		return I2C2_Read(devAddr, data, len);
}

uint8_t OS_Write(uint8_t devAddr, uint8_t mode, uint8_t *data, uint16_t len)
{
		return I2C2_Write(devAddr, mode, data, len);
}

// �ȶ�ȡheader��Ϊ�̶����ȣ�֮���ٶ�ȡ�����data����
uint8_t OS_Read2(uint8_t devAddr, uint8_t *buf1, uint8_t* buf2, uint16_t len1, uint16_t* len2)
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
		*len2 = (size_h << 8) | size_l;
		
    // 3. �������ݰ�ͷ����
    for(uint16_t i = 0; i < len1; i++) {
				timeout = 100000;
				while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_RECEIVED)) {
						if(--timeout == 0) {
								I2C_GenerateSTOP(I2C2, ENABLE);
								return 3;
						}
				}
				buf1[i] = I2C_ReceiveData(I2C2);
		}
		
		
		// 4. ����ʵ������
    for(uint16_t i = 0; i < *len2; i++) {
				// �ڽ������һ���ֽ�ǰ����ACK������STOP
				if(i == *len2 - 1) {
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
				buf2[i] = I2C_ReceiveData(I2C2);
				
		}
		
		I2C_AcknowledgeConfig(I2C2, ENABLE); // �ָ�ACKʹ��
    return 0;  // �ɹ�
}

uint8_t OS_Write2(uint8_t devAddr, uint8_t mode, uint8_t *buf1, uint8_t *buf2, uint16_t len1, uint16_t len2)
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
				
    // 4. ��������buf1
    for(uint16_t i = 0; i < len1; i++) {
        I2C_SendData(I2C2, buf1[i]);
        timeout = 100000;
        while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
            if(--timeout == 0) return 4;
        }
    }
    
		// 5. ��������buf2
    for(uint16_t i = 0; i < len2; i++) {
        I2C_SendData(I2C2, buf2[i]);
        timeout = 100000;                                                                                                                                                  
        while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
            if(--timeout == 0){ 
							I2C_GenerateSTOP(I2C2, ENABLE);
							return 5;
						}
        }
    }
		
    // 6. ����ֹͣ����
    I2C_GenerateSTOP(I2C2, ENABLE);
    return 0;  // �ɹ�
}


/*
* Э���ʵ��
*/

// 0x01 ��ȡprio���ȼ�ջ������
uint8_t OS_GetStackData(INT8U* prio, uint8_t devAddr, uint8_t* buf, uint16_t* size)
{	
		uint8_t status;
		//uint8_t devAddr = OSDevAddrTbl[prio];
		//uint8_t devAddr = I2C_SLAVE_ADDRESS;
		
		// 1.���� �ı�ģʽ
		status  = OS_Write(devAddr, GET_STACK_DATA, NULL, 0);
		if(status)
		{
				return status;
		}
		
		// 2.��ȡ����
		status = OS_Read2(devAddr, prio, buf, sizeof(INT8U), size);
		if(status)
		{
				return status;
		}
		
		return RES_OK;
}

uint8_t OS_SendStackData(INT8U target_prio, uint8_t devAddr, uint8_t* buf, uint16_t size)
{
		uint8_t status, size_h, size_l;
		//uint8_t devAddr = OSDevAddrTbl[prio];
		//uint8_t devAddr = I2C_SLAVE_ADDRESS;
	
		size_h = (size >> 8) & 0x00FF;
		size_l = size & 0x00FF;
	
		uint8_t buf_tmp[3] = { size_h, size_l, target_prio};
		
		status = OS_Write2(devAddr, SEND_STACK_DATA, buf_tmp, buf, 3, size);
		if(status)
		{
				return status;
		}
		
		return RES_OK;
}

uint8_t OS_GetVariableData(uint8_t devAddr, uint8_t* buf, uint16_t* size, uint32_t* address, uint8_t* type)
{
		uint8_t status;
		
		// 1.���� �ı�ģʽ
		status  = OS_Write(devAddr, GET_VARIABLE_DATA, NULL, 0);
		if(status)
		{
				return status;
		}
		
		// 2.��ȡ����
		uint8_t header[5];
		status = OS_Read2(devAddr, header, buf, 5, size);
		
		if(status)
		{
				return status;
		}
		
		// ������ַ
		char str[20];
		*address = (header[3] << 24) |
							 (header[2] << 16) |
							 (header[1] << 8)  |
							 (header[0] << 0);
//		for(uint8_t i = 0;i<4;i++){
//			*address |= (header[i] << 24);
//			// ������겻��Ҫ����
//			if(i != 3){
//				*address = *address >> 8;
//			}
//		}
		
		
		
		// �������ݵ�����
		*type = header[4];
		
		return RES_OK;
}

uint8_t OS_SendVariableData(uint8_t devAddr, uint8_t* buf, uint16_t size, uint32_t address, uint8_t type)
{
		uint8_t status;
		uint8_t header[6];
		
		header[0] = (size >> 8) & 0x00FF;
		header[1] = (size) & 0x00FF;
	
		for(uint8_t i = 0; i < 4; i++){
				header[i + 2] = address & 0x000000FF;
				address = address >> 8;
		}
		
		//header[6] = prio;
		
		status  = OS_Write2(devAddr, SEND_VARIABLE_DATA, header, buf, 6, size);
		if(status)
		{
				return status;
		}
		
		return RES_OK;
}

uint8_t OS_GetCpuUsage(uint8_t devAddr, uint16_t* count)
{
		uint8_t status;	
		uint16_t len;
	
		// 1.���� �ı�ģʽ
		status  = OS_Write(devAddr, GET_CPU_USAGE, NULL, 0);
		if(status)
		{
				return status;
		}
		
		// 2.��ȡ����
		status = OS_Read2(devAddr, NULL, (uint8_t*)count, 0, &len);
		if(status)
		{
				return status;
		}
	
		return RES_OK;
}

uint8_t OS_TaskSwitchRequest(uint8_t devAddr, INT8U* prio)
{
		uint8_t status;	
		uint16_t len;
	
		// 1.���� �ı�ģʽ
		status  = OS_Write(devAddr, TASK_SWITCH_REQUEST, NULL, 0);
		if(status)
		{
				return status;
		}
		
		// 2.��ȡ����
		status = OS_Read2(devAddr, NULL, (uint8_t*)prio, 0, &len);
		if(status)
		{
				return status;
		}
	
		return RES_OK;
}

uint8_t OS_IsBusy(uint8_t devAddr, uint8_t* isBusy)
{
		uint8_t status;	
		uint16_t len;
	
		// 1.���� �ı�ģʽ
		status  = OS_Write(devAddr, IS_BUSY, NULL, 0);
		if(status)
		{
				return status;
		}
		
		// 2.��ȡ����
		status = OS_Read2(devAddr, NULL, isBusy, 0, &len);
		if(status)
		{
				return status;
		}
	
		return RES_OK;

}

//// 0x02 ����ջ������
//uint8_t OS_SendStackData(INT8U prio, uint8_t* buf, uint16_t size)
//{
//		uint8_t status;
//		
//		OS_CommStart();
//	
//		// Э��ͷ
//		status = OS_PacketHeader(SEND_STACK_DATA);
//		if(status != RES_OK){
//				return status;
//		}
//		
//		// �������ݳ��� sizeΪ16λ
//		status = OS_SendData( (uint8_t*)&size, 2);
//		if(status != RES_OK){
//				return status;
//		}
//		
//		// �������� ����λsize
//		status = OS_SendData(buf, size);
//		if(status != RES_OK){
//				return status;
//		}
//		
//		OS_CommStop();
//		
//		return RES_OK;
//}

//// 0x03 ��ȡ����
//uint8_t OS_GetVariableData(INT8U *prio, uint16_t* size, uint32_t* address, uint8_t* buf)
//{
//		uint8_t status;
//	
//		OS_CommStart();
//		
//		// Э��ͷ
//		status = OS_PacketHeader(GET_VARIABLE_DATA);
//		if(status != RES_OK){
//				return status;
//		}
//		
//		// ��ȡ���ݳ���
//		
//}
/*
* 0:Success 
* 1:TASK not exists
*/

uint8_t OS_MultipleTaskSW(INT8U	  prio,
														OS_STK* stk,
														OS_STK  len)
{
    OS_TCB  *ptcb;	
		OS_STK  *p_stk;
    
    if(len == 0)
        return 2u;

    ptcb = OSTCBPrioTbl[prio];
    /* task is not exists */
    if(ptcb == (OS_TCB*)0){
				return 1u;
    }

		p_stk = ptcb->OSTCBStkBasePtr + 1u;

    for(int i = len - 1;i >= 0; i--){
			*(--p_stk) = stk[i];
		}

    /* set OSTCBStkPtr to new position */
    ptcb->OSTCBStkPtr = p_stk;
		
		OSTaskResume(prio);
    return 0u;
}

void OS_MultiCoreTaskInit(void)	
{
		OS_TCB* ptcb = OSTCBList;
		char str[20];
		while (ptcb->OSTCBPrio != OS_TASK_IDLE_PRIO){
				if(ptcb->InitCoreID != OSCoreID && ptcb->InitCoreID != ALL_CORES_ID){
						if(ptcb->OSTCBPrio != 0){
								//sprintf(str, ">> prio:%u | InitCoreID:%u\n", ptcb->OSTCBPrio, ptcb->InitCoreID);
								//Serial_SendString(str);
								OSTaskSuspend(ptcb->OSTCBPrio);
						}
				}
				
				ptcb = ptcb->OSTCBNext; 	
		}
}


// ��ʼ������
void OSInitQueue(OSQueue* q) {
	q->front = 0;
	q->rear  = 0;
	for(uint8_t i=0;i<OS_QUEUE_SIZE;i++){
			q->data[i] = 0;
	}
}

void OSEnterQueue(OSQueue* q, uint8_t value) {
	q->data[q->rear] = value;
	q->rear = (q->rear + 1) % OS_QUEUE_SIZE;
}

uint8_t OSOutQueue(OSQueue* q) {
	uint8_t out = q->data[q->front];
	q->front = (q->front + 1) % OS_QUEUE_SIZE;
	return out;
}
#endif
