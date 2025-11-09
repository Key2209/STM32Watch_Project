// /* apds9900.c */
#include "APDS9900.h"
#include "cmsis_os2.h"
#include "stm32f4xx_hal_i2c.h"
#include "string.h" // for memset
#include "string.h"
#include <stdint.h>

#include "i2c_dma_manager.h"

//堵塞读取：

// --- 全局变量定义 ---
uint16_t CH0_Data = 0;
uint16_t CH1_Data = 0;
uint16_t Proximity_Data = 0;

/**
 * @brief I2C 写寄存器阻塞式辅助函数
 * @param hi2c: I2C 句柄指针
 * @param tar_register: 目标寄存器地址 (0x00-0x11)
 * @param data: 要写入的数据
 * @retval None
 * @note 使用 HAL_I2C_Mem_Write 函数，它是阻塞式的，保证初始化顺序。
 */
void I2C_Write_Register_Blocking(I2C_HandleTypeDef *hi2c, uint8_t tar_register, uint8_t data) 
{
    // 构造 Command Byte: CMD=1 (0x80) | 寄存器地址
    uint8_t command_byte = APDS9900_CMD_HEADER | tar_register; 
    
    // HAL_I2C_Mem_Write：自动发送 [SlaveAddr+W] [Command Byte] [Data] [STOP]
    HAL_I2C_Mem_Write(hi2c, 
                      APDS9900_I2C_ADDR,      // 8位 I2C 地址
                      command_byte,           // Command Byte (作为 MemAddress)
                      1,                      // Command Byte 长度
                      &data,                  // 数据缓冲区
                      1,                      // 数据长度
                      1000);                   // 阻塞超时时间
}

/**
 * @brief APDS-9900/9901 传感器初始化配置
 * @param hi2c: I2C 句柄指针
 * @retval None
 * @note 严格按照伪代码的顺序进行阻塞式初始化。
 */
void APDS9900_Init(I2C_HandleTypeDef *hi2c)
{
    // ------------------------------------------------------------------------
    // 阶段 I: 清零和设置时间/计数 (伪代码 WriteRegData)
    // ------------------------------------------------------------------------
    
    // ATIME, PTIME, WTIME = 0xFF; PPCOUNT = 1
    uint8_t ATIME = 0xFF; 
    uint8_t PTIME = 0xFF;
    uint8_t WTIME = 0xFF;
    uint8_t PPCOUNT = 0x01;
    uint8_t CONTROL1 = 0x20; // 0x20: PDRIVE=100mA, PDIODE=CH1, PGAIN=1x, AGAIN=1x

    I2C_Write_Register_Blocking(hi2c, 0, 0);       // Disable and Powerdown
    I2C_Write_Register_Blocking(hi2c, 1, ATIME);   // 设置 ATIME
    I2C_Write_Register_Blocking(hi2c, 2, PTIME);   // 设置 PTIME
    I2C_Write_Register_Blocking(hi2c, 3, WTIME);   // 设置 WTIME
    I2C_Write_Register_Blocking(hi2c, 0x0E, PPCOUNT); // 设置 PPCOUNT

    // ------------------------------------------------------------------------
    // 阶段 II: 设置控制寄存器 1 (0x0F)
    // ------------------------------------------------------------------------
    I2C_Write_Register_Blocking(hi2c, 0x0F, CONTROL1); // 写入 0x20

    // ------------------------------------------------------------------------
    // 阶段 III: 启用功能 (PON, AEN, PEN, WEN = 0x0F)
    // ------------------------------------------------------------------------
    I2C_Write_Register_Blocking(hi2c, 0x00, APDS9900_ENABLE_ALL_FUNCTIONS); // 写入 0x0F
    
    // 伪代码: Wait(12);
    osDelay(12); 
}


// /**
//  * @brief 阻塞式读取 16 位字数据
//  * @param hi2c: I2C 句柄指针
//  * @param reg_addr: 16 位数据低字节的起始地址 (0x14, 0x16, 0x18)
//  * @retval uint16_t: 16 位拼接后的数据
//  * @note 对应伪代码中的 Read_Word(reg)，使用自动递增协议 (0xA0)。
//  */
uint16_t APDS9900_Read_Word_Blocking(I2C_HandleTypeDef *hi2c, uint8_t reg_addr)
{
    uint8_t read_buffer[2];
    uint16_t result = 0;
    
    // 构造 Command Byte: 0xA0 (Auto-Increment) | 寄存器地址
    uint8_t read_command = APDS9900_CMD_AUTO_INC | reg_addr; 
    
    // HAL_I2C_Mem_Read：阻塞式读取，自动执行 [SlaveAddr+W][Command Byte][Sr][SlaveAddr+R][Data][STOP]
    HAL_I2C_Mem_Read(hi2c, 
                     APDS9900_I2C_ADDR, 
                     read_command, 
                     1, // Command Byte 长度
                     read_buffer, 
                     2, // 读取 2 字节 (L + H)
                     500); // 阻塞超时时间 500ms

    // 数据拼接 (对应伪代码: barr[0] + 256 * barr[1])
    result = (read_buffer[1] << 8) | read_buffer[0];
    
    return result;
}


//DMA+互斥锁+信号读取：
/**
 * @brief 阻塞式读取 16 位字数据
 * @param hi2c: I2C 句柄指针
 * @param reg_addr: 16 位数据低字节的起始地址 (0x14, 0x16, 0x18)
 * @retval uint16_t: 16 位拼接后的数据
 * @note 对应伪代码中的 Read_Word(reg)，使用自动递增协议 (0xA0)。
 */
uint16_t APDS9900_Read_Word_DMA(I2C_HandleTypeDef *hi2c, uint8_t reg_addr,uint8_t timeout)
{
    uint8_t read_buffer[2];
    uint16_t result = 0;

    // 构造 Command Byte: 0xA0 (Auto-Increment) | 寄存器地址
    uint8_t read_command = APDS9900_CMD_AUTO_INC | reg_addr; 
    
    
    I2C_Manager_Read_DMA(
                     hi2c, 
                     APDS9900_I2C_ADDR, 
                     read_command, 
                     1, // Command Byte 长度
                     read_buffer,
                     2, 
                     timeout); // 阻塞超时时间 500ms


    // 数据拼接 (对应伪代码: barr[0] + 256 * barr[1])
    result = (read_buffer[1] << 8) | read_buffer[0];
    
    return result;
}
