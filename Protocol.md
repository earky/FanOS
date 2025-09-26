// RXNE函数第一个接收为初始化，若没有初始化则略过即可

//TXE回调函数为发送的数据，第一个与第二个一定需要发送长度数据

**1.get stack data**

recv:

uint8_t(size_h)

uint8_t(size_l)

uint8_t (stack_prio)

(data)

**2.send stack data**

send:

uint8_t (size_h)

uint8_t (size_l) 

uint8_t (stack_prio)

(data)

**3.get variable data**

recv:

uint8_t (size_h)

uint8_t (size_l)

uint32_t (address) (start with low)

(data)

**4.send variable data**

send:

uint8_t (size_h)

uint8_t (size_l)

uint32_t (address) 

(data)

**5.CPU usage report**

recv:

uint8_t(size_h)

uint8_t(size_l)

uint8_t(count_h) (value of Count is Task idle's count)

uint8_t(count_l)

**6.Task Switch Request**

recv:

uint8_t(size_h)

uint8_t(size_l)

uint8_t (target_prio)

(IS_SYS_TASK  --> NO)

**7.Is Busy**

recv:

uint8_t(size_h)

uint8_t(size_l)

uint8_t(is_busy)

**6.Task activate**

send:

uint8_t (target_prio)

**7.Task disactivate**

send:

uint8_t (target_prio)

**8.get Task prios** 

recv:

uint8_t (size_h)

uint8_t (size_l)

for(){

​	uint8_t (req_prio) -- uint8_t (prio)

}

```c

// type
#define GET_STACK_DATA     0x01
#define SEND_STACK_DATA    0x02
#define GET_VARIABLE_DATA  0x03 
#define SEND_VARIABLE_DATA 0x04
#define GET_CPU_USAGE      0x05
#define TASK_ACTIVATE      0x06
#define TASK_DISACTIVATE   0x07
#define TASK_ACTIVE_CHECK  0x08
#define TASK_ACTIVE_PRIOS  0x09

//require
#define REQ_SIZE           0x01
#define REQ_PRIO           0x02
#define REQ_ADDRESS        0x03
#define REQ_DATA           0x04
#define REQ_COUNT          0x05 // CPU usage
//respond
#define RES_OK             0x01
#define RES_BUSY           0x02
#define NOT_ALLOWED_SWITCH 0x03
#define TASK_NOT_ACTIVATE  0x04
#define TAST_IS_ACTIVATE   0x05
```







uint8_t (type) 

uint8_t (prio)

uint8_t (sum)

uint8_t (size)



0x01(type)   --> 0x13 (size)



for(int i=0;i<size;i++)

​	swap()



0x00 Task Switch

0x01 Variable Load

0x02  





 
