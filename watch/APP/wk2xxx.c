// #include "wk2xxx.h" // 包含本模块的头文件
// #include "cmsis_os2.h"

// /* 串口一扩四驱动 (HAL + FreeRTOS 版本) */

// // --- RT-Thread 线程函数替换为 FreeRTOS 任务函数 ---
// // 原：static int wk2xxx_thread_init(rt_uint32_t stack_size, rt_uint8_t priority, rt_uint32_t tick);
// // **替换：** FreeRTOS 任务函数原型已在头文件中声明。

// // -----------------------------------------------------------------------------
// // WK2114 宿主串口 (主控MCU连接WK2114的那个串口) 相关的全局变量
// // -----------------------------------------------------------------------------
// static volatile uint16_t WK2114_ExInterrupt_Count; // 外部中断计数器（保留，用于任务通知失败或中断积压计数）

// // **替换：** HAL/FreeRTOS 宿主串口数据管理
// static volatile uint8_t WK2114_RECV_STATUS = 0;   // 宿主串口数据接收标志：1表示接收到1字节
// static volatile char WK2114_RECV_BUF = 0;         // 宿主串口接收到的单个字节数据缓冲区
// // 原：#define WK2114_UART_NAME       "uart2"
// // 原：static rt_device_t WK2114_serial;

// // **替换：** 宿主串口句柄 (CubeMX 生成，假设使用 USART2)
// extern UART_HandleTypeDef huart2;                /* 宿主串口设备句柄 */

// // **替换：** FreeRTOS 任务句柄 (用于接收外部中断的通知)
// static osThreadId_t wk2xxxTaskHandle = NULL; 

// // 子串口接收回调函数数组 (用于存储4个子串口的接收处理函数)
// static void (*Wk2114_SlaveRecv[4])(char byte) = {NULL, NULL, NULL, NULL}; 


// // -----------------------------------------------------------------------------
// // 硬件中断回调函数
// // -----------------------------------------------------------------------------

// // --- IRQ 外部中断回调函数 (在 main.c/stm32f4xx_it.c 中的 HAL_GPIO_EXTI_Callback 中被调用) ---
// // 原：static void Wk2114_ExIrqCallback(void *args) { WK2114_ExInterrupt_Count++; }

// /**
//  * @brief WK2114 外部中断 (IRQ) 引脚的回调函数 (HAL 库的统一回调接口)
//  * * **替换：** 原始 RT-Thread 的回调函数被移植到此 HAL 接口中，并使用 FreeRTOS 任务通知机制。
//  * @param GPIO_Pin: 触发中断的引脚号 (在 main.c 中匹配)
//  */
// void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
// {
//     // 检查是否是 WK2114 IRQ 引脚触发的中断
//     if (GPIO_Pin == WK2114_IRQ_GPIO_PIN)
//     {
//         // **替换：** 使用 osThreadFlagsSet (FreeRTOS/CMSIS-RTOS V2) 从 ISR 中发送通知给任务
//         if (wk2xxxTaskHandle != NULL)
//         {
//             // osThreadFlagsSet 是 FreeRTOS xTaskNotifyFromISR 的 CMSIS 封装
//             osThreadFlagsSet(wk2xxxTaskHandle, WK2114_IRQ_NOTIFY_VALUE);
//         }
        
//         // 原始代码中的计数器保留，作为任务通知失败时的辅助计数/积压计数
//         WK2114_ExInterrupt_Count++;
//     }
// }

// // --- 宿主串口数据接收完成中断回调函数 (由 HAL 库调用) ---
// // 原：static rt_err_t Wk2114_UartIrqCallback(rt_device_t dev, rt_size_t size) { ... }

// /**
//  * @brief HAL 宿主串口接收一个字节完成后的回调函数
//  * * **替换：** 原始 RT-Thread 的接收指示回调函数逻辑被移植到此 HAL 接口中。
//  * @param huart: 串口句柄
//  */
// void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
// {
//     // 检查是否是宿主串口（huart2）
//     if (huart->Instance == huart2.Instance)
//     {
//         // 1. 设置接收标志 (用于 Wk2114_ReadByte 阻塞读取)
//         WK2114_RECV_STATUS = 1;     
        
//         // 2. 重新启动 HAL 库的单字节接收中断 (HAL 库中断接收需要手动重新开启)
//         // 准备接收下一个字节到 WK2114_RECV_BUF
//         HAL_UART_Receive_IT(&huart2, (uint8_t *)&WK2114_RECV_BUF, 1);
//     }
// }

// // -----------------------------------------------------------------------------
// // 宿主串口数据读写函数 (替换为 HAL/FreeRTOS)
// // -----------------------------------------------------------------------------

// /**
//  * @brief 读取宿主串口接收到的一个字节数据 (阻塞式)
//  * * **替换：** 替换了 RT-Thread 的设备读取/信号量等待机制，使用标志位和短延时实现阻塞等待。
//  * @return unsigned char: 接收到的数据字节，超时则返回 0
//  */
// unsigned char Wk2114_ReadByte(void) 
// {  
//   uint16_t t=0;
  
//   while(!WK2114_RECV_STATUS) // 等待接收标志被设置
//   {
//     osDelay(1); // 假设 delay_us 已在 apl_delay/delay.h 中实现
//     t++;
//     if(t > 899) // 简单的超时判断 (约 900us)
//       return 0; // 超时返回 0
//   }
  
//   // **替换：** 原始 rt_hw_interrupt_disable/enable 替换为 FreeRTOS 的临界区保护
//   taskENTER_CRITICAL(); // 进入临界区，防止中断修改全局变量
//   char data = WK2114_RECV_BUF;               // 读取数据
//   WK2114_RECV_STATUS = 0;                    // 清除接收标志
//   taskEXIT_CRITICAL();  // 退出临界区
  
//   return data;
// }

// /**
//  * @brief 通过宿主串口写入一个字节数据
//  * * **替换：** RT-Thread 的底层发送函数 `stm32_putc` 或 `rt_device_write` 替换为 HAL 库的同步发送函数。
//  * @param byte: 要发送的数据字节
//  */
// void Wk2114_WriteByte(unsigned char byte) 
// {
//     // 使用 HAL 库的同步发送函数 (轮询模式)，超时时间设置为 10ms
//     HAL_UART_Transmit(&huart2, &byte, 1, 10);
// } 

// // -----------------------------------------------------------------------------
// // WK2114 硬件控制函数 (保留核心逻辑，替换引脚操作)
// // -----------------------------------------------------------------------------

// /**
//  * @brief WK2114 芯片的硬复位操作 (通过REST引脚) 
//  */
// void Wk2114_Rest()
// {
//   WK2114_REST_H;    // 确保复位引脚在高电平 (使用 HAL 宏)
//   WK2114_REST_L;    // 拉低复位引脚 (使用 HAL 宏)
//   delay_ms(100);    // 延时
//   WK2114_REST_H;    // 释放复位引脚 (使用 HAL 宏)
//   delay_ms(20);     // 延时等待芯片启动
// }

// /**
//  * @brief WK2114 芯片控制引脚的初始化 (REST和IRQ)
//  * * **替换：** 原始 RT-Thread 的 `rt_pin_mode` 被移除，因为 GPIO 初始化已由 CubeMX 完成。
//  */
// void Wk2114_IOInit()
// {
//   // 初始拉高 REST 引脚 (CubeMX 中应已配置为输出)
//   WK2114_REST_H; 
  
//   // IRQ 引脚的中断配置和唤醒任务逻辑已在 CubeMX 和 HAL_GPIO_EXTI_Callback 中完成。
// }

// // -----------------------------------------------------------------------------
// // WK2114 寄存器读写函数 (通过宿主串口进行间接操作) (核心逻辑保持不变)
// // -----------------------------------------------------------------------------

// /**
//  * @brief 写入 WK2114 芯片的寄存器
//  * @param port: 子串口号 (1-4) 或全局端口 (WK2XXX_GPORT=1)
//  * @param reg: 寄存器地址
//  * @param byte: 要写入寄存器的数据
//  */
// void Wk2114WriteReg(unsigned char port,unsigned char reg,unsigned char byte)
// {    
//   Wk2114_WriteByte(((port-1)<<4)+reg);    // 发送写指令（高4位：端口号，低4位：寄存器地址）
//   Wk2114_WriteByte(byte);               // 发送数据
// }

// /**
//  * @brief 读取 WK2114 芯片的寄存器
//  * @param port: 子串口号 (1-4) 或全局端口 (WK2XXX_GPORT=1)
//  * @param reg: 寄存器地址
//  * @return unsigned char: 读取到的寄存器值
//  */
// unsigned char Wk2114ReadReg(unsigned char port,unsigned char reg)
// {    
//   unsigned char rec_bytea=0;
//   Wk2114_WriteByte(0x40+((port-1)<<4)+reg); // 发送读指令（0x40 标志位，高4位：端口号，低4位：寄存器地址）
//   rec_bytea=Wk2114_ReadByte();              // **调用 Wk2114_ReadByte 阻塞等待响应**                                
//   return rec_bytea;
// }

// // -----------------------------------------------------------------------------
// // 子串口初始化、关闭和波特率设置 (补充完整逻辑)
// // -----------------------------------------------------------------------------

// /**
//  * @brief 设置 WK2114 子串口波特率 (逻辑补充完整)
//  * @param port: 子串口号 (1-4)
//  * @param baud: 波特率值
//  */
// void Wk2114SetBaud(unsigned char port,int baud) 
// {
//     unsigned short dll_val = (unsigned short)(115200 * 16/baud); // 计算分频值
    
//     // 1. 进入波特率设置模式 (设置 LCR 的 DLAB 位)
//     Wk2114WriteReg(port, WK2XXX_SPAGE, 1);      // 切换到 PAGE1
//     Wk2114WriteReg(port, WK2XXX_LCR, WK2XXX_DLAB); // 设置 DLAB=1

//     // 2. 写入分频值
//     Wk2114WriteReg(port, WK2XXX_DLL, (unsigned char)dll_val);      // 写入 DLL (低位)
//     Wk2114WriteReg(port, WK2XXX_DLH, (unsigned char)(dll_val>>8)); // 写入 DLH (高位)

//     // 3. 退出波特率设置模式 (清除 LCR 的 DLAB 位)
//     Wk2114WriteReg(port, WK2XXX_LCR, WK2XXX_WL8); // 设置 DLAB=0, 8N1
//     Wk2114WriteReg(port, WK2XXX_SPAGE, 0);      // 切换回 PAGE0
// }


// /**
//  * @brief 初始化单个子串口 (逻辑补充完整)
//  * @param port: 子串口号 (1-4)
//  */
// void Wk2114PortInit(unsigned char port) 
// {
//     // 1. 禁用所有中断 (GIER & SIER)
//     Wk2114WriteReg(WK2XXX_GPORT, WK2XXX_GIER, 0); 
//     Wk2114WriteReg(port, WK2XXX_SIER, 0); 
    
//     // 2. 软复位 (GRST)
//     Wk2114WriteReg(WK2XXX_GPORT, WK2XXX_GRST, 0x5a); // 软复位 WK2114
    
//     // 3. 全局使能 (GENA)
//     Wk2114WriteReg(WK2XXX_GPORT, WK2XXX_GENA, WK2XXX_UARTEN); 
    
//     // 4. 设置线路控制寄存器 LCR (8N1)
//     Wk2114WriteReg(port, WK2XXX_LCR, WK2XXX_WL8); 

//     // 5. 设置 FIFO 控制寄存器 FCR (使能FIFO, 清除发送/接收FIFO)
//     Wk2114WriteReg(port, WK2XXX_FCR, WK2XXX_FIFOEN | WK2XXX_TFCLR | WK2XXX_RFCLR); 

//     // 6. 设置波特率 (默认 115200)
//     Wk2114SetBaud(port, B115200);

//     // 7. 设置 FIFO 模式寄存器 FMD (PAGE1)
//     Wk2114WriteReg(port, WK2XXX_SPAGE, 1);      // 切换到 PAGE1
//     Wk2114WriteReg(port, WK2XXX_FMD, 0x00);     // 默认模式
//     Wk2114WriteReg(port, WK2XXX_SPAGE, 0);      // 切换回 PAGE0
// }

// /**
//  * @brief 关闭单个子串口 (逻辑补充完整)
//  * @param port: 子串口号 (1-4)
//  */
// void Wk2114Close(unsigned char port)
// {
//     Wk2114WriteReg(port, WK2XXX_SIER, 0); // 禁用子串口中断
//     Wk2114WriteReg(port, WK2XXX_SCR, 0);  // 禁用子串口
// }


// /**
//  * @brief 波特率自适应 (假设不启用，逻辑补充完整)
//  */
// void Wk2114BaudAdaptive(void) 
// {
//     // WK2114 驱动默认不使用波特率自适应，此函数保留但为空实现。
// }


// // -----------------------------------------------------------------------------
// // 配置函数 (补充完整逻辑)
// // -----------------------------------------------------------------------------

// /**
//  * @brief 配置单个子串口，并启用接收中断
//  * @param port: 子串口号 (1-4)
//  * @param bps: 波特率
//  */
// void WK2114_Config_Port(int port, int bps)
// {
//     Wk2114PortInit(port);       // 1. 初始化串口寄存器 (8N1, FIFO等)
//     Wk2114SetBaud(port, bps);   // 2. 设置波特率
    
//     // 3. 使能接收中断：设置 SIER 寄存器 (仅使能接收数据中断)
//     Wk2114WriteReg(port, WK2XXX_SIER, 0x01); 
// }

// /**
//  * @brief WK2114 芯片全局配置 (补充完整逻辑)
//  */
// void Wk2114_config(void) 
// {
//     unsigned char port;
    
//     // 1. 硬件复位芯片
//     Wk2114_Rest();

//     // 2. 遍历使能的端口，进行配置
//     for(port = 1; port <= 4; port++)
//     {
//         if(WK2114_PORT_EN & (1<<(port-1))) // 检查该端口是否需要启用
//         {
//             // 配置为 115200bps，使能接收中断
//             WK2114_Config_Port(port, B115200); 
//         } else {
//             // 关闭未使能的端口
//             Wk2114Close(port); 
//         }
//     }
    
//     // 3. 全局中断使能 (GIER)：使能子串口1~4的中断
//     Wk2114WriteReg(WK2XXX_GPORT, WK2XXX_GIER, WK2114_PORT_EN);
    
//     // 4. 模式控制 GMUT (默认不使用 I2C)
//     Wk2114WriteReg(WK2XXX_GPORT, WK2XXX_GMUT, 0x00); 
// }

// /**
//  * @brief WK2114 驱动模块的整体初始化函数
//  */
// void WK2114_init()
// {
//     static uint8_t wk2114_init_ok = 0;
    
//     // 初始化状态检查
//     if(wk2114_init_ok == 0) {           
//         wk2114_init_ok = 1;     
//     } else if(wk2114_init_ok == 1) {    
//         // **替换：** rt_thread_mdelay 替换为 osDelay (FreeRTOS 延时)
//         osDelay(3000); 
//         return;
//     } else {
//         return; 
//     }

//     /* 1. 宿主串口配置：已由 CubeMX 和 MX_USART2_UART_Init() 完成 */
//     // **替换：** 启动宿主串口的单字节接收中断
//     HAL_UART_Receive_IT(&huart2, (uint8_t *)&WK2114_RECV_BUF, 1);
    
//     /* 2. 初始化 WK2114 的控制引脚 */
//     Wk2114_IOInit();
    
//     /* 3. 配置 WK2114 子串口 */
//     Wk2114_config();
    
//     /* 4. 创建并启动 WK2114 驱动工作任务 (FreeRTOS) */
//     // **替换：** rt_thread_create/startup 替换为 FreeRTOS/CMSIS-RTOS 任务创建
//     const osThreadAttr_t wk2xxxTask_attributes = {
//         .name = "wk2114_drv",         // 任务名称
//         .stack_size = 512 * 4,        // 栈大小 (假设 512 * 4 字节)
//         .priority = (osPriority_t) osPriorityNormal // 优先级
//     };
//     // 创建任务，并将任务句柄保存到 wk2xxxTaskHandle
//     wk2xxxTaskHandle = osThreadNew(wk2xxx_task_entry, NULL, &wk2xxxTask_attributes);

//     if (wk2xxxTaskHandle == NULL) {
//         // **替换：** rt_kprintf 替换为 printf 调试输出
//         printf("wk2114 task_create fail\r\n"); 
//     }
    
//     wk2114_init_ok = 2; // 初始化完成
// }

// // -----------------------------------------------------------------------------
// // 子串口发送数据函数 (补充完整逻辑)
// // -----------------------------------------------------------------------------

// /**
//  * @brief 通过子串口1发送一个字节 (逻辑补充完整)
//  */
// void Wk2114_Uart1SendByte(char dat)
// {
//     // 等待发送 FIFO 非满 (TDAT=1)
//     while(!(Wk2114ReadReg(1,WK2XXX_FSR) & WK2XXX_TDAT)); 
//     Wk2114WriteReg(1, WK2XXX_FDAT, dat); // 写入数据
// }

// /**
//  * @brief 通过子串口2发送一个字节 (逻辑补充完整)
//  */
// void Wk2114_Uart2SendByte(char dat)
// {
//     while(!(Wk2114ReadReg(2,WK2XXX_FSR) & WK2XXX_TDAT)); 
//     Wk2114WriteReg(2, WK2XXX_FDAT, dat); 
// }

// /**
//  * @brief 通过子串口3发送一个字节 (逻辑补充完整)
//  */
// void Wk2114_Uart3SendByte(char dat)
// {
//     while(!(Wk2114ReadReg(3,WK2XXX_FSR) & WK2XXX_TDAT)); 
//     Wk2114WriteReg(3, WK2XXX_FDAT, dat); 
// }

// /**
//  * @brief 通过子串口4发送一个字节 (逻辑补充完整)
//  */
// void Wk2114_Uart4SendByte(char dat)
// {
//     while(!(Wk2114ReadReg(4,WK2XXX_FSR) & WK2XXX_TDAT)); 
//     Wk2114WriteReg(4, WK2XXX_FDAT, dat); 
// }

// // -----------------------------------------------------------------------------
// // 子串口数据接收回调函数管理 (逻辑补充完整)
// // -----------------------------------------------------------------------------

// /**
//  * @brief 设置子串口的接收回调函数
//  * @param index: 子串口索引 (0-3 对应 Port1-Port4)
//  * @param func: 接收到一个字节时调用的函数指针
//  * @return uint8_t: 0-成功, 1-失败
//  */
// uint8_t Wk2114_SlaveRecv_Set(uint8_t index, void (*func)(char byte))
// {
//     if (index >= 4) return 1; // 索引越界
//     Wk2114_SlaveRecv[index] = func;
//     return 0;
// }

// // -----------------------------------------------------------------------------
// // 驱动工作任务 (Task) 函数
// // -----------------------------------------------------------------------------

// /**
//  * @brief 定期检查WK2114状态和清除错误标志 (逻辑补充完整)
//  * * **替换：** 原始 rt_kprintf 替换为 printf
//  * @param cycle: 任务循环延时时间 (ms)
//  */
// static void wk2xxx_check_status(uint16_t cycle)
// {
//   static uint16_t count;
  
//   count++;
//   if(count > 1000/cycle){ // 每秒执行一次 (1000ms / CYCLE ms)
//     count = 0;
    
//     /* 1. 清除错误状态：检查并清除各个子串口 FIFO 状态寄存器 (WK2XXX_FSR) 中的高4位错误标志 */
//     uint8_t i;
//     // 遍历 Port 1 到 Port 4
//     for (i = 1; i <= 4; i++) {
//         uint8_t fsr = Wk2114ReadReg(i, WK2XXX_FSR);
//         // FSR 的高四位 (0xF0) 是错误标志 (如 FE, PE 等)
//         if (fsr & 0xF0) { 
//             // 重新写入 FSR 清除错误，只保留低四位 (FIFO 计数等)
//             Wk2114WriteReg(i, WK2XXX_FSR, fsr & 0x0F);
//             // **替换：** rt_kprintf 替换为 printf
//             printf("WK2114: Port %d FSR Error (0x%02X) cleared.\r\n", i, fsr);
//         }
//     }

//     /* 2. 检查有无未处理的全局中断标志位 */
//     if((0x0f & Wk2114ReadReg(WK2XXX_GPORT,WK2XXX_GIFR))) 
//       WK2114_ExInterrupt_Count++; 
//   }
// }

// /**
//  * @brief WK2114 驱动主工作任务的入口函数 (FreeRTOS Task)
//  * * **替换：** 原始 RT-Thread 线程函数体被替换为 FreeRTOS 任务主循环，核心逻辑保持不变。
//  * @param argument: 任务参数，此处未使用
//  */
// void wk2xxx_task_entry(void *argument)
// {
// #define CYCLE          10      // 任务循环延时/等待超时 (ms)
// #define ERR_TIMES       10      // 单次中断处理的最大循环次数 (防止死循环)

//   uint16_t times;
  
//   // FreeRTOS 任务主循环
//   for(;;)
//   {
//     // **替换：** 使用 osThreadFlagsWait 阻塞等待来自 IRQ 的通知，超时时间为 CYCLE ms
//     uint32_t flags = osThreadFlagsWait(WK2114_IRQ_NOTIFY_VALUE, osFlagsWaitAny, CYCLE);

//     // 检查是否收到 IRQ 通知 (flags & WK2114_IRQ_NOTIFY_VALUE)，或是否有积压的中断 (WK2114_ExInterrupt_Count > 0)
//     if ((flags & WK2114_IRQ_NOTIFY_VALUE) || (WK2114_ExInterrupt_Count > 0))
//     {
//       uint8_t status=0;
//       char byte1,byte2,byte3,byte4;
      
//       // 循环读取 GIFR，处理 WK2114 上的所有子串口中断，直到 GIFR 清除
//       do
//       {
//         /* 错误检查 */
//         times = 0;
//         times ++;
//         if(times>ERR_TIMES){
//           // **替换：** rt_kprintf 替换为 printf
//           printf("WK2114: Number of cycles exceeded\r\n"); 
//           break; // 跳出循环，防止中断处理死循环
//         }
        
//         status=Wk2114ReadReg(1,WK2XXX_GIFR); // 读取全局中断标志寄存器
        
//         // 1. 处理子串口1中断 (UT1INT)
//         if(status&WK2XXX_UT1INT) 
//         {
//             // 循环读取 FIFO，直到 FSR 的 RDAT 位为 0 (FIFO 为空)
//             while((Wk2114ReadReg(1,WK2XXX_FSR)&WK2XXX_RDAT))
//             {
//                 byte1 = Wk2114ReadReg(1,WK2XXX_FDAT); // 读取数据
//                 // 调用子串口1的接收回调函数
//                 if(Wk2114_SlaveRecv[0] != NULL)
//                     Wk2114_SlaveRecv[0](byte1); 
//             }
//         }
        
//         // 2. 处理子串口2中断 (UT2INT)
//         if(status&WK2XXX_UT2INT) 
//         {
//             while((Wk2114ReadReg(2,WK2XXX_FSR)&WK2XXX_RDAT))
//             {
//                 byte2=Wk2114ReadReg(2,WK2XXX_FDAT);
//                 if(Wk2114_SlaveRecv[1] != NULL)
//                     Wk2114_SlaveRecv[1](byte2);
//             }
//         }
        
//         // 3. 处理子串口3中断 (UT3INT)
//         if(status&WK2XXX_UT3INT) 
//         {
//             while((Wk2114ReadReg(3,WK2XXX_FSR)&WK2XXX_RDAT))
//             {
//                 byte3 = Wk2114ReadReg(3,WK2XXX_FDAT);
//                 if(Wk2114_SlaveRecv[2] != NULL)
//                     Wk2114_SlaveRecv[2](byte3);
//             }
//         }
        
//         // 4. 处理子串口4中断 (UT4INT)
//         if(status&WK2XXX_UT4INT) 
//         {
//             while((Wk2114ReadReg(4,WK2XXX_FSR)&WK2XXX_RDAT))
//             {
//                 byte4 = Wk2114ReadReg(4,WK2XXX_FDAT);
//                 if(Wk2114_SlaveRecv[3] != NULL)
//                     Wk2114_SlaveRecv[3](byte4);
//             }
//         }

//       }
//       while((0x0f&Wk2114ReadReg(1,WK2XXX_GIFR))!=0); // 检查是否还有全局中断标志未清除

//       /* 减少中断计数器 */
//       // **替换：** rt_hw_interrupt_disable/enable 替换为 FreeRTOS 临界区
//       taskENTER_CRITICAL(); // 进入临界区
//       if (WK2114_ExInterrupt_Count > 0) {
//         WK2114_ExInterrupt_Count --;     // 外部中断计数减 1
//       }
//       taskEXIT_CRITICAL();  // 退出临界区
//     }
    
//     // 如果是超时唤醒 (flags & osFlagsErrorTimeout)，执行周期性检查
//     if (flags & osFlagsErrorTimeout) {
//       wk2xxx_check_status(CYCLE); // 周期性进行错误状态检查
//     }
//   }
// }




/*
  wk2xxx_hal.c
  HAL + FreeRTOS 版的 WK2xxx（WK2114）串口扩展驱动实现

  主要改动与实现说明：
  - 用 HAL_UART 发送/接收（中断接收单字节）替代 rt-device 串口设备。
  - 使用 FreeRTOS 任务替代 rt-thread 线程（使用 xTaskCreate）。
  - 使用 taskENTER_CRITICAL/taskEXIT_CRITICAL 来临界保护（替代 rt_hw_interrupt_disable/enable）。
  - EXTI（IRQ）中断回调需在 HAL_GPIO_EXTI_Callback 中转发到 Wk2114_ExIrqCallback()（见示例）。
  - HAL 的接收回调需在 HAL_UART_RxCpltCallback 中转发到 Wk2114_UART_RxCpltCallback()（见示例）。
  - 保持原函数名与行为（阻塞读：Wk2114_ReadByte，会等待 RX 标志被置位，带超时）。

  必要的 CubeMX 设置（简述，详见下方说明）：
  - 配置一个 UART（例：USART2）并使能中断（USART2 global interrupt）
  - 配置 IRQ 引脚为外部中断（例如 PD2 -> EXTI2），使能中断并设置优先级
  - 配置 RESET 引脚为普通 GPIO 输出（PA8）
  - 在 NVIC 中确保 UART 与 EXTI 的优先级不低于 FreeRTOS 的 configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 限制
*/

#include "wk2xxx.h"
#include "cmsis_os2.h"
#include <string.h>
#include <stdio.h>


extern UART_HandleTypeDef huart2; /* 工程里由 CubeMX 生成 */

/* recv 单字节中断缓冲与标志 */
static volatile uint8_t WK2114_RECV_STATUS = 0;
static volatile uint8_t WK2114_RECV_BUF = 0;

/* 中断计数（外部 IRQ 计数） */
static volatile uint16_t WK2114_ExInterrupt_Count = 0;

/* FreeRTOS 任务句柄 */
static TaskHandle_t wk2xxx_task_handle = NULL;

/* 回调数组：当从子串口读到一个字节后，任务会回调这些函数（index: 0..3 对应子串口1..4）*/
void (*Wk2114_SlaveRecv[4])(char byte) = {NULL};

/* 内部用：向芯片写 1 字节（通过 MCU -> WK2xxx 串口） */
void Wk2114_WriteByte(unsigned char byte)
{
    /* 采用阻塞式发送（HAL_UART_Transmit），发送超时设小一点 */
    HAL_UART_Transmit(&huart2, (uint8_t*)&byte, 1, 50);
}

/* 内部用：从芯片读 1 字节（阻塞等待由中断收到） */
unsigned char Wk2114_ReadByte(void)
{
    uint32_t t = 0;
    const uint32_t timeout = 100; /* ms 超时：100ms（可修改） */
    /* 启动 UART 单字节中断接收（确保持续开启） */
    /* 我们在 init 中已经通过 HAL_UART_Receive_IT 启动一次；在回调中会再次启动接收 */
    while(!WK2114_RECV_STATUS)
    {
        /* 用 FreeRTOS 延时来避免空转（如果在中断上下文调用此函数，会阻塞异常——原设计在任务上下文） */
        vTaskDelay(pdMS_TO_TICKS(1));
        t++;
        if(t > timeout)
            return 0; /* 超时 */
    }
    taskENTER_CRITICAL();
    WK2114_RECV_STATUS = 0;
    uint8_t b = WK2114_RECV_BUF;
    taskEXIT_CRITICAL();
    return b;
}

/* 外部中断（IRQ）回调：CubeMX 生成的 HAL_GPIO_EXTI_Callback 应转发到这里
   在 HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) 中：
     if(GPIO_Pin == WK2114_IRQ_Pin) Wk2114_ExIrqCallback();
*/
void Wk2114_ExIrqCallback(void)
{
    /* 仅增加计数，实际处理由任务完成（与原 RT-Thread 行为一致）*/
    WK2114_ExInterrupt_Count++;
}

/* UART 接收完成回调（HAL 中）：
   在 HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) 中：
     if(huart == &huart2) Wk2114_UART_RxCpltCallback(huart);
*/
void Wk2114_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    uint8_t rx;
    if(huart != &huart2) return;
    /* HAL 会把数据写入 huart->pRxBuffPtr 等，这里我们直接读取最近接收到的字节 */
    /* 为简洁与兼容，我们再次使用 HAL 接口从内部缓冲拷贝 1 字节（非阻塞，因为中断刚发生） */
    /* 注意：部分 HAL 实现中不建议在回调中再次调用 HAL_UART_Receive_IT， 但这是常见做法 */
    /* 我们假设 CubeMX 配置为接收中断（单字节）并且 HAL 已经把数据放入 huart->pRxBuffPtr */
    /* 更稳健的做法是使用单独的 rx_buf 并让 HAL 在启动接收时把地址指向该缓冲区 */
    // rx = huart->Instance->DR & 0xFF; /* 直接从寄存器读（STM32F4）——在多数 HAL 平台可行 */
    // WK2114_RECV_BUF = rx;
    WK2114_RECV_STATUS = 1;
    /* 重新开启下一个字节的接收（确保连续接收） */
    HAL_UART_Receive_IT(&huart2, (uint8_t *)&WK2114_RECV_BUF, 1);
}

/* ------------------ WK2114 寄存器读写（借用原逻辑） ------------------ */
static void Wk2114WriteReg(unsigned char port,unsigned char reg,unsigned char byte)
{
    /* 指令格式： ((port-1)<<4) + reg （与原实现一致） */
    Wk2114_WriteByte(((port-1)<<4)+reg);
    Wk2114_WriteByte(byte);
}

static unsigned char Wk2114ReadReg(unsigned char port,unsigned char reg)
{
    unsigned char rec_bytea = 0;
    Wk2114_WriteByte(0x40 + ((port-1)<<4) + reg);
    rec_bytea = Wk2114_ReadByte();
    return rec_bytea;
}

/* ---------- Wk2114PortInit / Wk2114SetBaud / Close / Config 等（移植自原代码） ---------- */
void Wk2114PortInit(unsigned char port)
{
    unsigned char gena,grst,gier,sier,scr;
    /* 读写寄存器基于串口命令接口 */
    gena = Wk2114ReadReg(WK2XXX_GPORT, WK2XXX_GENA);
    switch (port)
    {
        case 1:
            gena |= WK2XXX_UT1EN;
            Wk2114WriteReg(WK2XXX_GPORT, WK2XXX_GENA, gena);
            break;
        case 2:
            gena |= WK2XXX_UT2EN;
            Wk2114WriteReg(WK2XXX_GPORT, WK2XXX_GENA, gena);
            break;
        case 3:
            gena |= WK2XXX_UT3EN;
            Wk2114WriteReg(WK2XXX_GPORT, WK2XXX_GENA, gena);
            break;
        case 4:
            gena |= WK2XXX_UT4EN;
            Wk2114WriteReg(WK2XXX_GPORT, WK2XXX_GENA, gena);
            break;
    }
    /* 软件复位子串口 */
    grst = Wk2114ReadReg(WK2XXX_GPORT, WK2XXX_GRST);
    switch (port)
    {
        case 1:
            grst |= WK2XXX_UT1RST;
            Wk2114WriteReg(WK2XXX_GPORT, WK2XXX_GRST, grst);
            break;
        case 2:
            grst |= WK2XXX_UT2RST;
            Wk2114WriteReg(WK2XXX_GPORT, WK2XXX_GRST, grst);
            break;
        case 3:
            grst |= WK2XXX_UT3RST;
            Wk2114WriteReg(WK2XXX_GPORT, WK2XXX_GRST, grst);
            break;
        case 4:
            grst |= WK2XXX_UT4RST;
            Wk2114WriteReg(WK2XXX_GPORT, WK2XXX_GRST, grst);
            break;
    }
    /* 使能中断（子串口中断和接收触点） */
    gier = Wk2114ReadReg(WK2XXX_GPORT, WK2XXX_GIER);
    switch (port)
    {
        case 1:
            gier |= WK2XXX_UT1IE;
            Wk2114WriteReg(WK2XXX_GPORT, WK2XXX_GIER, gier);
            break;
        case 2:
            gier |= WK2XXX_UT2IE;
            Wk2114WriteReg(WK2XXX_GPORT, WK2XXX_GIER, gier);
            break;
        case 3:
            gier |= WK2XXX_UT3IE;
            Wk2114WriteReg(WK2XXX_GPORT, WK2XXX_GIER, gier);
            break;
        case 4:
            gier |= WK2XXX_UT4IE;
            Wk2114WriteReg(WK2XXX_GPORT, WK2XXX_GIER, gier);
            break;
    }
    /* SIER: 使能接收触点中断和接收超时中断 */
    sier = Wk2114ReadReg(port, WK2XXX_SIER);
    sier |= WK2XXX_RFTRIG_IEN | WK2XXX_RXOUT_IEN;
    sier |= WK2XXX_RFTRIG_IEN;
    Wk2114WriteReg(port, WK2XXX_SIER, sier);
    /* FIFO 初始化和触点 */
    Wk2114WriteReg(port, WK2XXX_FCR, 0xFF);
    Wk2114WriteReg(port, WK2XXX_SPAGE, 1);
    Wk2114WriteReg(port, WK2XXX_RFTL, 0x40);
    Wk2114WriteReg(port, WK2XXX_TFTL, 0x10);
    Wk2114WriteReg(port, WK2XXX_SPAGE, 0);
    /* 使能发送接收 */
    scr = Wk2114ReadReg(port, WK2XXX_SCR);
    scr |= WK2XXX_TXEN | WK2XXX_RXEN;
    Wk2114WriteReg(port, WK2XXX_SCR, scr);
}

void Wk2114Close(unsigned char port)
{
    unsigned char gena,grst;
    grst = Wk2114ReadReg(WK2XXX_GPORT, WK2XXX_GRST);
    switch (port)
    {
        case 1:
            grst &= ~WK2XXX_UT1RST;
            Wk2114WriteReg(WK2XXX_GPORT, WK2XXX_GRST, grst);
            break;
        case 2:
            grst &= ~WK2XXX_UT2RST;
            Wk2114WriteReg(WK2XXX_GPORT, WK2XXX_GRST, grst);
            break;
        case 3:
            grst &= ~WK2XXX_UT3RST;
            Wk2114WriteReg(WK2XXX_GPORT, WK2XXX_GRST, grst);
            break;
        case 4:
            grst &= ~WK2XXX_UT4RST;
            Wk2114WriteReg(WK2XXX_GPORT, WK2XXX_GRST, grst);
            break;
    }
    gena = Wk2114ReadReg(WK2XXX_GPORT, WK2XXX_GENA);
    switch (port)
    {
        case 1:
            gena &= ~WK2XXX_UT1EN;
            Wk2114WriteReg(WK2XXX_GPORT, WK2XXX_GENA, gena);
            break;
        case 2:
            gena &= ~WK2XXX_UT2EN;
            Wk2114WriteReg(WK2XXX_GPORT, WK2XXX_GENA, gena);
            break;
        case 3:
            gena &= ~WK2XXX_UT3EN;
            Wk2114WriteReg(WK2XXX_GPORT, WK2XXX_GENA, gena);
            break;
        case 4:
            gena &= ~WK2XXX_UT4EN;
            Wk2114WriteReg(WK2XXX_GPORT, WK2XXX_GENA, gena);
            break;
    }
}

void Wk2114SetBaud(unsigned char port,int baud)
{
    unsigned char baud1,baud0,pres,scr;
    /* 与原来保持一致：根据晶振选择波特率参数 */
    switch (baud)
    {
        case 600:   baud1=0x4; baud0=0x7f; pres=0; break;
        case 1200:  baud1=0x2; baud0=0x3F; pres=0; break;
        case 2400:  baud1=0x1; baud0=0x1f; pres=0; break;
        case 4800:  baud1=0x00; baud0=0x8f; pres=0; break;
        case 9600:  baud1=0x00; baud0=0x47; pres=0; break;
        case 19200: baud1=0x00; baud0=0x23; pres=0; break;
        case 38400: baud1=0x00; baud0=0x11; pres=0; break;
        case 76800: baud1=0x00; baud0=0x08; pres=0; break;
        case 115200:baud1=0x00; baud0=0x05; pres=0; break;
        case 230400:baud1=0x00; baud0=0x02; pres=0; break;
        default: baud1=0x00; baud0=0x00; pres=0; break;
    }
    /* 关收发 */
    scr = Wk2114ReadReg(port, WK2XXX_SCR);
    Wk2114WriteReg(port, WK2XXX_SCR, 0);
    /* 设置波特率 */
    Wk2114WriteReg(port, WK2XXX_SPAGE, 1);
    Wk2114WriteReg(port, WK2XXX_BAUD1, baud1);
    Wk2114WriteReg(port, WK2XXX_BAUD0, baud0);
    Wk2114WriteReg(port, WK2XXX_PRES, pres);
    Wk2114WriteReg(port, WK2XXX_SPAGE, 0);
    /* 恢复 SCR */
    Wk2114WriteReg(port, WK2XXX_SCR, scr);
}

void Wk2114BaudAdaptive(void)
{
    /* 复位并发送 0x55 测试波特自适应（沿用原逻辑） */
    WK2114_REST_H;
    WK2114_REST_L;
    //HAL_Delay(100);
    osDelay(pdMS_TO_TICKS(100));
    WK2114_REST_H;
    //HAL_Delay(20);
    osDelay(pdMS_TO_TICKS(20));
    Wk2114_WriteByte(0x55);
    vTaskDelay(pdMS_TO_TICKS(300));
}

/* 根据原实现的 Wk2114_config */
void Wk2114_config(void)
{
    Wk2114BaudAdaptive();
#if WK2114_PORT_EN & WK2114_PORT1
    Wk2114PortInit(1);
    Wk2114SetBaud(1, B9600);
#endif
#if WK2114_PORT_EN & WK2114_PORT2
    Wk2114PortInit(2);
    Wk2114SetBaud(2, B38400);
#endif
#if WK2114_PORT_EN & WK2114_PORT3
    Wk2114PortInit(3);
    Wk2114SetBaud(3, B115200);
#endif
#if WK2114_PORT_EN & WK2114_PORT4
    Wk2114PortInit(4);
    Wk2114SetBaud(4, B38400);
#endif
    vTaskDelay(pdMS_TO_TICKS(10));
}

void WK2114_Config_Port(int port, int bps)
{
    Wk2114PortInit(port);
    Wk2114SetBaud(port, bps);
    
}

/* 设置回调 */
uint8_t Wk2114_SlaveRecv_Set(uint8_t index, void (*func)(char byte))
{
    if((index==0) || (index>4)) return 1;
    Wk2114_SlaveRecv[index-1] = func;
    return 0;
}

/* 子串口发送函数（写 FDAT 寄存器） */
void Wk2114_Uart1SendByte(char byte) { Wk2114WriteReg(1, WK2XXX_FDAT, byte); }
void Wk2114_Uart2SendByte(char byte) { Wk2114WriteReg(2, WK2XXX_FDAT, byte); }
void Wk2114_Uart3SendByte(char byte) { Wk2114WriteReg(3, WK2XXX_FDAT, byte); }
void Wk2114_Uart4SendByte(char byte) { Wk2114WriteReg(4, WK2XXX_FDAT, byte); }

/* 错误检查与清理（周期性） */
static void wk2xxx_check_status(uint16_t cycle)
{
    static uint16_t count = 0;
    uint8_t status;
    count++;
    if(count > (1000 / cycle))
    {
        count = 0;
#if WK2114_PORT_EN & WK2114_PORT1
        status = Wk2114ReadReg(1, WK2XXX_FSR);
        if(status & 0xf0) Wk2114WriteReg(1, WK2XXX_FSR, status & 0x0f);
#endif
#if WK2114_PORT_EN & WK2114_PORT2
        status = Wk2114ReadReg(2, WK2XXX_FSR);
        if(status & 0xf0) Wk2114WriteReg(2, WK2XXX_FSR, status & 0x0f);
#endif
#if WK2114_PORT_EN & WK2114_PORT3
        status = Wk2114ReadReg(3, WK2XXX_FSR);
        if(status & 0xf0) Wk2114WriteReg(3, WK2XXX_FSR, status & 0x0f);
#endif
#if WK2114_PORT_EN & WK2114_PORT4
        status = Wk2114ReadReg(4, WK2XXX_FSR);
        if(status & 0xf0) Wk2114WriteReg(4, WK2XXX_FSR, status & 0x0f);
#endif
        /* 检查有无中断标志位 */
        if((0x0f & Wk2114ReadReg(WK2XXX_GPORT, WK2XXX_GIFR)))
            WK2114_ExInterrupt_Count++;
    }
}

/* FreeRTOS 任务：从外部 IRQ 事件中处理子串口数据（原 rt-thread 线程移植） */
static void wk2xxx_task_entry(void *param)
{
    const uint16_t CYCLE = 10;
    const uint16_t ERR_TIMES = 10;
    uint16_t times;
    for(;;)
    {
        times = 0;
        if(WK2114_ExInterrupt_Count)
        {
            uint8_t status = 0;
            char b1,b2,b3,b4;
            do
            {
                times++;
                if(times > ERR_TIMES)
                {
                    /* debug 输出 */
                    // printf("Number of cycles exceeded\r\n");
                    break;
                }
                status = Wk2114ReadReg(1, WK2XXX_GIFR); /* 判断中断源 */
                if(status & WK2XXX_UT1INT)
                {
                    while((Wk2114ReadReg(1, WK2XXX_FSR) & WK2XXX_RDAT))
                    {
                        b1 = Wk2114ReadReg(1, WK2XXX_FDAT);
                        if(Wk2114_SlaveRecv[0] != NULL) Wk2114_SlaveRecv[0](b1);
                    }
                }
                if(status & WK2XXX_UT2INT)
                {
                    while((Wk2114ReadReg(2, WK2XXX_FSR) & WK2XXX_RDAT))
                    {
                        b2 = Wk2114ReadReg(2, WK2XXX_FDAT);
                        if(Wk2114_SlaveRecv[1] != NULL) Wk2114_SlaveRecv[1](b2);
                    }
                }
                if(status & WK2XXX_UT3INT)
                {
                    while((Wk2114ReadReg(3, WK2XXX_FSR) & WK2XXX_RDAT))
                    {
                        b3 = Wk2114ReadReg(3, WK2XXX_FDAT);
                        if(Wk2114_SlaveRecv[2] != NULL) Wk2114_SlaveRecv[2](b3);
                    }
                }
                if(status & WK2XXX_UT4INT)
                {
                    while((Wk2114ReadReg(4, WK2XXX_FSR) & WK2XXX_RDAT))
                    {
                        b4 = Wk2114ReadReg(4, WK2XXX_FDAT);
                        if(Wk2114_SlaveRecv[3] != NULL) Wk2114_SlaveRecv[3](b4);
                    }
                }
            } while((0x0f & Wk2114ReadReg(1, WK2XXX_GIFR)) != 0);

            /* 减少外部中断计数（临界保护） */
            taskENTER_CRITICAL();
            if(WK2114_ExInterrupt_Count) WK2114_ExInterrupt_Count--;
            taskEXIT_CRITICAL();
        }
        wk2xxx_check_status(CYCLE);
        vTaskDelay(pdMS_TO_TICKS(CYCLE));
    }
}

/* 创建 FreeRTOS 任务 */
static int wk2xxx_task_init(uint16_t stack_size, UBaseType_t priority)
{
    BaseType_t res;
    if(wk2xxx_task_handle != NULL) return 0;
    res = xTaskCreate(wk2xxx_task_entry, "wk2114", stack_size, NULL, priority, &wk2xxx_task_handle);
    if(res != pdPASS)
    {
        printf("wk2114 task create fail\r\n");
        return -1;
    }
    return 0;
}

/* 初始化函数（替代原 WK2114_init），保持函数名与行为：
   - 启动 UART 单字节中断接收
   - 配置 IRQ 引脚外部中断由 CubeMX 完成，HAL will call HAL_GPIO_EXTI_Callback
   - 启动 FreeRTOS 任务
*/
void WK2114_init(void)
{
    static uint8_t wk2114_init_ok = 0;
    if(wk2114_init_ok == 0) {
        wk2114_init_ok = 1;
    } else if(wk2114_init_ok == 1) {
        vTaskDelay(pdMS_TO_TICKS(3000));
        return;
    } else {
        return;
    }

    /* 启动 UART 单字节接收（中断）*/
    /* 注意：将接收缓冲区指向 WK2114_RECV_BUF，使 HAL 在接收完成时把字节写到该地址 */
    HAL_UART_Receive_IT(&huart2, (uint8_t *)&WK2114_RECV_BUF, 1);

    /* 复位与 IO 初始化（RESET 引脚已在 CubeMX 配置为 GPIO 输出） */
    //HAL_GPIO_WritePin(WK2114_RESET_GPIO_Port, WK2114_RESET_Pin, GPIO_PIN_SET);
    WK2114_REST_H;
    osDelay(10); /* 10ms */

    /* 如果需要做更多 IO 配置（例如上拉/下拉），请在 CubeMX 中设置 */

    /* 运行芯片配置 */
    Wk2114_config();

    /* 启动 FreeRTOS 任务（堆栈 1024、优先级 5） - 你可根据工程调整 */
    wk2xxx_task_init(256, tskIDLE_PRIORITY + 3);

    //wk2xxx_init_ok_label:
    wk2114_init_ok = 2;
}

/* 提供中断 / 回调接入示例（在工程的 HAL 回调中调用）：
   - 在 stm32xx_it.c 或 main.c 中的 HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)：
       if (GPIO_Pin == WK2114_IRQ_Pin) Wk2114_ExIrqCallback();
   - 在 HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)：
       if (huart == &huart2) Wk2114_UART_RxCpltCallback(huart);
*/

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == WK2114_IRQ_Pin) {
        Wk2114_ExIrqCallback();
    }
}

/* WK2114 串口模块的接收回调 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart2) {
        Wk2114_UART_RxCpltCallback(huart);
    }
}

/* end of file */
