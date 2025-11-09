#ifndef __PCF8563_H
#define __PCF8563_H

#include "main.h"
#include "cmsis_os2.h"
// --- PCF8563 芯片定义 ---
// PCF8563 的 7 位 I2C 从机地址是 0x51
// HAL 库要求传入 8 位地址 (0x51 << 1)，即 0xA2
#define PCF8563_DEV_ADDR     (0x51 << 1)

// 寄存器地址定义
#define PCF8563_REG_CONTROL1 0x00  // 控制/状态寄存器 1
#define PCF8563_REG_VL_SECONDS 0x02 // 时间/日期数据的起始地址：秒寄存器
#define PCF8563_REG_WRITE_COUNT 7  // 要读取的字节数（从秒到年共 7 个字节）

// BCD 码存储的 RTC 时间结构体
typedef struct
{
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;
    uint8_t weekday;
    uint8_t month;
    uint8_t year;
} PCF8563_Time_t;

// --- 函数声明 ---
// 1. 初始化和设置
HAL_StatusTypeDef PCF8563_Init(I2C_HandleTypeDef *hi2c);
osStatus_t PCF8563_Set_Time(I2C_HandleTypeDef *hi2c, PCF8563_Time_t *init_time);

// 2. 读操作和 BCD 转换
uint8_t BCD_to_DEC(uint8_t bcd_val);
osStatus_t PCF8563_Read_Time(I2C_HandleTypeDef *hi2c, PCF8563_Time_t *time);

#endif /* __PCF8563_H */
