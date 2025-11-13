// // wk2114.c
// #include "WK2114.h"

// #include <string.h>
// // 外部引用 CubeMX 生成的句柄 (假设使用 USART2)
// extern UART_HandleTypeDef huart2; 
// osThreadId_t Wk2114TaskHandle;



// /**
//  * @brief WK2114 写寄存器操作 (阻塞)
//  */
// HAL_StatusTypeDef WK2114_Write_Reg(uint8_t channel, uint8_t reg_addr, uint8_t data)
// {
//     uint8_t tx_buf[2];
//     // 组合命令字：CMD = 00 C1C0 A3A2A1A0
//     tx_buf[0] = WK2114_CMD_W_REG(channel, reg_addr);
//     tx_buf[1] = data;
//     // 使用阻塞模式发送命令，超时100ms
//     return HAL_UART_Transmit(&huart2, tx_buf, 2, 100); 
// }

// /**
//  * @brief WK2114 读寄存器操作 (阻塞)
//  */
// HAL_StatusTypeDef WK2114_Read_Reg(uint8_t channel, uint8_t reg_addr, uint8_t *data)
// {
//     uint8_t cmd_byte = WK2114_CMD_R_REG(channel, reg_addr);
    
//     // 1. 发送读命令 (阻塞)
//     if (HAL_UART_Transmit(&huart2, &cmd_byte, 1, 100) != HAL_OK)
//         return HAL_ERROR;
        
//     // 2. 接收返回的数据 (阻塞)
//     // 芯片会返回寄存器值
//     return HAL_UART_Receive(&huart2, data, 1, 100); 
// }

// /**
//  * @brief WK2114 硬件复位 (对应 Wk2114_Rest)
//  */
// void WK2114_Rest(void)
// {
// // 1. 设置为高电平 (确保初始状态为非复位)
//   // 对应原代码：WK2114_REST_H;
//   HAL_GPIO_WritePin(WK2114_REST_GPIO_Port, WK2114_REST_Pin, GPIO_PIN_SET); 

//   // 2. 触发复位 (拉低电平)
//   // 对应原代码：WK2114_REST_L;
//   HAL_GPIO_WritePin(WK2114_REST_GPIO_Port, WK2114_REST_Pin, GPIO_PIN_RESET); 
  
//   // 3. 保持低电平一段时间 (100ms)
//   // 对应原代码：delay_ms(100);
//   osDelay(100); 
  
//   // 4. 释放复位 (拉高电平)
//   // 对应原代码：WK2114_REST_H;
//   HAL_GPIO_WritePin(WK2114_REST_GPIO_Port, WK2114_REST_Pin, GPIO_PIN_SET); 
  
//   // 5. 等待芯片稳定 (20ms)
//   // 对应原代码：delay_ms(20);
//   osDelay(20);
// }

// /**
//  * @brief WK2114 波特率自适应 (对应 Wk2114BaudAdaptive)
//  */
// void WK2114_BaudAdaptive(void)
// {
//   WK2114_Rest();
//   uint8_t data = 0x55;
//   HAL_UART_Transmit(&huart2, &data, 1, 100);
//   osDelay(300);
// }


// /**
//  * @brief WK2114 初始化所有使能的通道（一口气配置）
//  * @param baud_rate: 目标波特率 (例如 115200)
//  */
// HAL_StatusTypeDef WK2114_Init_All(int baud_rate)
// {
//     HAL_StatusTypeDef status = HAL_OK;
//     uint8_t i;
//     uint8_t baud1 = 0x00;
//     uint8_t baud0 = 0x05; // 默认为 115200bps (0x0005)，对应 11.0592MHz 晶振
    
//     // 1. 波特率自适应
//     WK2114_BaudAdaptive();

//     // 2. 全局寄存器配置：一次性使能/复位/中断所有 4 个通道
//     const uint8_t ALL_CH_MASK = 0x0F; 

//     // 使能所有 4 个子串口的时钟 (GENA: 0x0F)
//     status = WK2114_Write_Reg(WK2XXX_GPORT, WK2XXX_GENA, ALL_CH_MASK);
//     if(status != HAL_OK) return status;

//     // 软件复位所有 4 个子串口 (GRST: 0x0F)
//     status = WK2114_Write_Reg(WK2XXX_GPORT, WK2XXX_GRST, ALL_CH_MASK);
//     if(status != HAL_OK) return status;

//     // 使能所有 4 个子串口的总中断 (GIER: 0x0F)
//     status = WK2114_Write_Reg(WK2XXX_GPORT, WK2XXX_GIER, ALL_CH_MASK);
//     if(status != HAL_OK) return status;
    
//     // 3. 确定波特率值 (从 Wk2114SetBaud 逻辑中获取 115200bps 对应的 0x0005)
//     if (baud_rate == 115200) {
//         baud1 = 0x00;
//         baud0 = 0x05;
//     } 
//     // ⚠️ 可以添加其他波特率的 switch-case 逻辑，参考 Wk2114SetBaud

//     // 4. 循环配置每个通道的细节 (从 CH1 到 CH4)
//     for (i = 1; i <= 4; i++) 
//     {
//         // a. 配置 LCR (数据格式): 8N1 -> 写入 0x00
//         status = WK2114_Write_Reg(i, WK2XXX_LCR, 0x00);
//         if(status != HAL_OK) return status;

//         // b. 配置 FCR (FIFO): 启用 FIFO, 复位 FIFO, 设置固定触点 -> 写入 0xFF
//         // ⚠️ 原代码使用 0xFF，实际应根据手册配置，这里沿用原代码值
//         status = WK2114_Write_Reg(i, WK2XXX_FCR, 0XFF); 
//         if(status != HAL_OK) return status;

//         // c. 设置波特率 (Page 1)
//         status = WK2114_Write_Reg(i, WK2XXX_SPAGE, WK2XXX_SPAGE1); // 切换到 page1
//         if(status != HAL_OK) return status;
        
//         status = WK2114_Write_Reg(i, WK2XXX_BAUD1, baud1);
//         status = WK2114_Write_Reg(i, WK2XXX_BAUD0, baud0);
//         if(status != HAL_OK) return status;

//         // 设置 FIFO 触发点 (对应原代码中的 RFTL=0x40, TFTL=0x10)
//         status = WK2114_Write_Reg(i, WK2XXX_RFTL, 0x40); // 接收触点 1 字节
//         status = WK2114_Write_Reg(i, WK2XXX_TFTL, 0x10); // 发送触点 256 字节
//         if(status != HAL_OK) return status;
        
//         status = WK2114_Write_Reg(i, WK2XXX_SPAGE, WK2XXX_SPAGE0); // 切换回 page0
//         if(status != HAL_OK) return status;

//         // d. SIER (使能接收触点中断和超时中断) (RFTRIG_IEN | RXOUT_IEN)
//         // ⚠️ 原代码使用了 RMW，这里简化为直接写入 
//         status = WK2114_Write_Reg(i, WK2XXX_SIER, WK2XXX_RFTRIG_IEN | WK2XXX_RXOUT_IEN); 
//         if(status != HAL_OK) return status;

//         // e. SCR (使能 RX/TX) -> 0x03
//         status = WK2114_Write_Reg(i, WK2XXX_SCR, WK2XXX_TXEN | WK2XXX_RXEN);
//         if(status != HAL_OK) return status;
//     }

//     return HAL_OK;
// }


// // 外部中断标志计数器 (对应 WK2114_ExInterrupt_Count)
// volatile uint16_t WK2114_ExInterrupt_Count = 0; 


// /**
//  * @brief 外部中断处理函数 (EXTI Handler)
//  * ⚠️ 放置在 stm32f4xx_it.c 中的对应 EXTIx_IRQHandler 函数内
//  */
// void WK2114_IRQ_Handler(void)
// {
//     // 增加中断计数
//     WK2114_ExInterrupt_Count++;
//     // 通知 FreeRTOS 任务：中断发生，请立即处理
//     if (Wk2114TaskHandle != NULL) {
//         osThreadFlagsSet(Wk2114TaskHandle, 0x01);
//     }
// }



// /**
//  * @brief WK2114 通道数据发送函数 (阻塞/同步模式)
//  * WK2114 内部的 UART TX FIFO 会自动将这些数据发送出去。
//  * 
//  * * @param channel: 通道号 (1-4)
//  * @param data: 待发送数据指针
//  * @param len: 待发送数据长度
//  * @return HAL_StatusTypeDef: 发送状态
//  */
// HAL_StatusTypeDef WK2114_Send_Data(uint8_t channel, const uint8_t *data, uint16_t len)
// {
//     // 检查通道有效性
//     if (channel < 1 || channel > 4) {
//         return HAL_ERROR;
//     }
    
//     // 检查指针和长度
//     if (data == NULL || len == 0) {
//         return HAL_OK;
//     }

//     HAL_StatusTypeDef status = HAL_OK;
//     uint16_t i;
    
//     // 遍历要发送的每个字节
//     for (i = 0; i < len; i++)
//     {
//         // 核心操作：将数据写入 FDAT 寄存器
//         // ⚠️ 在这里，我们直接使用封装好的 WK2114_Write_Reg 函数
//         status = WK2114_Write_Reg(channel, WK2XXX_FDAT, data[i]);
        
//         if (status != HAL_OK)
//         {
//             // 如果写操作失败，立即返回
//             return status; 
//         }

//         // 简易的 FIFO 检查 (可选): 
//         // 可以在这里读取 FSR 寄存器，判断 TX FIFO 是否已满，如果满则等待。
//         // 但由于 FCR 设置的 TX FIFO 触发点通常很大 (如 256 字节)，
//         // 且底层 SPI (huart2) 发送很快，通常不必额外等待。
//     }

//     return HAL_OK;
// }