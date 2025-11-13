// #ifndef _WK2XXX_H_ // 头文件保护宏，防止重复包含
// #define _WK2XXX_H_

// // --- RT-Thread 头文件替换为 HAL/FreeRTOS 头文件 ---

// // 原：#include <rtthread.h>   // RT-Thread 内核头文件
// // 原：#include <rtdevice.h>   // RT-Thread 设备驱动框架头文件

// #include "main.h"               // **替换：** 包含 CubeMX 生成的主头文件，包含了 HAL 库和外设句柄。
// #include "cmsis_os.h"           // **替换：** 包含 FreeRTOS/CMSIS-RTOS V2 (CubeMX默认) 内核头文件。
// //#include "usart.h"              // **替换：** 包含 HAL 串口驱动头文件，用于宿主串口 huart2 句柄声明。
// //#include "apl_delay/delay.h"    // 延时函数头文件 (假设已封装 HAL 库延时函数)。
// #include <stdio.h>              // 包含标准输入输出头文件，用于调试打印（替换 rt_kprintf）。
// #include <string.h>             // 包含字符串处理头文件，确保完整性。
// //#include "gpio.h"               // 包含 HAL GPIO 头文件，确保引脚定义可用。

// // -----------------------------------------------------------------------------
// // WK2XXX 系列串口扩展芯片的寄存器地址定义 (硬件定义，保持不变)
// // -----------------------------------------------------------------------------

// // Global Rigister Address Defines (全局寄存器地址定义)
// #define 	WK2XXX_GENA     0X00 // Global Enable Register (全局使能寄存器)
// #define 	WK2XXX_GRST     0X01 // Global Reset Register (全局复位寄存器)
// #define		WK2XXX_GMUT     0X02 // Global Mode Control Register (全局模式控制寄存器)
// #define 	WK2XXX_GIER     0X10 // Global Interrupt Enable Register (全局中断使能寄存器)
// #define 	WK2XXX_GIFR     0X11 // Global Interrupt Flag Register (全局中断标志寄存器)
// #define 	WK2XXX_GPDIR    0X21 // Global GPIO Direction Register (全局GPIO方向寄存器)
// #define 	WK2XXX_GPDAT    0X31 // Global GPIO Data Register (全局GPIO数据寄存器)
// #define 	WK2XXX_GPORT    1    // 用于寄存器读写函数中的端口号标识：全局寄存器组的PORT参数

// // Slave Uarts Rigister Address Defines (子串口寄存器地址定义)
// #define 	WK2XXX_SPAGE    0X03 // Slave Port Page Select Register (子端口页选择寄存器)
// // PAGE0
// #define 	WK2XXX_SCR      0X04 // Serial Control Register (串口控制寄存器)
// #define 	WK2XXX_LCR      0X05 // Line Control Register (线路控制寄存器)
// #define 	WK2XXX_FCR      0X06 // FIFO Control Register (FIFO控制寄存器)
// #define 	WK2XXX_SIER     0X07 // Slave Interrupt Enable Register (子中断使能寄存器)
// #define 	WK2XXX_SIFR     0X08 // Slave Interrupt Flag Register (子中断标志寄存器)
// #define 	WK2XXX_TFCNT    0X09 // TX FIFO Count Register (发送FIFO计数寄存器)
// #define 	WK2XXX_RFCNT    0X0A // RX FIFO Count Register (接收FIFO计数寄存器)
// #define 	WK2XXX_FSR      0X0B // FIFO Status Register (FIFO状态寄存器)
// #define 	WK2XXX_LSR      0X0C // Line Status Register (线路状态寄存器)
// #define 	WK2XXX_MSR      0X0D // Modem Status Register (调制解调器状态寄存器)
// #define 	WK2XXX_SPR      0X0E // Scratchpad Register (暂存寄存器)
// #define 	WK2XXX_FDAT     0X0F // FIFO Data Register (FIFO数据寄存器)
// // PAGE1
// #define 	WK2XXX_DLL      0X04 // Divisor Latch Low Register (分频锁存低位)
// #define 	WK2XXX_DLH      0X05 // Divisor Latch High Register (分频锁存高位)
// #define 	WK2XXX_FCFG     0X06 // FIFO Configuration Register (FIFO配置寄存器)
// #define 	WK2XXX_RBST     0X07 // Baudrate Source Selection Register (波特率源选择寄存器)
// #define 	WK2XXX_MISR     0X08 // Mode and Interrupt Source Register (模式和中断源寄存器)
// #define 	WK2XXX_FMD      0X09 // FIFO Mode Register (FIFO模式寄存器)
// #define 	WK2XXX_TXEMP    0X0A // TX Empty Threshold Register (发送空闲阈值寄存器)
// #define 	WK2XXX_RXFTH    0X0B // RX FIFO Threshold Register (接收FIFO阈值寄存器)
// #define 	WK2XXX_RWTO     0X0C // RX Timeout Register (接收超时寄存器)
// #define 	WK2XXX_XON1     0X0D // XON1 Character Register (XON1字符寄存器)
// #define 	WK2XXX_XOFF1    0X0E // XOFF1 Character Register (XOFF1字符寄存器)
// #define 	WK2XXX_XON2     0X0F // XON2 Character Register (XON2字符寄存器)

// // -----------------------------------------------------------------------------
// // WK2XXX 寄存器的位定义 (保持不变)
// // -----------------------------------------------------------------------------
// // GENA
// #define     WK2XXX_UARTEN   0x01
// // GMUT
// #define     WK2XXX_I2CEN    0x02
// // GIER & GIFR
// #define     WK2XXX_UT4INT   0x08 // UT4 中断标志
// #define     WK2XXX_UT3INT   0x04 // UT3 中断标志
// #define     WK2XXX_UT2INT   0x02 // UT2 中断标志
// #define     WK2XXX_UT1INT   0x01 // UT1 中断标志

// // LCR
// #define     WK2XXX_DLAB     0x80
// #define     WK2XXX_WL8      0x03
// // FCR
// #define     WK2XXX_FIFOEN   0x01
// #define     WK2XXX_TFCLR    0x02
// #define     WK2XXX_RFCLR    0x04
// // FSR
// #define 	WK2XXX_RDAT     0x01
// #define 	WK2XXX_TDAT     0x02
// #define 	WK2XXX_FE       0x04
// #define 	WK2XXX_PE       0x08
// // MISR
// #define 	WK2XXX_SRTS     0x02

// // -----------------------------------------------------------------------------
// // 常用波特率宏定义 (保持不变)
// // -----------------------------------------------------------------------------
// #define     B600      600
// #define 	B1200	  1200
// #define 	B2400	  2400
// #define 	B4800     4800
// #define 	B9600	  9600
// #define 	B19200	  19200
// #define 	B38400	  38400
// #define 	B57600	  57600
// #define		B115200	  115200
// #define		B230400	  230400

// // -----------------------------------------------------------------------------
// // WK2114 硬件引脚定义 (替换为 HAL 库宏)
// // 假设 IRQ 连接到 PD2 (EXTI2)，REST 连接到 PA8 (请根据实际 CubeMX 配置修改)
// // -----------------------------------------------------------------------------

// // 原：#define WK2114_IRQ_PIN      GET_PIN(D,2)  // RT-Thread 引脚宏
// // **替换：** IRQ 引脚端口和引脚号
// #define WK2114_IRQ_GPIO_PORT      GPIOD           
// #define WK2114_IRQ_GPIO_PIN       GPIO_PIN_2      


// // WK2114 复位引脚操作宏 (替换为 HAL 库 GPIO 操作函数)
// // 原：#define WK2114_REST_L       rt_pin_write(WK2114_REST_PIN, 0)
// // **替换：** HAL_GPIO_WritePin 函数 (复位引脚拉低)
// #define WK2114_REST_L       HAL_GPIO_WritePin(WK2114_REST_GPIO_PORT, WK2114_REST_GPIO_PIN, GPIO_PIN_RESET) 

// // 原：#define WK2114_REST_H       rt_pin_write(WK2114_REST_PIN, 1)
// // **替换：** HAL_GPIO_WritePin 函数 (复位引脚拉高)
// #define WK2114_REST_H       HAL_GPIO_WritePin(WK2114_REST_GPIO_PORT, WK2114_REST_GPIO_PIN, GPIO_PIN_SET) 

// // -----------------------------------------------------------------------------
// // FreeRTOS 任务通知值定义 (用于 IRQ 唤醒任务)
// // -----------------------------------------------------------------------------
// #define WK2114_IRQ_NOTIFY_VALUE   (0x01) // FreeRTOS 任务通知标志：外部中断触发

// // -----------------------------------------------------------------------------
// // WK2114 子串口使能宏定义 (保持不变)
// // -----------------------------------------------------------------------------
// #define WK2114_PORT1        0x01        
// #define WK2114_PORT2        0x02
// #define WK2114_PORT3        0x04
// #define WK2114_PORT4        0x08
// #define WK2114_PORT_EN      (WK2114_PORT1 | WK2114_PORT3) // 示例：仅使能端口1和3

// // -----------------------------------------------------------------------------
// // 函数声明 (Function Prototypes) 
// // -----------------------------------------------------------------------------

// void Wk2114PortInit(unsigned char port);          // 子串口初始化
// void Wk2114Close(unsigned char port);             // 子串口关闭
// void Wk2114SetBaud(unsigned char port,int baud);  // 子串口设置波特率
// void Wk2114BaudAdaptive(void);                   // 波特率自适应 (如果支持)
// void Wk2114_config(void);                        // WK2114 芯片全局配置
// void WK2114_Config_Port(int port, int bps);      // 配置单个子串口
// void Wk2114WriteReg(unsigned char port,unsigned char reg,unsigned char byte); // 写入寄存器
// unsigned char Wk2114ReadReg(unsigned char port,unsigned char reg);            // 读取寄存器

// // 子串口发送数据函数
// void Wk2114_Uart1SendByte(char dat);
// void Wk2114_Uart2SendByte(char dat);
// void Wk2114_Uart3SendByte(char dat);
// void Wk2114_Uart4SendByte(char dat);

// // 设置子串口接收回调函数
// uint8_t Wk2114_SlaveRecv_Set(uint8_t index, void (*func)(char byte));

// void WK2114_init(void);                          // WK2114 驱动模块的整体初始化函数

// // -----------------------------------------------------------------------------
// // FreeRTOS 任务函数声明
// // -----------------------------------------------------------------------------
// void wk2xxx_task_entry(void *argument);          // 驱动工作任务入口函数

// #endif // _WK2XXX_H_




#ifndef _WK2XXX_H_
#define _WK2XXX_H_

/*
  wk2xxx_hal.h
  HAL + FreeRTOS 版的 WK2xxx（WK2114）串口扩展驱动头文件
  - 保留原有寄存器、位定义和 API 名称
  - 把 RT-Thread/rtdevice 相关宏替换为 STM32 HAL 风格的宏
  使用说明见 wk2xxx_hal.c 的顶部注释（CubeMX 配置说明）
*/

#include "main.h"
#include "cmsis_os.h"
#include <stdint.h>

/* --- 寄存器与位宏，保持和原文件一致 --- */
/* wkxxxx  Global rigister address defines */
#define 	WK2XXX_GENA     0X00
#define 	WK2XXX_GRST     0X01
#define		WK2XXX_GMUT     0X02
#define 	WK2XXX_GIER     0X10
#define 	WK2XXX_GIFR     0X11
#define 	WK2XXX_GPDIR    0X21
#define 	WK2XXX_GPDAT    0X31
#define 	WK2XXX_GPORT    1 // global port

/* slave uarts registers */
#define 	WK2XXX_SPAGE    0X03
/* PAGE0 */
#define 	WK2XXX_SCR      0X04
#define 	WK2XXX_LCR      0X05
#define 	WK2XXX_FCR      0X06
#define 	WK2XXX_SIER     0X07
#define 	WK2XXX_SIFR     0X08
#define 	WK2XXX_TFCNT    0X09
#define 	WK2XXX_RFCNT    0X0A
#define 	WK2XXX_FSR      0X0B
#define 	WK2XXX_LSR      0X0C
#define 	WK2XXX_FDAT     0X0D
#define 	WK2XXX_FWCR     0X0E
#define 	WK2XXX_RS485    0X0F
/* PAGE1 */
#define 	WK2XXX_BAUD1    0X04
#define 	WK2XXX_BAUD0    0X05
#define 	WK2XXX_PRES     0X06
#define 	WK2XXX_RFTL     0X07
#define 	WK2XXX_TFTL     0X08
#define 	WK2XXX_FWTH     0X09
#define 	WK2XXX_FWTL     0X0A
#define 	WK2XXX_XON1     0X0B
#define 	WK2XXX_XOFF1    0X0C
#define 	WK2XXX_SADR     0X0D
#define 	WK2XXX_SAEN     0X0E
#define 	WK2XXX_RRSDLY   0X0F

/* bits and flags (same as original) */
/* GENA bits */
#define 	WK2XXX_UT4EN	0x08
#define 	WK2XXX_UT3EN	0x04
#define 	WK2XXX_UT2EN	0x02
#define 	WK2XXX_UT1EN	0x01
/* GRST */
#define 	WK2XXX_UT4SLEEP	0x80
#define 	WK2XXX_UT3SLEEP	0x40
#define 	WK2XXX_UT2SLEEP	0x20
#define 	WK2XXX_UT1SLEEP	0x10
#define 	WK2XXX_UT4RST	0x08
#define 	WK2XXX_UT3RST	0x04
#define 	WK2XXX_UT2RST	0x02
#define 	WK2XXX_UT1RST	0x01
/* GIER */
#define 	WK2XXX_UT4IE	0x08
#define 	WK2XXX_UT3IE	0x04
#define 	WK2XXX_UT2IE	0x02
#define 	WK2XXX_UT1IE	0x01
/* GIFR */
#define 	WK2XXX_UT4INT	0x08
#define 	WK2XXX_UT3INT	0x04
#define 	WK2XXX_UT2INT	0x02
#define 	WK2XXX_UT1INT	0x01
/* SPAGE */
#define 	WK2XXX_SPAGE0	0x00
#define 	WK2XXX_SPAGE1   0x01
/* SCR */
#define 	WK2XXX_SLEEPEN	0x04
#define 	WK2XXX_TXEN     0x02
#define 	WK2XXX_RXEN     0x01
/* LCR */
#define 	WK2XXX_BREAK	0x20
#define 	WK2XXX_IREN     0x10
#define 	WK2XXX_PAEN     0x08
#define 	WK2XXX_PAM1     0x04
#define 	WK2XXX_PAM0     0x02
#define 	WK2XXX_STPL     0x01
/* SIER */
#define 	WK2XXX_FERR_IEN      0x80
#define 	WK2XXX_CTS_IEN       0x40
#define 	WK2XXX_RTS_IEN       0x20
#define 	WK2XXX_XOFF_IEN      0x10
#define 	WK2XXX_TFEMPTY_IEN   0x08
#define 	WK2XXX_TFTRIG_IEN    0x04
#define 	WK2XXX_RXOUT_IEN     0x02
#define 	WK2XXX_RFTRIG_IEN    0x01
/* FSR bits */
#define 	WK2XXX_RFOE     0x80
#define 	WK2XXX_RFBI     0x40
#define 	WK2XXX_RFFE     0x20
#define 	WK2XXX_RFPE     0x10
#define 	WK2XXX_RDAT     0x08
#define 	WK2XXX_TDAT     0x04
#define 	WK2XXX_TFULL    0x02
#define 	WK2XXX_TBUSY    0x01
/* LSR bits */
#define 	WK2XXX_OE       0x08
#define 	WK2XXX_BI       0x04
#define 	WK2XXX_FE       0x02
#define 	WK2XXX_PE       0x01

/* 常用波特率宏 */
#define     B600      600
#define 	B1200	  1200
#define 	B2400	  2400
#define 	B4800     4800
#define 	B9600	  9600
#define 	B19200	  19200
#define 	B38400	  38400
#define 	B1800	  1800
#define 	B3600	  3600
#define		B7200     7200
#define 	B14400    14400
#define  	B28800	  28800
#define     B57600	  57600
#define		B115200	  115200
#define		B230400	  230400

/* --- 硬件连接宏（按你的板子修改） ---
   默认：
     - MCU <-> WK2xxx 的控制 / IRQ 引脚： RESET -> PA8, IRQ -> PD2 (EXTI)
     - MCU 与 WK2xxx 的命令/数据通信用 UART2 (huart2)
   如果你使用其他 UART 或 GPIO，请在工程中修改这些宏
*/
#ifndef WK2114_IRQ_GPIO_Port
#define WK2114_IRQ_GPIO_Port    GPIOD
#endif
#ifndef WK2114_IRQ_Pin
#define WK2114_IRQ_Pin          GPIO_PIN_2   /* PD2 */
#endif




/* 操作 RESET 的快捷宏 */
#define WK2114_REST_L  HAL_GPIO_WritePin(WK2114_REST_GPIO_Port, WK2114_REST_Pin, GPIO_PIN_RESET)
#define WK2114_REST_H  HAL_GPIO_WritePin(WK2114_REST_GPIO_Port, WK2114_REST_Pin, GPIO_PIN_SET)

/* 如果需要筛选启用某些子串口可以配置下面宏（保持和原来相同） */
#define WK2114_PORT1        0x01        //GPS
#define WK2114_PORT2        0x02
#define WK2114_PORT3        0x04        //指纹
#define WK2114_PORT4        0x08
#define WK2114_PORT_EN      (WK2114_PORT1 | WK2114_PORT3)

/* --- API 原型（与原实现保持一致） --- */
void Wk2114PortInit(unsigned char port);
void Wk2114SetBaud(unsigned char port,int baud);

void Wk2114BaudAdaptive(void);
void Wk2114_config(void);
void WK2114_Config_Port(int port, int bps);
void WK2114_init(void);

void Wk2114_Uart1SendByte(char dat);
void Wk2114_Uart2SendByte(char dat);
void Wk2114_Uart3SendByte(char dat);
void Wk2114_Uart4SendByte(char dat);

uint8_t Wk2114_SlaveRecv_Set(uint8_t index, void (*func)(char byte));

/* 下面是被内部使用但保留为 static 的函数在 .c 中实现：
   static void Wk2114WriteReg(..);
   static unsigned char Wk2114ReadReg(..);
   unsigned char Wk2114_ReadByte(void);
   void Wk2114_WriteByte(unsigned char byte);
   等
*/

/* 外部需提供（在 main.c/或 stm32xx_it.c 中）：
   - UART_HandleTypeDef huart2; (或修改为你工程的串口 handle)
   - 在 HAL_UART_RxCpltCallback 中调用 Wk2114_UART_RxCpltCallback(&huartX);
   - 在 HAL_GPIO_EXTI_Callback 中，当 IRQ pin 被触发时调用 Wk2114_ExIrqCallback();
*/
void Wk2114_UART_RxCpltCallback(UART_HandleTypeDef *huart);
extern UART_HandleTypeDef huart2;

#endif /* _WK2XXX_HAL_H_ */
