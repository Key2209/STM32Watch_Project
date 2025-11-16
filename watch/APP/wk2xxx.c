
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
#include "ft6x36.h"

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

        // 假设 FT6X36 的 IRQ 引脚是 GPIO_PIN_1
    if (GPIO_Pin == GPIO_PIN_1) 
    {
        // 确保只在 FT6x36 的引脚上调用
        //ft6x36_Exti_Callback(); // 调用 FT6x36 的中断处理函数（它会触发 FreeRTOS 信号量）
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
