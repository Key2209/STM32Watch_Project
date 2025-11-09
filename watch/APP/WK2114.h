// wk2114.h
#ifndef __WK2114_H
#define __WK2114_H

#include "main.h" // 包含 STM32 HAL 库
#include "cmsis_os2.h" // 包含 FreeRTOS 相关定义




#define EXTI_WK2114_IRQn EXTI2_IRQn



// --- 主接口 UART 句柄 (假设使用 USART2) ---
extern UART_HandleTypeDef huart2; 
// --- FreeRTOS 任务句柄 (用于中断通知) ---
//extern osThreadId_t Wk2114TaskHandle;

// --- WK2114 寄存器地址定义 (摘自您提供的 wk2xxx.h) ---
#define WK2XXX_GPORT    1
#define WK2XXX_GENA     0x00
#define WK2XXX_GRST     0x01
#define WK2XXX_GIER     0x10
#define WK2XXX_GIFR     0x11
#define WK2XXX_SPAGE    0x03
#define WK2XXX_SCR      0x04
#define WK2XXX_LCR      0x05
#define WK2XXX_FCR      0x06
#define WK2XXX_SIER     0x07
#define WK2XXX_SIFR     0x08
#define WK2XXX_FSR      0x0B
#define WK2XXX_FDAT     0x0D

// --- Page 1 寄存器地址 ---
#define WK2XXX_BAUD1    0x04
#define WK2XXX_BAUD0    0x05
#define WK2XXX_RFTL     0x07
#define WK2XXX_TFTL     0x08

// --- 寄存器位定义 (简化常用位) ---
#define WK2XXX_SPAGE1   0x01
#define WK2XXX_SPAGE0   0x00
#define WK2XXX_TXEN     0x02
#define WK2XXX_RXEN     0x01
#define WK2XXX_RFTRIG_IEN 0x01  // 接收触点中断使能
#define WK2XXX_RXOUT_IEN  0x02  // 接收超时中断使能
#define WK2XXX_UT1INT   0x01
#define WK2XXX_UT2INT   0x02
#define WK2XXX_UT3INT   0x04
#define WK2XXX_UT4INT   0x08
#define WK2XXX_RDAT     0x08    // FSR 寄存器位：RX FIFO 有数据

// --- 命令宏定义 ---
// 写寄存器命令：00 C1C0 A3A2A1A0
#define WK2114_CMD_W_REG(CH_NO, REG_ADDR)   ((uint8_t)(((CH_NO-1) << 4) + REG_ADDR))
// 读寄存器命令：01 C1C0 A3A2A1A0 -> 0x40 + ...
#define WK2114_CMD_R_REG(CH_NO, REG_ADDR)   ((uint8_t)(0x40 + (((CH_NO-1) << 4) + REG_ADDR)))


// --- 函数声明 ---
HAL_StatusTypeDef WK2114_Write_Reg(uint8_t channel, uint8_t reg_addr, uint8_t data);
HAL_StatusTypeDef WK2114_Read_Reg(uint8_t channel, uint8_t reg_addr, uint8_t *data);
void WK2114_Rest(void);
HAL_StatusTypeDef WK2114_Init_All(int baud_rate);
void WK2114_IRQ_Handler(void);
void StartWk2114Task(void *argument);



#endif // __WK2114_H

