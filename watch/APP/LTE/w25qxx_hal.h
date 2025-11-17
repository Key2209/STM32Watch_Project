// #ifndef __W25QXX_HAL_H
// #define __W25QXX_HAL_H

// #include "main.h" // 包含 HAL 库和 CubeMX 生成的定义
// #include "cmsis_os.h" // 引入 FreeRTOS/CMSIS-RTOS V2

// #define W25QXX_CS_PORT          GPIOA // 假设 CS 引脚为 PA15
// #define W25QXX_CS_PIN           GPIO_PIN_15
// #define W25QXX_SPI_HANDLE       hspi1 // 假设使用 SPI1，请根据 CubeMX 更改

// // 外部 SPI 句柄声明
// extern SPI_HandleTypeDef W25QXX_SPI_HANDLE;

// // 宏定义替换 RT-Thread 宏
// #define W25QXX_CS_L()           HAL_GPIO_WritePin(W25QXX_CS_PORT, W25QXX_CS_PIN, GPIO_PIN_RESET)
// #define W25QXX_CS_H()           HAL_GPIO_WritePin(W25QXX_CS_PORT, W25QXX_CS_PIN, GPIO_PIN_SET)

// // Flash 芯片类型和指令（保留原定义）
// // ... (W25QXX_TYPE, W25X_WriteEnable, W25X_ReadData 等指令定义，从原 w25qxx.h 复制)
// #define W25Q64 0XEF16
// extern uint16_t W25QXX_TYPE;

// // 外部函数声明
// void W25QXX_Init(void);
// void W25QXX_Read(void *pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead);
// void W25QXX_Write(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);

// #endif
