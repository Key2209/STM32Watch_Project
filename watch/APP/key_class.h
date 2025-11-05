#ifndef KEY_CLASS_H
#define KEY_CLASS_H

#include "stm32f4xx_hal.h" // 根据您的MCU型号可能需要修改
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include <stdint.h>
#include <stdbool.h>

// --- 类型定义和宏 ---

// 按键事件类型
typedef enum {
    KEY_EVENT_NONE = 0,
    KEY_EVENT_PRESS,       // 按下
    KEY_EVENT_SHORT_CLICK, // 短按（松开时触发）
    KEY_EVENT_LONG_PRESS,  // 长按（达到阈值后触发）
} Key_Event_Type_t;

// 按键事件回调函数指针
typedef void (*Key_Callback_t)(void *arg);

// 按键配置结构体（模拟类成员变量）
typedef struct {
    // 硬件配置
    GPIO_TypeDef *gpio_port;
    uint16_t gpio_pin;
    GPIO_PinState active_level; // 按键有效电平 (e.g., GPIO_PIN_RESET for low active)
    
    // 事件配置
    uint32_t long_press_ms; // 长按时间阈值 (ms)
    uint32_t short_press_ms; // 短按最小时间阈值 (ms, 用于过滤快速误触)

    // 事件回调
    Key_Callback_t short_click_cb;
    Key_Callback_t long_press_cb;
    void *cb_arg; // 传递给回调函数的参数
    
    // 运行时状态 (私有成员)
    uint32_t last_tick;         // 按下时的时间戳
    GPIO_PinState current_level; // 当前电平（去抖后）
    bool is_pressed;             // 当前是否处于按下状态
    bool long_press_triggered;   // 标记长按是否已触发
    
    // 其他标识
    const char *name; // 按键名称，方便调试
    
} Key_t;


// --- 接口函数声明 ---

/**
 * @brief 初始化一个按键对象
 * * @param key_obj 按键对象指针
 * @param port GPIO端口
 * @param pin GPIO引脚
 * @param active_level 按键有效电平
 * @param long_ms 长按阈值（ms）
 * @param name 按键名称
 */
void Key_Init(Key_t *key_obj, 
              GPIO_TypeDef *port, uint16_t pin, 
              GPIO_PinState active_level, 
              uint32_t long_ms, 
              const char *name);

/**
 * @brief 注册按键事件回调函数
 * * @param key_obj 按键对象指针
 * @param event_type 事件类型 (KEY_EVENT_SHORT_CLICK 或 KEY_EVENT_LONG_PRESS)
 * @param cb 回调函数
 * @param arg 传递给回调函数的参数
 */
void Key_RegisterCallback(Key_t *key_obj, 
                          Key_Event_Type_t event_type, 
                          Key_Callback_t cb, 
                          void *arg);

/**
 * @brief 按键对象主循环处理函数，需在FreeRTOS任务中或软件定时器中周期性调用
 * * @param key_obj 按键对象指针
 */
void Key_Process(Key_t *key_obj);


#endif // KEY_CLASS_H
