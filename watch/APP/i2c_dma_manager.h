#ifndef INC_I2C_DMA_MANAGER_H_
#define INC_I2C_DMA_MANAGER_H_
#include "FreeRTOS.h"          // 必须最先包含
#include "stm32f4xx_hal.h" // 
#include "cmsis_os.h"


extern I2C_HandleTypeDef hi2c1;

// FreeRTOS 句柄 (由 CubeMX 在 freertos.c 中定义)
extern osMutexId_t i2cBusMutexHandle;
extern osSemaphoreId_t i2cDmaCpltSemHandle;

/**
 * @brief 线程安全的 I2C 内存写操作 (DMA)
 * @param devAddr:   7位I2C从机地址 (HAL 会自动左移)
 * @param memAddr:   寄存器地址
 * @param pData:     指向要发送数据的指针
 * @param dataSize:  数据长度
 * @param timeout:   本次事务的总超时时间 (ms)
 * @retval osStatus_t: osOK (成功), osError (启动失败), osErrorTimeout (超时)
 */
//osStatus_t I2C_Manager_Write_DMA(uint16_t devAddr, uint16_t memAddr, uint8_t *pData, uint16_t dataSize, uint32_t timeout);
osStatus_t I2C_Manager_Write_DMA(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size,uint32_t timeout);
/**
 * @brief 线程安全的 I2C 内存读操作 (DMA)
 * @param devAddr:   7位I2C从机地址
 * @param memAddr:   寄存器地址
 * @param pData:     指向接收缓冲区的指针 (DMA将数据直接写入这里)
 * @param dataSize:  要读取的数据长度
 * @param timeout:   本次事务的总超时时间 (ms)
 * @retval osStatus_t: osOK (成功), osError (启动失败), osErrorTimeout (超时)
 */

osStatus_t I2C_Manager_Read_DMA(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size,uint32_t timeout);

// --- 回调函数 (在 stm32f4xx_it.c 或 i2c_dma_manager.c 中实现) ---
// 你需要确保 HAL_I2C_...Callback 函数被定义在某处
// (如果CubeMX没生成，就放在 i2c_dma_manager.c)

#endif /* INC_I2C_DMA_MANAGER_H_ */