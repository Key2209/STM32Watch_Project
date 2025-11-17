#ifndef SRAM_DRIVER_H
#define SRAM_DRIVER_H

#include "main.h"
#include "FreeRTOS.h"
#include "semphr.h"

// 假设您在 CubeMX 中设置的 SPI 句柄
extern SPI_HandleTypeDef hspi1;

// 假设您在 CubeMX 中设置的 CS 引脚
#define SRAM_CS_GPIO_Port GPIOE
#define SRAM_CS_Pin       GPIO_PIN_0

// SRAM 命令定义 (来自规格书)
#define SRAM_CMD_WRITE_ENABLE   0x06 // 写使能
#define SRAM_CMD_WRITE_DISABLE  0x04 // 写禁用
#define SRAM_CMD_READ           0x03 // 慢速读
#define SRAM_CMD_FAST_READ      0x0B // 快速读 (推荐)
#define SRAM_CMD_WRITE          0x02 // 写入
#define SRAM_CMD_RESET_EN       0x66 // 重置使能
#define SRAM_CMD_RESET          0x99 // 重置

// Fast Read 需要的虚拟周期 (Dummy Cycles)
#define SRAM_FAST_READ_DUMMY    1

// DMA 完成信号量
extern SemaphoreHandle_t xSramDmaSemaphore;

// CS 操作宏
#define SRAM_CS_HIGH() HAL_GPIO_WritePin(SRAM_CS_GPIO_Port, SRAM_CS_Pin, GPIO_PIN_SET)
#define SRAM_CS_LOW()  HAL_GPIO_WritePin(SRAM_CS_GPIO_Port, SRAM_CS_Pin, GPIO_PIN_RESET)

// 函数声明
void SRAM_Init_Sequence(void);
BaseType_t SRAM_Write_DMA(uint32_t Address, const uint8_t *pData, uint32_t Size);
BaseType_t SRAM_Read_DMA(uint32_t Address, uint8_t *pData, uint32_t Size);
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi);
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi);

#endif // SRAM_DRIVER_H

