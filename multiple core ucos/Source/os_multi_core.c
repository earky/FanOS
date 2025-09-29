#include "os_cfg.h"

#include "I2C.h"
#include "GPIO.h"
#include "ucos_ii.h"
#include <string.h>

#if OS_MULTIPLE_CORE > 0u

/**
	* OS所需要的全局变量
	*/

/* OS核对应的ID号，主核ID号一定为0!!! */
uint8_t OSCoreID = 1;

OS_EVENT* GetStackSem;				//获取堆栈的信号量
OS_EVENT* SendDataSem;				//发送堆栈的信号量
OS_EVENT* TaskSuspendSem;			//任务挂起的信号量
OS_EVENT* DataTransferSem;		//数据传输的信号量

OSQueue os_queue;							//OS循环队列，主要用于数据传输临时存放对应的外核I2C地址

uint8_t OSDevAddrs[] = {I2C_MASTER_ADDRESS, I2C_SLAVE_ADDRESS};		//通过OSCoreID来访问外核I2C地址，
uint8_t OSDevNums = sizeof(OSDevAddrs) / sizeof(uint8_t);					//多核的数量（主核＋外核）

uint8_t  OSMinCountPrio = OS_TASK_IDLE_PRIO;	//OS当前占用率最低任务的优先级
uint16_t OSMinCount     = USAGE_MAX_COUNT;		//OS当前占用率最低任务的占用计数
uint32_t total_count;													//OS总计数

uint8_t OSSuspendTaskPrio = 0;								//需要挂起任务的优先级

uint8_t Task_Switch_Buffer[512];							//任务切换的缓冲区
uint8_t Data_Transfer_Buffer[512];            //数据传输的缓冲区
OS_STK Task_Multi_Core_Sched_STK[128];				//负载均衡任务栈
OS_STK Task_Multi_Core_Suspend_STK[128];			//任务挂起任务栈
OS_STK Task_Multi_Core_Data_Transfer_STK[128];//数据传输任务栈

/**
	*   多核ucos与硬件层的抽象函数
	*   如果需要使用非SPI的其它硬件实现
	*   可以修改该层
	*/

/**
	* @brief  从外核读取数据
  * @param  外核的I2C地址
  * @param  指向数据的指针
  * @param  长度指针，用于获取长度
  * @retval 当前状态
  */
uint8_t OS_Read(uint8_t devAddr, uint8_t *data, uint16_t* len)
{
		return I2C2_Read(devAddr, data, len);
}

/**
	* @brief  向外核写数据
  * @param  外核的I2C地址
  * @param  写入的模式
  * @param  指向数据的指针
  * @param  长度
  * @retval 当前状态
  */
uint8_t OS_Write(uint8_t devAddr, uint8_t mode, uint8_t *data, uint16_t len)
{
		return I2C2_Write(devAddr, mode, data, len);
}

/**
	* @brief  从外核读取数据，但会先读len1指定长度到buf1，再读取size长度到buf2，并存放再len2中
  * @param  外核的I2C地址
  * @param  指向数据的指针1
  * @param  指向数据的指针1
  * @param  长度，用于预先读取数据包头
  * @param  长度指针，用于获取数据长度
  * @retval 当前状态
  */
uint8_t OS_Read2(uint8_t devAddr, uint8_t *buf1, uint8_t* buf2, uint16_t len1, uint16_t* len2)
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
		
    // 3. 接收数据包头数据
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
		
		
		// 4. 接收实际数据
    for(uint16_t i = 0; i < *len2; i++) {
				// 在接收最后一个字节前禁用ACK并发送STOP
				if(i == *len2 - 1) {
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
				buf2[i] = I2C_ReceiveData(I2C2);
				
		}
		
		I2C_AcknowledgeConfig(I2C2, ENABLE); // 恢复ACK使能
    return 0;  // 成功
}

/**
	* @brief  向外核写数据，先写入buf1，再写入buf2
  * @param  外核的I2C地址
  * @param  写入的模式
  * @param  指向数据的指针1
  * @param  指向数据的指针2
  * @param  长度1
  * @param  长度2
  * @retval 当前状态
  */
uint8_t OS_Write2(uint8_t devAddr, uint8_t mode, uint8_t *buf1, uint8_t *buf2, uint16_t len1, uint16_t len2)
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
				
    // 4. 发送数据buf1
    for(uint16_t i = 0; i < len1; i++) {
        I2C_SendData(I2C2, buf1[i]);
        timeout = 100000;
        while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
            if(--timeout == 0) return 4;
        }
    }
    
		// 5. 发送数据buf2
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
		
    // 6. 发送停止条件
    I2C_GenerateSTOP(I2C2, ENABLE);
    return 0;  // 成功
}


/**
	* 协议的实现
	*/

/**
  * @brief  从外核获取堆栈的数据，长度，优先级
  * @param  任务优先级指针，用于获取优先级
  * @param  外核的I2C地址
  * @param  指向堆栈的指针，强转为uint8_t*类型
  * @param  长度指针，用于获取长度
  * @retval 当前状态
  */
uint8_t OS_GetStackData(INT8U* prio, uint8_t devAddr, uint8_t* buf, uint16_t* size)
{	
		uint8_t status; 

		/* 改变外核状态机模式 */
		status  = OS_Write(devAddr, GET_STACK_DATA, NULL, 0);
		if(status)
		{
				return status;
		}
		
		/* 获取数据 */
		status = OS_Read2(devAddr, prio, buf, sizeof(INT8U), size);
		if(status)
		{
				return status;
		}
		
		return RES_OK;
}

/**
	* @brief  向外核发送堆栈的数据，长度，优先级
  * @param  任务优先级
  * @param  外核的I2C地址
  * @param  指向堆栈的指针，强转为uint8_t*类型
  * @param  长度
  * @retval 当前状态
  */
uint8_t OS_SendStackData(INT8U target_prio, uint8_t devAddr, uint8_t* buf, uint16_t size)
{
		uint8_t status, size_h, size_l;

		/* 将size拆分为高低地址用于传输 */
		size_h = (size >> 8) & 0x00FF;
		size_l = size & 0x00FF;
	
		uint8_t header[3] = { size_h, size_l, target_prio};
		
		/* 发送数据包头和数据 */
		status = OS_Write2(devAddr, SEND_STACK_DATA, header, buf, 3, size);
		if(status)
		{
				return status;
		}
		
		return RES_OK;
}

/**
	* @brief  从外核获取伪共享内存的数据，长度，地址，类型
  * @param  外核的I2C地址
  * @param  指向数据的指针，强转为uint8_t*类型
  * @param  长度指针，用于获取长度
  * @param  地址指针，用于获取地址
  * @param  类型指针，用于获取类型
  * @retval 当前状态
  */
uint8_t OS_GetVariableData(uint8_t devAddr, uint8_t* buf, uint16_t* size, uint32_t* address, uint8_t* type)
{
		uint8_t status;
		
		/* 改变外核状态机模式 */
		status  = OS_Write(devAddr, GET_VARIABLE_DATA, NULL, 0);
		if(status)
		{
				return status;
		}
		
	  /* 获取数据 */
		uint8_t header[5];
		status = OS_Read2(devAddr, header, buf, 5, size);
		
		if(status)
		{
				return status;
		}
		
		/* 解析地址 */
		*address = (header[3] << 24) |
							 (header[2] << 16) |
							 (header[1] << 8)  |
							 (header[0] << 0);
		
		/* 解析数据的类型 */
		*type = header[4];
		
		return RES_OK;
}

/**
	* @brief  向外核发送伪共享内存的数据，长度，地址，类型
  * @param  外核的I2C地址
  * @param  指向数据的指针，强转为uint8_t*类型
  * @param  长度
  * @param  地址
  * @param  类型
  * @retval 当前状态
  */
uint8_t OS_SendVariableData(uint8_t devAddr, uint8_t* buf, uint16_t size, uint32_t address, uint8_t type)
{
		uint8_t status;
		uint8_t header[7];
		
		/* 将长度拆分为高低地址 */
		header[0] = (size >> 8) & 0x00FF;
		header[1] = (size) & 0x00FF;
	
		/* 将地址拆分为4部分 */
		for(uint8_t i = 2; i < 6; i++){
				header[i] = address & 0x000000FF;
				address = address >> 8;
		}
		
		/* 数据的类型 */
		header[6] = type;
		
		/* 传输数据 */
		status  = OS_Write2(devAddr, SEND_VARIABLE_DATA, header, buf, 7, size);
	
		if(status)
		{
				return status;
		}
	
		return RES_OK;
}

/**
	* @brief  获取外核CPU的使用率
  * @param  外核的I2C地址
  * @param  计数指针，用于获取计数
  * @retval 当前状态
  */
uint8_t OS_GetCpuUsage(uint8_t devAddr, uint16_t* count)
{
		uint8_t status;	
		uint16_t len;
	
		/* 改变外核状态机模式 */
		status  = OS_Write(devAddr, GET_CPU_USAGE, NULL, 0);
		if(status)
		{
				return status;
		}
		
		/* 获取数据 */
		status = OS_Read2(devAddr, NULL, (uint8_t*)count, 0, &len);
		if(status)
		{
				return status;
		}
	
		return RES_OK;
}

/**
	* @brief  主核向外核发起任务切换的请求，让外核先停止任务
  * @param  外核的I2C地址
  * @param  计数指针，用于获取计数
  * @retval 当前状态
  */
uint8_t OS_TaskSwitchRequest(uint8_t devAddr, INT8U* prio)
{
		uint8_t status;	
		uint16_t len;
	
		/* 改变外核状态机模式 */
		status  = OS_Write(devAddr, TASK_SWITCH_REQUEST, NULL, 0);
		if(status)
		{
				return status;
		}
		
		/* 获取数据 */
		status = OS_Read2(devAddr, NULL, (uint8_t*)prio, 0, &len);
		if(status)
		{
				return status;
		}
	
		return RES_OK;
}

/**
	* @brief  主核向外核询问是否处于忙状态，若忙主核则应该等待
  * @param  外核的I2C地址
  * @param  是否忙指针，用于获取外核的状态
  * @retval 当前状态
  */
uint8_t OS_IsBusy(uint8_t devAddr, uint8_t* isBusy)
{
		uint8_t status;	
		uint16_t len;
	
		/* 改变外核状态机模式 */
		status  = OS_Write(devAddr, IS_BUSY, NULL, 0);
		if(status)
		{
				return status;
		}
		
		/* 获取数据 */
		status = OS_Read2(devAddr, NULL, isBusy, 0, &len);
		if(status)
		{
				return status;
		}
	
		return RES_OK;

}

/**
	*	多核有关的初始化和任务切换函数
	*/

/**
  * @brief  多核初始化任务，对于不属于自己任务的ID需要挂起
  * @param  任务优先级
  * @param  堆栈指针
  * @param  堆栈长度
  * @retval 无
  */
uint8_t OS_MultipleTaskSW(INT8U	  prio,
													OS_STK* stk,
													OS_STK  len)
{
    OS_TCB  *ptcb;	
		OS_STK  *p_stk;
    
		/* 任务栈不能为0 */
    if(len == 0){
        return TASK_SWITCH_BUFFER_IS_ZERO;
		}
		
    ptcb = OSTCBPrioTbl[prio];
    /* 任务不存在 */
    if(ptcb == (OS_TCB*)0){
				return TASK_NOT_EXITS;
    }

		p_stk = ptcb->OSTCBStkBasePtr + 1u;

    for(int i = len - 1;i >= 0; i--){
			*(--p_stk) = stk[i];
		}

    /* 更新对应的Stk指针		*/
    ptcb->OSTCBStkPtr = p_stk;
		/* 唤醒任务 */
		OSTaskResume(prio);
    return TASK_SWITCH_SUCCESS;
}

/**
  * @brief  多核初始化任务，对于不属于自己任务的ID需要挂起
  * @param  无
  * @retval 无
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
  * @brief  多核初始化信号量
  * @param  无
  * @retval 无
  */
void OSMultiSemInit(void)
{
		GetStackSem = OSSemCreate(0);
		SendDataSem = OSSemCreate(0);
		TaskSuspendSem = OSSemCreate(0);
		DataTransferSem = OSSemCreate(0);
}

/**
  * @brief  多核硬件初始化
  * @param  无
  * @retval 无
  */
void OSMultiHardwareInit(void)
{
	/* 主核初始化 */
	if(OSCoreID == 0){
		I2C2_Master_Init();
		GPIO_EXTI_Init();
	}
	else{	/* 外核初始化 */
		I2C2_Slave_Init(I2C_SLAVE_ADDRESS);
		Slave_GPIO_Init();
	}
}

/**
  * @brief  多核硬件初始化
  * @param  无
  * @retval 无
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
  *		队列相关函数
  */

/**
  * @brief  OSQueue对象初始化
  * @param  OSQueue指针
  * @retval 无
  */
void OSInitQueue(OSQueue* q) {
	q->front = 0;
	q->rear  = 0;
	for(uint8_t i=0;i<OS_QUEUE_SIZE;i++){
			q->data[i] = 0;
	}
}

/**
  * @brief  OSQueue对象入队列
  * @param  OSQueue指针
  * @param  一个字节数据
  * @retval 无
  */
void OSEnterQueue(OSQueue* q, uint8_t value) {
	q->data[q->rear] = value;
	q->rear = (q->rear + 1) % OS_QUEUE_SIZE;
}

/**
  * @brief  OSQueue对象出队列
  * @param  OSQueue指针
  * @retval 一个字节数据
  */
uint8_t OSOutQueue(OSQueue* q) {
	uint8_t out = q->data[q->front];
	q->front = (q->front + 1) % OS_QUEUE_SIZE;
	return out;
}

/**
  * @brief  FanOS实现自主负载均衡的任务，将CPU占用率最大的核中
	*					最小任务（占用率最小）调度到CPU占用率最小的核中
  */
void OS_Multi_Core_Sched(void* p_arg){
	
#if OS_CRITICAL_METHOD == 3u                                /* Allocate storage for CPU status register     */
    OS_CPU_SR  cpu_sr = 0u;
#endif
		
		/* 任务初始化 */
		OS_MultiCoreTaskInit();
		uint8_t status;
		/* 延时，防止count全为0的时候开始调度 */
		OSTimeDly(USAGE_MAX_COUNT);
	
		/* 主核的调度 */
		if(OSCoreID == 0){
				uint8_t min_id = 0x00;
				uint8_t max_id = 0x00;
				uint16_t min_count = 0xFFFF;
				uint16_t max_count = 0x0000;
				uint16_t count = 0;
						
				while(1){
						/* 将主核设置为当前的参数 */
						min_id = max_id = 0;
						OS_ENTER_CRITICAL();
						min_count = max_count = OSTCBPrioTbl[OS_TASK_IDLE_PRIO]->OSTCBCountSend;
						OS_EXIT_CRITICAL();
					
						/* 遍历外核的CPU占用率 */
						for(uint8_t pos = 1; pos<OSDevNums;pos++){
								OS_ENTER_CRITICAL();
								status = OS_GetCpuUsage(OSDevAddrs[pos], &count);
								OS_EXIT_CRITICAL();
								/* 状态错误，询问下一个核 */
								if(status != RES_OK){
										continue;
								}
								/* 计算得到最小和最大占用率的id， 将最大占用率的任务调度到最小占用率的核中 */
								if(count < min_count){
										min_id = pos;
										min_count	 = count;
								}else if(count > max_count){
										max_id = pos;
										max_count = count;
								}
						}
												
						/* 进行调度,仅在差值大于我们设置的间隔计数才进行调度 */
						if(max_count - min_count > DIFF_COUNT){
								INT8U prio;
								uint16_t size;
								uint8_t* BufferPtr;
							
								if(OSCoreID == min_id){	/* 当前最大占用率为主核，将主核的任务调度出去 */
										OS_ENTER_CRITICAL();
										prio = OSMinCountPrio;
										OS_EXIT_CRITICAL();
										
										/* 如果为0或者是IDLE任务直接延时＋Continue */
										if(IS_OS_TASK(prio)){
												OSTimeDly(OS_MULTI_CORE_SCHED_DELAY);
												continue;
										}
										
										/* 暂停主核占用率最小的任务 */
										OSTaskSuspend(prio);	
										/* 设置Stack栈指针和长度 */
										OS_TCB* ptcb = OSTCBPrioTbl[prio];
										BufferPtr = (uint8_t*)ptcb->OSTCBStkPtr;
										size = (ptcb->OSTCBStkBasePtr - ptcb->OSTCBStkPtr + 1u) * 4;		
								}else{	/* 当前最大占用率为外核，将外核的任务调度进主核，暂存在RAM中 */
										OS_ENTER_CRITICAL();
										status = OS_TaskSwitchRequest(OSDevAddrs[min_id], &prio);
										OS_EXIT_CRITICAL();
									
										if(status != RES_OK){
												continue;
										}	
										/* 询问外核是否在忙，忙则等地 */
										uint8_t isBusy = 1;
										while(isBusy){
												OS_ENTER_CRITICAL();
												status = OS_IsBusy(OSDevAddrs[min_id], &isBusy);
												OS_EXIT_CRITICAL();
												OSTimeDly(OS_BUSY_WAIT_DELAY);
										}
									
										/* 获取对应id核的栈，并设置栈指针 */
										OS_ENTER_CRITICAL();
										status = OS_GetStackData(&prio, OSDevAddrs[min_id], Task_Switch_Buffer, &size);
										BufferPtr = Task_Switch_Buffer;
										OS_EXIT_CRITICAL();
										
								}
								
								if(status != RES_OK){
										/* 打印错误信息 */
								}else if(IS_OS_TASK(prio)){
									/* 如果对应的优先级为IDLE优先级，则不能进行任务调度 */
								}
								else{
									/* 将栈数据传送至到目前占用率最小的CPU */
										if(OSCoreID == max_id){ /* 当前最小的id为主核，则无需发送栈，直接进行拷贝即可 */
												/* 拷贝栈 数据 */
												OS_ENTER_CRITICAL();
												OS_MultipleTaskSW(prio, (OS_STK*)BufferPtr, size/4);
												OS_EXIT_CRITICAL();
												/* 恢复对应的任务 */
												OSTaskResume(prio);
											
										}else{		/* 当前最小的id为外核，需要将栈数据传输出去 */
												OS_ENTER_CRITICAL();
												status = OS_SendStackData(prio, OSDevAddrs[max_id], BufferPtr, size);
												OS_EXIT_CRITICAL();
													
												if( status != RES_OK ) {
														/* 打印错误信息 */
												}
										}
								}
						}
						
						
						
						/* 延时 */
						OSTimeDly(OS_MULTI_CORE_SCHED_DELAY);
			}
			
		}else	{	/* 外核的调度，通过信号量，接收到数据之后则进行任务切换 */
				INT8U err;
				INT8U prio;
				uint16_t size;
			
				while(1){
						/* 等待信号量 */
						OSSemPend(GetStackSem, 0, &err);
												
						/* 拷贝到任务栈 */					
						prio = iflag.prio_recv;
						size = iflag.size_recv;
						
						/* 任务切换 */
						OS_MultipleTaskSW(prio, (OS_STK*)Task_Switch_Buffer, size/4);
						/* 唤醒任务 */
						OSTaskResume(prio);
				}
		}
}

/**
  * @brief  FanOS中外核用于任务挂起的函数，任务挂起不能放在中断中，
	*					当在中断中挂起当前运行的任务会出错
  */
void OS_Multi_Core_Task_Suspend(void* p_arg){
		if(OSCoreID == 0){
				/* 主核不需要该任务，会在OS_Multi_Core_Sched函数中自己挂起 */
				OSTaskDel(OS_MULTI_SUSPEND_PRIO);
		}else{
				INT8U err;
				/* 外核在接收到信号量之后需要将对应的任务挂起 */
				while(1){
						/* 等待信号量 */
						OSSemPend(TaskSuspendSem, 0, &err);
						/* 如果不为系统任务，则挂起，否则不挂起 */
						if(!IS_OS_TASK(OSSuspendTaskPrio)){
								OSTaskSuspend(OSSuspendTaskPrio);
						}
						
						/* 将是否忙置为不忙 */
						iflag.is_busy = 0;
				}
		}
}

/**
  * @brief  FanOS在OS_Multi_Core_Data_Transfer中的回调函数
	*					当接收到数据的时候，则会触发，可以通过type来触发不同的分支
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
  * @brief  FanOS的数据传输任务，主要是多核间的数据传输和拷贝到本核的ram中
	*/
void OS_Multi_Core_Data_Transfer(void* p_arg){
	
#if OS_CRITICAL_METHOD == 3u                               /* Allocate storage for CPU status register     */
    OS_CPU_SR  cpu_sr = 0u;
#endif
	
		/* 主核进行数据的转发 */
		if(OSCoreID == 0){
				INT8U err;
				uint8_t status;
				uint16_t size;
				uint32_t address;
				uint8_t type;
				uint8_t tmpDeAddr;
				while(1){
						/* 等待信号量 */
						OSSemPend(DataTransferSem, 0, &err);
						
						/* 获取数据 */
						OS_ENTER_CRITICAL();
						tmpDeAddr = OSOutQueue(&os_queue);
						status = OS_GetVariableData(tmpDeAddr, Data_Transfer_Buffer, &size, &address, &type);
						OS_EXIT_CRITICAL();
						if(status != RES_OK){
								continue;
						}
						/* 发送数据 */
						for(uint8_t i=1;i<OSDevNums;i++){
								/* 获取数据的核不进行发送 */
								if(OSDevAddrs[i] == tmpDeAddr)
										continue;
								
								OS_ENTER_CRITICAL();
								status = OS_SendVariableData(OSDevAddrs[i], Data_Transfer_Buffer, size, address, type);
								OS_EXIT_CRITICAL();
								
								if(status != RES_OK){
										continue;
								}
						}
						
						/* 拷贝到本核ram中 */
						memcpy((uint8_t*) address, Data_Transfer_Buffer, size);
						/* 调用回调函数 */
						OS_Data_Transfer_Switch_Callback(type);
				}
		}else{
				INT8U err;
				uint8_t type;
				while(1){
						/* 等待信号量 */
						OSSemPend(DataTransferSem, 0, &err);
						/* 获取数据包类型 */
						OS_ENTER_CRITICAL();
						type = iflag.type_recv;
						OS_EXIT_CRITICAL();
						/* 调用回调函数 */
						OS_Data_Transfer_Switch_Callback(type);
					
				}
		}
}

#endif
