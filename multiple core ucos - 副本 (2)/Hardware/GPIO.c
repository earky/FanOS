#include "stm32f10x.h"
#include "GPIO.h"
#include "ucos_ii.h" 
#include "I2C.h"

void GPIO_EXTI_Init(void)
{
    // 1. ʹ��GPIOB��AFIOʱ�ӣ��ؼ�������
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    
    // 2. ����PB12Ϊ����ģʽ������
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD; // ��������
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // 3. ��GPIOB12ӳ�䵽EXTI12�ߣ��ؼ�������
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource12);
    
    // 4. ����EXTI12
    EXTI_InitTypeDef EXTI_InitStructure;
    EXTI_InitStructure.EXTI_Line = EXTI_Line12;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising; // �����ش���
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
    
    // 5. �����ж����ȼ�
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}


char tr[30];
static volatile uint32_t post_count = 0;
// �жϷ�������PB12ʹ��EXTI15_10_IRQn��
void EXTI15_10_IRQHandler(void)
{
		
    // ����Ƿ���EXTI12���ж�
    if (EXTI_GetITStatus(EXTI_Line12) != RESET)
    {
				Serial_SendString("IQ\n");
//				OS_SEM_DATA sem_data_before, sem_data_after;
//        INT8U err;
//        
//        // ��ѯPOSTǰ��״̬
//        OSSemQuery(DataTransferSem, &sem_data_before);
        
        /* �ͷ��ź��� */
        OSSemPost(DataTransferSem);
        
//        // ��ѯPOST���״̬  
//        OSSemQuery(DataTransferSem, &sem_data_after);
//        
//        sprintf(tr, "sem: before=%d, after=%d\n", 
//                sem_data_before.OSCnt, sem_data_after.OSCnt);
//        Serial_SendString(tr);
        
        EXTI_ClearITPendingBit(EXTI_Line12);
    }
}


// �ӻ��ˣ�GPIO��ʼ����PB12��
void Slave_GPIO_Init(void)
{
    // 1. ʹ��GPIOBʱ��
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    // 2. ����PB12Ϊ�������ģʽ���ӻ���Ҫ����źţ�
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;          // ѡ��PB12����
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;    // �������ģʽ
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;   // 50MHz�ٶ�
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // 3. ��ʼ��GPIO״̬Ϊ�͵�ƽ�������ϵ�ʱ�󴥷���
    GPIO_ResetBits(GPIOB, GPIO_Pin_12);
}

// �ӻ��ˣ��������ݺ�����PB12��
void Slave_SendData(void)
{		
		/* ������λ��Ϊ1 */
		iflag.is_send = 1;
		Serial_SendString("Send_Data\n");
		GPIO_SetBits(GPIOB, GPIO_Pin_12);  // ����GPIO��������������
		OSTimeDly(1000u);
		GPIO_ResetBits(GPIOB, GPIO_Pin_12);// ����GPIO
}
