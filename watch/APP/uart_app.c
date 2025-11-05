#include "uart_app.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h> // for memcpy

// 使用 FreeRTOS 提供的内存管理函数
#include "portable.h"

// --- 全局和静态变量 ---

// QueueHandle_t xRxDataQueue = NULL;
// UART_HandleTypeDef *s_huart = NULL;
//  DMA 环形缓冲区内存
// static uint8_t s_rx_dma_buffer[UART_RX_BUF_SIZE];
//  记录上次处理的位置（环形缓冲区的“Tail”）
// static volatile uint16_t s_last_read_pos = 0;

// --- 发送模块全局变量 ---
// QueueHandle_t xTxQueue = NULL; // 外部声明
// static volatile bool s_is_transmitting = false; // 传输状态标志
// static UartTxMsg_t s_current_tx_msg = {NULL, 0, false}; //
// 正在发送的数据块信息
UartRxStruct Uart1_Rx = {0};
// =======================================================
//                   初始化和启动
// =======================================================

void UartRxStruct_Init(UartRxStruct *uartRxStruct, uint16_t buffer_size,
                       UART_HandleTypeDef *huart_ptr, uint8_t rx_qurue_len) {
  // 1. 创建 FreeRTOS 资源 (Rx 和 Tx 队列)
  uartRxStruct->xRxDataQueue = xQueueCreate(rx_qurue_len, sizeof(UartRxMsg_t));
  if (uartRxStruct->xRxDataQueue == NULL) {
    return;
  }

  uartRxStruct->huart = huart_ptr;
  uartRxStruct->rx_buf_size = buffer_size;
  // 分配 DMA 缓冲区
  uartRxStruct->rx_dma_buffer =
      (uint8_t *)pvPortMalloc(buffer_size * sizeof(uint8_t));
  if (uartRxStruct->rx_dma_buffer == NULL) {
    // 内存分配失败，无需清理其他资源，直接返回
    return;
  }
  uartRxStruct->Read_From_Circular_Buffer = Read_From_Circular_Buffer;
  uartRxStruct->UART_IDLE_IRQHandler = UART_IDLE_IRQHandler;

  // 2. 启动 DMA 环形接收 (Circular Mode)
  // 关键：huart 的 DMA 接收必须在 CubeMX 中配置为 Circular 模式
  if (HAL_UART_Receive_DMA(uartRxStruct->huart, uartRxStruct->rx_dma_buffer,
                           uartRxStruct->rx_buf_size) != HAL_OK) {
    // 错误处理：DMA 启动失败
    // 在这里也应该释放内存和删除队列
    vPortFree(uartRxStruct->rx_dma_buffer);
    uartRxStruct->rx_dma_buffer = NULL;
    vQueueDelete(uartRxStruct->xRxDataQueue);
    uartRxStruct->xRxDataQueue = NULL;
    return;
    return;
  }

  // 3. 启用 UART IDLE 中断 (空闲中断)
  // 不定长数据处理
  __HAL_UART_ENABLE_IT(uartRxStruct->huart, UART_IT_IDLE);
}

// =======================================================
//                   IDLE 中断处理 (ISR 核心)
// =======================================================

/**
 * @brief 在 USARTx_IRQHandler 中被调用的 IDLE 中断处理函数
 * * 原理：IDLE 中断触发时，DMA 暂停，我们计算出 DMA 走过的距离，
 * 将这部分数据视为一帧，通知任务去处理。
 */

void UART_IDLE_IRQHandler(UartRxStruct *uartRxStruct,
                          UART_HandleTypeDef *huart) {
  if (huart != uartRxStruct->huart) {
    return;
  }

  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  uint16_t current_dma_ndtr;
  uint16_t current_write_pos;
  uint16_t received_length;
  UartRxMsg_t msg;

  uint16_t RX_BUFFER_SIZE = uartRxStruct->rx_buf_size;
  volatile uint16_t *s_last_read_pos = &uartRxStruct->s_last_read_pos;
  QueueHandle_t xRxDataQueue = uartRxStruct->xRxDataQueue;
  // 清除 IDLE 标志，**必须先清标志**，才能避免中断再次触发
  __HAL_UART_CLEAR_IDLEFLAG(huart);

  // 1. 获取 DMA 状态
  // NDTR (Number of Data to Transfer)：DMA 还需要搬运的数据量
  current_dma_ndtr = huart->hdmarx->Instance->NDTR;

  // 2. 计算当前 DMA 的写入位置 (Head)
  // Head = 总长度 - 剩余长度

  current_write_pos = RX_BUFFER_SIZE - current_dma_ndtr;

  // 3. 计算本次接收到的数据长度 (Length = Head - Tail)
  if (current_write_pos >= *s_last_read_pos) {
    // Case A: 未环绕。数据位于 [s_last_read_pos, current_write_pos) 之间。
    received_length = current_write_pos - *s_last_read_pos;
  } else {
    // Case B: 发生环绕。数据位于 [s_last_read_pos, end) 和 [start,
    // current_write_pos) 两段。
    // 由于我们只关心长度和起始位置，这里计算总长度即可。
    received_length = RX_BUFFER_SIZE - *s_last_read_pos + current_write_pos;
  }

  // 4. 发送通知 (只有在收到有效数据时才通知)
  if (received_length > 0) {
    // 消息中传递本次接收到的长度
    msg.length = received_length;
    // 核心计算逻辑：在 ISR 中计算 start_pos
    uint16_t start_pos;
    if (current_write_pos >= received_length) {
      start_pos = current_write_pos - received_length;
    } else {
      // 处理环绕情况
      start_pos = RX_BUFFER_SIZE - (received_length - current_write_pos);
    }
    msg.start_pos = start_pos; // 将起点放入消息

    // 发送到 FreeRTOS 队列
    xQueueSendFromISR(xRxDataQueue, &msg, &xHigherPriorityTaskWoken);

    // 更新尾部位置 (Tail)
    *s_last_read_pos = current_write_pos;
  }

  // 唤醒高优先级任务（如果队列发送唤醒了等待任务）
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// =======================================================
//                    数据读取函数
// =======================================================

/**
 * @brief 从环形缓冲区中读取数据（供应用任务调用）
 * @param buffer 目标缓冲区指针
 * @param length 要读取的数据长度
 * @param start_pos 数据在环形缓冲区中的起始位置
 */
void Read_From_Circular_Buffer(UartRxStruct *uartRxStruct, uint8_t *buffer,
                               uint16_t length, uint16_t start_pos) {
  if (length == 0 || start_pos >= uartRxStruct->rx_buf_size)
    return;

  // 计算从 start_pos 到缓冲区末尾的剩余空间
  uint16_t end_of_buffer_len = uartRxStruct->rx_buf_size - start_pos;

  if (length <= end_of_buffer_len) {
    // Case A: 数据未跨越环形边界
    memcpy(buffer, &uartRxStruct->rx_dma_buffer[start_pos], length);
  } else {
    // Case B: 数据跨越环形边界 (最复杂的情况)

    // 1. 复制从起始位置到缓冲区末尾的部分
    memcpy(buffer, &uartRxStruct->rx_dma_buffer[start_pos], end_of_buffer_len);

    // 2. 复制从缓冲区起始地址开始的剩余部分
    uint16_t remaining_len = length - end_of_buffer_len;
    // 加地址会直接偏移end_of_buffer_len*typeof(buffer)字节
    memcpy(buffer + end_of_buffer_len, &uartRxStruct->rx_dma_buffer[0],
           remaining_len);
  }
}

// =======================================================
//                   发送接口 (Tx)
// =======================================================
// void UartTxStruct_TxCpltCallback(UartTxStruct
// *uartTxStruct,UART_HandleTypeDef *huart); bool UART_DMA_Send(UartTxStruct
// *uartTxStruct, uint8_t *data, uint16_t size, uint32_t timeout_ms, bool
// dynamic_allocator);
static void Handle_Tx_Request_Task(UartTxStruct *uartTxStruct);
UartTxStruct Uart1_Tx = {0}; // 全局 UART1 发送结构体实例

void UartTxStruct_Init(UartTxStruct *Uart_Tx, UART_HandleTypeDef *huart,
                       uint8_t uart_tx_queue_len) {

  // 1. 创建 FreeRTOS 资源

  Uart_Tx->xTxQueue = xQueueCreate(uart_tx_queue_len, sizeof(UartTxMsg_t));
  if (Uart_Tx->xTxQueue == NULL) {
    return;
  }
  // 2. 初始化 UartTxStruct 结构体
  Uart_Tx->huart = huart;
  Uart_Tx->is_busy = false;
  Uart_Tx->current_msg.data = NULL;
  Uart_Tx->current_msg.size = 0;
  Uart_Tx->current_msg.is_dynamic = false;

  Uart_Tx->Send_DMA = UART_DMA_Send;
  Uart_Tx->TxCpltCallback = UartTxStruct_TxCpltCallback;
}

/*
  * @brief 应用层发送接口（非阻塞，将数据放入队列）
  * @param data 待发送数据指针
  * @param size 待发送数据大小
  * @param timeout_ms 等待队列空间的超时时间（ms）
  * @param dynamic_allocator 是否是动态分配的数据（需要 TxCpltCallback 释放）
  * @retval true: 发送请求成功放入队列; false: 队列满
  使用该函数请求发送数据，
  如果是第一次请求(空闲) 调用Handle_Tx_Request_Task();启动发送链条。
  该函数会启动DMA发送，发送完成后会调用HAL_UART_TxCpltCallback回调函数，
  此时队列的数据就会在回调函数中被再次发送
 */
bool UART_DMA_Send(UartTxStruct *uartTxStruct, uint8_t *data, uint16_t size,
                   uint32_t timeout_ms, bool dynamic_allocator) {
  UartTxMsg_t msg = {data, size, dynamic_allocator};
  TickType_t ticks = pdMS_TO_TICKS(timeout_ms);
  QueueHandle_t xTxQueue = uartTxStruct->xTxQueue;
  // 1. 将发送请求放入队列 (非阻塞操作)
  if (xQueueSend(xTxQueue, &msg, ticks) != pdPASS) {
    // 队列满，xQueueSend发送失败则返回false,直接退出函数
    return false;
  }

  // 2. 检查并启动发送链条
  // 仅当 UART 处于 READY 或 ERROR 状态时（即当前没有 DMA
  // 正在进行），才需要手动启动。 注意：HAL_UART_STATE_READY 代表设备空闲。
  UART_HandleTypeDef *s_huart = uartTxStruct->huart;
  if (s_huart->gState == HAL_UART_STATE_READY ||
      s_huart->gState == HAL_UART_STATE_ERROR) {
    // 直接调用启动函数。因为 DMA 状态检查本身是原子性的（读取一个寄存器），
    // 且 DMA 状态转换为 BUSY 发生在 HAL_UART_Transmit_DMA 内部，
    // 极大地降低了与 TxCpltCallback 之间的竞态风险。
    Handle_Tx_Request_Task(uartTxStruct);
  }

  return true; // 请求成功放入队列
}

/**
 * @brief 处理发送请求（非 ISR，启动 DMA）
 * * 目标：启动发送链条的第一块数据
 */
static void Handle_Tx_Request_Task(UartTxStruct *uartTxStruct) {
  UartTxMsg_t next_msg;
  QueueHandle_t xTxQueue = uartTxStruct->xTxQueue;

  // 尝试从队列中获取下一块数据（非阻塞）
  if (xQueueReceive(xTxQueue, &next_msg, 0) == pdPASS) {
    // 保存当前消息信息（供 TxCpltCallback 使用）
    uartTxStruct->current_msg = next_msg;
    // 启动 DMA 发送
    UART_HandleTypeDef *s_huart = uartTxStruct->huart;
    HAL_UART_Transmit_DMA(s_huart, next_msg.data, next_msg.size);
  }
}

// =======================================================
//                DMA 发送完成回调 (ISR 核心 - Tx)
// =======================================================

/**
 * @brief HAL 库发送完成回调函数 (由 DMA 中断触发)
 * * 目标：释放内存，接力启动下一块数据
 */
void UartTxStruct_TxCpltCallback(UartTxStruct *uartTxStruct,
                                 UART_HandleTypeDef *huart) {
  if (huart->Instance != uartTxStruct->huart->Instance) {
    return; // 不是我们关心的 UART 实例
  }

  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  UartTxMsg_t next_msg;

  UART_HandleTypeDef *s_huart = uartTxStruct->huart;
  // 1. 内存管理：释放前一块数据（如果它是动态分配的）
  if (uartTxStruct->current_msg.is_dynamic &&
      uartTxStruct->current_msg.data != NULL) {
    // 在中断中释放内存
    vPortFree(uartTxStruct->current_msg.data);
    uartTxStruct->current_msg.data = NULL;
  }
  // 2. 检查发送队列中是否有下一块数据
  if (xQueueReceiveFromISR(uartTxStruct->xTxQueue, &next_msg,
                           &xHigherPriorityTaskWoken) == pdPASS) {
    // 3. 启动下一块数据的发送 (接力)
    uartTxStruct->current_msg = next_msg;
    HAL_UART_Transmit_DMA(s_huart, next_msg.data, next_msg.size);
  }
  // 4. 任务调度 检查是否有高优先级任务被唤醒。
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
  Uart1_Tx.TxCpltCallback(&Uart1_Tx, huart);
}
