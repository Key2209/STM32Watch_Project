#include "Uart_Task.h"
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "task.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h> // for printf
#include <string.h>
#include "uart_app.h"
// 声明 Read_From_Circular_Buffer 函数（通常在 .h 文件中）


uint8_t * message1 = (uint8_t *)"Hello from UartTask!\r\n";

// --- 任务：接收数据处理器 ---
void UART_Debug_RxProcessTask(void *pvParameters){

  UartRxMsg_t received_msg;

  // 假设 huart1 已经初始化
    extern UART_HandleTypeDef huart1;
    UartRxStruct_Init(&Uart1_Rx,512,&huart1,10);// 初始化 UART1 DMA 接收

  for (;;) {
    // 1. 阻塞等待 IDLE 中断发送的通知 (portMAX_DELAY 表示永久等待)
    if (xQueueReceive(Uart1_Rx.xRxDataQueue, &received_msg, portMAX_DELAY) == pdPASS) {
      // ** 接收到有效帧的通知！ **
      uint16_t data_len = received_msg.length;
      uint16_t start_pos = received_msg.start_pos;

      // 3. 准备应用层处理缓冲区
      
        // 【使用 RTOS 堆内存分配】
            uint8_t *local_processing_buffer = (uint8_t *)pvPortMalloc(data_len + 1);
            
            if (local_processing_buffer == NULL) {
                // 内存分配失败，直接跳过本次处理
                continue; 
            }
      // 4. 从环形缓冲区中读取数据到本地缓冲区 (由 Read_From_Circular_Buffer
      // 完成复杂的跨界复制)
      Uart1_Rx.Read_From_Circular_Buffer(&Uart1_Rx,local_processing_buffer, data_len, start_pos);
      local_processing_buffer[data_len] = '\0'; // 添加终止符

      // 5. ** 执行耗时的业务逻辑！ **
    UART_DMA_Send(&Uart1_Tx, (uint8_t *)local_processing_buffer, strlen((char*)local_processing_buffer), 100,
                  false);

        //Uart1_Tx.Send_DMA(&Uart1_Tx, (uint8_t *)message1, strlen((char *)message1), 100,false);
    }
  }

  osDelay(pdMS_TO_TICKS(1));
}



