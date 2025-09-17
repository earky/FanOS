#include "os_cfg.h"

#if OS_MULTIPLE_CORE > 0u

#include "I2C.h"
#include "ucos_ii.h"

uint8_t OSDevAddrTbl[64] = {I2C_SLAVE_ADDRESS, I2C_SLAVE_ADDRESS, I2C_SLAVE_ADDRESS, I2C_SLAVE_ADDRESS, I2C_SLAVE_ADDRESS, I2C_SLAVE_ADDRESS, I2C_SLAVE_ADDRESS, I2C_SLAVE_ADDRESS,
	I2C_SLAVE_ADDRESS, I2C_SLAVE_ADDRESS,I2C_SLAVE_ADDRESS,I2C_SLAVE_ADDRESS,I2C_SLAVE_ADDRESS,I2C_SLAVE_ADDRESS,I2C_SLAVE_ADDRESS};
OS_STK* OSStackPtrTbl[64] = {;

uint8_t buffer[128];
uint16_t buffer_len;

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

// �ȶ�ȡlen1����buf���ٶ�ȡlen2��������
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
uint8_t OS_GetStackData(INT8U prio, uint8_t* buf, uint16_t* size)
{	
		uint8_t status;
		uint8_t devAddr = OSDevAddrTbl[prio];
		//uint8_t devAddr = I2C_SLAVE_ADDRESS;
	
		// 1.���� �ı�ģʽ
		status  = OS_Write(devAddr, GET_STACK_DATA, NULL, 0);
		if(status)
		{
				return status;
		}
		
		// 2.��ȡ����
		status = OS_Read(devAddr, buf, size);
		if(status)
		{
				return status;
		}
		
		return RES_OK;
}

uint8_t OS_SendStackData(uint8_t devAddr, uint8_t* buf, uint16_t size)
{
		uint8_t status, size_h, size_l;
		//uint8_t devAddr = OSDevAddrTbl[prio];
		//uint8_t devAddr = I2C_SLAVE_ADDRESS;
	
		size_h = (size >> 8) & 0x00FF;
		size_l = size & 0x00FF;
	
		uint8_t buf_tmp[3] = { size_h, size_l, prio };
		
		status = OS_Write2(devAddr, SEND_STACK_DATA, buf_tmp, buf, 3, size);
		if(status)
		{
				return status;
		}
		
		return RES_OK;
}

uint8_t OS_GetVariableData(INT8U prio, INT8U* target_prio, uint8_t* buf, uint16_t* size, uint32_t* address)
{
		uint8_t status;
		uint8_t devAddr = OSDevAddrTbl[prio];
		
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
		for(uint8_t i = 0;i<4;i++){
			*address |= header[i];
			*address = *address << 8;
		}
		
		// ����Ŀ�����ȼ�
		*target_prio = header[4];
		return RES_OK;
}

uint8_t OS_SendVariableData(INT8U prio, uint8_t* buf, uint16_t size, uint16_t address)
{
		uint8_t status;
		uint8_t devAddr = OSDevAddrTbl[prio];
		uint8_t header[7];
		
		header[0] = (size >> 8) & 0x00FF;
		header[1] = (size) & 0x00FF;
	
		for(uint8_t i = 0; i < 4; i++){
				header[i + 2] = address & 0x000000FF;
				address = address >> 8;
		}
		
		header[6] = prio;
		
		status  = OS_Write2(devAddr, SEND_VARIABLE_DATA, header, buf, 7, size);
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


#endif