## 1.简介

FanOS 是一个基于 uC/OS-II 内核的轻量级多核操作系统，专为嵌入式系统设计，目前提供I2C版本通信机制实现核间任务调度和数据交换，让只支持并发处理能力的单片机可以具有多核并行处理。

## 2.FanOS特性

- **多核支持**：基于 uC/OS-II 扩展，支持多核处理器架构
- **核间通信**：通过 I2C 实现高效核间通信，包括任务切换、数据传输等功能

- **负载均衡**：支持自动负载均衡，无需手动在核间进行任务切换，主核会自动完成多核间的任务调度
- **特殊任务类型：**任务可以设置为特殊任务与非特殊任务，其中特殊任务不参加核间任务调度，非特殊任务才会参与调度
- **核ID设置任务：**可以通过核的ID快速设置不同核的任务，其中也可以直接使用宏`ALL_CORES_ID`直接设置全部核都运行对应的任务
- **中断线发送数据：**FanOS通过一根中断线发送数据，保证了数据传输的实时性
- **伪内存共享：**FanOS提供了函数可以实现同一地址数据的拷贝，同时提供了回调函数，保证在其它核接收到数据之后，可以马上进行数据处理（之所以称为伪内存共享，是因为不同核没有使用同一个RAM，只实现同一地址的数据拷贝）

## 3.使用方法

以下我们通过两核来介绍FanOS的使用（在实际使用过程中你可以根据自己的需求选择N核）

### 3.1  物理连接

![image-20251005101322048](C:\Users\ASUS\AppData\Roaming\Typora\typora-user-images\image-20251005101322048.png)

### **3.2** 软件使用

FanOS的使用流程与 uC/OS-II 基本一致，只在此基础上多加上了一些其它变量，如果你有使用uC/OS-II的经验，你可以直接跳转到3.2.2

#### 3.2.1 uC/OS-II使用

uC/OS-II的使用可以参考以下代码

```c
OS_STK Task_PWM_Led_STK[128]; // 呼吸灯任务栈

// 呼吸灯
void PWM_Led(void * p_arg){
		int i = 0;
		while(1){	
				for (i = 0; i <= 100; i++)
				{
					PWM_SetCompare1(i);	
					OSTimeDly(10u);				
				}
				for (i = 0; i <= 100; i++)
				{
					PWM_SetCompare1(100 - i);	
					OSTimeDly(10u);				
				}
			for(int i = 0; i<1000000; i++);		
		}
}

int main(void)
{
	PWM_Init();			// PWM初始化
	OSInit();			// OS初始化

	OSTaskCreate(PWM_Led, (void *)0, &Task_PWM_Led_STK[127] , 7);
	//          任务函数     任务参数       任务栈地址         任务优先级
	SysTick_Config(40000u);	// 设置SysTick
	OSStart();			   // 开启OS
	return 0;
}
```

#### 3.2.2 FanOS的使用

FanOS的基础使用仅多出了核ID，任务特殊性两部分，在3.2.3与3.2.4会介绍如何使用该参数

```c
OS_STK Task_PWM_Led_STK[128]; // 呼吸灯任务栈

// 呼吸灯
void PWM_Led(void * p_arg){
		int i = 0;
		while(1){	
				for (i = 0; i <= 100; i++)
				{
					PWM_SetCompare1(i);	
					OSTimeDly(10u);				
				}
				for (i = 0; i <= 100; i++)
				{
					PWM_SetCompare1(100 - i);	
					OSTimeDly(10u);				
				}
			for(int i = 0; i<1000000; i++);		
		}
}

int main(void)
{
	PWM_Init();			// PWM初始化
	OSInit();			// OS初始化

	OSTaskCreate(PWM_Led, (void *)0, &Task_PWM_Led_STK[127] , 7,          0 ,    SPECIFIC_FALSE);
	//          任务函数     任务参数       任务栈地址         任务优先级    核ID       任务特殊性
	SysTick_Config(40000u);	// 设置SysTick
	OSStart();			   // 开启OS
	return 0;
}
```

#### 3.2.3 核ID的使用及核I2C地址映射

在`os_multi_core.c`中有`OS_CoreIDInit`函数，用户可以自己定义，来让不同核获取自己的ID号（后续更新会提供Flash读取ID号例程），在烧录不同STM32遵循下面条件：

- 主核ID号必须为0，同时必须有一个主核
- 所有核的ID号必须不同
- 增加核需要在 `OSDevAddrs`中增加对应的I2C地址
- 外核ID号必须小于`OSDevNums`，中间ID号允许空出来，如：OSDevNums为3时，其中一个外核为3是不被允许的，而一共多核组ID为0，2，中间的ID号1空出来是允许的

```c
// os_multi_core.c

uint8_t OSDevAddrs[] = {I2C_MASTER_ADDRESS, I2C_SLAVE_ADDRESS1, I2C_SLAVE_ADDRESS2};//通过OSCoreID来访问外核I2C地址，
uint8_t OSDevNums = sizeof(OSDevAddrs) / sizeof(uint8_t);					      //多核的数量（主核＋外核）

/**
  * @brief  FanOS的初始化OSCoreID函数，可以自己定义
	*/
__weak uint8_t OS_CoreIDInit(void){
		return OSCoreID;
}
```

在`main.c`中，则可以通过倒数第二个参数指定任务所属的核

```c
// 该任务在核0中开始运行
OSTaskCreate(PWM_Led, (void *)0, &Task_PWM_Led_STK[127] , 7, 0 , SPECIFIC_FALSE);
// 该任务在所有核中同时运行
OSTaskCreate(PWM_Led, (void *)0, &Task_PWM_Led_STK[127] , 7, ALL_CORES_ID , SPECIFIC_FALSE);
```

同时你也可以在任务中进行核ID的区分，可以根据不同ID运行不同的任务

```c
void PWM_Led(void * p_arg){
    	// 主核不运行该任务，直接删除
    	if(OSCoreID == 0){
           OSTaskDel(7);	// 删除该任务
        }
    
		int i = 0;
		while(1){	
				for (i = 0; i <= 100; i++)
				{
					PWM_SetCompare1(i);			
					OSTimeDly(10u);			
				}
				for (i = 0; i <= 100; i++)
				{
					PWM_SetCompare1(100 - i);	
					OSTimeDly(10u);		
				}
			for(int i = 0; i<1000000; i++);		
		}
}
```

#### 3.2.4 任务特殊性

在下面的代码中`PWM_Led1`设置为了`SPECIFIC_TRUE`，而`PWM_Led2`设置为了`SPECIFIC_FALSE`，在调度过程中`PWM_Led1`则会一直保留在核0中，而`PWM_Led2`则可能会进行调度

``` c
OS_STK Task_PWM_Led_STK1[128];
OS_STK Task_PWM_Led_STK2[128];

void PWM_Led1(void * p_arg){
		int i = 0;
		while(1){	
				for (i = 0; i <= 100; i++)
				{
					PWM_SetCompare1(i);			
					OSTimeDly(10u);			
				}
				for (i = 0; i <= 100; i++)
				{
					PWM_SetCompare1(100 - i);	
					OSTimeDly(10u);			
				}
			for(int i = 0; i<1000000; i++);		
		}
}

void PWM_Led2(void * p_arg){
		int i = 0;
		while(1){	
				for (i = 0; i <= 100; i++)
				{
					PWM_SetCompare2(i);			
					OSTimeDly(10u);			
				}
				for (i = 0; i <= 100; i++)
				{
					PWM_SetCompare2(100 - i);	
					OSTimeDly(10u);			
				}
			for(int i = 0; i<1000000; i++);		
		}
}

int main(void)
{
	
	PWM_Init();			//PWM初始化

	OSInit();
	OSTaskCreate(PWM_Led1, (void *)0, &Task_PWM_Led_STK1[127] , 7,  0 ,SPECIFIC_TRUE);
	OSTaskCreate(PWM_Led2, (void *)0, &Task_PWM_Led_STK2[127] , 8,  0 ,SPECIFIC_FALSE);

	SysTick_Config(40000u);
	OSStart();
	return 0;
}
```

#### 3.2.5 伪共享内存使用

想要在核间进行数据传输，主要依赖于两个函数，`OS_Data_Transfer_Switch_Callback`和`OS_SendData`，这两个函数的定义如下：

```c
/**
  * @brief  FanOS在OS_Multi_Core_Data_Transfer中的回调函数
	*					当接收到数据的时候，则会触发，可以通过type来触发不同的分支
	*/
__weak void OS_Data_Transfer_Switch_Callback(uint8_t type){
		switch(type){
			case 0:
				break;
			default:
			
				break;
		}
}

/**
	* @brief  多核伪共享内存发送数据
	* @param  发送的数据地址
	* @param  发送的数据长度
	* @param  发送的数据类型
  * @retval 函数状态值
  */
uint8_t OS_SendData(uint8_t* buf, uint16_t size, uint8_t type);
```

我们在使用前需要定义`OS_Data_Transfer_Switch_Callback`函数在里面自己实现一个`switch`来处理不同类型的数据，在使用的时候只需要准备好数据，然后调用`OS_SendData`函数即可，下面是一个使用例程：

```c
// 1.烧录到主核，ID号一定为0
uint8_t OS_CoreIDInit(void){
	return 0;
}
// 2.烧录到外核，ID号为1
//uint8_t OS_CoreIDInit(void){
//	return 1;
//}

OS_STK T[128];
uint8_t Data[] = {1,2,3,4,5,6,7,8,9};
void OS_Data_Transfer_Switch_Callback(uint8_t type){
		switch(type){
			case 0:
				Serial_SendString("OS_Data_Transfer_Switch_Callback 0\n");
				break;
			case 1:
				Serial_SendString("OS_Data_Transfer_Switch_Callback 1\n");
                 // 打印Data数据
                 for(int i=0;i<9;i++){
						sprintf(str, "%u ", Data[i]);
						Serial_SendString(str);
				}
				Serial_SendString("\n");
				break;
			default:
			
				break;
		}
}

void Send_Shared_Memory_data(void* p_arg){
		while(1){
			// 模拟处理数据			
			for(int i=0;i<9;i++)
				Data[i]++;
			// 发送数据，tpye为1
			OS_SendData(Data, 9, 1);
			OSTimeDly(1000u);
		}	
}

int main(void)
{
	
	PWM_Init();			//PWM初始化
	Serial_Init();		//Serial初始化

	OSInit();
	OSTaskCreate(Send_Shared_Memory_data, (void *)0, &T[127]  , 6, 1, SPECIFIC_TRUE);
    
	SysTick_Config(40000u);
	OSStart();
	return 0;
}
```

该例程实现了从核1发送数据到核0中，同时核0在接收到数据之后会调用`OS_Data_Transfer_Switch_Callback`，最后会在串口处打印数据

```
// 串口打印
OS_Data_Transfer_Switch_Callback 1
2 3 4 5 6 7 8 9 10

OS_Data_Transfer_Switch_Callback 1
2 3 4 5 6 7 8 9 10 11
```

每一次调用`OS_Data_Transfer_Switch_Callback`后Data数据每一位都会加1，即为核1处理后的数据

#### 3.2.6 宏定义更改

在`ucos_ii.h`中有以下宏定义可以更改，下面列出的宏都可以根据注释和自己的需求更改

```c
#define USAGE_MAX_COUNT           10000u       /* CPU使用率最大计数值 */
#define DIFF_COUNT                10u          /* 采样计数差值 */
#define OS_MULTI_CORE_SCHED_DELAY 10000u       /* 主核调度间隔 */
#define SLAVE_DATA_TRANSFER_WAIT_DELAY 1000u   /* 外核等待主核数据处理的延时 */
#define OS_QUEUE_SIZE                  6u      /* 任务队列大小 */
#define OS_SEND_DATA_DELAY             10u	   /* 当前一个数据还未传输完成时需要的延时 */
#define OS_SEND_DATA_TIMEOUT           1000    /* 数据传输的超时常量 */
#define OS_BUSY_WAIT_DELAY    10u              /* 忙等待延时 */
```

