#ifndef UART_APP_H
#define UART_APP_H

#ifdef __cplusplus
extern "C" {
#endif


#include "FreeRTOS.h"
#include "queue.h"
#include "stm32f4xx_hal.h"

#include "stdbool.h"


// --- 配置宏 ---
#define UART_RX_BUF_SIZE    512       // DMA 接收缓冲区总大小
#define UART_RX_QUEUE_LEN   10        // 接收消息队列长度

#define UART_TX_QUEUE_LEN   20        // 发送请求队列长度









// --- 接收消息结构体：ISR 与任务间的信物 ---
typedef struct {
    uint16_t length;            // 本次接收到的实际数据长度
    uint16_t start_pos;        // 数据在环形缓冲区中的起始位置
} UartRxMsg_t;
struct UartRxStruct;
typedef struct UartRxStruct {
    UART_HandleTypeDef *huart;//串口句柄
    uint16_t rx_buf_size;//接收缓冲区大小
    uint8_t *rx_dma_buffer;//接收缓冲区指针
    volatile uint16_t s_last_read_pos;//上次读取位置
    QueueHandle_t xRxDataQueue;
    UartRxMsg_t current_msg;
    void (*UART_IDLE_IRQHandler)(struct UartRxStruct* uartRxStruct,UART_HandleTypeDef *huart);
    void (*Read_From_Circular_Buffer)(struct UartRxStruct* uartRxStruct,uint8_t *buffer, uint16_t length,
                                      uint16_t start_pos);

} UartRxStruct;


void UART_IDLE_IRQHandler(UartRxStruct* uartRxStruct,UART_HandleTypeDef *huart);
void UartRxStruct_Init(UartRxStruct *uartRxStruct,uint16_t buffer_size,UART_HandleTypeDef *huart_ptr,uint8_t rx_qurue_len);
void Read_From_Circular_Buffer(UartRxStruct* uartRxStruct,uint8_t *buffer, uint16_t length,
                                      uint16_t start_pos);
// --- 发送消息结构体：任务 -> ISR 的信物 ---
typedef struct {
    uint8_t *data;      // 指向待发送数据的指针
    uint16_t size;      // 待发送数据的大小
    bool    is_dynamic; // 标志：数据是否为动态分配（需要 TxCpltCallback 释放）
} UartTxMsg_t;
struct UartTxStruct;
typedef struct UartTxStruct {
    UART_HandleTypeDef *huart;
    QueueHandle_t xTxQueue;
    UartTxMsg_t current_msg;
    bool is_busy;
    bool (*Send_DMA)(struct UartTxStruct *uartTxStruct, uint8_t *data, uint16_t size, uint32_t timeout_ms, bool dynamic_allocator);
    void (*TxCpltCallback)(struct UartTxStruct *uartTxStruct,UART_HandleTypeDef *huart);

} UartTxStruct;
void UartTxStruct_TxCpltCallback(UartTxStruct *uartTxStruct,UART_HandleTypeDef *huart);
bool UART_DMA_Send(UartTxStruct *uartTxStruct, uint8_t *data, uint16_t size, uint32_t timeout_ms, bool dynamic_allocator);
void UartTxStruct_Init(UartTxStruct *Uart_Tx,UART_HandleTypeDef *huart,uint8_t uart_tx_queue_len);







// --- 全局实例声明 ---
extern UartTxStruct Uart1_Tx;
extern UartRxStruct Uart1_Rx;


#ifdef __cplusplus
}
#endif







#endif // UART_APP_H
