#ifndef UART_APP_H
#define UART_APP_H

#include "cmsis_os.h"
#ifdef __cplusplus
extern "C" {
#endif


#include "FreeRTOS.h"          // 必须最先包含
#include "task.h"
#include "queue.h"
#include "stream_buffer.h"

#include "stm32f4xx_hal.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>


// --- 配置宏 ---
#define UART_RX_BUF_SIZE 512 // DMA 接收缓冲区总大小
#define UART_RX_QUEUE_LEN 10 // 接收消息队列长度

#define UART_TX_QUEUE_LEN 20 // 发送请求队列长度

// --- 接收消息结构体：ISR 与任务间的信物 ---
typedef struct {
  uint16_t length;    // 本次接收到的实际数据长度
  uint16_t start_pos; // 数据在环形缓冲区中的起始位置
} UartRxMsg_t;
struct UartRxStruct;
typedef struct UartRxStruct {
  UART_HandleTypeDef *huart;         // 串口句柄
  uint16_t rx_buf_size;              // 接收缓冲区大小
  uint8_t *rx_dma_buffer;            // 接收缓冲区指针
  volatile uint16_t s_last_read_pos; // 上次读取位置
  QueueHandle_t xRxDataQueue;
  UartRxMsg_t current_msg;
  void (*UART_IDLE_IRQHandler)(struct UartRxStruct *uartRxStruct,
                               UART_HandleTypeDef *huart);
  void (*Read_From_Circular_Buffer)(struct UartRxStruct *uartRxStruct,
                                    uint8_t *buffer, uint16_t length,
                                    uint16_t start_pos);

} UartRxStruct;

void UART_IDLE_IRQHandler(UartRxStruct *uartRxStruct,
                          UART_HandleTypeDef *huart);
void UartRxStruct_Init(UartRxStruct *uartRxStruct, uint16_t buffer_size,
                       UART_HandleTypeDef *huart_ptr, uint8_t rx_qurue_len);
void Read_From_Circular_Buffer(UartRxStruct *uartRxStruct, uint8_t *buffer,
                               uint16_t length, uint16_t start_pos);














/* --- 配置区域 --- */
#define UART_STREAM_SIZE    1024U       // stream buffer 总容量
#define UART_LOCAL_BUF_SZ   256U        // tx task 本地缓冲区大小
#define UART_TASK_STACK     512
#define UART_TASK_PRIO      (tskIDLE_PRIORITY + 2)
#define SEND_BLOCK_TICKS    pdMS_TO_TICKS(0) // 生产者在缓冲区满时最多阻塞时间（0 表示不阻塞）

struct UartTxStruct;
typedef struct UartTxStruct {
  UART_HandleTypeDef *huart;

  TaskHandle_t xTxDriveTaskHandle; // 驱动任务句柄，用于通知

  // **Stream Buffer 资源**
  StreamBufferHandle_t xTxStreamBuffer;
  StaticStreamBuffer_t xTxStreamBufferStruct;
  uint8_t ucStreamBufferStorage[UART_STREAM_SIZE];
    /* 本地发送缓冲区 —— **必须独立于 StreamBuffer 存储区** */
  uint8_t tx_local_buf[UART_LOCAL_BUF_SZ];

  void (*UartTxStruct_TxCpltCallback)(struct UartTxStruct *uartTxStruct,
                         UART_HandleTypeDef *huart);

  BaseType_t (*Uart_Send)(struct UartTxStruct *uartTxStruct,const uint8_t *data, size_t len);
  BaseType_t (*Uart_SendFromISR)(struct UartTxStruct *uartTxStruct,const uint8_t *data, size_t len, BaseType_t *pxHigherPriorityTaskWoken);                      
} UartTxStruct;
void UartTxStruct_Init(UartTxStruct *Uart_Tx, UART_HandleTypeDef *huart);
BaseType_t Uart_Send(UartTxStruct *Uart_Tx,const uint8_t *data, size_t len);
BaseType_t Uart_SendFromISR(UartTxStruct *Uart_Tx,const uint8_t *data, size_t len, BaseType_t *pxHigherPriorityTaskWoken);
void UartTxStruct_TxCpltCallback(UartTxStruct *uartTxStruct, UART_HandleTypeDef *huart);



// --- 全局实例声明 ---
extern UartTxStruct Uart1_Tx;
extern UartRxStruct Uart1_Rx;

#ifdef __cplusplus
}
#endif

#endif // UART_APP_H
