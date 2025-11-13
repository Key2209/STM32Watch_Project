#include "main.h"

#include "WK2114.h"
#include "cmsis_os2.h"
#include <stdio.h>
#include <string.h>

// // 外部引用通道接收回调函数数组
// extern void (*Wk2114_SlaveRecv[4])(char byte);
// extern volatile uint16_t WK2114_ExInterrupt_Count; 





// #define RX_BUFFER_SIZE 256

// typedef struct {
//     uint8_t buffer[RX_BUFFER_SIZE];
//     volatile uint16_t head;
//     volatile uint16_t tail;
// } RingBuffer_t;


// RingBuffer_t Wk2114_RxBuffer[4];

// // --- 环形缓冲区操作函数声明 ---
// void RingBuffer_Put(uint8_t channel, char data);

// void wk2114_ch1_recv(char byte);
// void wk2114_ch2_recv(char byte);
// void wk2114_ch3_recv(char byte);
// void wk2114_ch4_recv(char byte);

// // 外部引用通道接收回调函数数组
// void (*Wk2114_SlaveRecv[4])(char byte) = {
//     wk2114_ch1_recv, // 通道 1 (索引 0)
//     wk2114_ch2_recv, // 通道 2 (索引 1)
//     wk2114_ch3_recv,            // 通道 3 (如果不需要可以置空)
//     wk2114_ch4_recv             // 通道 4
// };

// /**
//  * @brief 环形缓冲区写入函数 (由 WK2114_SlaveRecv 调用)
//  */
// void RingBuffer_Put(uint8_t channel, char data)
// {
//     // 确保通道号有效 (0-3)
//     if (channel >= 4) return;
    
//     RingBuffer_t *rb = &Wk2114_RxBuffer[channel];
//     uint16_t next_head = (rb->head + 1) % RX_BUFFER_SIZE;

//     // 检查是否溢出 (如果下一个头等于尾，说明满了)
//     if (next_head != rb->tail) {
//         rb->buffer[rb->head] = data;
//         rb->head = next_head;
//     }
// }



// /**
//  * @brief 通道 1 接收回调函数实现
//  */
// void wk2114_ch1_recv(char byte)
// {
//     // 索引为 0
//     RingBuffer_Put(0, byte); 
// }

// /**
//  * @brief 通道 2 接收回调函数实现
//  */
// void wk2114_ch2_recv(char byte)
// {
//     // 索引为 1
//     RingBuffer_Put(1, byte); 
// }

// /**
//  * @brief 通道 3 接收回调函数实现
//  */
// void wk2114_ch3_recv(char byte)
// {
//     // 索引为 2
//     RingBuffer_Put(2, byte); 
// }

// /**
//  * @brief 通道 4 接收回调函数实现
//  */
// void wk2114_ch4_recv(char byte)
// {
//     // 索引为 3
//     RingBuffer_Put(3, byte); 
// }









// /**
//  * @brief FreeRTOS 任务：WK2114 中断处理与数据获取任务 (对应 wk2xxx_thread_entry)
//  */
// void StartWk2114Task(void *argument)
// {
//     uint8_t channel_id;
//     uint8_t gifr_status;
//     char rx_byte;
    
//     // 1. 初始化 WK2114 芯片
//     // if (WK2114_Init_All(115200) == HAL_OK)
//     // {
//     //     printf("WK2114 Init OK. All 4 channels: 115200bps, 8N1.\r\n");
//     // }
//     // else
//     // {
//     //     printf("WK2114 Init FAILED!\r\n");
//     // }

//     for(;;)
//     {
//         // 任务等待中断通知 (osThreadFlagsWait)
//         // ⚠️ 使用 osWaitForever 替换原代码中的 while(1) + rt_thread_mdelay
//         osThreadFlagsWait(0x01, osFlagsWaitAny, osWaitForever);
        
//         // --- 收到中断信号，开始处理 ---
        
//         // 1. 检查中断计数，并进入循环处理中断
//         if (WK2114_ExInterrupt_Count > 0)
//         {
//             // 使用 do-while 循环处理中断源，直到 GIFR 中断标志全部清除 (与原代码逻辑相同)
//             do
//             {
//                 // 读取全局中断状态寄存器 (GIFR)
//                 if (WK2114_Read_Reg(WK2XXX_GPORT, WK2XXX_GIFR, &gifr_status) != HAL_OK) {
//                     gifr_status = 0;
//                 }
                
//                 // 仅判断低 4 位 (UT1INT-UT4INT)
//                 gifr_status &= 0x0F; 
                
//                 // 遍历并处理有中断的通道 (原代码逻辑)
//                 for (channel_id = 1; channel_id <= 4; channel_id++)
//                 {
//                     // 检查对应通道的中断位
//                     if (gifr_status & (1 << (channel_id - 1))) 
//                     {
//                         // 通道有中断，进入 FIFO 读取循环 (对应原代码 while(FSR & RDAT))
//                         uint8_t fsr_value;
//                         // 循环读取 FDAT 寄存器，直到 FSR 寄存器显示 FIFO 为空
//                         while (WK2114_Read_Reg(channel_id, WK2XXX_FSR, &fsr_value) == HAL_OK && (fsr_value & WK2XXX_RDAT))
//                         {
//                             // 从 FDAT 读取数据
//                             if (WK2114_Read_Reg(channel_id, WK2XXX_FDAT, (uint8_t*)&rx_byte) == HAL_OK)
//                             {
//                                 // 调用对应的通道回调函数 (对应 Wk2114_SlaveRecv[i-1](byte))
//                                 if (Wk2114_SlaveRecv[channel_id - 1] != NULL)
//                                 {
//                                     Wk2114_SlaveRecv[channel_id - 1](rx_byte);
//                                 }
//                             }
//                         }
//                     }
//                 }
//             }
//             while ((gifr_status != 0)); // 再次检查 GIFR，直到无有效中断源

//             // 清除外部中断计数
//             HAL_NVIC_DisableIRQ(EXTI_WK2114_IRQn); // ⚠️ 禁用中断，确保操作原子性
//             WK2114_ExInterrupt_Count--;
//             HAL_NVIC_EnableIRQ(EXTI_WK2114_IRQn);
//         }
        
//         // 4. 周期性检查状态和错误复位 (对应 wk2xxx_check_status)
//         // ⚠️ 建议将此检查移出 osWaitForever 循环，通过 osDelay 定时进行。
//         // 或者，在中断处理完后，让任务短暂休眠，确保低优先级任务有机会运行。
//         osDelay(10); // 替换原代码的 rt_thread_mdelay(CYCLE) 
//     }
// }





