/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#define DEBUG_LED_Pin GPIO_PIN_3
#define DEBUG_LED_GPIO_Port GPIOC
#define RTS_IN_Pin GPIO_PIN_0
#define RTS_IN_GPIO_Port GPIOA
#define LTS_IN_Pin GPIO_PIN_1
#define LTS_IN_GPIO_Port GPIOA
#define NEXT_PAGE_Pin GPIO_PIN_2
#define NEXT_PAGE_GPIO_Port GPIOA
#define HORN_MCU_Pin GPIO_PIN_7
#define HORN_MCU_GPIO_Port GPIOA
#define PTT_MCU_Pin GPIO_PIN_10
#define PTT_MCU_GPIO_Port GPIOB
#define CRUISE_CONTROL_Pin GPIO_PIN_13
#define CRUISE_CONTROL_GPIO_Port GPIOB
#define CRUISE_CONTROL_EXTI_IRQn EXTI15_10_IRQn
#define CRUISE_DEC_Pin GPIO_PIN_8
#define CRUISE_DEC_GPIO_Port GPIOA
#define CRUISE_INC_Pin GPIO_PIN_9
#define CRUISE_INC_GPIO_Port GPIOA
#define REGEN_Pin GPIO_PIN_12
#define REGEN_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
