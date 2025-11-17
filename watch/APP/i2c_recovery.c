#include "i2c_recovery.h"
#include "bsp_dwt.h"
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "string.h"
#include "stdio.h"
#include "stdbool.h"
// 修改下面的宏以匹配你的 I2C 引脚
// For STM32F407 I2C1 typical 
#define I2C_SCL_PORT GPIOB
#define I2C_SCL_PIN  GPIO_PIN_8
#define I2C_SDA_PORT GPIOB
#define I2C_SDA_PIN  GPIO_PIN_9

// 外部对象 (由你的 main.c / CubeMX 生成)
extern I2C_HandleTypeDef hi2c1;
extern osMutexId_t i2cBusMutexHandle;
extern osSemaphoreId_t i2cDmaCpltSemHandle;
extern void Uart1_Tx_Send(const char *s); // 你可以替换成你现有的串口发送函数

volatile bool g_i2c_recovery_error = false;         // set in ISR minimal
volatile bool g_i2c_recover_request = false; // set when we want recovery (task should handle)

void I2C_Recovery_Init(void) {
    // nothing special for now; just ensure flags clear
    g_i2c_recovery_error = false;
    g_i2c_recover_request = false;
}

/**
 * @brief 用 GPIO 生成 9 个 SCL 时钟并产生 STOP，释放被拉低的 SDA。
 *        必须在任务上下文（非 ISR）中或在安全时机调用，因为使用 HAL_GPIO 和 DWT_Delay。
 */
void I2C_Bus_Clock_Pulse_And_Stop(void) {
    GPIO_InitTypeDef gpio = {0};

    // 1. 使能 GPIO 时钟（PB 假设已开，如果未开可调用 __HAL_RCC_GPIOB_CLK_ENABLE）
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // 2. 配置 SCL/SDA 为开漏输出（手动驱动）
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;

    gpio.Pin = I2C_SCL_PIN;
    HAL_GPIO_Init(I2C_SCL_PORT, &gpio);

    gpio.Pin = I2C_SDA_PIN;
    HAL_GPIO_Init(I2C_SDA_PORT, &gpio);

    // 3. 确保 SDA 处于拉高（释放）
    HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_SET);
    DWT_Delay(0.001f);

    // 4. 产生 9 个 SCL 脉冲（CLK recovery）
    for (int i = 0; i < 9; ++i) {
        HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_RESET);
        DWT_Delay(0.001f);
        HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
        DWT_Delay(0.001f);
    }

    // 5. 产生 STOP： SDA 从 0 -> 1 时 SCL=1
    HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_RESET);
    DWT_Delay(0.001f);
    HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
    DWT_Delay(0.001f);
    HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_SET);
    DWT_Delay(0.001f);

    // 6. 不在这里切换回 AF 模式； HAL_I2C_Init 将根据配置设置 AF
}

/**
 * @brief 在任务上下文中做恢复（可安全调用 HAL_I2C_DeInit/HAL_I2C_Init/DWT_Delay）。
 *        这个函数会在检测到 g_i2c_recover_request 时由上层任务调用。
 */
void I2C_Try_Recovery_Task_Context(void) {
    // 如果没有恢复请求，不做任何事
    if (!g_i2c_recover_request && !g_i2c_recovery_error) return;

    // 清除标志，进入恢复
    g_i2c_recover_request = false;
    g_i2c_recovery_error = false;

    // 发 debug 信息（把这个函数替换为你的 UART 发送）


    // 1) DeInit I2C 外设（释放 HAL 状态机）
    HAL_I2C_DeInit(&hi2c1);
    DWT_Delay(0.001f);

    // 2) 产生 9 个 SCL 脉冲 + STOP（释放 SDA）
    I2C_Bus_Clock_Pulse_And_Stop();
    DWT_Delay(0.001f);

    // 3) Re-Init I2C
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {

        // 如果初始化失败，可以多次尝试或记录错误
    } else {

    }
}

/**
 * @brief 检查 I2C 硬件是否处于 BUSY，如果是则尝试在任务上下文进行恢复（非阻塞地请求恢复）。
 * @return true 表示已经发起恢复请求（任务应调用 I2C_Try_Recovery_Task_Context 执行）
 */
bool I2C_Check_And_Attempt_Recovery(void) {
    // 如果 HAL 报忙（BUSY），发起恢复请求
    //if (HAL_I2C_GetState(&hi2c1) == HAL_I2C_STATE_BUSY) {
        g_i2c_recover_request = true;
        g_i2c_recovery_error = true;
        return true;
    //}
    return false;
}


void I2C_Try_Recovery_Task(void *argument) {

    for (;;) {

        // 检查并尝试恢复
        I2C_Try_Recovery_Task_Context();

        // 延时，避免频繁检查
        osDelay(pdMS_TO_TICKS(100));
    }
}

