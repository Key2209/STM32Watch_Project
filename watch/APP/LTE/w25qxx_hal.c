// #include "w25qxx_hal.h"
// //#include <cstdint>
// #include <string.h>

// // 假设 SPI 句柄为 hspi1，请确保在 main.c 中正确声明
// extern SPI_HandleTypeDef W25QXX_SPI_HANDLE; 
// uint16_t W25QXX_TYPE = W25Q64;

// // FreeRTOS Mutex for thread safety (原代码中的 rt_mutex_t 替换)
// osMutexId_t w25qxxMutexHandle;

// // *******************************************************************
// // HAL 替换：SPI 读写一个字节 (替换 W25qxxSPI_ReadWriteByte)
// // *******************************************************************
// static uint8_t W25qxxSPI_ReadWriteByte(uint8_t data) {
//     uint8_t recv;
//     // 使用 HAL_SPI_TransmitReceive 替换 rt_spi_transfer
//     if (HAL_SPI_TransmitReceive(&W25QXX_SPI_HANDLE, &data, &recv, 1, 1000) != HAL_OK) {
//         // Error handling
//     }
//     return recv;
// }

// // *******************************************************************
// // 内部操作函数 (简化替换)
// // *******************************************************************

// static void W25QXX_Wait_Busy(void) {
//     uint8_t status;
//     do {
//         W25QXX_CS_L();
//         W25qxxSPI_ReadWriteByte(0x05); // Read Status Reg
//         status = W25qxxSPI_ReadWriteByte(0xFF);
//         W25QXX_CS_H();
//         osDelay(1); // FreeRTOS delay
//     } while (status & 0x01); // BUSY 标志位
// }

// static void W25QXX_Write_Enable(void) {
//     W25QXX_CS_L();
//     W25qxxSPI_ReadWriteByte(0x06); // Write Enable
//     W25QXX_CS_H();
// }

// static void W25QXX_Erase_Sector(uint32_t Dst_Addr) {
//     W25QXX_Write_Enable();
//     W25QXX_Wait_Busy();
//     W25QXX_CS_L();
//     W25qxxSPI_ReadWriteByte(0x20); // Sector Erase
//     W25qxxSPI_ReadWriteByte((uint8_t)(Dst_Addr >> 16));
//     W25qxxSPI_ReadWriteByte((uint8_t)(Dst_Addr >> 8));
//     W25qxxSPI_ReadWriteByte((uint8_t)Dst_Addr);
//     W25QXX_CS_H();
//     W25QXX_Wait_Busy();
// }

// // *******************************************************************
// // 外部接口实现 (使用 Mutex 保护)
// // *******************************************************************

// void W25QXX_Init(void) {
//     // 确保 CS 引脚设置为高电平
//     W25QXX_CS_H();
//     w25qxxMutexHandle = osMutexNew(NULL); // 创建 Mutex
//     // ... 可以添加 W25QXX_ReadID 等初始化检查 ...
// }

// void W25QXX_Read(void *pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead) {
//     if (osMutexAcquire(w25qxxMutexHandle, osWaitForever) != osOK) return;

//     W25QXX_CS_L();
//     W25qxxSPI_ReadWriteByte(0x03); // Read Data
//     W25qxxSPI_ReadWriteByte((uint8_t)(ReadAddr >> 16));
//     W25qxxSPI_ReadWriteByte((uint8_t)(ReadAddr >> 8));
//     W25qxxSPI_ReadWriteByte((uint8_t)ReadAddr);

//     // 使用 HAL_SPI_Receive 替换 rt_spi_transfer_setup
//     HAL_SPI_Receive(&W25QXX_SPI_HANDLE, pBuffer, NumByteToRead, 5000);
//     W25QXX_CS_H();

//     osMutexRelease(w25qxxMutexHandle);
// }

// void W25QXX_Write(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite) {
//     uint16_t pageremain;
//     uint32_t sector_addr = WriteAddr;

//     if (osMutexAcquire(w25qxxMutexHandle, osWaitForever) != osOK) return;

//     // 写入前先擦除所在的扇区 (简化处理，直接擦除 4K 扇区)
//     W25QXX_Erase_Sector(sector_addr);
//     W25QXX_Wait_Busy(); // 确保擦除完成

//     // ... (Page Program 逻辑，此处省略详细字节写入循环，与原驱动类似)
//     // 核心是：W25QXX_Write_Enable() -> 发送 PageProgram 指令 -> HAL_SPI_Transmit -> W25QXX_Wait_Busy()

//     // 简化：直接写入
//     W25QXX_Write_Enable();
//     W25QXX_CS_L();
//     W25qxxSPI_ReadWriteByte(0x02); // Page Program
//     W25qxxSPI_ReadWriteByte((uint8_t)(WriteAddr >> 16));
//     W25qxxSPI_ReadWriteByte((uint8_t)(WriteAddr >> 8));
//     W25qxxSPI_ReadWriteByte((uint8_t)WriteAddr);
    
//     HAL_SPI_Transmit(&W25QXX_SPI_HANDLE, pBuffer, NumByteToWrite, 5000);
//     W25QXX_CS_H();
//     W25QXX_Wait_Busy();

//     osMutexRelease(w25qxxMutexHandle);
// }


