/* apds9900.h */
#ifndef APDS9900_H
#define APDS9900_H
#include "main.h"   // 包含 HAL 和 hi2c1 的声明（CubeMX 生成）
#include <stdint.h>



#define APDS9900_I2C_ADDR       (0x39 << 1) // 8位 I2C 地址: 0x72

// --- 命令字节结构定义 ---
#define APDS9900_CMD_HEADER     0x80  // CMD=1 (Normal Mode/Write)
#define APDS9900_CMD_AUTO_INC   0xA0  // CMD=1 | TYPE=01 (Auto-Increment Protocol/Read)

// --- 寄存器地址定义 (ADD) ---
#define APDS9900_REG_ENABLE     0x00 
#define APDS9900_REG_ATIME      0x01
#define APDS9900_REG_PTIME      0x02
#define APDS9900_REG_WTIME      0x03
#define APDS9900_REG_PPCOUNT    0x0E
#define APDS9900_REG_CONTROL1   0x0F // 控制寄存器 1
#define APDS9900_REG_CH0DATAL   0x14 // CH0 环境光数据低字节
#define APDS9900_REG_CH1DATAL   0x16 // CH1 环境光数据低字节
#define APDS9900_REG_PDATAL     0x18 // 接近度数据低字节

// --- 启用寄存器 (ENABLE) 位定义 ---
#define APDS9900_ENABLE_ALL_FUNCTIONS (0x0F) // WEN | PEN | AEN | PON

// --- 全局变量声明 (用于存储读取到的数据) ---
extern I2C_HandleTypeDef hi2c1; // 假设 I2C 句柄名为 hi2c1

extern uint16_t CH0_Data;
extern uint16_t CH1_Data;
extern uint16_t Proximity_Data;

// --- 函数声明 ---
void APDS9900_Init(I2C_HandleTypeDef *hi2c);
//uint16_t APDS9900_Read_Word_Blocking(I2C_HandleTypeDef *hi2c, uint8_t reg_addr);
uint16_t APDS9900_Read_Word_DMA(I2C_HandleTypeDef *hi2c, uint8_t reg_addr,uint8_t timeout);

#endif
