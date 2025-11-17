// #include "at_core.h"
// #include <string.h>
// #include <stdio.h>

// // 【重要】引入 FreeRTOS 原生头文件，用于 ISR 安全的 API
// #include "FreeRTOS.h" 
// #include "task.h"       // 包含 portYIELD_FROM_ISR
// #include "semphr.h"     // 包含 xSemaphoreGiveFromISR

// // 假设 UART 句柄为 huart4，请确保在 main.c 中已定义
// extern UART_HandleTypeDef EC20_UART_HANDLE; 

// // DMA 接收缓冲区 (固定大小，用于 DMA 环形接收)
// uint8_t g_at_dma_rx_buffer[EC20_DMA_RX_SIZE];

// // 全局接收缓冲区（用于缓存和解析）
// static char g_at_recv_buffer[EC20_DMA_RX_SIZE * 2];
// static size_t g_at_recv_len = 0;

// // FreeRTOS 同步对象（定义）
// osSemaphoreId_t g_at_response_sem = NULL; 
// osMutexId_t g_at_recv_mutex = NULL;       

// // AT 命令期望响应和状态
// static const char *g_expected_resp = NULL;
// static at_status_t g_at_cmd_status = AT_TIMEOUT;

// // *******************************************************************
// // 核心函数：AT 核心初始化
// // *******************************************************************
// void AT_Core_Init(void) {
//     // 1. 创建同步对象
//     const osMutexAttr_t mutex_attributes = { .name = "at_recv_mutex" };
//     g_at_recv_mutex = osMutexNew(&mutex_attributes);

//     const osSemaphoreAttr_t semaphore_attributes = { .name = "at_resp_sem" };
//     g_at_response_sem = osSemaphoreNew(1, 0, &semaphore_attributes); 

//     if (g_at_recv_mutex == NULL || g_at_response_sem == NULL) {
//         printf("[AT_CORE] Init failed: Semaphore/Mutex creation error!\r\n");
//     }

//     // 2. 启用 UART DMA 循环接收 (Circular Mode)
//     memset(g_at_dma_rx_buffer, 0, EC20_DMA_RX_SIZE);
    
//     // 开启 UART DMA 接收，设置为循环模式
//     HAL_UART_Receive_DMA(&EC20_UART_HANDLE, g_at_dma_rx_buffer, EC20_DMA_RX_SIZE);
    
//     // 3. 启用 UART IDLE 中断
//     __HAL_UART_ENABLE_IT(&EC20_UART_HANDLE, UART_IT_IDLE);

//     printf("[AT_CORE] DMA+IDLE initialized.\r\n");
// }

// // *******************************************************************
// // 核心函数：IDLE 中断回调 (在 STM32 的 IRQHandler 中调用)
// // *******************************************************************
// void AT_UART_IDLE_Callback(void) {
//     uint32_t current_pos;
//     uint32_t data_len;
    
//     // FreeRTOS ISR 标志：用于指示是否需要进行任务切换
//     BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
//     // 1. 禁用 IDLE 中断和 DMA 接收 (防止数据竞争)
//     __HAL_UART_DISABLE_IT(&EC20_UART_HANDLE, UART_IT_IDLE);
    
//     // 2. 计算接收到的数据长度
//     // 【修正点 1A】使用 HAL 提供的宏安全地获取 DMA 计数器
//     current_pos = __HAL_DMA_GET_COUNTER(EC20_UART_HANDLE.hdmarx);
//     // 已传输的字节数 = 总大小 - 剩余未传输的字节数
//     data_len = EC20_DMA_RX_SIZE - current_pos;

//     // 3. 将数据从 DMA 缓冲区复制到全局解析缓冲区 (g_at_recv_buffer)
//     if (data_len > 0) {
//         if ((g_at_recv_len + data_len) < sizeof(g_at_recv_buffer)) {
//             memcpy(&g_at_recv_buffer[g_at_recv_len], g_at_dma_rx_buffer, data_len);
//             g_at_recv_len += data_len;
//             g_at_recv_buffer[g_at_recv_len] = '\0'; // 确保字符串结束
//         }
//         // 清理 DMA 缓冲区中已处理的数据区域 (可选，但推荐)
//         memset(g_at_dma_rx_buffer, 0, data_len);
//     }
    
//     // 4. 重新开始 DMA 接收
//     // 清除 UART 错误标志 (防止 HAL_UART_Receive_DMA 内部报错)
//     __HAL_UART_CLEAR_FLAG(&EC20_UART_HANDLE, UART_FLAG_ORE | UART_FLAG_NE | UART_FLAG_FE | UART_FLAG_PE);

//     // 【修正点 1B】最安全的方式是 Abort 再重新启动 DMA 接收
//     HAL_UART_AbortReceive(EC20_UART_HANDLE.hdmarx);
//     HAL_UART_Receive_DMA(&EC20_UART_HANDLE, g_at_dma_rx_buffer, EC20_DMA_RX_SIZE);

//     // 5. 重新开启 IDLE 中断
//     __HAL_UART_ENABLE_IT(&EC20_UART_HANDLE, UART_IT_IDLE);

//     // 6. 检查和通知：解析和唤醒任务 (AT_SendCmd)
//     if (g_expected_resp) {
//         // 收到期望响应
//         if (strstr(g_at_recv_buffer, g_expected_resp)) {
//             g_at_cmd_status = AT_OK;
//             // 【修复点】使用 ISR 安全的信号量释放函数
//             xSemaphoreGiveFromISR(g_at_response_sem, &xHigherPriorityTaskWoken);
//         } 
//         // 检查通用错误响应（如 ERROR, +CME ERROR）
//         else if (strstr(g_at_recv_buffer, "ERROR") || strstr(g_at_recv_buffer, "+CME ERROR")) {
//             g_at_cmd_status = AT_ERROR;
//             xSemaphoreGiveFromISR(g_at_response_sem, &xHigherPriorityTaskWoken);
//         }
//     }

//     // 7. 中断退出检查：如果释放信号量唤醒了更高优先级任务，则请求任务切换
//     portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
// }


// // *******************************************************************
// // 任务函数：发送 AT 命令并等待响应
// // *******************************************************************
// at_status_t AT_SendCmd(const char *cmd, const char *resp, uint32_t timeout) {
//     at_status_t status = AT_TIMEOUT;
    
//     g_expected_resp = resp;
//     g_at_cmd_status = AT_TIMEOUT;

//     // 清空接收缓冲区（使用 Mutex 保护）
//     if (osMutexAcquire(g_at_recv_mutex, osWaitForever) == osOK) { 
//         g_at_recv_len = 0;
//         memset(g_at_recv_buffer, 0, sizeof(g_at_recv_buffer));
//         osMutexRelease(g_at_recv_mutex); // 立即释放 Mutex
//     }

//     // 发送命令
//     if (strlen(cmd) > 0) {
//         char full_cmd[1024];
//         sprintf(full_cmd, "%s\r\n", cmd);
//         HAL_UART_Transmit(&EC20_UART_HANDLE, (uint8_t*)full_cmd, strlen(full_cmd), 1000);
//     }

//     // 阻塞等待 IDLE 中断释放信号量 (等待模块响应)
//     if (osSemaphoreAcquire(g_at_response_sem, pdMS_TO_TICKS(timeout)) == osOK) {
//         status = g_at_cmd_status;
//     }
    
//     g_expected_resp = NULL; // 清除期望响应
    
//     // 额外的逻辑：如果连接成功，检查 URC 码
//     if (strstr(g_at_recv_buffer, "+QIOPEN: 0,0")) {
//         return AT_CONNECT_OK;
//     }
    
//     return status;
// }

// // *******************************************************************
// // 任务函数：直接发送原始数据 (例如 TCP/UDP 数据)
// // *******************************************************************
// void UART_Direct_Transmit(uint8_t *pdata, uint16_t len) {
//     // 【修复点】解决 ec20_zhiyun.c 中未声明函数的问题
//     HAL_UART_Transmit(&EC20_UART_HANDLE, pdata, len, 5000);
// }

// // *******************************************************************
// // 任务函数：读取模块主动推送的数据（URCs 或 TCP 数据）
// // *******************************************************************
// int AT_ReadData(char *precv, uint16_t len) {
//     int read_len = 0;
    
//     // 任务中尝试非阻塞获取 Mutex
//     if (osMutexAcquire(g_at_recv_mutex, 0) == osOK) { 
//         read_len = (g_at_recv_len < len) ? g_at_recv_len : len;
        
//         memcpy(precv, g_at_recv_buffer, read_len);
        
//         // 清空缓冲区
//         g_at_recv_len = 0;
//         memset(g_at_recv_buffer, 0, sizeof(g_at_recv_buffer));
        
//         osMutexRelease(g_at_recv_mutex);
//     }
//     return read_len;
// }