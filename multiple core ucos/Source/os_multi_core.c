#include "os_cfg.h"

#include "I2C.h"
#include "GPIO.h"
#include "ucos_ii.h"
#include <string.h>

#if OS_MULTIPLE_CORE > 0u

/**
	* OS����Ҫ��ȫ�ֱ���
	*/

/* OS�˶�Ӧ��ID�ţ�����ID��һ��Ϊ0!!! */
uint8_t OSCoreID = 1;

OS_EVENT* GetStackSem;				//��ȡ��ջ���ź���
OS_EVENT* SendDataSem;				//���Ͷ�ջ���ź���
OS_EVENT* TaskSuspendSem;			//���������ź���
OS_EVENT* DataTransferSem;		//���ݴ�����ź���

OSQueue os_queue;							//OSѭ�����У���Ҫ�������ݴ�����ʱ��Ŷ�Ӧ�����I2C��ַ

uint8_t OSDevAddrs[] = {I2C_MASTER_ADDRESS, I2C_SLAVE_ADDRESS};		//ͨ��OSCoreID���������I2C��ַ��
uint8_t OSDevNums = sizeof(OSDevAddrs) / sizeof(uint8_t);					//��˵����������ˣ���ˣ�

uint8_t  OSMinCountPrio = OS_TASK_IDLE_PRIO;	//OS��ǰռ���������������ȼ�
uint16_t OSMinCount     = USAGE_MAX_COUNT;		//OS��ǰռ������������ռ�ü���
uint32_t total_count;													//OS�ܼ���

uint8_t OSSuspendTaskPrio = 0;								//��Ҫ������������ȼ�

uint8_t Task_Switch_Buffer[512];							//�����л��Ļ�����
uint8_t Data_Transfer_Buffer[512];            //���ݴ���Ļ�����
OS_STK Task_Multi_Core_Sched_STK[128];				//���ؾ�������ջ
OS_STK Task_Multi_Core_Suspend_STK[128];			//�����������ջ
OS_STK Task_Multi_Core_Data_Transfer_STK[128];//���ݴ�������ջ

/**
	*   ���ucos��Ӳ����ĳ�����
	*   �����Ҫʹ�÷�SPI������Ӳ��ʵ��
	*   �����޸ĸò�
	*/

/**
	* @brief  ����˶�ȡ����
  * @param  ��˵�I2C��ַ
  * @param  ָ�����ݵ�ָ��
  * @param  ����ָ�룬���ڻ�ȡ����
  * @retval ��ǰ״̬
  */
uint8_t OS_Read(uint8_t devAddr, uint8_t *data, uint16_t* len)
{
		return I2C2_Read(devAddr, data, len);
}

/**
	* @brief  �����д����
  * @param  ��˵�I2C��ַ
  * @param  д���ģʽ
  * @param  ָ�����ݵ�ָ��
  * @param  ����
  * @retval ��ǰ״̬
  */
uint8_t OS_Write(uint8_t devAddr, uint8_t mode, uint8_t *data, uint16_t len)
{
		return I2C2_Write(devAddr, mode, data, len);
}

/**
	* @brief  ����˶�ȡ���ݣ������ȶ�len1ָ�����ȵ�buf1���ٶ�ȡsize���ȵ�buf2���������len2��
  * @param  ��˵�I2C��ַ
  * @param  ָ�����ݵ�ָ��1
  * @param  ָ�����ݵ�ָ��1
  * @param  ���ȣ�����Ԥ�ȶ�ȡ���ݰ�ͷ
  * @param  ����ָ�룬���ڻ�ȡ���ݳ���
  * @retval ��ǰ״̬
  */
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

/**
	* @brief  �����д���ݣ���д��buf1����д��buf2
  * @param  ��˵�I2C��ַ
  * @param  д���ģʽ
  * @param  ָ�����ݵ�ָ��1
  * @param  ָ�����ݵ�ָ��2
  * @param  ����1
  * @param  ����2
  * @retval ��ǰ״̬
  */
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


/**
	* Э���ʵ��
	*/

/**
  * @brief  ����˻�ȡ��ջ�����ݣ����ȣ����ȼ�
  * @param  �������ȼ�ָ�룬���ڻ�ȡ���ȼ�
  * @param  ��˵�I2C��ַ
  * @param  ָ���ջ��ָ�룬ǿתΪuint8_t*����
  * @param  ����ָ�룬���ڻ�ȡ����
  * @retval ��ǰ״̬
  */
uint8_t OS_GetStackData(INT8U* prio, uint8_t devAddr, uint8_t* buf, uint16_t* size)
{	
		uint8_t status; 

		/* �ı����״̬��ģʽ */
		status  = OS_Write(devAddr, GET_STACK_DATA, NULL, 0);
		if(status)
		{
				return status;
		}
		
		/* ��ȡ���� */
		status = OS_Read2(devAddr, prio, buf, sizeof(INT8U), size);
		if(status)
		{
				return status;
		}
		
		return RES_OK;
}

/**
	* @brief  ����˷��Ͷ�ջ�����ݣ����ȣ����ȼ�
  * @param  �������ȼ�
  * @param  ��˵�I2C��ַ
  * @param  ָ���ջ��ָ�룬ǿתΪuint8_t*����
  * @param  ����
  * @retval ��ǰ״̬
  */
uint8_t OS_SendStackData(INT8U target_prio, uint8_t devAddr, uint8_t* buf, uint16_t size)
{
		uint8_t status, size_h, size_l;

		/* ��size���Ϊ�ߵ͵�ַ���ڴ��� */
		size_h = (size >> 8) & 0x00FF;
		size_l = size & 0x00FF;
	
		uint8_t header[3] = { size_h, size_l, target_prio};
		
		/* �������ݰ�ͷ������ */
		status = OS_Write2(devAddr, SEND_STACK_DATA, header, buf, 3, size);
		if(status)
		{
				return status;
		}
		
		return RES_OK;
}

/**
	* @brief  ����˻�ȡα�����ڴ�����ݣ����ȣ���ַ������
  * @param  ��˵�I2C��ַ
  * @param  ָ�����ݵ�ָ�룬ǿתΪuint8_t*����
  * @param  ����ָ�룬���ڻ�ȡ����
  * @param  ��ַָ�룬���ڻ�ȡ��ַ
  * @param  ����ָ�룬���ڻ�ȡ����
  * @retval ��ǰ״̬
  */
uint8_t OS_GetVariableData(uint8_t devAddr, uint8_t* buf, uint16_t* size, uint32_t* address, uint8_t* type)
{
		uint8_t status;
		
		/* �ı����״̬��ģʽ */
		status  = OS_Write(devAddr, GET_VARIABLE_DATA, NULL, 0);
		if(status)
		{
				return status;
		}
		
	  /* ��ȡ���� */
		uint8_t header[5];
		status = OS_Read2(devAddr, header, buf, 5, size);
		
		if(status)
		{
				return status;
		}
		
		/* ������ַ */
		*address = (header[3] << 24) |
							 (header[2] << 16) |
							 (header[1] << 8)  |
							 (header[0] << 0);
		
		/* �������ݵ����� */
		*type = header[4];
		
		return RES_OK;
}

/**
	* @brief  ����˷���α�����ڴ�����ݣ����ȣ���ַ������
  * @param  ��˵�I2C��ַ
  * @param  ָ�����ݵ�ָ�룬ǿתΪuint8_t*����
  * @param  ����
  * @param  ��ַ
  * @param  ����
  * @retval ��ǰ״̬
  */
uint8_t OS_SendVariableData(uint8_t devAddr, uint8_t* buf, uint16_t size, uint32_t address, uint8_t type)
{
		uint8_t status;
		uint8_t header[7];
		
		/* �����Ȳ��Ϊ�ߵ͵�ַ */
		header[0] = (size >> 8) & 0x00FF;
		header[1] = (size) & 0x00FF;
	
		/* ����ַ���Ϊ4���� */
		for(uint8_t i = 2; i < 6; i++){
				header[i] = address & 0x000000FF;
				address = address >> 8;
		}
		
		/* ���ݵ����� */
		header[6] = type;
		
		/* �������� */
		status  = OS_Write2(devAddr, SEND_VARIABLE_DATA, header, buf, 7, size);
	
		if(status)
		{
				return status;
		}
	
		return RES_OK;
}

/**
	* @brief  ��ȡ���CPU��ʹ����
  * @param  ��˵�I2C��ַ
  * @param  ����ָ�룬���ڻ�ȡ����
  * @retval ��ǰ״̬
  */
uint8_t OS_GetCpuUsage(uint8_t devAddr, uint16_t* count)
{
		uint8_t status;	
		uint16_t len;
	
		/* �ı����״̬��ģʽ */
		status  = OS_Write(devAddr, GET_CPU_USAGE, NULL, 0);
		if(status)
		{
				return status;
		}
		
		/* ��ȡ���� */
		status = OS_Read2(devAddr, NULL, (uint8_t*)count, 0, &len);
		if(status)
		{
				return status;
		}
	
		return RES_OK;
}

/**
	* @brief  ��������˷��������л��������������ֹͣ����
  * @param  ��˵�I2C��ַ
  * @param  ����ָ�룬���ڻ�ȡ����
  * @retval ��ǰ״̬
  */
uint8_t OS_TaskSwitchRequest(uint8_t devAddr, INT8U* prio)
{
		uint8_t status;	
		uint16_t len;
	
		/* �ı����״̬��ģʽ */
		status  = OS_Write(devAddr, TASK_SWITCH_REQUEST, NULL, 0);
		if(status)
		{
				return status;
		}
		
		/* ��ȡ���� */
		status = OS_Read2(devAddr, NULL, (uint8_t*)prio, 0, &len);
		if(status)
		{
				return status;
		}
	
		return RES_OK;
}

/**
	* @brief  ���������ѯ���Ƿ���æ״̬����æ������Ӧ�õȴ�
  * @param  ��˵�I2C��ַ
  * @param  �Ƿ�æָ�룬���ڻ�ȡ��˵�״̬
  * @retval ��ǰ״̬
  */
uint8_t OS_IsBusy(uint8_t devAddr, uint8_t* isBusy)
{
		uint8_t status;	
		uint16_t len;
	
		/* �ı����״̬��ģʽ */
		status  = OS_Write(devAddr, IS_BUSY, NULL, 0);
		if(status)
		{
				return status;
		}
		
		/* ��ȡ���� */
		status = OS_Read2(devAddr, NULL, isBusy, 0, &len);
		if(status)
		{
				return status;
		}
	
		return RES_OK;

}

/**
	*	����йصĳ�ʼ���������л�����
	*/

/**
  * @brief  ��˳�ʼ�����񣬶��ڲ������Լ������ID��Ҫ����
  * @param  �������ȼ�
  * @param  ��ջָ��
  * @param  ��ջ����
  * @retval ��
  */
uint8_t OS_MultipleTaskSW(INT8U	  prio,
													OS_STK* stk,
													OS_STK  len)
{
    OS_TCB  *ptcb;	
		OS_STK  *p_stk;
    
		/* ����ջ����Ϊ0 */
    if(len == 0){
        return TASK_SWITCH_BUFFER_IS_ZERO;
		}
		
    ptcb = OSTCBPrioTbl[prio];
    /* ���񲻴��� */
    if(ptcb == (OS_TCB*)0){
				return TASK_NOT_EXITS;
    }

		p_stk = ptcb->OSTCBStkBasePtr + 1u;

    for(int i = len - 1;i >= 0; i--){
			*(--p_stk) = stk[i];
		}

    /* ���¶�Ӧ��Stkָ��		*/
    ptcb->OSTCBStkPtr = p_stk;
		/* �������� */
		OSTaskResume(prio);
    return TASK_SWITCH_SUCCESS;
}

/**
  * @brief  ��˳�ʼ�����񣬶��ڲ������Լ������ID��Ҫ����
  * @param  ��
  * @retval ��
  */
void OS_MultiCoreTaskInit(void)	
{
		OS_TCB* ptcb = OSTCBList;
		while (ptcb->OSTCBPrio != OS_TASK_IDLE_PRIO){
				if(ptcb->InitCoreID != OSCoreID && ptcb->InitCoreID != ALL_CORES_ID){
						if(ptcb->OSTCBPrio != 0){
								OSTaskSuspend(ptcb->OSTCBPrio);
						}
				}
				
				ptcb = ptcb->OSTCBNext; 	
		}
		
		return;
}

/**
  * @brief  ��˳�ʼ���ź���
  * @param  ��
  * @retval ��
  */
void OSMultiSemInit(void)
{
		GetStackSem = OSSemCreate(0);
		SendDataSem = OSSemCreate(0);
		TaskSuspendSem = OSSemCreate(0);
		DataTransferSem = OSSemCreate(0);
}

/**
  * @brief  ���Ӳ����ʼ��
  * @param  ��
  * @retval ��
  */
void OSMultiHardwareInit(void)
{
	/* ���˳�ʼ�� */
	if(OSCoreID == 0){
		I2C2_Master_Init();
		GPIO_EXTI_Init();
	}
	else{	/* ��˳�ʼ�� */
		I2C2_Slave_Init(I2C_SLAVE_ADDRESS);
		Slave_GPIO_Init();
	}
}

/**
  * @brief  ���Ӳ����ʼ��
  * @param  ��
  * @retval ��
  */
void OS_Multi_Core_Sched(void* p_arg);
void OS_Multi_Core_Data_Transfer(void* p_arg);
void OS_Multi_Core_Task_Suspend(void* p_arg);
void OSMultiTaskInit(void)
{
		OSTaskCreate(OS_Multi_Core_Sched, (void*)0, &Task_Multi_Core_Sched_STK[127], 0, ALL_CORES_ID, SPECIFIC_TRUE);
		OSTaskCreate(OS_Multi_Core_Task_Suspend, (void*)0, &Task_Multi_Core_Suspend_STK[127], 1, ALL_CORES_ID, SPECIFIC_TRUE);
		OSTaskCreate(OS_Multi_Core_Data_Transfer, (void*)0, &Task_Multi_Core_Data_Transfer_STK[127], 2, ALL_CORES_ID, SPECIFIC_TRUE);
	
}
/**
  *		������غ���
  */

/**
  * @brief  OSQueue�����ʼ��
  * @param  OSQueueָ��
  * @retval ��
  */
void OSInitQueue(OSQueue* q) {
	q->front = 0;
	q->rear  = 0;
	for(uint8_t i=0;i<OS_QUEUE_SIZE;i++){
			q->data[i] = 0;
	}
}

/**
  * @brief  OSQueue���������
  * @param  OSQueueָ��
  * @param  һ���ֽ�����
  * @retval ��
  */
void OSEnterQueue(OSQueue* q, uint8_t value) {
	q->data[q->rear] = value;
	q->rear = (q->rear + 1) % OS_QUEUE_SIZE;
}

/**
  * @brief  OSQueue���������
  * @param  OSQueueָ��
  * @retval һ���ֽ�����
  */
uint8_t OSOutQueue(OSQueue* q) {
	uint8_t out = q->data[q->front];
	q->front = (q->front + 1) % OS_QUEUE_SIZE;
	return out;
}

/**
  * @brief  FanOSʵ���������ؾ�������񣬽�CPUռ�������ĺ���
	*					��С����ռ������С�����ȵ�CPUռ������С�ĺ���
  */
void OS_Multi_Core_Sched(void* p_arg){
	
#if OS_CRITICAL_METHOD == 3u                                /* Allocate storage for CPU status register     */
    OS_CPU_SR  cpu_sr = 0u;
#endif
		
		/* �����ʼ�� */
		OS_MultiCoreTaskInit();
		uint8_t status;
		/* ��ʱ����ֹcountȫΪ0��ʱ��ʼ���� */
		OSTimeDly(USAGE_MAX_COUNT);
	
		/* ���˵ĵ��� */
		if(OSCoreID == 0){
				uint8_t min_id = 0x00;
				uint8_t max_id = 0x00;
				uint16_t min_count = 0xFFFF;
				uint16_t max_count = 0x0000;
				uint16_t count = 0;
						
				while(1){
						/* ����������Ϊ��ǰ�Ĳ��� */
						min_id = max_id = 0;
						OS_ENTER_CRITICAL();
						min_count = max_count = OSTCBPrioTbl[OS_TASK_IDLE_PRIO]->OSTCBCountSend;
						OS_EXIT_CRITICAL();
					
						/* ������˵�CPUռ���� */
						for(uint8_t pos = 1; pos<OSDevNums;pos++){
								OS_ENTER_CRITICAL();
								status = OS_GetCpuUsage(OSDevAddrs[pos], &count);
								OS_EXIT_CRITICAL();
								/* ״̬����ѯ����һ���� */
								if(status != RES_OK){
										continue;
								}
								/* ����õ���С�����ռ���ʵ�id�� �����ռ���ʵ�������ȵ���Сռ���ʵĺ��� */
								if(count < min_count){
										min_id = pos;
										min_count	 = count;
								}else if(count > max_count){
										max_id = pos;
										max_count = count;
								}
						}
												
						/* ���е���,���ڲ�ֵ�����������õļ�������Ž��е��� */
						if(max_count - min_count > DIFF_COUNT){
								INT8U prio;
								uint16_t size;
								uint8_t* BufferPtr;
							
								if(OSCoreID == min_id){	/* ��ǰ���ռ����Ϊ���ˣ������˵�������ȳ�ȥ */
										OS_ENTER_CRITICAL();
										prio = OSMinCountPrio;
										OS_EXIT_CRITICAL();
										
										/* ���Ϊ0������IDLE����ֱ����ʱ��Continue */
										if(IS_OS_TASK(prio)){
												OSTimeDly(OS_MULTI_CORE_SCHED_DELAY);
												continue;
										}
										
										/* ��ͣ����ռ������С������ */
										OSTaskSuspend(prio);	
										/* ����Stackջָ��ͳ��� */
										OS_TCB* ptcb = OSTCBPrioTbl[prio];
										BufferPtr = (uint8_t*)ptcb->OSTCBStkPtr;
										size = (ptcb->OSTCBStkBasePtr - ptcb->OSTCBStkPtr + 1u) * 4;		
								}else{	/* ��ǰ���ռ����Ϊ��ˣ�����˵�������Ƚ����ˣ��ݴ���RAM�� */
										OS_ENTER_CRITICAL();
										status = OS_TaskSwitchRequest(OSDevAddrs[min_id], &prio);
										OS_EXIT_CRITICAL();
									
										if(status != RES_OK){
												continue;
										}	
										/* ѯ������Ƿ���æ��æ��ȵ� */
										uint8_t isBusy = 1;
										while(isBusy){
												OS_ENTER_CRITICAL();
												status = OS_IsBusy(OSDevAddrs[min_id], &isBusy);
												OS_EXIT_CRITICAL();
												OSTimeDly(OS_BUSY_WAIT_DELAY);
										}
									
										/* ��ȡ��Ӧid�˵�ջ��������ջָ�� */
										OS_ENTER_CRITICAL();
										status = OS_GetStackData(&prio, OSDevAddrs[min_id], Task_Switch_Buffer, &size);
										BufferPtr = Task_Switch_Buffer;
										OS_EXIT_CRITICAL();
										
								}
								
								if(status != RES_OK){
										/* ��ӡ������Ϣ */
								}else if(IS_OS_TASK(prio)){
									/* �����Ӧ�����ȼ�ΪIDLE���ȼ������ܽ���������� */
								}
								else{
									/* ��ջ���ݴ�������Ŀǰռ������С��CPU */
										if(OSCoreID == max_id){ /* ��ǰ��С��idΪ���ˣ������跢��ջ��ֱ�ӽ��п������� */
												/* ����ջ ���� */
												OS_ENTER_CRITICAL();
												OS_MultipleTaskSW(prio, (OS_STK*)BufferPtr, size/4);
												OS_EXIT_CRITICAL();
												/* �ָ���Ӧ������ */
												OSTaskResume(prio);
											
										}else{		/* ��ǰ��С��idΪ��ˣ���Ҫ��ջ���ݴ����ȥ */
												OS_ENTER_CRITICAL();
												status = OS_SendStackData(prio, OSDevAddrs[max_id], BufferPtr, size);
												OS_EXIT_CRITICAL();
													
												if( status != RES_OK ) {
														/* ��ӡ������Ϣ */
												}
										}
								}
						}
						
						
						
						/* ��ʱ */
						OSTimeDly(OS_MULTI_CORE_SCHED_DELAY);
			}
			
		}else	{	/* ��˵ĵ��ȣ�ͨ���ź��������յ�����֮������������л� */
				INT8U err;
				INT8U prio;
				uint16_t size;
			
				while(1){
						/* �ȴ��ź��� */
						OSSemPend(GetStackSem, 0, &err);
												
						/* ����������ջ */					
						prio = iflag.prio_recv;
						size = iflag.size_recv;
						
						/* �����л� */
						OS_MultipleTaskSW(prio, (OS_STK*)Task_Switch_Buffer, size/4);
						/* �������� */
						OSTaskResume(prio);
				}
		}
}

/**
  * @brief  FanOS����������������ĺ�������������ܷ����ж��У�
	*					�����ж��й���ǰ���е���������
  */
void OS_Multi_Core_Task_Suspend(void* p_arg){
		if(OSCoreID == 0){
				/* ���˲���Ҫ�����񣬻���OS_Multi_Core_Sched�������Լ����� */
				OSTaskDel(OS_MULTI_SUSPEND_PRIO);
		}else{
				INT8U err;
				/* ����ڽ��յ��ź���֮����Ҫ����Ӧ��������� */
				while(1){
						/* �ȴ��ź��� */
						OSSemPend(TaskSuspendSem, 0, &err);
						/* �����Ϊϵͳ��������𣬷��򲻹��� */
						if(!IS_OS_TASK(OSSuspendTaskPrio)){
								OSTaskSuspend(OSSuspendTaskPrio);
						}
						
						/* ���Ƿ�æ��Ϊ��æ */
						iflag.is_busy = 0;
				}
		}
}

/**
  * @brief  FanOS��OS_Multi_Core_Data_Transfer�еĻص�����
	*					�����յ����ݵ�ʱ����ᴥ��������ͨ��type��������ͬ�ķ�֧
	*/
void OS_Data_Transfer_Switch_Callback(uint8_t type){
		switch(type){
			case 0:
				break;
			default:
			
				break;
		}
}

/**
  * @brief  FanOS�����ݴ���������Ҫ�Ƕ�˼�����ݴ���Ϳ��������˵�ram��
	*/
void OS_Multi_Core_Data_Transfer(void* p_arg){
	
#if OS_CRITICAL_METHOD == 3u                               /* Allocate storage for CPU status register     */
    OS_CPU_SR  cpu_sr = 0u;
#endif
	
		/* ���˽������ݵ�ת�� */
		if(OSCoreID == 0){
				INT8U err;
				uint8_t status;
				uint16_t size;
				uint32_t address;
				uint8_t type;
				uint8_t tmpDeAddr;
				while(1){
						/* �ȴ��ź��� */
						OSSemPend(DataTransferSem, 0, &err);
						
						/* ��ȡ���� */
						OS_ENTER_CRITICAL();
						tmpDeAddr = OSOutQueue(&os_queue);
						status = OS_GetVariableData(tmpDeAddr, Data_Transfer_Buffer, &size, &address, &type);
						OS_EXIT_CRITICAL();
						if(status != RES_OK){
								continue;
						}
						/* �������� */
						for(uint8_t i=1;i<OSDevNums;i++){
								/* ��ȡ���ݵĺ˲����з��� */
								if(OSDevAddrs[i] == tmpDeAddr)
										continue;
								
								OS_ENTER_CRITICAL();
								status = OS_SendVariableData(OSDevAddrs[i], Data_Transfer_Buffer, size, address, type);
								OS_EXIT_CRITICAL();
								
								if(status != RES_OK){
										continue;
								}
						}
						
						/* ����������ram�� */
						memcpy((uint8_t*) address, Data_Transfer_Buffer, size);
						/* ���ûص����� */
						OS_Data_Transfer_Switch_Callback(type);
				}
		}else{
				INT8U err;
				uint8_t type;
				while(1){
						/* �ȴ��ź��� */
						OSSemPend(DataTransferSem, 0, &err);
						/* ��ȡ���ݰ����� */
						OS_ENTER_CRITICAL();
						type = iflag.type_recv;
						OS_EXIT_CRITICAL();
						/* ���ûص����� */
						OS_Data_Transfer_Switch_Callback(type);
					
				}
		}
}

#endif
