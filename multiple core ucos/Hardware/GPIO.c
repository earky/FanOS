#include "stm32f10x.h"
#include "GPIO.h"
#include "ucos_ii.h" 

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


// �жϷ�������PB12ʹ��EXTI15_10_IRQn��
void EXTI15_10_IRQHandler(void)
{
		
    // ����Ƿ���EXTI12���ж�
    if (EXTI_GetITStatus(EXTI_Line12) != RESET)
    {
				/* �ͷ��ź��� */
				OSSemPost(DataTransferSem);
				/* ����Ӧ�ĺ�I2C��ַ��� */
				OSEnterQueue(&os_queue, OSDevAddrs[1]);
				// ����жϱ�־λ
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
		GPIO_ResetBits(GPIOB, GPIO_Pin_12);// ������GPIO
		GPIO_SetBits(GPIOB, GPIO_Pin_12);  // ����GPIO��������������
}
