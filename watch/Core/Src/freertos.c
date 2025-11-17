/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for sensor_task */
osThreadId_t sensor_taskHandle;
const osThreadAttr_t sensor_task_attributes = {
  .name = "sensor_task",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for UartDebugTask */
osThreadId_t UartDebugTaskHandle;
const osThreadAttr_t UartDebugTask_attributes = {
  .name = "UartDebugTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Task_read_APDS9 */
osThreadId_t Task_read_APDS9Handle;
const osThreadAttr_t Task_read_APDS9_attributes = {
  .name = "Task_read_APDS9",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Task_Read_LIS3D */
osThreadId_t Task_Read_LIS3DHandle;
const osThreadAttr_t Task_Read_LIS3D_attributes = {
  .name = "Task_Read_LIS3D",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Task_read_PCF85 */
osThreadId_t Task_read_PCF85Handle;
const osThreadAttr_t Task_read_PCF85_attributes = {
  .name = "Task_read_PCF85",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for StartWk2114task */
osThreadId_t StartWk2114taskHandle;
const osThreadAttr_t StartWk2114task_attributes = {
  .name = "StartWk2114task",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Task_finger */
osThreadId_t Task_fingerHandle;
const osThreadAttr_t Task_finger_attributes = {
  .name = "Task_finger",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for lvgl_Task */
osThreadId_t lvgl_TaskHandle;
const osThreadAttr_t lvgl_Task_attributes = {
  .name = "lvgl_Task",
  .stack_size = 2048 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for I2C_Try_Recover */
osThreadId_t I2C_Try_RecoverHandle;
const osThreadAttr_t I2C_Try_Recover_attributes = {
  .name = "I2C_Try_Recover",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void SensorTask(void *argument);
void UART_Debug_RxProcessTask(void *argument);
void Task_Read_APDS9900(void *argument);
void Task_Read_LIS3DH(void *argument);
void Task_Read_PCF8563(void *argument);
void StartWk2114Task(void *argument);
void Task_Finger(void *argument);
void Task_LVGL(void *argument);
void I2C_Try_Recovery_Task(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of sensor_task */
  sensor_taskHandle = osThreadNew(SensorTask, NULL, &sensor_task_attributes);

  /* creation of UartDebugTask */
  UartDebugTaskHandle = osThreadNew(UART_Debug_RxProcessTask, NULL, &UartDebugTask_attributes);

  /* creation of Task_read_APDS9 */
  Task_read_APDS9Handle = osThreadNew(Task_Read_APDS9900, NULL, &Task_read_APDS9_attributes);

  /* creation of Task_Read_LIS3D */
  Task_Read_LIS3DHandle = osThreadNew(Task_Read_LIS3DH, NULL, &Task_Read_LIS3D_attributes);

  /* creation of Task_read_PCF85 */
  Task_read_PCF85Handle = osThreadNew(Task_Read_PCF8563, NULL, &Task_read_PCF85_attributes);

  /* creation of StartWk2114task */
  StartWk2114taskHandle = osThreadNew(StartWk2114Task, NULL, &StartWk2114task_attributes);

  /* creation of Task_finger */
  Task_fingerHandle = osThreadNew(Task_Finger, NULL, &Task_finger_attributes);

  /* creation of lvgl_Task */
  lvgl_TaskHandle = osThreadNew(Task_LVGL, NULL, &lvgl_Task_attributes);

  /* creation of I2C_Try_Recover */
  I2C_Try_RecoverHandle = osThreadNew(I2C_Try_Recovery_Task, NULL, &I2C_Try_Recover_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_SensorTask */
/**
* @brief Function implementing the sensor_task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_SensorTask */
__weak void SensorTask(void *argument)
{
  /* USER CODE BEGIN SensorTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END SensorTask */
}

/* USER CODE BEGIN Header_UART_Debug_RxProcessTask */
/**
* @brief Function implementing the UartDebugTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_UART_Debug_RxProcessTask */
__weak void UART_Debug_RxProcessTask(void *argument)
{
  /* USER CODE BEGIN UART_Debug_RxProcessTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END UART_Debug_RxProcessTask */
}

/* USER CODE BEGIN Header_Task_Read_APDS9900 */
/**
* @brief Function implementing the Task_read_APDS9 thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Task_Read_APDS9900 */
__weak void Task_Read_APDS9900(void *argument)
{
  /* USER CODE BEGIN Task_Read_APDS9900 */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END Task_Read_APDS9900 */
}

/* USER CODE BEGIN Header_Task_Read_LIS3DH */
/**
* @brief Function implementing the Task_Read_LIS3D thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Task_Read_LIS3DH */
__weak void Task_Read_LIS3DH(void *argument)
{
  /* USER CODE BEGIN Task_Read_LIS3DH */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END Task_Read_LIS3DH */
}

/* USER CODE BEGIN Header_Task_Read_PCF8563 */
/**
* @brief Function implementing the Task_read_PCF85 thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Task_Read_PCF8563 */
__weak void Task_Read_PCF8563(void *argument)
{
  /* USER CODE BEGIN Task_Read_PCF8563 */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END Task_Read_PCF8563 */
}

/* USER CODE BEGIN Header_StartWk2114Task */
/**
* @brief Function implementing the StartWk2114task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartWk2114Task */
__weak void StartWk2114Task(void *argument)
{
  /* USER CODE BEGIN StartWk2114Task */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartWk2114Task */
}

/* USER CODE BEGIN Header_Task_Finger */
/**
* @brief Function implementing the Task_finger thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Task_Finger */
__weak void Task_Finger(void *argument)
{
  /* USER CODE BEGIN Task_Finger */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END Task_Finger */
}

/* USER CODE BEGIN Header_Task_LVGL */
/**
* @brief Function implementing the lvgl_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Task_LVGL */
__weak void Task_LVGL(void *argument)
{
  /* USER CODE BEGIN Task_LVGL */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END Task_LVGL */
}

/* USER CODE BEGIN Header_I2C_Try_Recovery_Task */
/**
* @brief Function implementing the I2C_Try_Recover thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_I2C_Try_Recovery_Task */
__weak void I2C_Try_Recovery_Task(void *argument)
{
  /* USER CODE BEGIN I2C_Try_Recovery_Task */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END I2C_Try_Recovery_Task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

