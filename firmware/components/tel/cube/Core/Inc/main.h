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
#include "stm32f1xx_hal.h"

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
#define G_SAFEBOOT_Pin GPIO_PIN_6
#define G_SAFEBOOT_GPIO_Port GPIOA
#define MCU_WHEEL_TICK_Pin GPIO_PIN_7
#define MCU_WHEEL_TICK_GPIO_Port GPIOA
#define G__RESET_Pin GPIO_PIN_0
#define G__RESET_GPIO_Port GPIOB
#define G_DIRECTION_Pin GPIO_PIN_1
#define G_DIRECTION_GPIO_Port GPIOB
#define G_WAKE_ON_MOTION_Pin GPIO_PIN_12
#define G_WAKE_ON_MOTION_GPIO_Port GPIOB
#define G_FIX_LED_Pin GPIO_PIN_13
#define G_FIX_LED_GPIO_Port GPIOB
#define DEBUG_LED_1_Pin GPIO_PIN_14
#define DEBUG_LED_1_GPIO_Port GPIOB
#define I_NRST_Pin GPIO_PIN_6
#define I_NRST_GPIO_Port GPIOC
#define I_BOOTN_Pin GPIO_PIN_7
#define I_BOOTN_GPIO_Port GPIOC
#define I_INTN_Pin GPIO_PIN_8
#define I_INTN_GPIO_Port GPIOC
#define R_CTS_Pin GPIO_PIN_9
#define R_CTS_GPIO_Port GPIOC
#define R_RTS_Pin GPIO_PIN_8
#define R_RTS_GPIO_Port GPIOA
#define R_RESET_Pin GPIO_PIN_9
#define R_RESET_GPIO_Port GPIOA
#define R_RSSI_Pin GPIO_PIN_10
#define R_RSSI_GPIO_Port GPIOA
#define R_UART_TX_Pin GPIO_PIN_10
#define R_UART_TX_GPIO_Port GPIOC
#define R_UART_RX_Pin GPIO_PIN_11
#define R_UART_RX_GPIO_Port GPIOC

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
