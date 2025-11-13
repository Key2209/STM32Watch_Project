// /*********************************************************************
// * 文件：FingerprintSetDLG.c
// * 说明：指纹设置对话框（应用层）的逻辑处理
// * 注释：包含指纹注册、验证的业务流程和 FreeRTOS 任务封装
// **********************************************************************
// */

// #include "FingerprintSetDLG.h"
// #include "wk2xxx.h" // 引入 WK2114 驱动
// #include "finger.h" // 引入指纹模块驱动
// #include <rtthread.h>
// #include <rtdevice.h>

// // 全局结构体，保存指纹信息（ID、模式等）
// struct _Fingerprint{
//   unsigned char id; // 当前操作的指纹 ID
//   unsigned char mode; // 当前指纹操作模式
// }Fingerprint;

// // 指纹处理任务的状态机变量
// static unsigned char FingerprintMode = 0; // 0: 闲置, 1: 注册流程

// // 声明外部按键检测函数
// extern unsigned char key_short_press(void);

// /**
//  * @brief 注册指纹业务流程函数
//  * * @param id 目标注册 ID
//  * @return unsigned char 1: 成功, 0: 失败
//  */
// unsigned char finger_enroll(unsigned char id)
// {
//     unsigned char ret = 0;
//     enrollNum = 0; // 清除注册计数
    
//     // 1. 设置注册模式
//     finger_setMode(1);
    
//     // 2. 指示灯显示蓝色呼吸，等待第一次按指纹
//     finger_rgb_ctrl(LED_BREATHE, LED_B, 0x00, 0x00);
//     rt_kprintf("Enroll Start: Please press finger %d times\r\n", ENROLL_NUM);
    
//     // 3. 完整的注册流程在 FingerprintProcessPoll 中通过状态机实现，这里只设置 ID 和模式
//     Fingerprint.id = id;
//     FingerprintMode = 1; // 启动注册状态机
    
//     return 1; // 启动成功，结果在状态机中处理
// }

// /**
//  * @brief 指纹业务逻辑初始化
//  */
// void FingerprintProcessInit(void)
// {
//     // 1. 设置 WK2114 子串口 UT3 的数据接收回调函数为指纹模块的数据解析函数
//     Wk2114_SlaveRecv_Set(3, finger_uartHandle);
    
//     // 2. 设置指纹模块的底层发送函数为 WK2114 子串口 UT3 的发送函数
//     set_fingerSend(Wk2114_Uart3SendByte); // 注意：Wk2114_Uart3SendByte 是自定义宏或 wk2xxx.c 中的函数别名
    
//     // 3. 禁用其他可能影响指纹的模块
//     // ec20_close(); 
// }

// /**
//  * @brief 指纹业务逻辑循环轮询函数（状态机实现）
//  */
// void FingerprintProcessPoll(void)
// {
//     int ret = 0;
//     // ... GUI 相关的按键和对话框刷新逻辑省略 ...

//     // 指纹模块处理逻辑
//     if (FingerprintMode == 1) // 注册模式
//     {
//       if (enrollNum < ENROLL_NUM) // 尚未完成所有次采集
//       {
//         if (finger_detect() == 1) // 检测到手指按下
//         {
//           rt_kprintf("finger_detect ok\r\n");
          
//           // 1. 采集指纹图像到 CharBuffer1 (GET_IMAGE 0x0020)
//           ret = finger_get_image();
//           rt_kprintf("finger_get_image: %d\r\n", ret);
          
//           if (ret == E_SUCCESS) 
//           {
//             // 2. 生成指纹特征到 CharBuffer1/2 (GENERATE 0x0060)
//             ret = finger_generate(enrollNum + 1); // 第一次存 CharBuffer1, 第二次存 CharBuffer2
//             rt_kprintf("finger_generate: %d\r\n", ret);
            
//             if (ret == E_SUCCESS)
//             {
//               // 采集成功，计数器加 1
//               enrollNum++;
//               // 指示灯快速闪烁绿色
//               finger_rgb_ctrl(LED_F_BLINK, LED_G, 0x00, 0x00);
//               rt_kprintf("Please release finger. Times: %d\r\n", enrollNum);
              
//               // 阻塞等待手指抬起
//               while (finger_detect() == 1) { 
//                 rt_thread_mdelay(500);
//               }
              
//               // 如果采集次数未满，继续呼吸灯，等待下一次采集
//               if (enrollNum < ENROLL_NUM) {
//                 finger_rgb_ctrl(LED_BREATHE, LED_B, 0x00, 0x00);
//               }
//             } else {
//               // 生成特征失败，指示灯快速闪烁红色，并等待手指抬起
//               finger_rgb_ctrl(LED_F_BLINK, LED_R, 0x00, 0x00);
//               while (finger_detect() == 1) {
//                 rt_thread_mdelay(500);
//               }
//               // 重新开始蓝色呼吸灯等待采集
//               finger_rgb_ctrl(LED_BREATHE, LED_B, 0x00, 0x00);
//             }
//           }
//         }
//       }
//       else // 采集次数已满 ENROLL_NUM
//       {
//         // 3. 融合指纹模板 (MERGE 0x0061)
//         ret = finger_merge(1, ENROLL_NUM - 1); // 融合 CharBuffer1/2 的模板到 CharBuffer1
//         rt_kprintf("finger_merge: %d\r\n", ret);
        
//         if (ret == E_SUCCESS) 
//         {
//           // 4. 存储模板到指定 ID (STORE_CHAR 0x0042)
//           ret = finger_store_char(1, Fingerprint.id); 
//           rt_kprintf("finger_store_char: %d\r\n", ret);
          
//           // 注册完成，指示灯快速闪烁绿色
//           finger_rgb_ctrl(LED_F_BLINK, LED_G, 0x00, 0x00);
//         } else {
//           // 融合失败或存储失败，指示灯快速闪烁红色
//           finger_rgb_ctrl(LED_F_BLINK, LED_R, 0x00, 0x00);
//         }
        
//         // 5. 流程结束，退出注册模式
//         // 持续等待手指抬起
//         while (finger_detect() == 1) {
//           rt_thread_mdelay(500);
//         }
//         FingerprintMode = 0; // 退出状态机
//         // finger_rgb_ctrl(LED_OFF, LED_ALL, 0x00, 0x00); // 最终关闭灯
//       }
//     }
// }

// /**
//  * @brief 指纹处理任务的入口函数
//  * * @param parameter 
//  */
// void fingerprint_thread_entry(void *parameter)
// {
//   (void)parameter;
  
//   FingerprintProcessInit(); // 初始化业务逻辑（连接 WK2114）
//   finger_init();            // 初始化指纹驱动（创建锁和事件）
  
//   while (1)
//   {  
//     rt_thread_mdelay(100); // 周期性执行
//     FingerprintProcessPoll(); // 执行指纹状态机轮询
//   }
// }

// // 指纹处理任务的句柄
// rt_thread_t fingerprint_thread = RT_NULL;
// // 指纹处理任务的事件集（此事件集与 finger.c 中的 finger_event 不同，用于任务间通知）
// rt_event_t fingerprint_event = RT_NULL;

// /**
//  * @brief 指纹处理任务创建和启动
//  * * @return int 
//  */
// int fingerprint_thread_init(void) {
  
//   // 1. 创建事件集
//   fingerprint_event = rt_event_create("finger_event", RT_IPC_FLAG_PRIO);

//   // 2. 创建并启动指纹处理任务
//   fingerprint_thread = rt_thread_create("finger",
//                           fingerprint_thread_entry,
//                           RT_NULL,
//                           4096, // 堆栈大小
//                           25,   // 优先级
//                           20);  // 时间片
//   if (fingerprint_thread != RT_NULL)
//   {
//       rt_thread_startup(fingerprint_thread);
//       return RT_EOK;
//   }
//   return -RT_ERROR;
// }