



#include "LIS3DH.h"
#include "stm32f4xx_hal_i2c.h"
#include "string.h" // for memset
#include "string.h"
#include <stdint.h>
#include "i2c_dma_manager.h"



#define LIS3DH_I2C_ADDRESS  (0x19 << 1) 

// LIS3DH 寄存器地址
#define LIS3DH_CTRL_REG1    (0x20)
#define LIS3DH_CTRL_REG4    (0x23)
#define LIS3DH_OUT_X_L      (0x28)
#define LIS3DH_READ_BURST   (LIS3DH_OUT_X_L | 0x80) // 0xA8



// 外部依赖的 I2C DMA 读写函数
extern osStatus_t I2C_Manager_Write_DMA(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size,uint32_t timeout);
extern osStatus_t I2C_Manager_Read_DMA(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size,uint32_t timeout);


/**
 * @brief LIS3DH 传感器初始化配置 (阻塞式)
 * * @param hi2c: I2C 句柄指针
 * @param timeout: HAL 阻塞超时时间 (毫秒)
 * @retval HAL_StatusTypeDef: HAL_OK 或错误代码
 */
HAL_StatusTypeDef LIS3DH_Init(I2C_HandleTypeDef *hi2c, uint32_t timeout)
{
    uint8_t tx_data;
    HAL_StatusTypeDef hal_status;

    // --- 1. 配置 CTRL_REG1 (0x20): ODR=50Hz, X/Y/Z 轴启用, Normal Mode (0x47) ---
    // 目的: 启用传感器，设置数据速率。
    tx_data = 0x47; 
    hal_status = HAL_I2C_Mem_Write(hi2c, 
                                   LIS3DH_I2C_ADDRESS, 
                                   LIS3DH_CTRL_REG1, 
                                   1, // 寄存器地址长度为 1 字节
                                   &tx_data, 
                                   1, // 数据长度为 1 字节
                                   timeout);

    // 检查第一次写入是否成功
    if (hal_status != HAL_OK)
    {
        return hal_status; // 如果失败，立即返回错误状态
    }
// --- 2. 配置 CTRL_REG4 (0x23): BDU 启用, Full Scale = +/-2g, High Resolution (0x88) ---
    // 目的: 启用 BDU 保证数据一致性，启用 High Resolution 获得 12-bit 精度，设置 +/-2g 量程。
    tx_data = 0x88; // BDU=1, FS=00 (+/-2g), HR=1
    hal_status = HAL_I2C_Mem_Write(hi2c, 
                                   LIS3DH_I2C_ADDRESS, 
                                   LIS3DH_CTRL_REG4, 
                                   1, 
                                   &tx_data, 
                                   1, 
                                   timeout);
    
    // 返回最终操作结果
    return hal_status;
}

// 2. 读取数据函数
osStatus_t LIS3DH_Read_Acc_DMA(I2C_HandleTypeDef *hi2c, LIS3DH_Data_t *data, uint32_t timeout)
{
    uint8_t buffer[6];
    osStatus_t status;
    
    // **读取操作：**
    // MemAddress = LIS3DH_READ_BURST (0xA8)，表示从 0x28 开始连续读 6 个字节
    // HAL 库中，MemAddress 对应要读取的寄存器地址。
    status = I2C_Manager_Read_DMA(hi2c, 
                                LIS3DH_I2C_ADDRESS, 
                                LIS3DH_READ_BURST,  // 0xA8, 带有 MSB=1 的起始寄存器地址
                                1,                  // MemAddSize=1，表示寄存器地址是 1 个字节
                                buffer,             // 接收 X_L, X_H, Y_L, Y_H, Z_L, Z_H
                                6,                  // 读取 6 字节
                                timeout);

    if (status != osOK) return status;
// 1. 组合 16-bit 原始值 (小端模式) 并右移 4 位以提取 12-bit 有效数据。
    // 关键修复：先组合成 int16_t，再进行右移，确保负数符号位正确扩展。

    // X 轴
    int16_t raw_x_16bit = (int16_t)((buffer[1] << 8) | buffer[0]); // 组合成带符号的 16 位整数
    int16_t raw_x_shifted = raw_x_16bit >> 4;                       // 对带符号整数进行算术右移
    
    // Y 轴
    int16_t raw_y_16bit = (int16_t)((buffer[3] << 8) | buffer[2]);
    int16_t raw_y_shifted = raw_y_16bit >> 4; 
    
    // Z 轴
    int16_t raw_z_16bit = (int16_t)((buffer[5] << 8) | buffer[4]);
    int16_t raw_z_shifted = raw_z_16bit >> 4; 

    // 2. 转换为 g 值
    // 12-bit HR Mode, +/-2g 的灵敏度是 1000.0 LSB/g 
    #define LIS3DH_SENSITIVITY_2G_12BIT (1000.0f) 

    data->x_g = (float)raw_x_shifted / LIS3DH_SENSITIVITY_2G_12BIT;
    data->y_g = (float)raw_y_shifted / LIS3DH_SENSITIVITY_2G_12BIT;
    data->z_g = (float)raw_z_shifted / LIS3DH_SENSITIVITY_2G_12BIT;

    return osOK;












}
