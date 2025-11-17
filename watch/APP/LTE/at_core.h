// #ifndef AT_CORE_H
// #define AT_CORE_H

// #include "main.h" 
// #include "cmsis_os.h" 

// // 假设 UART 句柄为 huart4
// #define EC20_UART_HANDLE        huart4             
// // DMA 接收缓冲区大小，必须足够大以容纳最大的 AT 响应和 URCs
// #define EC20_DMA_RX_SIZE        1024                

// typedef enum { AT_OK, AT_ERROR, AT_TIMEOUT, AT_CONNECT_OK } at_status_t;

// // DMA 接收缓冲区 (外部变量)
// extern uint8_t g_at_dma_rx_buffer[EC20_DMA_RX_SIZE];

// // FreeRTOS 同步对象 (外部变量)
// extern osSemaphoreId_t g_at_response_sem;
// extern osMutexId_t g_at_recv_mutex; // 确保声明

// // 外部函数声明
// void AT_Core_Init(void);
// // IDLE 中断回调函数
// void AT_UART_IDLE_Callback(void);
// at_status_t AT_SendCmd(const char *cmd, const char *resp, uint32_t timeout);
// int AT_ReadData(char *precv, uint16_t len);
// void UART_Direct_Transmit(uint8_t *pdata, uint16_t len);
// #endif