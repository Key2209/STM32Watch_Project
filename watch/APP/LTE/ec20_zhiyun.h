// #ifndef EC20_ZHIYUN_H
// #define EC20_ZHIYUN_H

// #include <stdint.h>
// #include "cmsis_os.h" // FreeRTOS/CMSIS-RTOS V2 API

// // --- 配置宏 (从原 ec20.h 移植) ---
// #define SPIFLASH_ADDR_ZHIYUN    0x1000
// #define ZHIYUN_SAVE_FLAG        0x1A2B3C4D

// #define ZHIYUN_IP_STR           "47.99.214.175" // 智云服务器 IP
// #define ZHIYUN_PORT             28082           // 智云服务器端口
// #define ZHIYUN_ID               "814814773034"  // 默认设备 ID
// // 默认设备 Key (长密钥)
// #define ZHIYUN_KEY              "CQcNDAQNAAADAgMEQxNCWV1dU15HEgkUCQYMCAgNAAEECR8UGl9VVVQWDRcRXEZHURNJXQ"

// #define EC20_AT_CLIENT_ID       0               // 模块连接 ID
// #define EC20_RESET_PORT         GPIOD           // 假设 RESET 引脚为 PD6
// #define EC20_RESET_PIN          GPIO_PIN_6

// // --- 结构体 ---
// typedef struct
// {
//     uint32_t flag;              // 配置是否已存储的标志位
//     char zhiyun_id[40];         // 智云 ID
//     char zhiyun_key[120];       // 智云 Key
// } t_zhiyun_config;

// // --- 外部接口声明 ---

// // 任务入口函数：由 FreeRTOS 调度器调用
// void EC20_MainTask(void *argument);

// // 初始化函数：在 osKernelStart() 前调用
// void EC20_Port_Init(void);

// // 外部数据发送接口
// int ec20_zhiyun_send_sensor_data(char *pdata);

// // 控制命令接收回调函数（用户需实现此函数，处理来自服务器的控制指令）
// void zxbee_onrecv_fun(char *pdata, uint16_t len);
// void Initialize_EC20_System(void);
// #endif // EC20_ZHIYUN_H