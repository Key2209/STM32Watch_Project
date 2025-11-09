
#include "Sensor_Task.h"
#include "APDS9900.h"
#include "LIS3DH.h"
#include "i2c.h"
#include "i2c_dma_manager.h"
#include "key_class.h"
#include "main.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_uart.h"
#include "uart_app.h"
#include "usart.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/_intsup.h>
#include "PCF8563.h"
Key_t key1;
Key_t key2;
Key_t key3;

void KEY_SHORT_CLICK_Callback(void *arg) {}
void KEY_LONG_PRESS_Callback(void *arg) {}

float lux_value = 0.0f;
float test = 0.0f;

float float_test = 9.375;
char str_test[] = "float_test";
HAL_StatusTypeDef apd_status = HAL_BUSY;
int flag = 1;
void SensorTask(void *argument) {
  /* USER CODE BEGIN SensorTask */
  /* Infinite loop */
  // Key_Init(&key1, KEY1_GPIO_Port, KEY1_Pin, GPIO_PIN_RESET, 1000, "KEY1");
  // Key_Init(&key2, KEY2_GPIO_Port, KEY2_Pin, GPIO_PIN_RESET, 1000, "KEY2");
  // Key_Init(&key3, KEY3_GPIO_Port, KEY3_Pin, GPIO_PIN_RESET, 1000, "KEY3");
  // Key_RegisterCallback(&key2, KEY_EVENT_SHORT_CLICK,
  // KEY_SHORT_CLICK_Callback,
  //                      NULL);
  // Key_RegisterCallback(&key3, KEY_EVENT_LONG_PRESS, KEY_LONG_PRESS_Callback,
  //                      NULL);

  // 传感器初始化 (阻塞式)

  for (;;) {
    // //轮询按键状态
    // Key_Process(&key1);
    // Key_Process(&key2);
    // Key_Process(&key3);

    // 任务周期延迟
    osDelay(pdMS_TO_TICKS(500));
  }
  /* USER CODE END SensorTask */
}

// APDS9900 寄存器和地址
PCF8563_Time_t rtc_time;
PCF8563_Time_t init_time = {
    .seconds = 0,
    .minutes = 0,
    .hours = 12,
    .day = 9,
    .weekday = 2,
    .month = 11,
    .year = 25,
};
void Task_Read_APDS9900(void *argument) {

  //APDS9900_Init(&hi2c1);

  for (;;) {
    

    CH0_Data =
        APDS9900_Read_Word_DMA(&hi2c1, APDS9900_REG_CH0DATAL, 100); // 0x14
    CH1_Data =
        APDS9900_Read_Word_DMA(&hi2c1, APDS9900_REG_CH1DATAL, 100); // 0x16
    Proximity_Data =
        APDS9900_Read_Word_DMA(&hi2c1, APDS9900_REG_PDATAL, 100); // 0x18
    // Uart_Send(&Uart1_Tx, (uint8_t *)"----------------------\n", 24);
    static int count = 0;
    count++;
    if (count % 20 == 0) {
      printf("---- APDS9900 Read Count: %d ----\n", count / 20);
      printf("CH0_ALS: %.2f\n", (float)CH0_Data);
      printf("CH1_ALS: %.2f\n", (float)CH1_Data);
      printf("Proximity: %.2f\n", (float)Proximity_Data);
      count = 0;
    }

    // 3. 延时，出让 CPU
    osDelay(pdMS_TO_TICKS(50));
  }
}

LIS3DH_Data_t Accel_Data;

void Task_Read_LIS3DH(void *argument) {
  // 1. 初始化传感器
  //printf("LIS3DH Initializing...\n");
  //LIS3DH_Init(&hi2c1, 1000);
  for (;;) {

    // // 2. 读取加速度数据 (使用 DMA 模式，阻塞等待 100ms)
    osStatus_t status = LIS3DH_Read_Acc_DMA(&hi2c1, &Accel_Data, 100);

    if (status == osOK) {

      static int count = 0;
      count++;
      if (count % 20 == 0) {
        printf("Acc [g]: X=%.3f, Y=%.3f, Z=%.3f\n", Accel_Data.x_g,
               Accel_Data.y_g, Accel_Data.z_g);
        count = 0;
      }
      // 3. 打印结果

    } else if (status == osErrorTimeout) {
      printf("Error: LIS3DH Read Timeout!\n");
    } else {
      printf("Error: LIS3DH Read Failed (Status: %d)!\n", (int)status);
    }

    // 4. 延时，出让 CPU
    osDelay(pdMS_TO_TICKS(50)); // 20Hz 读取速率
  }
}



void Task_Read_PCF8563(void *argument) {

  //PCF8563_Init(&hi2c1);
  PCF8563_Set_Time(&hi2c1, &init_time);
  for (;;) {

    PCF8563_Read_Time(&hi2c1, &rtc_time);

      static int count = 0;
      count++;
      if (count % 20 == 0) {
      printf("RTC Time: 20%02d-%02d-%02d %02d:%02d:%02d\n", rtc_time.year,
           rtc_time.month, rtc_time.day, rtc_time.hours, rtc_time.minutes,
           rtc_time.seconds);
        count = 0;
      }


    // 3. 延时，出让 CPU
    osDelay(pdMS_TO_TICKS(50));
  }
}

