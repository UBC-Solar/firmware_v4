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
#define DEBUG_LED_Pin GPIO_PIN_0
#define DEBUG_LED_GPIO_Port GPIOA
#define DEBUG_LEDA1_Pin GPIO_PIN_1
#define DEBUG_LEDA1_GPIO_Port GPIOA
#define DISPLAY__CS_Pin GPIO_PIN_4
#define DISPLAY__CS_GPIO_Port GPIOA
#define DISPLAY_SCL_Pin GPIO_PIN_5
#define DISPLAY_SCL_GPIO_Port GPIOA
#define DISPLAY_A0_Pin GPIO_PIN_6
#define DISPLAY_A0_GPIO_Port GPIOA
#define DISPLAY_RESET_Pin GPIO_PIN_10
#define DISPLAY_RESET_GPIO_Port GPIOB
#define RTS_OUT_Pin GPIO_PIN_12
#define RTS_OUT_GPIO_Port GPIOB
#define ESTOP_Pin GPIO_PIN_14
#define ESTOP_GPIO_Port GPIOB
#define BL_LIGHTS_Pin GPIO_PIN_15
#define BL_LIGHTS_GPIO_Port GPIOB
#define BR_LIGHTS_Pin GPIO_PIN_6
#define BR_LIGHTS_GPIO_Port GPIOC
#define FLT_MCU_Pin GPIO_PIN_7
#define FLT_MCU_GPIO_Port GPIOC
#define LTS_OUT_Pin GPIO_PIN_8
#define LTS_OUT_GPIO_Port GPIOC
#define BRK_IN_Pin GPIO_PIN_9
#define BRK_IN_GPIO_Port GPIOC
#define DRIVE_STATE_NEXT_Pin GPIO_PIN_10
#define DRIVE_STATE_NEXT_GPIO_Port GPIOA
#define DRIVE_STATE_PREV_Pin GPIO_PIN_11
#define DRIVE_STATE_PREV_GPIO_Port GPIOA
#define HAZARD_Pin GPIO_PIN_6
#define HAZARD_GPIO_Port GPIOB
#define ECO_POWER_Pin GPIO_PIN_7
#define ECO_POWER_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
