
#include "key_class.h"

#include "main.h"
#include "stm32f407xx.h"
#include "stm32f4xx.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_uart.h"
#include "uart_app.h"
#include "usart.h"
#include <stdint.h>
#include <string.h>
#include <sys/_intsup.h>
#include "Sensor_Task.h"
Key_t key1;
Key_t key2;
Key_t key3;

void KEY_SHORT_CLICK_Callback(void *arg) {}
void KEY_LONG_PRESS_Callback(void *arg) {}






char *message = "Hello from SensorTask!";

void SensorTask(void *argument) {
  /* USER CODE BEGIN SensorTask */
  /* Infinite loop */
  Key_Init(&key1, KEY1_GPIO_Port, KEY1_Pin, GPIO_PIN_RESET, 1000, "KEY1");
  Key_Init(&key2, KEY2_GPIO_Port, KEY2_Pin, GPIO_PIN_RESET, 1000, "KEY2");
  Key_Init(&key3, KEY3_GPIO_Port, KEY3_Pin, GPIO_PIN_RESET, 1000, "KEY3");
  Key_RegisterCallback(&key2, KEY_EVENT_SHORT_CLICK, KEY_SHORT_CLICK_Callback,
                       NULL);
  Key_RegisterCallback(&key3, KEY_EVENT_LONG_PRESS, KEY_LONG_PRESS_Callback,
                       NULL);


  for (;;) {
    //轮询按键状态
    Key_Process(&key1);
    Key_Process(&key2);
    Key_Process(&key3);


    osDelay(pdMS_TO_TICKS(1));
  }
  /* USER CODE END SensorTask */
}
