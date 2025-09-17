//#include "stm32f10x.h"                  // Device header
//#include "ucos_ii.h"

///**
//  * ��    ����SPIдSS���ŵ�ƽ��SS�������ģ��
//  * ��    ����BitValue Э��㴫��ĵ�ǰ��Ҫд��SS�ĵ�ƽ����Χ0~1
//  * �� �� ֵ����
//  * ע������˺�����Ҫ�û�ʵ�����ݣ���BitValueΪ0ʱ����Ҫ��SSΪ�͵�ƽ����BitValueΪ1ʱ����Ҫ��SSΪ�ߵ�ƽ
//  */
//void SPI_W_SS(uint8_t BitValue)
//{
//	GPIO_WriteBit(GPIOA, GPIO_Pin_4, (BitAction)BitValue);		//����BitValue������SS���ŵĵ�ƽ
//}

///**
//  * ��    ����SPI��ʼ��
//  * ��    ������
//  * �� �� ֵ����
//  */
//void SPI_Master_Init(void)
//{
//	/*����ʱ��*/
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//����GPIOA��ʱ��
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);	//����SPI1��ʱ��
//	
//	/*GPIO��ʼ��*/
//	GPIO_InitTypeDef GPIO_InitStructure;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_Init(GPIOA, &GPIO_InitStructure);					//��PA4���ų�ʼ��Ϊ�������
//	
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_Init(GPIOA, &GPIO_InitStructure);					//��PA5��PA7���ų�ʼ��Ϊ�����������
//	
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_Init(GPIOA, &GPIO_InitStructure);					//��PA6���ų�ʼ��Ϊ��������
//	
//	/*SPI��ʼ��*/
//	SPI_InitTypeDef SPI_InitStructure;						//����ṹ�����
//	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;			//ģʽ��ѡ��ΪSPI��ģʽ
//	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;	//����ѡ��2��ȫ˫��
//	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;		//���ݿ�ȣ�ѡ��Ϊ8λ
//	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;		//����λ��ѡ���λ����
//	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_128;	//�����ʷ�Ƶ��ѡ��128��Ƶ
//	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;				//SPI���ԣ�ѡ��ͼ���
//	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;			//SPI��λ��ѡ���һ��ʱ�ӱ��ز��������Ժ���λ����ѡ��SPIģʽ0
//	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;				//NSS��ѡ�����������
//	SPI_InitStructure.SPI_CRCPolynomial = 7;				//CRC����ʽ����ʱ�ò�������Ĭ��ֵ7
//	SPI_Init(SPI1, &SPI_InitStructure);						//���ṹ���������SPI_Init������SPI1
//	
//	/*SPIʹ��*/
//	SPI_Cmd(SPI1, ENABLE);									//ʹ��SPI1����ʼ����
//	
//	/*����Ĭ�ϵ�ƽ*/
//	SPI_W_SS(1);											//SSĬ�ϸߵ�ƽ
//}

///**
//  * ��    ����SPI��ʼ��
//  * ��    ������
//  * �� �� ֵ����
//  */
//void SPI_Slave_Init(void)
//{
//    /*����ʱ��*/
//    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);    //����GPIOA��ʱ��
//    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);    //����SPI1��ʱ��
//    
//    /*GPIO��ʼ�� - ��������*/
//    GPIO_InitTypeDef GPIO_InitStructure;
//    
//    // NSS����(PA4)Ӧ����Ϊ����ģʽ������������
//    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;  // ��Ϊ��������
//    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
//    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//    GPIO_Init(GPIOA, &GPIO_InitStructure);
//    
//		/*����PA4���ⲿ�ж�*/
//    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource4); // ��PA4���ӵ�EXTI4
//    
//    EXTI_InitTypeDef EXTI_InitStructure;
//    EXTI_InitStructure.EXTI_Line = EXTI_Line4;
//    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
//    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling; // �����ش����������ͷ�NSS��
//    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
//    EXTI_Init(&EXTI_InitStructure);
//	
//    // SCK(PA5)��MOSI(PA7)Ӧ����Ϊ����������ù���
//    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;  // �� GPIO_Mode_AF_PP
//    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
//    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//    GPIO_Init(GPIOA, &GPIO_InitStructure);
//    
//    // MISO(PA6)��Ϊ�ӻ������Ӧ����Ϊ�����������
//    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
//    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
//    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//    GPIO_Init(GPIOA, &GPIO_InitStructure);
//    
//		
//    /*SPI��ʼ�� - ��������*/
//    SPI_InitTypeDef SPI_InitStructure;
//    SPI_InitStructure.SPI_Mode = SPI_Mode_Slave;            // ��ȷ���ӻ�ģʽ
//    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex; // ��ȷ
//    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;       // ��ȷ
//    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;      // ��ȷ
//    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2; // �ӻ�ģʽ�´˲�����Ч�����豣��һ����Чֵ
//    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;              // ����������һ��
//    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;            // ����������һ��
//    SPI_InitStructure.SPI_NSS = SPI_NSS_Hard;               // ��ΪӲ������NSS
//    SPI_InitStructure.SPI_CRCPolynomial = 7;                // ��ȷ
//    SPI_Init(SPI1, &SPI_InitStructure);
//    
//		// ʹ��SPI�����ж�
//		SPI_I2S_ITConfig(SPI1, SPI_I2S_IT_RXNE, ENABLE);

//		// ����NVIC��Ƕ�������жϿ�������
//		NVIC_InitTypeDef NVIC_InitStructure;
//		NVIC_InitStructure.NVIC_IRQChannel = SPI1_IRQn;
//		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01;
//		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01;
//		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//		NVIC_Init(&NVIC_InitStructure);
//		
//		/*����EXTI4�ж����ȼ�*/
//		NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;
//    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
//    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//    NVIC_Init(&NVIC_InitStructure);
//		
//    /*SPIʹ��*/
//    SPI_Cmd(SPI1, ENABLE);
//    
//    /*ɾ��SS��ƽ���ô��룬��ΪNSS��������������*/
//    // SPI_W_SS(1); // ����Ӧ��ɾ��
//}
///**
//  * ��    ����SPI��ʼ
//  * ��    ������
//  * �� �� ֵ����
//  */
//void SPI_Start(void)
//{
//	SPI_W_SS(0);				//����SS����ʼʱ��
//}

///**
//  * ��    ����SPI��ֹ
//  * ��    ������
//  * �� �� ֵ����
//  */
//void SPI_Stop(void)
//{
//	SPI_W_SS(1);				//����SS����ֹʱ��
//}

///**
//  * ��    ����SPI��������һ���ֽڣ�ʹ��SPIģʽ0
//  * ��    ����ByteSend Ҫ���͵�һ���ֽ�
//  * �� �� ֵ�����յ�һ���ֽ�
//  */
//uint8_t SPI_SwapByte(uint8_t ByteSend)
//{
//	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) != SET);	//�ȴ��������ݼĴ�����
//	
//	SPI_I2S_SendData(SPI1, ByteSend);								//д�����ݵ��������ݼĴ�������ʼ����ʱ��
//	
//	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) != SET);	//�ȴ��������ݼĴ����ǿ�
//	
//	return SPI_I2S_ReceiveData(SPI1);								//��ȡ���յ������ݲ�����
//}

//// wait to finish
///*
//*	����0˵�������� ����1˵������
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
//			// ��ȡĿ���ջ���ȼ�
//			case REQ_PRIO:
//				prio = data;
//				size = 0;
//				// �ж������Ƿ񴴽�
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
//        // ��ȡ���յ�������
//        uint8_t data = SPI_I2S_ReceiveData(SPI1);
//				sprintf(tr, "%u\n", data);
//				Serial_SendString(tr);
////				SPI_I2S_SendData(SPI1, data + 1);
////				return;
//			
//				// ��ȡ��һ��������ΪType
//				if(isFirstData){
//						Type = data;
//						
//						// ��ʼ��Mode
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
//					// 0x01 ��ȡջ����
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
//        // ͬʱ����׼��Ҫ���͵�����
//        //SPI_I2S_SendData(SPI1, data + 1);
//    }
//}

//// EXTI4�жϷ������
//void EXTI4_IRQHandler(void)
//{
//    if(EXTI_GetITStatus(EXTI_Line4) != RESET)
//    {
//        // ���NSS�����Ƿ�Ϊ�ߵ�ƽ�������ͷ�Ƭѡ��
//        //if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_4) == SET)
//        //{
//            // ���ô�����ɱ�־
//            //spi_transfer_complete = 1;
//            
//            // ������Դ�����յ�������
//            // ����: process_received_data(spi_rx_buffer, spi_rx_index);
//            Status_Clear();
//						Serial_SendString("EXIT4_IQHandler\n");
//            // ���ý�������
//            //spi_rx_index = 0;
//        //}
//        
//        // ���EXTI4�жϱ�־
//        EXTI_ClearITPendingBit(EXTI_Line4);
//    }
//}
