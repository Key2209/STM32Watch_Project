// /*********************************************************************************************
// * 文件：finger.h
// * 作者：Zhouchj 2020.07.23
// * 说明：指纹模块驱动头文件
// * 修改：
// * 注释：
// *********************************************************************************************/
// #ifndef __FINGER_H_
// #define __FINGER_H_
// #include "stm32f4xx.h"

// // 协议版本宏定义，影响 RGB 灯控制的数值
// // 0x03 对应 ID_SYNONU2_ID811A1_FPC1021_Inner(80fp) V1.4
// #define FINGER_VERSION      0x03

// // 指纹通信协议帧头和地址
// #define PREFIX              0xAA55  // 帧头
// #define SID                 0x00    // 发送地址
// #define DID                 0x00    // 目标地址

// // 注册采集次数
// #define ENROLL_NUM          0x03    // 注册采集指纹次数 (2次或3次)

// // 模块状态标志位定义 (触摸、注册成功/失败、验证成功/失败)
// #define TOUCH               0x20    // 触摸标志
// #define ENROLL_T            0x40    // 注册成功
// #define ENROLL_F            0x80    // 注册失败
// #define IDENTIFY_T          0x40    // 指纹验证成功
// #define IDENTIFY_F          0x80    // 指纹验证失败

// // 验证相关参数
// #define SEARCH_NUM          0x05    // 指纹验证模板数量
// #define IDENTIFY_INTERVAL   0x03    // 验证间隔时长 单位：S

// // RGB LED 灯控制宏定义 (数值依赖 FINGER_VERSION)
// extern unsigned int LED_G, LED_R, LED_B;              // 颜色定义
// extern unsigned int LED_BREATHE, LED_F_BLINK, LED_S_BLINK, LED_ON, LED_OFF; // 动作定义
// #define LED_ALL             0x07    // 全部灯
// // ... 其他 LED 定义在 finger.c 中

// // **************************** 命令字 CMD ****************************
// #define DEV_INFO            0x0001    // 获取设备信息
// #define GET_IMAGE           0x0020    // 采集指纹图像
// #define REMOVE_ALL          0x0041    // 删除所有指纹模板
// #define STORE_CHAR          0x0042    // 存储模板
// #define LOAD_CHAR           0x0043    // 读取模板
// #define GET_PARAM           0x0044    // 获取系统参数
// #define SET_PARAM           0x0045    // 设置系统参数
// #define GET_STATUS          0x0046    // 获取注册状态
// #define GENERATE            0X0060    // 生成模板 (将图像转换为特征)
// #define MERGE               0x0061    // 融合模块 (多次采集的特征融合为一个模板)
// #define SEARCH              0x0063    // 1:N 验证 (搜索全部)
// #define VERIFY              0x0064    // 1:1 验证 (指定 ID 比对)
// #define RGB_CTRL            0x0091    // RGB 灯控制

// // **************************** 响应码 RET ****************************
// #define E_SUCCESS           0x00    // 成功
// #define E_FAIL              0x01    // 失败
// #define E_VERIFY            0x10    // 1:1 验证不通过
// #define E_IDENTIFY          0x11    // 1:N 搜索不通过
// #define E_TMPL_EMPTY        0x12    // 指定模板为空
// #define E_MERGE_FAIL        0x1A    // 模板融合失败

// // **************************** 驱动外部接口函数 ****************************

// // 回调函数设置
// void set_fingerSend(void (*fun)(char)); // 设置底层发送单个字节的回调函数
// int finger_input(char ch);              // 指纹数据输入函数（由串口接收回调调用）
// void finger_uartHandle(char ch);        // 指纹数据帧解析（在新协议中可能替代 finger_input）

// // 驱动初始化与状态获取
// void finger_init(void);                                             // 指纹驱动初始化
// void finger_setMode(unsigned char mode);                            // 设置指纹工作模式
// unsigned char finger_getMode(void);                                 // 获取指纹工作模式
// void finger_get_param(unsigned char type);                          // 获取系统参数
// int finger_dev_info(char *buf, int len);                            // 获取设备信息
// int finger_detect(void);                                            // 检测是否有手指触摸

// // 指纹操作命令
// int finger_get_status(unsigned char id);                             // 获取指定 ID 的模板状态
// int finger_get_image(void);                                         // 采集指纹图像到 CharBuffer1
// int finger_rgb_ctrl(unsigned char type, unsigned char start, unsigned char end, unsigned char num); // RGB灯控制
// int finger_generate(unsigned char num);                             // 生成指纹特征（到 CharBuffer1/2）
// int finger_merge(unsigned char num, unsigned char count);           // 融合 CharBuffer1/2 中的模板
// int finger_store_char(unsigned char num, unsigned char id);         // 存储模板到指定 ID

// // 业务逻辑函数
// int finger_search(unsigned char num, unsigned char startId, unsigned char endId); // 搜索指纹
// unsigned char finger_enroll(unsigned char id);                       // 完整的指纹注册流程

// #endif







#ifndef __FINGER_H_
#define __FINGER_H_

#include "main.h"
#include <string.h>
#include <stdio.h>      // for printf
#include "FreeRTOS.h"   // FreeRTOS 基础定义
#include "semphr.h"     // 信号量（SemaphoreHandle_t, xSemaphoreTake 等）
#include "event_groups.h" // 事件组（EventGroupHandle_t, xEventGroupSetBits 等）


//VERSION：0x02对应ID_SEODU2_ID812_BYD160_Inne，0x03对应ID_SYNONU2_ID811A1_FPC1021_Inner(80fp) V1.4
#define FINGER_VERSION      0x03

#define PREFIX              0xAA55
#define SID                 0x00
#define DID                 0x00
        
#define ENROLL_NUM          0x03                                // 注册采集指纹次数 2/3
#define TOUCH               0x20                                // 触摸
#define ENROLL_T            0x40                                // 注册成功
#define ENROLL_F            0x80                                // 注册失败
#define IDENTIFY_T          0x40                                // 指纹验证成功
#define IDENTIFY_F          0x80                                // 指纹验证失败

#define SEARCH_NUM          0x05                                // 指纹验证模板数量
#define IDENTIFY_INTERVAL   0x03                                // 验证间隔时长 单位：S
                      
// LED                      



#define LED_ALL             0x07                                // 全部灯
//#define LED_OFF             0x00                                // 关闭
//#define LED_ON              0x01 
extern unsigned int LED_G, LED_R, LED_B;
extern unsigned int LED_BREATHE,LED_F_BLINK,LED_S_BLINK, LED_ON,LED_OFF;

#if 0
#define OLD_F 0
#if OLD_F
#define LED_G               0x01                                // 绿灯
#define LED_R               0x02                                // 红灯
#define LED_B               0x04                                // 蓝灯

#define LED_ON              0x01                                // 打开
#define LED_BREATHE         0x01                                // 呼吸
#define LED_F_BLINK         0x02                                // 快闪烁
#define LED_LONG_ON         0x03                                // 常开
#define LED_LONG_OFF        0x04                                // 常闭
#define LED_GRADUALLY_ON    0x05                                // 渐开
#define LED_GRADUALLY_OFF   0x06                                // 渐关
#define LED_S_BLINK         0x07                                // 慢闪烁
#else
#define LED_B               0x01                                // 蓝灯
#define LED_R               0x02                                // 红灯
#define LED_G               0x04                                // 绿灯

#define LED_ON              0x01                                // 打开
#define LED_BREATHE         0x02                                // 呼吸

#define LED_S_BLINK         0x03                                // 慢闪烁
#define LED_F_BLINK         0x04                                // 快闪烁

#define LED_LONG_ON         0x01                                // 常开
#define LED_LONG_OFF        0x00                                // 常闭
#define LED_GRADUALLY_ON    1                                // 渐开
#define LED_GRADUALLY_OFF   0                                // 渐关

#endif
#endif

// CMD              
#define GET_PARAM           0x0003                              // 获取参数
#define GET_IMAGE           0x0020                              // 获取图像
#define FINGER_DETECT       0x0021                              // 检测指纹
#define SLED_CTRL           0x0024                              // 背光控制
#define STORE_CHAR          0x0040                              // 注册到编号库
#define GET_STATUS          0x0046                              // 获取注册状态
#define GENERATE            0X0060                              // 生成模板
#define MERGE               0x0061                              // 融合模块
#define SEARCH              0x0063                              // 1:N 验证
#define VERIFY              0x0064                              // 1:1 验证
  
// RET  
#define E_SUCCESS           0x00
#define E_FAIL              0x01
#define E_VERIFY            0x10
#define E_IDENTIFY          0x11
#define E_TMPL_EMPTY        0x12
#define E_MERGE_FAIL        0x1A

/* 你要求的：通过 WK2114 的第 3 个 UART 通道转发 */
#define FINGER_WK2114_PORT  3

/* 对外 API —— 与原 finger.h 保持一致 */
void set_fingerSend(void (*fun)(char));
void finger_input(char ch);
void finger_uartHandle(char ch);
void finger_init(void);
void finger_setMode(unsigned char mode);
unsigned char finger_getMode(void);
void finger_get_param(unsigned char type);

int finger_dev_info(char *buf, int len);
int finger_detect(void);
int finger_get_status(unsigned char id);
int finger_get_image(void);
int finger_rgb_ctrl(unsigned char type, unsigned char start, unsigned char end, unsigned char num);
int finger_generate(unsigned char num);
int finger_merge(unsigned char num, unsigned char count);
int finger_store_char(unsigned char num, unsigned char id);
int finger_search(unsigned char num, unsigned char startId, unsigned char endId);

unsigned char finger_enroll(unsigned char id);
unsigned char finger_identify(void);

unsigned char finger_enrollLoop(unsigned char fingerId);
unsigned char finger_identifyLoop(unsigned short time);

/* 保留原来内部串口发送（如果你有更高阶的 Wk2114_Send，请在项目中替换） */
void Wk2114_Send_Port3(const uint8_t *buf, uint16_t len);

#endif /* __FINGER_HAL_H_ */




