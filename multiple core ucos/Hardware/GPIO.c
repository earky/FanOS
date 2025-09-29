#include "stm32f10x.h"
#include "GPIO.h"
#include "ucos_ii.h" 

void GPIO_EXTI_Init(void)
{
    // 1. 使能GPIOB和AFIO时钟（关键修正）
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    
    // 2. 配置PB12为输入模式，上拉
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD; // 上拉输入
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // 3. 将GPIOB12映射到EXTI12线（关键修正）
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource12);
    
    // 4. 配置EXTI12
    EXTI_InitTypeDef EXTI_InitStructure;
    EXTI_InitStructure.EXTI_Line = EXTI_Line12;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising; // 上升沿触发
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
    
    // 5. 配置中断优先级
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}


// 中断服务函数（PB12使用EXTI15_10_IRQn）
void EXTI15_10_IRQHandler(void)
{
		
    // 检查是否是EXTI12的中断
    if (EXTI_GetITStatus(EXTI_Line12) != RESET)
    {
				/* 释放信号量 */
				OSSemPost(DataTransferSem);
				/* 将对应的核I2C地址入队 */
				OSEnterQueue(&os_queue, OSDevAddrs[1]);
				// 清除中断标志位
        EXTI_ClearITPendingBit(EXTI_Line12);
    }
}


// 从机端：GPIO初始化（PB12）
void Slave_GPIO_Init(void)
{
    // 1. 使能GPIOB时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    // 2. 配置PB12为推挽输出模式（从机需要输出信号）
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;          // 选择PB12引脚
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;    // 推挽输出模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;   // 50MHz速度
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // 3. 初始化GPIO状态为低电平（避免上电时误触发）
    GPIO_ResetBits(GPIOB, GPIO_Pin_12);
}

// 从机端：发送数据函数（PB12）
void Slave_SendData(void)
{		
		GPIO_ResetBits(GPIOB, GPIO_Pin_12);// 先拉低GPIO
		GPIO_SetBits(GPIOB, GPIO_Pin_12);  // 拉高GPIO，请求主机处理
}
