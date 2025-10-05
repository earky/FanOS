#ifndef __I2C_H
#define __I2C_H

#include "stm32f10x.h"  
#include "ucos_ii.h"
// 从机地址定义
#define I2C_SLAVE_ADDRESS1  0x40 
#define I2C_SLAVE_ADDRESS2  0x30

#define I2C_MASTER_ADDRESS 0x20		//可以用于切换


typedef struct{
	uint16_t idx_recv;
	uint16_t idx_send;
	
	uint8_t* ptr_recv;
	uint8_t* ptr_send;

	uint16_t size_recv;
	uint16_t size_send;
	
	INT8U prio_recv;
	INT8U prio_send;
	
	uint8_t type_recv;
	uint8_t type_send;
	
	uint32_t address_recv;
	uint32_t address_send;
	
	uint8_t  is_first_data;
	uint8_t  mode;
	
	uint8_t  is_busy;
	uint8_t  is_send;
}I2C_Flag;

extern uint8_t I2C_Address[3];
extern I2C_Flag iflag;

void I2C2_Master_Init(void);
void I2C2_Slave_Init(uint8_t address);
uint8_t I2Cx_Write(uint8_t devAddr, uint8_t *pData, uint16_t len);
uint8_t I2Cx_Read(uint8_t devAddr, uint8_t *pData, uint16_t len);
void I2C_Slave_Init(uint8_t address);
void I2C1_EV_IRQHandler(void);

uint8_t I2C2_Read(uint8_t devAddr, uint8_t *data, uint16_t* len);
uint8_t I2C2_Write(uint8_t devAddr, uint8_t mode, uint8_t *data, uint16_t len);

extern int i;
#endif
