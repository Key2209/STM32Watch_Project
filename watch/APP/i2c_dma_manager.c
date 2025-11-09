#include "i2c_dma_manager.h"
#include "cmsis_os2.h"
#include "main.h"
#include <stdbool.h>

// I2C 句柄 (CubeMX 在 main.c 中定义)
extern I2C_HandleTypeDef hi2c1; 

// FreeRTOS 句柄 (main.c中定义)
extern osMutexId_t i2cBusMutexHandle;
extern osSemaphoreId_t i2cDmaCpltSemHandle;

// 内部错误标志
static volatile bool g_i2c_error = false;


/**
 * @brief 线程安全的 I2C 内存写操作 (DMA)
 */
osStatus_t I2C_Manager_Write_DMA(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size,uint32_t timeout)
{
    HAL_StatusTypeDef hal_status;
    osStatus_t os_status;

    // 1. [线程安全] 获取 I2C 总线互斥锁，开始“排队”
    //    如果总线被其他任务占用，本任务将在此处“睡眠”


    if (osMutexAcquire(i2cBusMutexHandle, timeout) != osOK)
    {

        
        return osErrorTimeout; // 获取互斥锁超时
    }
    // 0U 表示不阻塞，立即返回。
    osSemaphoreAcquire(i2cDmaCpltSemHandle, 0U);
    // --- 锁已获取，总线现在归本任务独占 ---
    
    g_i2c_error = false; // 清除错误标志

    // 2. 启动 I2C DMA 传输
    //    注意：pData 是调用者传入的缓冲区，它必须在DMA传输期间保持有效！
    //    (因为本函数会等待DMA完成，所以栈上的局部变量也是安全的)
    hal_status = HAL_I2C_Mem_Write_DMA(hi2c, 
                                     DevAddress, 
                                     MemAddress, 
                                     MemAddSize, 
                                     pData, 
                                     Size);

    if (hal_status != HAL_OK)
    {
        // 启动失败 (例如 I2C_BUSY)
        osMutexRelease(i2cBusMutexHandle); // 必须释放锁！
        return osError; 
    }

    // 3. [RTOS 高效等待] 等待 DMA 完成的“信号”
    //    本任务在此“睡眠”，不消耗 CPU
    os_status = osSemaphoreAcquire(i2cDmaCpltSemHandle, timeout);

    // 4. 检查是“正常唤醒”还是“超时”
    if (os_status != osOK)
    {
        // 超时！I2C 总线可能卡死了
        // 尝试中止传输 (这是必要的总线恢复步骤)
        HAL_I2C_Master_Abort_IT(hi2c, DevAddress);
        os_status = osErrorTimeout;
    }
    else if (g_i2c_error)
    {
        // 是被 ErrorCallback 唤醒的
        os_status = osError;
    }
    
    // 5. [线程安全] 释放 I2C 总线互斥锁，让其他任务可以“排队”
    osMutexRelease(i2cBusMutexHandle);
    
    return os_status; // 返回 osOK 或 osErrorTimeout
}


/**
 * @brief 线程安全的 I2C 内存读操作 (DMA)
 * @note 逻辑与 Write_DMA 完全相同
 */
osStatus_t I2C_Manager_Read_DMA(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size,uint32_t timeout)
{
    HAL_StatusTypeDef hal_status;
    osStatus_t os_status;

    // 1. [线程安全] 获取 I2C 总线互斥锁
    if (osMutexAcquire(i2cBusMutexHandle, timeout) != osOK)
    {
        return osErrorTimeout;
    }

    // --- 锁已获取 ---
    // 🚨 关键修复点：清除任何先前遗留的信号量计数
    // 即使信号量被错误回调释放了，这一步也会将其计数归零。
    // 0U 表示不阻塞，立即返回。
    osSemaphoreAcquire(i2cDmaCpltSemHandle, 0U);
    g_i2c_error = false;
    
    // 2. 启动 I2C DMA 读传输
    hal_status = HAL_I2C_Mem_Read_DMA(hi2c, 
                                    DevAddress, 
                                    MemAddress, 
                                    MemAddSize,
                                    pData, // DMA 将数据直接写入调用者的缓冲区
                                    Size);

    if (hal_status != HAL_OK)
    {
        osMutexRelease(i2cBusMutexHandle);
        return osError;
    }

    // 3. [RTOS 高效等待] 等待 DMA 完成信号
    os_status = osSemaphoreAcquire(i2cDmaCpltSemHandle, timeout);

    // 4. 检查唤醒状态
    if (os_status != osOK)
    {
        // 超时
        HAL_I2C_Master_Abort_IT(hi2c, DevAddress );
        os_status = osErrorTimeout;
    }
    else if (g_i2c_error)
    {
        os_status = osError;
    }

    // 5. [线程安全] 释放 I2C 总线互斥锁
    osMutexRelease(i2cBusMutexHandle);
    
    return os_status;
}


// -------------------------------------------------------------------
// --- HAL I2C 回调函数 ---
// 必须实现这些函数，它们由 I2C 中断 (ISR) 调用
// -------------------------------------------------------------------

/**
 * @brief 内存写传输完成回调
 */
void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == hi2c1.Instance)
    {
        // 释放信号量，唤醒正在等待的任务
        osSemaphoreRelease(i2cDmaCpltSemHandle);

    }
}

/**
 * @brief 内存读传输完成回调
 */
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == hi2c1.Instance)
    {
        // 释放信号量，唤醒正在等待的任务
        osSemaphoreRelease(i2cDmaCpltSemHandle);
    }
}

/**
 * @brief I2C 错误回调
 */
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == hi2c1.Instance)
    {
        g_i2c_error = true; // 设置错误标志
        
        // **至关重要**：即使出错了，也必须释放信号量！
        // 否则，等待的任务将永久卡死 (死锁)
        osSemaphoreRelease(i2cDmaCpltSemHandle);
    }
}


