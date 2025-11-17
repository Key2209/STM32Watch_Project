#include "SRAM_Driver.h"

// 声明信号量
SemaphoreHandle_t xSramDmaSemaphore = NULL;

/**
  * @brief  发送通用命令（非DMA操作）
  * @param  pTxData: 发送数据指针 (Command + Address)
  * @param  Size: 发送数据长度
  * @param  Timeout: 超时时间
  */
static void SRAM_Command_Transmit(uint8_t *pTxData, uint16_t Size, uint32_t Timeout)
{

    SRAM_CS_LOW(); // 拉低片选

    // 使用阻塞模式发送命令和地址
    HAL_SPI_Transmit(&hspi1, pTxData, Size, Timeout);

    // 注意：CS 在 DMA 数据传输完成后才拉高，或者在不需要数据传输的命令完成后立刻拉高
    // 对于 Read/Write 操作，CS 保持低电平。
    // 对于 Reset 等无数据操作，在这里拉高 CS。
}

/**
  * @brief  芯片上电初始化序列 (Reset Enable -> Reset)
  */
void SRAM_Init_Sequence(void)
{
    // // 1. 创建信号量（在 FreeRTOS 初始化后调用）
    // if (xSramDmaSemaphore == NULL) {
    //     xSramDmaSemaphore = xSemaphoreCreateBinary();
    // }
    
    // 2. 发送 Reset Enable (0x66)
    uint8_t tx_buf[4];
    tx_buf[0] = SRAM_CMD_RESET_EN;
    
    SRAM_CS_LOW();
    HAL_SPI_Transmit(&hspi1, tx_buf, 1, HAL_MAX_DELAY);
    SRAM_CS_HIGH(); // 终止命令

    // 3. 发送 Reset (0x99)
    tx_buf[0] = SRAM_CMD_RESET;
    
    SRAM_CS_LOW();
    HAL_SPI_Transmit(&hspi1, tx_buf, 1, HAL_MAX_DELAY);
    SRAM_CS_HIGH(); // 终止命令

    // 4. 发送写使能 (0x06)，确保设备可写
    tx_buf[0] = SRAM_CMD_WRITE_ENABLE;
    
    SRAM_CS_LOW();
    HAL_SPI_Transmit(&hspi1, tx_buf, 1, HAL_MAX_DELAY);
    SRAM_CS_HIGH();
}

/**
  * @brief  使用 DMA 写入数据到 SRAM (Command 0x02)
  * @param  Address: 24位地址 (0x000000 ~ 0x7FFFFF)
  * @param  pData: 待写入数据指针
  * @param  Size: 写入字节数
  * @retval pdPASS: 成功; pdFAIL: 失败或超时
  */
BaseType_t SRAM_Write_DMA(uint32_t Address, const uint8_t *pData, uint32_t Size)
{
    // 1. 发送写使能 (0x06) - 必须在每次写操作前执行
    uint8_t cmd_we = SRAM_CMD_WRITE_ENABLE;
    SRAM_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd_we, 1, HAL_MAX_DELAY);
    SRAM_CS_HIGH();

    // 2. 准备 Command (0x02) + 24-bit Address
    uint8_t header[4];
    header[0] = SRAM_CMD_WRITE;
    header[1] = (uint8_t)((Address >> 16) & 0xFF); // A23-A16
    header[2] = (uint8_t)((Address >> 8) & 0xFF);  // A15-A8
    header[3] = (uint8_t)(Address & 0xFF);         // A7-A0

    // 3. 阻塞发送 Command + Address
    SRAM_CS_LOW();
    HAL_SPI_Transmit(&hspi1, header, 4, HAL_MAX_DELAY); // 阻塞发送 Command 和 Address

    // 4. 启动 DMA 传输数据 (TX)
    if (HAL_SPI_Transmit_DMA(&hspi1, (uint8_t *)pData, Size) != HAL_OK)
    {
        SRAM_CS_HIGH();
        return pdFAIL;
    }

    // 5. 等待 DMA 传输完成信号量 (FreeRTOS 同步)
    // 假设等待 100ms 超时
    if (xSemaphoreTake(xSramDmaSemaphore, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        return pdPASS; // 传输完成，CS 在中断回调中拉高
    }
    else
    {
        // 超时或失败，需要手动停止 DMA 并拉高 CS
        HAL_SPI_DMAStop(&hspi1);
        SRAM_CS_HIGH();
        return pdFAIL;
    }
}

/**
  * @brief  使用 DMA 读取数据从 SRAM (Command 0x0B - Fast Read)
  * @param  Address: 24位地址 (0x000000 ~ 0x7FFFFF)
  * @param  pData: 接收数据缓冲区指针
  * @param  Size: 读取字节数
  * @retval pdPASS: 成功; pdFAIL: 失败或超时
  */
BaseType_t SRAM_Read_DMA(uint32_t Address, uint8_t *pData, uint32_t Size)
{
    // 1. 准备 Command (0x0B) + 24-bit Address + Dummy Byte
    uint8_t header[5];
    header[0] = SRAM_CMD_FAST_READ;
    header[1] = (uint8_t)((Address >> 16) & 0xFF);
    header[2] = (uint8_t)((Address >> 8) & 0xFF);
    header[3] = (uint8_t)(Address & 0xFF);
    header[4] = 0x00; // 虚拟字节 (Dummy Byte)

    // 2. 阻塞发送 Command + Address + Dummy Byte
    SRAM_CS_LOW();
    HAL_SPI_Transmit(&hspi1, header, 4 + SRAM_FAST_READ_DUMMY, HAL_MAX_DELAY);

    // 3. 启动 DMA 接收数据 (RX)
    if (HAL_SPI_Receive_DMA(&hspi1, pData, Size) != HAL_OK)
    {
        SRAM_CS_HIGH();
        return pdFAIL;
    }
    
    // 4. 等待 DMA 传输完成信号量
    if (xSemaphoreTake(xSramDmaSemaphore, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        return pdPASS; // 传输完成，CS 在中断回调中拉高
    }
    else
    {
        HAL_SPI_DMAStop(&hspi1);
        SRAM_CS_HIGH();
        return pdFAIL;
    }
}


// ============== FreeRTOS 中断回调函数 ==============

/**
  * @brief  SPI 发送完成回调函数（用于写操作）
  */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == hspi1.Instance)
    {
        SRAM_CS_HIGH(); // 传输完成，拉高片选终止操作
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(xSramDmaSemaphore, &xHigherPriorityTaskWoken); // 释放信号量
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/**
  * @brief  SPI 接收完成回调函数（用于读操作）
  */
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == hspi1.Instance)
    {
        SRAM_CS_HIGH(); // 传输完成，拉高片选终止操作
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(xSramDmaSemaphore, &xHigherPriorityTaskWoken); // 释放信号量
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

