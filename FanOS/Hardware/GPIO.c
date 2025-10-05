#include "stm32f10x.h"
#include "GPIO.h"
#include "ucos_ii.h" 

void GPIO_EXTI_Init(void)
{
		return;
    // 1. ʹ��GPIOB��AFIOʱ�ӣ��ؼ�������
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    
    // 2. ����PB12Ϊ����ģʽ������
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // 3. ��GPIOB12ӳ�䵽EXTI12�ߣ��ؼ�������
		GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource12);
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource13);
    
    // 4. ����EXTI12
    EXTI_InitTypeDef EXTI_InitStructure;
    EXTI_InitStructure.EXTI_Line = EXTI_Line12;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; // �½��ش���
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
    
    
    // 6. �����ж����ȼ�
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}


// �жϷ�������PB12ʹ��EXTI15_10_IRQn��
char s[20];

void EXTI15_10_IRQHandler(void)
{
#if OS_CRITICAL_METHOD == 3u                 /* Allocate storage for CPU status register               */
    OS_CPU_SR   cpu_sr = 0u;
#endif
	
		OS_ENTER_CRITICAL();
		OSIntEnter();
		OS_EXIT_CRITICAL();
		// ����Ƿ���EXTI12���ж�
    if (EXTI_GetITStatus(EXTI_Line12) != RESET)
    {
				/* �ͷ��ź��� */
				OSQPost(DataTransferQueue, &OSDevAddrs[1]);
        EXTI_ClearITPendingBit(EXTI_Line12);
		}
		
		OSIntExit();
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
		for(volatile int i=0;i<1000;i++)	 // �ʵ���ʱ
		GPIO_SetBits(GPIOB, GPIO_Pin_12);  // ����GPIO��������������
}
