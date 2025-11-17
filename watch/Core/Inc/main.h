/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define KEY2_Pin GPIO_PIN_3
#define KEY2_GPIO_Port GPIOE
#define relay2_Pin GPIO_PIN_13
#define relay2_GPIO_Port GPIOC
#define KEY1_Pin GPIO_PIN_3
#define KEY1_GPIO_Port GPIOC
#define KEY3_Pin GPIO_PIN_1
#define KEY3_GPIO_Port GPIOA
#define motor1027_Pin GPIO_PIN_0
#define motor1027_GPIO_Port GPIOB
#define LCD_PWM_Pin GPIO_PIN_1
#define LCD_PWM_GPIO_Port GPIOB
#define WK2114_REST_Pin GPIO_PIN_8
#define WK2114_REST_GPIO_Port GPIOA
#define debug_uart_tx_Pin GPIO_PIN_9
#define debug_uart_tx_GPIO_Port GPIOA
#define debug_uart_rx_Pin GPIO_PIN_10
#define debug_uart_rx_GPIO_Port GPIOA
#define W25QXX_CS_Pin GPIO_PIN_15
#define W25QXX_CS_GPIO_Port GPIOA
#define relay1_Pin GPIO_PIN_12
#define relay1_GPIO_Port GPIOC
#define EXTI_WK2114_IRQn_Pin GPIO_PIN_2
#define EXTI_WK2114_IRQn_GPIO_Port GPIOD
#define EXTI_WK2114_IRQn_EXTI_IRQn EXTI2_IRQn
#define APDS_SCL_Pin GPIO_PIN_8
#define APDS_SCL_GPIO_Port GPIOB
#define APDS_SDA_Pin GPIO_PIN_9
#define APDS_SDA_GPIO_Port GPIOB
#define SPI_CS_PSRAM_Pin GPIO_PIN_0
#define SPI_CS_PSRAM_GPIO_Port GPIOE
#define LCD_INT_Pin GPIO_PIN_1
#define LCD_INT_GPIO_Port GPIOE
#define LCD_INT_EXTI_IRQn EXTI1_IRQn

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
