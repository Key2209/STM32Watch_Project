#include "key_class.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

// 内部宏定义
#define DEBOUNCE_TIME_MS  50 // 软件去抖时间，可根据实际情况调整

/**
 * @brief 读取按键当前实时电平
 */
static inline GPIO_PinState Key_Read_GPIO(Key_t *key_obj) {
    return HAL_GPIO_ReadPin(key_obj->gpio_port, key_obj->gpio_pin);
}

/**
 * @brief 初始化一个按键对象
 */
void Key_Init(Key_t *key_obj, 
              GPIO_TypeDef *port, uint16_t pin, 
              GPIO_PinState active_level, 
              uint32_t long_ms, 
              const char *name) 
{
    // 配置
    key_obj->gpio_port = port;
    key_obj->gpio_pin = pin;
    key_obj->active_level = active_level;
    key_obj->long_press_ms = long_ms;
    key_obj->short_press_ms = DEBOUNCE_TIME_MS; // 短按最小时间，小于此时间认为是抖动

    // 初始化状态
    key_obj->last_tick = 0;
    key_obj->is_pressed = false;
    key_obj->long_press_triggered = false;
    key_obj->current_level = !active_level; // 初始为非激活状态
    
    // 回调清空
    key_obj->short_click_cb = NULL;
    key_obj->long_press_cb = NULL;
    key_obj->cb_arg = NULL;
    
    key_obj->name = name;
}

/**
 * @brief 注册按键事件回调函数
 */
void Key_RegisterCallback(Key_t *key_obj, 
                          Key_Event_Type_t event_type, 
                          Key_Callback_t cb, 
                          void *arg) 
{
    if (event_type == KEY_EVENT_SHORT_CLICK) {
        key_obj->short_click_cb = cb;
    } else if (event_type == KEY_EVENT_LONG_PRESS) {
        key_obj->long_press_cb = cb;
    }
    key_obj->cb_arg = arg;
}

/**
 * @brief 按键对象主循环处理函数
 */
void Key_Process(Key_t *key_obj) {
    // 1. 读取当前原始电平
    GPIO_PinState current_raw_level = Key_Read_GPIO(key_obj);

    // 2. 状态机逻辑
    if (!key_obj->is_pressed) { 
        // --- 未按下状态 ---
        if (current_raw_level == key_obj->active_level) {
            // 检测到按下
            key_obj->is_pressed = true;
            key_obj->last_tick = xTaskGetTickCount() * portTICK_PERIOD_MS; // 记录按下时间戳 (ms)
            key_obj->long_press_triggered = false;
            


        }
    } else { 
        // --- 已按下状态 ---
        
        uint32_t press_duration = xTaskGetTickCount() * portTICK_PERIOD_MS - key_obj->last_tick;
        
        // 2a. 长按检测
        if (!key_obj->long_press_triggered && press_duration >= key_obj->long_press_ms) {
            // 达到长按阈值，触发长按事件
            key_obj->long_press_triggered = true;
            if (key_obj->long_press_cb) {
                key_obj->long_press_cb(key_obj->cb_arg);
            }
        }
        
        // 2b. 松开检测
        if (current_raw_level != key_obj->active_level) {
            // 检测到松开
            key_obj->is_pressed = false;
            
            // 短按事件（只有在未触发长按且按下时间大于去抖时间时才算短按）
            if (!key_obj->long_press_triggered && press_duration >= key_obj->short_press_ms) {
                if (key_obj->short_click_cb) {
                    key_obj->short_click_cb(key_obj->cb_arg);
                }
            } 
            
            // 状态复位
            key_obj->last_tick = 0;
        }
        
        // 注意：这里没有严格的软件去抖，而是利用了 short_press_ms 来过滤极短的抖动。
        // 如果需要更严格的软件去抖，需要添加一个计数器。
    }
    
    // 3. 更新当前去抖后的电平（可选，主要用于调试）
    key_obj->current_level = current_raw_level;
}








