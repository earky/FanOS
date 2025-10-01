//#include "stm32f10x.h"                  // Device header
//#include "ucos_ii.h"

///**
//  * 函    数：SPI写SS引脚电平，SS仍由软件模拟
//  * 参    数：BitValue 协议层传入的当前需要写入SS的电平，范围0~1
//  * 返 回 值：无
//  * 注意事项：此函数需要用户实现内容，当BitValue为0时，需要置SS为低电平，当BitValue为1时，需要置SS为高电平
//  */
//void SPI_W_SS(uint8_t BitValue)
//{
//	GPIO_WriteBit(GPIOA, GPIO_Pin_4, (BitAction)BitValue);		//根据BitValue，设置SS引脚的电平
//}

///**
//  * 函    数：SPI初始化
//  * 参    数：无
//  * 返 回 值：无
//  */
//void SPI_Master_Init(void)
//{
//	/*开启时钟*/
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//开启GPIOA的时钟
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);	//开启SPI1的时钟
//	
//	/*GPIO初始化*/
//	GPIO_InitTypeDef GPIO_InitStructure;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_Init(GPIOA, &GPIO_InitStructure);					//将PA4引脚初始化为推挽输出
//	
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_Init(GPIOA, &GPIO_InitStructure);					//将PA5和PA7引脚初始化为复用推挽输出
//	
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_Init(GPIOA, &GPIO_InitStructure);					//将PA6引脚初始化为上拉输入
//	
//	/*SPI初始化*/
//	SPI_InitTypeDef SPI_InitStructure;						//定义结构体变量
//	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;			//模式，选择为SPI主模式
//	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;	//方向，选择2线全双工
//	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;		//数据宽度，选择为8位
//	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;		//先行位，选择高位先行
//	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_128;	//波特率分频，选择128分频
//	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;				//SPI极性，选择低极性
//	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;			//SPI相位，选择第一个时钟边沿采样，极性和相位决定选择SPI模式0
//	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;				//NSS，选择由软件控制
//	SPI_InitStructure.SPI_CRCPolynomial = 7;				//CRC多项式，暂时用不到，给默认值7
//	SPI_Init(SPI1, &SPI_InitStructure);						//将结构体变量交给SPI_Init，配置SPI1
//	
//	/*SPI使能*/
//	SPI_Cmd(SPI1, ENABLE);									//使能SPI1，开始运行
//	
//	/*设置默认电平*/
//	SPI_W_SS(1);											//SS默认高电平
//}

///**
//  * 函    数：SPI初始化
//  * 参    数：无
//  * 返 回 值：无
//  */
//void SPI_Slave_Init(void)
//{
//    /*开启时钟*/
//    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);    //开启GPIOA的时钟
//    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);    //开启SPI1的时钟
//    
//    /*GPIO初始化 - 修正部分*/
//    GPIO_InitTypeDef GPIO_InitStructure;
//    
//    // NSS引脚(PA4)应配置为输入模式，由主机控制
//    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;  // 改为浮空输入
//    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
//    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//    GPIO_Init(GPIOA, &GPIO_InitStructure);
//    
//		/*配置PA4的外部中断*/
//    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource4); // 将PA4连接到EXTI4
//    
//    EXTI_InitTypeDef EXTI_InitStructure;
//    EXTI_InitStructure.EXTI_Line = EXTI_Line4;
//    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
//    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling; // 上升沿触发（主机释放NSS）
//    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
//    EXTI_Init(&EXTI_InitStructure);
//	
//    // SCK(PA5)和MOSI(PA7)应配置为浮空输入或复用功能
//    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;  // 或 GPIO_Mode_AF_PP
//    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
//    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//    GPIO_Init(GPIOA, &GPIO_InitStructure);
//    
//    // MISO(PA6)作为从机输出，应配置为复用推挽输出
//    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
//    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
//    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//    GPIO_Init(GPIOA, &GPIO_InitStructure);
//    
//		
//    /*SPI初始化 - 修正部分*/
//    SPI_InitTypeDef SPI_InitStructure;
//    SPI_InitStructure.SPI_Mode = SPI_Mode_Slave;            // 正确：从机模式
//    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex; // 正确
//    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;       // 正确
//    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;      // 正确
//    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2; // 从机模式下此参数无效，但需保留一个有效值
//    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;              // 必须与主机一致
//    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;            // 必须与主机一致
//    SPI_InitStructure.SPI_NSS = SPI_NSS_Hard;               // 改为硬件控制NSS
//    SPI_InitStructure.SPI_CRCPolynomial = 7;                // 正确
//    SPI_Init(SPI1, &SPI_InitStructure);
//    
//		// 使能SPI接收中断
//		SPI_I2S_ITConfig(SPI1, SPI_I2S_IT_RXNE, ENABLE);

//		// 配置NVIC（嵌套向量中断控制器）
//		NVIC_InitTypeDef NVIC_InitStructure;
//		NVIC_InitStructure.NVIC_IRQChannel = SPI1_IRQn;
//		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01;
//		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01;
//		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//		NVIC_Init(&NVIC_InitStructure);
//		
//		/*配置EXTI4中断优先级*/
//		NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;
//    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
//    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//    NVIC_Init(&NVIC_InitStructure);
//		
//    /*SPI使能*/
//    SPI_Cmd(SPI1, ENABLE);
//    
//    /*删除SS电平设置代码，因为NSS现在由主机控制*/
//    // SPI_W_SS(1); // 这行应该删除
//}
///**
//  * 函    数：SPI起始
//  * 参    数：无
//  * 返 回 值：无
//  */
//void SPI_Start(void)
//{
//	SPI_W_SS(0);				//拉低SS，开始时序
//}

///**
//  * 函    数：SPI终止
//  * 参    数：无
//  * 返 回 值：无
//  */
//void SPI_Stop(void)
//{
//	SPI_W_SS(1);				//拉高SS，终止时序
//}

///**
//  * 函    数：SPI交换传输一个字节，使用SPI模式0
//  * 参    数：ByteSend 要发送的一个字节
//  * 返 回 值：接收的一个字节
//  */
//uint8_t SPI_SwapByte(uint8_t ByteSend)
//{
//	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) != SET);	//等待发送数据寄存器空
//	
//	SPI_I2S_SendData(SPI1, ByteSend);								//写入数据到发送数据寄存器，开始产生时序
//	
//	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) != SET);	//等待接收数据寄存器非空
//	
//	return SPI_I2S_ReceiveData(SPI1);								//读取接收到的数据并返回
//}

//// wait to finish
///*
//*	返回0说明不存在 返回1说明存在
//*/
//OS_STK Stack1[] = {
//0000,0x2710,0000,0x7070707,0x8080808,0x9090909,0x10101010,
//0x11111111,0xfffffffd,0000,0x20000295,0x40013800,0x80,0x12121212,
//0x800152d,0x800152c,0x21000000,0x6e,0x5050505,0x6060606,0x800228b,0000
//};

//OS_STK Stack_len1 = sizeof(Stack1) / sizeof(OS_STK);
//uint8_t Is_Prio_Task_Exists(INT8U prio)
//{
//	return 1;
//}

//uint32_t Get_Stack_Size(INT8U prio)
//{
//	return 829;
//}

//uint8_t* Get_Stack_Ptr(INT8U prio)
//{
//	return (uint8_t*)Stack1;
//}

//uint8_t  Type = 0xFF;
//uint8_t  Mode = 0;
//uint8_t  isFirstData = 1;
//uint32_t idx = 0;
//void Status_Clear(void)
//{
//	Type = 0xFF;
//	isFirstData = 1;
//	idx = 0;
//	Mode = 0;
//}

//void Get_Stack_Data_Handler(uint8_t data)
//{
//		static INT8U    prio = 0;
//		static uint32_t size = 0;
//		static uint8_t* ptr;
//		uint8_t t_size;
//	
//		switch(Mode){
//			// 获取目标的栈优先级
//			case REQ_PRIO:
//				prio = data;
//				size = 0;
//				// 判断任务是否创建
//				if(!Is_Prio_Task_Exists(prio)){
//					SPI_I2S_SendData(SPI1, STACK_NOT_CREATED);
//					Status_Clear();
//				}else{
//					SPI_I2S_SendData(SPI1, RES_OK);
//					Mode = REQ_SIZE;
//					size = Get_Stack_Size(prio);
//					ptr  = Get_Stack_Ptr(prio);
//				}
//				break;
//				
//			case REQ_SIZE:
//				t_size = size & 0x000000FF;
//				SPI_I2S_SendData(SPI1, t_size);
//				
//				size = (size & 0xFFFFFF00) >> 8;
//				
//				break;
//			
//			case REQ_DATA:
//				SPI_I2S_SendData(SPI1, ptr[idx++]);
//				break;
//			
//			default:
//				SPI_I2S_SendData(SPI1, PROTOCOL_ERROR);
//				Status_Clear();
//				break;
//		}

//}

//// 0x01 GetStackData
//char tr[20];
//void SPI1_IRQHandler(void)
//{
//    if (SPI_I2S_GetITStatus(SPI1, SPI_I2S_IT_RXNE) != RESET)
//    {
//        // 读取接收到的数据
//        uint8_t data = SPI_I2S_ReceiveData(SPI1);
//				sprintf(tr, "%u\n", data);
//				Serial_SendString(tr);
////				SPI_I2S_SendData(SPI1, data + 1);
////				return;
//			
//				// 获取第一个数据作为Type
//				if(isFirstData){
//						Type = data;
//						
//						// 初始化Mode
//						switch(Type){
//							case GET_STACK_DATA:
//								SPI_I2S_SendData(SPI1, RES_OK);
//								Mode = REQ_PRIO;
//								break;
//							
//							default:
//								Status_Clear();
//								return;
//								break;
//						}
//						
//						isFirstData = 0;
//						return;
//				}
//				
//				
//				switch(Type){
//					// 0x01 获取栈数据
//					case GET_STACK_DATA:
//						Get_Stack_Data_Handler(data);
//						break;
//						
//					default:
//						SPI_I2S_SendData(SPI1, PROTOCOL_ERROR);
//						Status_Clear();
//						break;				
//				}
//			
//        // 同时可以准备要发送的数据
//        //SPI_I2S_SendData(SPI1, data + 1);
//    }
//}

//// EXTI4中断服务程序
//void EXTI4_IRQHandler(void)
//{
//    if(EXTI_GetITStatus(EXTI_Line4) != RESET)
//    {
//        // 检查NSS引脚是否为高电平（主机释放片选）
//        //if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_4) == SET)
//        //{
//            // 设置传输完成标志
//            //spi_transfer_complete = 1;
//            
//            // 这里可以处理接收到的数据
//            // 例如: process_received_data(spi_rx_buffer, spi_rx_index);
//            Status_Clear();
//						Serial_SendString("EXIT4_IQHandler\n");
//            // 重置接收索引
//            //spi_rx_index = 0;
//        //}
//        
//        // 清除EXTI4中断标志
//        EXTI_ClearITPendingBit(EXTI_Line4);
//    }
//}
