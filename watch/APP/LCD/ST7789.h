#ifndef _ST7789_H_
#define _ST7789_H_

// 包含标准HAL库头文件
#include "stm32f4xx_hal.h"
#include <stdint.h>

#ifdef HAL_SRAM_MODULE_ENABLED

// 根据CubeMX配置的背光引脚进行调整
#define LCD_BL_Port         GPIOB
#define LCD_BL_Pin          GPIO_PIN_1

#define ST7789_Light_ON()   HAL_GPIO_WritePin(LCD_BL_Port, LCD_BL_Pin, GPIO_PIN_SET)
#define ST7789_Light_OFF()  HAL_GPIO_WritePin(LCD_BL_Port, LCD_BL_Pin, GPIO_PIN_RESET)

// FSMC/FMC 地址宏定义 (假设使用Bank 1, RS引脚为A16)
// RS引脚: A16 (ST7789_RS_PIN = 17)
#define ST7789_RS_PIN       17 // FSMC_A16
// 命令地址 (RS = 0)
#define ST7789_REG          (*((volatile uint16_t *)(0x60000000)))
// 数据地址 (RS = 1, 0x60000000 | (1 << (A16+1)))
#define ST7789_DAT          (*((volatile uint16_t *)(0x60000000 | (1<<(ST7789_RS_PIN+1))))) 

// FSMC操作宏
#define write_command(x)   (ST7789_REG=(x))
#define write_data(x)      (ST7789_DAT=(x))
#define read_data()        ST7789_DAT

// 兼容性的宏，保留
#define St7789_WCMD(x)      (ST7789_REG=(x))
#define St7789_WDATA(x)     (ST7789_DAT=(x))
#define St7789_RDATA()      ST7789_DAT

// ST7789 指令
#define ST7789_WRAM     0X2C
#define ST7789_RRAM     0X2E
#define ST7789_SETX     0X2A
#define ST7789_SETY     0X2B

// LCD分辨率 (根据您的屏幕型号调整)
#define ST7789_WIDE     320
#define ST7789_HIGH     240

typedef struct
{
    short wide;           // 宽度
    short high;           // 高度
    uint16_t id;          // ID
    uint8_t dir;          // 方向控制：0，竖屏；1，横屏。
    uint8_t wramcmd;      // 开始写gram指令
    uint8_t rramcmd;      // 开始读gram指令
    uint8_t setxcmd;      // 设置x坐标指令
    uint8_t setycmd;      // 设置y坐标指令
}screen_dev_t;

extern screen_dev_t screen_dev;

// 核心函数声明
void St7789_PrepareWrite(void);
void St7789_SetCursorPos(short x,short y);
void St7789_PrepareFill(short x1, short y1, short x2, short y2);

uint8_t St7789_init(void);
void St7789_DrawPoint(short x,short y,uint32_t color);
uint32_t St7789_ReadPoint(short x,short y);
void St7789_FillColor(short x1,short y1,short x2,short y2,uint32_t color);
void St7789_FillData(short x1,short y1,short x2,short y2,unsigned short* dat);

// 移除了所有rt_开头的RT-Thread兼容函数
#endif  //#ifdef HAL_SRAM_MODULE_ENABLED
#endif
