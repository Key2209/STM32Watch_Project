#include "uart_app.h"
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
// -----------------------------------------------------------------------------
//                                       UART TX Stream Buffer 配置
// -----------------------------------------------------------------------------
UartTxStruct Uart1_Tx = {0}; // 全局 UART1 发送结构体实例

void UartTxTask(void *pvParameters);
/* ---------------- 初始化 ---------------- */
void UartTxStruct_Init(UartTxStruct *Uart_Tx, UART_HandleTypeDef *huart)
{
  configASSERT(Uart_Tx != NULL && huart != NULL);

    Uart_Tx->huart = huart;


    Uart_Tx->xTxStreamBuffer = xStreamBufferCreateStatic(
        UART_STREAM_SIZE,
        1,
        Uart_Tx->ucStreamBufferStorage,
        &Uart_Tx->xTxStreamBufferStruct);
    configASSERT(Uart_Tx->xTxStreamBuffer != NULL);


    /* 创建 tx task，并把 Uart_Tx 作为参数传入 */
    BaseType_t x = xTaskCreate(
        UartTxTask,
        "UartTx",
        UART_TASK_STACK,
        (void *)Uart_Tx,   // 这里传入指针（非常重要）
        UART_TASK_PRIO,
        &Uart_Tx->xTxDriveTaskHandle); // 保存任务句柄到结构体
    configASSERT(x == pdPASS);

    Uart_Tx->Uart_Send = Uart_Send;
    Uart_Tx->Uart_SendFromISR = Uart_SendFromISR;
    Uart_Tx->UartTxStruct_TxCpltCallback= UartTxStruct_TxCpltCallback;
}

/* ---------------- 生产者 API（用于在任务上下文写入） ---------------- */
BaseType_t Uart_Send(UartTxStruct *Uart_Tx,const uint8_t *data, size_t len)
{
    if (Uart_Tx->xTxStreamBuffer == NULL) return pdFALSE;
    size_t sent = xStreamBufferSend(Uart_Tx->xTxStreamBuffer, (void *)data, (size_t)len, SEND_BLOCK_TICKS);
    return (sent == len) ? pdTRUE : pdFALSE;
}

/* ISR 里写入的 API */
BaseType_t Uart_SendFromISR(UartTxStruct *Uart_Tx,const uint8_t *data, size_t len, BaseType_t *pxHigherPriorityTaskWoken)
{
    if (Uart_Tx->xTxStreamBuffer  == NULL) return pdFALSE;
    size_t sent = xStreamBufferSendFromISR(Uart_Tx->xTxStreamBuffer , (void *)data, (size_t)len, pxHigherPriorityTaskWoken);
    return (sent == len) ? pdTRUE : pdFALSE;
}

/* ---------------- tx task（消费者）实现 ---------------- */
void UartTxTask(void *pvParameters)
{   
 UartTxStruct *uart = (UartTxStruct *)pvParameters;
    configASSERT(uart != NULL);

    for (;;)
    {
        /* 从 StreamBuffer 读取到本地缓冲区（不要使用 StreamBuffer 的底层存储区） */
        size_t xReceived = xStreamBufferReceive(
            uart->xTxStreamBuffer,
            uart->tx_local_buf,
            sizeof(uart->tx_local_buf),
            portMAX_DELAY
        );

        if (xReceived == 0) {
            continue;
        }

        /* D-Cache: 若 MCU 需要，在这里清理缓存 */
        /* SCB_CleanDCache_by_Addr((uint32_t*)uart->tx_local_buf, xReceived); */

        /* 启动 DMA（任务上下文，HAL safe） */
        if (HAL_UART_Transmit_DMA(uart->huart, uart->tx_local_buf, (uint16_t)xReceived) == HAL_OK)
        {
            /* 等待 ISR 通知发送完成 */
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

/* ---------------- HAL DMA 完成回调（ISR） ----------------
   注意：这个回调在 HAL 库中通常由 DMA 中断触发（ISR）
*/
void UartTxStruct_TxCpltCallback(UartTxStruct *uartTxStruct, UART_HandleTypeDef *huart)
{
    if (uartTxStruct == NULL || huart == NULL) return;
    if (huart->Instance != uartTxStruct->huart->Instance) return;

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    /* 用结构体保存的任务句柄通知对应任务 */
    vTaskNotifyGiveFromISR(uartTxStruct->xTxDriveTaskHandle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{

    /* 转发给我们的模块（如果有多个实例，需要判断实例或循环查找） */
    Uart1_Tx.UartTxStruct_TxCpltCallback(&Uart1_Tx, huart);
}

/* ---------------- printf 重定向示例（把 printf 写入 StreamBuffer） ----------------
   适用于 newlib _write 或 retarget 方案。下面是 newlib 的 _write 示例。
*/
int _write(int file, char *ptr, int len)
{
    /* 非阻塞写入 debug stream（避免在 printf 中死锁系统） */
    Uart1_Tx.Uart_Send(&Uart1_Tx,(const uint8_t *)ptr, (size_t)len);
    return len;
}



