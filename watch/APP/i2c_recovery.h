#ifndef I2C_RECOVERY_H
#define I2C_RECOVERY_H

#include "main.h"
#include "cmsis_os2.h"
#include <stdbool.h>
#include "stdio.h"
#include "bsp_dwt.h"
#ifdef __cplusplus
extern "C" {
#endif

// 外部 I2C 句柄（CubeMX 生成）
extern I2C_HandleTypeDef hi2c1;

// 外部 RTOS 同步对象（你的主工程已定义）
extern osMutexId_t i2cBusMutexHandle;
extern osSemaphoreId_t i2cDmaCpltSemHandle;

// 错误/恢复标志（由 ISR 设置，由任务处理）
extern volatile bool g_i2c_recovery_error;
extern volatile bool g_i2c_recover_request;

// 初始化（在系统启动时调用一次）
void I2C_Recovery_Init(void);

// 在任务上下文中执行恢复（DeInit + bus pulse + Init）
void I2C_Try_Recovery_Task_Context(void);

// 直接在任务/错误处理里调用的解锁函数（会切换为 GPIO 驱动）
void I2C_Bus_Clock_Pulse_And_Stop(void);

// 便利：在检测到 BUSY 时调用此函数（非阻塞）
bool I2C_Check_And_Attempt_Recovery(void);

#ifdef __cplusplus
}
#endif

#endif // I2C_RECOVERY_H
