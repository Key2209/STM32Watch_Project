// #include <stdio.h>
// #include <string.h>
// #include <stdlib.h>
// #include "main.h" 
// #include "cmsis_os.h" 
// #include "at_core.h" 
// #include "w25qxx_hal.h" 
// #include "ec20_zhiyun.h" 

// // --- 外部依赖声明 (必须确保这些句柄在 CubeMX 生成的文件中被声明) ---
// extern UART_HandleTypeDef huart4;
// extern SPI_HandleTypeDef hspi1;
// extern void W25QXX_Read(void *pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead);
// extern void W25QXX_Write(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);

// // --- 全局变量 ---
// t_zhiyun_config zhiyun_cfg = {
//     ZHIYUN_SAVE_FLAG, ZHIYUN_ID, ZHIYUN_KEY
// };
// static int zhiyun_sock = -1;    // Socket ID，-1 表示未连接
// static int zhiyun_auth = -1;    // 认证状态，-1 未认证，1 已认证
// static int zhiyun_heart = 0;    // 心跳计数器
// osMutexId_t ec20MutexHandle;    // 应用层操作互斥锁
// char ec20_imei[20] = "LTE:IMEI_PLACEHOLDER"; // 存储模块 IMEI

// // --- 弱符号回调函数定义（用户可在其他地方重写） ---
// __WEAK void zxbee_onrecv_fun(char *pdata, uint16_t len)
// {
//     printf("[EC20] RECV CTRL: %s\r\n", pdata);
//     // 默认行为：打印控制命令。TODO: 在此处添加您的设备控制逻辑
// }

// // ----------------------------------------------------------------------
// //                      底层硬件和配置函数
// // ----------------------------------------------------------------------

// /**
//  * @brief 硬件复位 EC20 模块
//  */
// static void ec20_reset(void) {
//     HAL_GPIO_WritePin(EC20_RESET_PORT, EC20_RESET_PIN, GPIO_PIN_SET);
//     osDelay(pdMS_TO_TICKS(100));
//     HAL_GPIO_WritePin(EC20_RESET_PORT, EC20_RESET_PIN, GPIO_PIN_RESET);
//     osDelay(pdMS_TO_TICKS(1000)); // 保持低电平 1 秒，模块复位
//     HAL_GPIO_WritePin(EC20_RESET_PORT, EC20_RESET_PIN, GPIO_PIN_SET);
//     osDelay(pdMS_TO_TICKS(5000)); // 等待模块重启
// }

// /**
//  * @brief 初始化和读取智云配置
//  */
// static void ec20_cfg_init(void)
// {
//     W25QXX_Read(&zhiyun_cfg, SPIFLASH_ADDR_ZHIYUN, sizeof(t_zhiyun_config));

//     if (zhiyun_cfg.flag != ZHIYUN_SAVE_FLAG)
//     {
//         printf("[EC20] Config not found. Writing default...\r\n");
//         // 写入默认配置
//         zhiyun_cfg.flag = ZHIYUN_SAVE_FLAG;
//         W25QXX_Write((uint8_t*) &zhiyun_cfg, SPIFLASH_ADDR_ZHIYUN, sizeof(t_zhiyun_config));
//     }
//     printf("[EC20] Loaded AID: %s\r\n", zhiyun_cfg.zhiyun_id);
// }

// /**
//  * @brief 辅助函数：发送 AT 命令并打印
//  */
// static at_status_t ec20_at_command(const char *cmd, const char *resp, uint32_t timeout)
// {
//     at_status_t status;
//     char rx_buffer[1024] = {0}; // 准备大缓冲区来接收详细信息

//     printf("AT> %s\r\n", cmd);
//     status = AT_SendCmd(cmd, resp, timeout);
    
//     // 额外读取数据并打印，用于调试和查看详细响应
//     if (status != AT_TIMEOUT) {
//         // 读取模块响应的详细数据，即使只有 "OK" 也会被读取
//         AT_ReadData(rx_buffer, sizeof(rx_buffer) - 1); 
//         if (strlen(rx_buffer) > 0) {
//             printf("[AT_RESP] Received: %s\r\n", rx_buffer);
//         }
//     }
    
//     return status;
// }

// // ----------------------------------------------------------------------
// //                      EC20 AT Socket 核心函数
// // ----------------------------------------------------------------------

// /**
//  * @brief 断开 TCP 连接并清除状态
//  */
// static void ec20_disconnect(void) {
//     char cmd[32];
//     if (zhiyun_sock >= 0) {
//         // AT+QICLOSE=connID
//         sprintf(cmd, "AT+QICLOSE=%d", zhiyun_sock);
//         ec20_at_command(cmd, "OK", 2000);
//         zhiyun_sock = -1;
//         zhiyun_auth = -1;
//     }
// }

// /**
//  * @brief 连接网络（PDP 激活）和建立 TCP 连接（AT+QIOPEN）
//  */
// static int ec20_connect_zhiyun(void)
// {
//     char cmd[150];
//     at_status_t status;

//     // 1. 检查模块是否正常
//     if (ec20_at_command("AT", "OK", 1000) != AT_OK) {
//         printf("[EC20] Module not responding.\r\n");
//         return -1;
//     }
    
//     // 2. 激活 PDP 上下文 (AT+QIACT=1)
//     if (ec20_at_command("AT+QIACT=1", "OK", 15000) != AT_OK) {
//         printf("[EC20] PDP Activation failed.\r\n");
//         return -1;
//     }
    
//     // 3. 关闭可能存在的旧连接 (使用 EC20_AT_CLIENT_ID)
//     sprintf(cmd, "AT+QICLOSE=%d", EC20_AT_CLIENT_ID);
//     ec20_at_command(cmd, "OK", 2000); 

//     // 4. 建立 TCP 连接 (AT+QIOPEN)
//     // AT+QIOPEN=1,connID,"TCP","IP",port
//     sprintf(cmd, "AT+QIOPEN=1,%d,\"TCP\",\"%s\",%d", EC20_AT_CLIENT_ID, ZHIYUN_IP_STR, ZHIYUN_PORT); 
    
//     // 等待连接成功 URC 响应（+QIOPEN: 0,0）
//     // 此处 AT_SendCmd 内部会等待 "+QIOPEN: 0,0"
//     status = ec20_at_command(cmd, "+QIOPEN: 0,0", 30000); 
    
//     if (status == AT_OK) {
//         zhiyun_sock = EC20_AT_CLIENT_ID;
//         printf("[EC20] TCP Connect Success! ID: %d\r\n", zhiyun_sock);
//         return zhiyun_sock;
//     }
    
//     printf("[EC20] TCP Connect failed. Status: %d.\r\n", status);
//     return -1;
// }

// /**
//  * @brief 发送原始数据 (使用 AT+QISEND)
//  */
// static int ec20_send(char *pdata)
// {
//     if (zhiyun_sock < 0) return -1;
    
//     char cmd[64];
//     int len = strlen(pdata);
    
//     // 1. 发送 QISEND 命令，进入数据输入模式
//     sprintf(cmd, "AT+QISEND=%d,%d", zhiyun_sock, len);
    
//     // 等待 '>' 提示符 (进入数据模式)
//     if (AT_SendCmd(cmd, ">", 2000) != AT_OK) {
//         printf("[EC20] QISEND failed or no '>' prompt.\r\n");
//         return -1;
//     }
    
//     // 2. 实际发送数据 (调用 at_core.c 中的 UART_Direct_Transmit)
//     // 注意：这里不再是 ec20_at_command，必须使用 UART_Direct_Transmit
//     UART_Direct_Transmit((uint8_t*)pdata, len);
//     printf("[EC20] SENT RAW DATA: %s\r\n", pdata);

//     // 3. 等待发送成功响应 (SEND OK)
//     // 发送数据后必须发送 0x1A (Ctrl+Z) 结束，但 UART_Direct_Transmit 仅发送了数据。
//     // 在这里我们假设数据发送后模块会自动处理，或者 'AT_SendCmd("", "SEND OK", 5000)' 
//     // 实际上是等待模块响应。对于 QISEND，数据本身就是命令的一部分，不应再发送\r\n。
    
//     // 修正：QISEND 在数据发送完成后，模块会回复 SEND OK
//     if (AT_SendCmd("", "SEND OK", 5000) == AT_OK) {
//         return len;
//     } else {
//         printf("[EC20] Data send failed (no SEND OK).\r\n");
//         return -1;
//     }
// }

// // ----------------------------------------------------------------------
// //                      智云应用逻辑 (Demo 协议数据)
// // ----------------------------------------------------------------------

// /**
//  * @brief 智云认证 (Demo 中第一次发送的数据)
//  */
// static int ec20_zhiyun_auth(void)
// {
//     if (osMutexAcquire(ec20MutexHandle, osWaitForever) != osOK) return -1;

//     int recv_len;
//     char auth_data[256], recv_data[512] = {0};

//     // 构造和 Demo 一模一样的认证 JSON 数据
//     sprintf(auth_data,
//             "{\"method\":\"authenticate\",\"uid\":\"%s\",\"key\":\"%s\",\"addr\":\"%s\",\"version\":\"0.1.0\",\"autodb\":true}",
//             zhiyun_cfg.zhiyun_id, zhiyun_cfg.zhiyun_key, ec20_imei);
    
//     if (ec20_send(auth_data) <= 0)
//     {
//         osMutexRelease(ec20MutexHandle);
//         return -1;
//     }
    
//     // 等待服务器响应
//     osDelay(pdMS_TO_TICKS(1000)); 
//     // 【重要】这里读取的是服务器通过 URC (+QIURC: "recv",...) 推送给 AT 核心的数据
//     recv_len = AT_ReadData(recv_data, 511); 

//     if (recv_len <= 0)
//     {
//         printf("[EC20] Auth: No server response received.\r\n");
//         osMutexRelease(ec20MutexHandle);
//         return -1;
//     }
    
//     // 简单解析认证响应
//     if (strstr(recv_data, "\"method\":\"authenticate_rsp\"") != NULL && strstr(recv_data, "\"status\":\"ok\"") != NULL)
//     {
//         printf("[EC20] Zhiyun auth successed! Resp: %s\r\n", recv_data);
//         osMutexRelease(ec20MutexHandle);
//         return 1;
//     }
    
//     printf("[EC20] Zhiyun auth failed! Resp: %s\r\n", recv_data);
//     osMutexRelease(ec20MutexHandle);
//     return -1;
// }

// /**
//  * @brief 智云心跳包
//  */
// static int ec20_zhiyun_heart(void)
// {
//     // 使用短时间等待 Mutex，防止长时间阻塞心跳
//     if (osMutexAcquire(ec20MutexHandle, pdMS_TO_TICKS(100)) != osOK) return -1;
    
//     char heart_data[128];
//     // 构造心跳包
//     sprintf(heart_data, "{\"method\":\"echo\",\"timestamp\":%lu,\"seq\":%lu}", osKernelGetTickCount(), 0UL);
    
//     if (ec20_send(heart_data) > 0) {
//         osMutexRelease(ec20MutexHandle);
//         return 1;
//     }
    
//     osMutexRelease(ec20MutexHandle);
//     return -1;
// }

// /**
//  * @brief 简化 JSON 提取控制数据
//  */
// static int ec20_zhiyun_ctrlget(char *praw, char *pdata)
// {
//     // 查找 "method":"control" 和 "data":...
//     char *pfind_method = strstr(praw, "\"method\":\"control\"");
//     if (pfind_method != NULL)
//     {
//         char *pfind_data = strstr(praw, "\"data\":");
//         if (pfind_data != NULL)
//         {
//             char *start = pfind_data + 7; // 跳过 "data":"
//             char *end = strrchr(start, '}'); 
//             if (end && end > start) {
//                 int len = end - start + 1;
//                 strncpy(pdata, start, len);
//                 pdata[len] = '\0';
//                 return len;
//             }
//         }
//     }
//     return -1;
// }

// // ----------------------------------------------------------------------
// //                      FreeRTOS 主任务入口
// // ----------------------------------------------------------------------
// at_status_t EC20_CheckNetworkRegistration(void) {
//     at_status_t status;
//     char rx_buffer[128] = {0};

//     // 1. 检查信号质量 (AT+CSQ)
//     status = AT_SendCmd("AT+CSQ", "+CSQ:", 500);
//     if (status != AT_OK) {
//         printf("[EC20_VERIFY] Signal check failed or timeout.\n");
//         return status;
//     }
//     // 读取 AT+CSQ 响应，用于清除 AT 核心缓冲区中的 CSQ 响应，为 CREG 腾出空间
//     AT_ReadData(rx_buffer, sizeof(rx_buffer));

//     // 2. 检查网络注册状态 (AT+CREG?)
//     status = AT_SendCmd("AT+CREG?", "+CREG:", 1000);
    
//     if (status == AT_OK) {
//         AT_ReadData(rx_buffer, sizeof(rx_buffer));
        
//         // CREG: <n>,<stat>  <stat> = 1 (注册, home) 或 5 (注册, roaming)
//         if (strstr(rx_buffer, "+CREG: 0,1") || strstr(rx_buffer, "+CREG: 0,5")) {
//             printf("[EC20_VERIFY] Network registered OK.\n");
//             return AT_OK; // 网络注册成功
//         } else {
//             printf("[EC20_VERIFY] Network not registered status: %s\n", rx_buffer);
//             return AT_ERROR; // 注册进行中或失败
//         }
//     } else {
//         printf("[EC20_VERIFY] CREG command failed or timeout.\n");
//         return status;
//     }
// }

// void EC20_MainTask(void *argument)
// {
//     uint8_t try_times = 0;
    
//     // 1. 初始化配置和硬件
//     ec20_cfg_init();
//     ec20_reset(); 
    
//     printf("[EC20] Main Task Started.\r\n");

//     while (1)
//     {
//         // 2. 检查网络连接并连接到智云服务器
//         while (zhiyun_sock < 0)
//         {
//             // 【验证点 1：网络连接状态】
//             if (EC20_CheckNetworkRegistration() != AT_OK) 
//             {
//                 // 如果网络未注册成功，则等待一段时间并跳过 Socket 连接，继续检查网络状态
//                 osDelay(pdMS_TO_TICKS(5000)); // 延长等待时间
//                 continue; 
//             }

//             // 网络就绪，尝试连接 Socket
//             ec20_disconnect(); // 确保旧连接已关闭
//             zhiyun_sock = ec20_connect_zhiyun();
//             if (zhiyun_sock < 0)
//             {
//                 // 连接失败，等待一段时间重试
//                 osDelay(pdMS_TO_TICKS(5000)); 
//             }
//         }

//         // 3. 认证
//         try_times = 0;
//         zhiyun_auth = -1; // 每次重新连接后都要重新认证
//         while (try_times < 3)
//         {
//             if (ec20_zhiyun_auth() > 0)
//             {
//                 zhiyun_auth = 1;
//                 break;
//             }
//             try_times++;
//             osDelay(pdMS_TO_TICKS(5000));
//         }
        
//         if (zhiyun_auth < 0)
//         {
//             printf("[EC20] Auth failed 3 times! Reconnecting...\r\n");
//             ec20_disconnect();
//             continue; // 跳到外层循环重新连接
//         }

//         // 4. 主运行循环 (心跳和数据接收)
//         int recv_len;
//         int heart_try = 0;
//         char recv_data[512] = {0}, ctrl_data[256] = {0};
        
//         while (zhiyun_auth > 0)
//         {
//             // 接收数据
//             recv_len = AT_ReadData(recv_data, 511);
            
//             if (recv_len > 0)
//             {
//                 // 处理控制命令
//                 int ctrl_len = ec20_zhiyun_ctrlget(recv_data, ctrl_data);
//                 if (ctrl_len > 0)
//                 {
//                     zxbee_onrecv_fun(ctrl_data, ctrl_len);
//                 }
//                 // 清空 recv_data，防止下次读取时误判
//                 memset(recv_data, 0, sizeof(recv_data));
//             }
            
//             // 心跳计时（每 10 秒发送一次）
//             if (zhiyun_heart >= 100) // 100 * 100ms = 10s
//             {
//                 zhiyun_heart = 0;
                
//                 // ec20_zhiyun_heart() 检查发送是否成功
//                 if (ec20_zhiyun_heart() < 0) 
//                 {
//                     heart_try++;
//                 }
//                 else
//                 {
//                     heart_try = 0;
//                 }
                
//                 if (heart_try >= 3)
//                 {
//                     printf("[EC20] Heartbeat failed 3 times. Reconnecting...\r\n");
//                     zhiyun_auth = -1; // 退出内部循环
//                     ec20_disconnect();
//                     break;
//                 }
//             }
            
//             zhiyun_heart++;
//             osDelay(pdMS_TO_TICKS(100)); // 100ms 延时，保持心跳轮询
//         }
//     }
// }
// /**
//  * @brief 外部模块调用：发送传感器数据 (Demo 数据)
//  * @param pdata 传感器数据的 JSON 片段，例如 "{\"temp\":25.5}"
//  */
// int ec20_zhiyun_send_sensor_data(char *pdata)
// {
//     if (zhiyun_auth < 0) return -1;
//     if (osMutexAcquire(ec20MutexHandle, pdMS_TO_TICKS(1000)) != osOK)
//         return -1;
        
//     char sensor_data[256];
    
//     // 构造 sensor JSON (Demo 格式)
//     sprintf(sensor_data, "{\"method\":\"sensor\", \"addr\":\"%s\", \"data\":%s}", ec20_imei, pdata);
    
//     if (ec20_send(sensor_data) <= 0)
//     {
//         osMutexRelease(ec20MutexHandle);
//         return -1;
//     }

//     osMutexRelease(ec20MutexHandle);
//     return 1;
// }

// /**
//  * @brief 移植初始化函数 (在 osKernelStart() 前调用)
//  */
// void EC20_Port_Init(void)
// {
//     // 创建应用层互斥锁
//     ec20MutexHandle = osMutexNew(NULL);
// }

// /**
//  * @brief 系统总初始化函数
//  */
// void Initialize_EC20_System(void)
// {
//     // 【重要】句柄映射 - 必须在头文件(at_core.h/w25qxx_hal.h)中声明 extern，并在 main.c 中定义
//     // 在这里进行赋值，确保 AT 核心和 Flash 驱动使用的是正确的 HAL 句柄
//     extern UART_HandleTypeDef EC20_UART_HANDLE;
//     extern UART_HandleTypeDef huart4;
//     EC20_UART_HANDLE = huart4;
    
//     extern SPI_HandleTypeDef W25QXX_SPI_HANDLE;
//     extern SPI_HandleTypeDef hspi1;
//     W25QXX_SPI_HANDLE = hspi1;

//     // --- 初始化 ---
//     // W25QXX_Init() 应在 w25qxx_hal.c 中负责创建 w25qxxMutexHandle
//     W25QXX_Init(); 
//     AT_Core_Init(); 
//     EC20_Port_Init(); 
// }
