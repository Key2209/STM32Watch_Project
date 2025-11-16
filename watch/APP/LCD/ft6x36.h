#ifndef _FT6X36_H_
#define _FT6X36_H_

#include "main.h"
#include "ST7789.h"         // 用于获取屏幕尺寸等信息

/*********************************************************************************************
* 宏定义
*********************************************************************************************/
#define FT6X36_IIC_ADDR   0x38  // FT6x36 I2C 7位地址 (0x70/0x71, 所以 7位地址是 0x38)

#define TOUCH_NUM         2     // 最大支持触摸点数

// FT6x36 关键寄存器地址
#define FT_REG_P_STATUS   0x02  // 触摸状态寄存器
#define FT_REG_P1_XH      0x03  // 触摸点 1 X 坐标高 8 位
#define FT_REG_CHIPID     0xA8  // 芯片 ID 寄存器

#define FT6X36_RST_PORT        GPIOC
#define FT6X36_RST_PIN         GPIO_PIN_5

#define FT6X36_RESET_H()       HAL_GPIO_WritePin(FT6X36_RST_PORT, FT6X36_RST_PIN, GPIO_PIN_SET)
#define FT6X36_RESET_L()       HAL_GPIO_WritePin(FT6X36_RST_PORT, FT6X36_RST_PIN, GPIO_PIN_RESET)
/*********************************************************************************************
* 结构体定义
*********************************************************************************************/
typedef struct
{
    uint8_t direction;    // 屏幕方向：与 LCD 保持一致
    uint16_t wide;        // 屏幕宽度
    uint16_t high;        // 屏幕高度
    uint16_t id;          // 芯片 ID
    uint8_t status;       // 触摸状态 (触摸点数量)
    int16_t x[TOUCH_NUM]; // 触摸点 X 坐标
    int16_t y[TOUCH_NUM]; // 触摸点 Y 坐标
} TOUCH_Dev_t;


/*********************************************************************************************
* 外部变量声明
*********************************************************************************************/
// FT6x36 触摸屏设备结构体
extern TOUCH_Dev_t TouchDev;

// 【重要】声明在 main.c/i2c.c 中初始化的 I2C 句柄
// 假设您的 I2C 外设是 I2C1
extern I2C_HandleTypeDef hi2c1; 

/*********************************************************************************************
* 函数声明
*********************************************************************************************/
// 设置外部中断回调函数（用于 RTOS 任务通知等）
void TouchIrqSet(void (*func)(void));
// 触摸屏初始化
int ft6x36_init(void);
// 触摸屏扫描，读取触摸点数据
void ft6x36_TouchScan(void);
// FT6x36 外部中断处理函数（在 GPIO_EXTI_IRQHandler 中调用）
void ft6x36_Exti_Callback(void);

#endif /* _FT6X36_H_ */
