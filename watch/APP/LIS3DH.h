#ifndef LIS3DH_H
#define LIS3DH_H
#include "main.h"
#include "cmsis_os2.h"
#include "stm32f4xx_hal_i2c.h"

// -------------------------------------------------------------------
// 宏定义和常量
// -------------------------------------------------------------------

// LIS3DH I2C 地址 (8位，已左移)
// 假设 LIS3DH 的 SA0/SDO 引脚接 GND (7-bit = 0x1C)
#define LIS3DH_I2C_ADDRESS_7BIT  (0x1C)
#define LIS3DH_I2C_ADDRESS_8BIT  (LIS3DH_I2C_ADDRESS_7BIT << 1) // 0x38

// LIS3DH 寄存器地址
#define LIS3DH_REG_WHO_AM_I      (0x0F) // 身份识别码，应为 0x33
#define LIS3DH_REG_CTRL_REG1     (0x20) // ODR 和轴启用
#define LIS3DH_REG_CTRL_REG4     (0x23) // Full Scale 和 BDU
#define LIS3DH_REG_OUT_X_L       (0x28) // X 轴数据低字节

// 读取多个寄存器时，寄存器地址 MSB 必须置 1，启用自动递增
#define LIS3DH_READ_BURST_ADDR   (LIS3DH_REG_OUT_X_L | 0x80) // 0xA8

// 灵敏度（在 +/-2g 模式下，通常为 1 LSB/mg = 1000 LSB/g）
#define LIS3DH_SENSITIVITY_2G    (1000.0f) 

// 加速度计数据结构体
typedef struct {
    float x_g;
    float y_g;
    float z_g;
} LIS3DH_Data_t;

typedef struct{
  unsigned char tumbleEnableStatus : 1;
  unsigned char stepEnableStatus : 1;
  unsigned char SedentaryEnableStatus : 1;
  unsigned char tumbleStatus : 1;
  unsigned char SedentaryStatus : 1;
  unsigned short stepCount;
  unsigned char SedentaryInterval;
  unsigned short SedentaryTime;
}lis3dh;

// -------------------------------------------------------------------
// 函数声明
// -------------------------------------------------------------------


HAL_StatusTypeDef LIS3DH_Init(I2C_HandleTypeDef *hi2c, uint32_t timeout);

/**
 * @brief 使用 DMA 从 LIS3DH 读取三轴加速度数据
 * @param hi2c: I2C 句柄指针
 * @param data: 指向存储结果的结构体
 * @param timeout: RTOS 阻塞超时时间
 * @retval osStatus_t: osOK 或错误代码
 */
osStatus_t LIS3DH_Read_Acc_DMA(I2C_HandleTypeDef *hi2c, LIS3DH_Data_t *data, uint32_t timeout);
extern LIS3DH_Data_t Accel_Data;
int lis3dh_init(void);
void get_lis3dhInfo(float *accX, float *accY, float *accZ);
int lis3dh_tumble(float x, float y, float z);
int run_lis3dh_arithmetic(void);
void set_lis3dh_enableStatus(unsigned char cmd);
unsigned char get_lis3dh_enableStatus(void);
int get_lis3dh_tumbleStatus(void);
int get_lis3dh_stepCount(void);
void del_lis3dh_stepCount(void);
void set_lis3dh_SedentaryTime(unsigned short interval, unsigned short time);
int get_lis3dh_SedentaryStatus(void);

#endif // LIS3DH_H
