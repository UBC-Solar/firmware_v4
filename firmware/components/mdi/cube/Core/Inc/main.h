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
#define FWD_LED_Pin GPIO_PIN_1
#define FWD_LED_GPIO_Port GPIOC
#define REV_LED_Pin GPIO_PIN_2
#define REV_LED_GPIO_Port GPIOC
#define ACCEL_LED_Pin GPIO_PIN_0
#define ACCEL_LED_GPIO_Port GPIOA
#define REGEN_LED_Pin GPIO_PIN_1
#define REGEN_LED_GPIO_Port GPIOA
#define ECO_LED_Pin GPIO_PIN_2
#define ECO_LED_GPIO_Port GPIOA
#define SPI1_RDY_Pin GPIO_PIN_3
#define SPI1_RDY_GPIO_Port GPIOA
#define SPI1_CS_Pin GPIO_PIN_4
#define SPI1_CS_GPIO_Port GPIOA
#define PWR_Pin GPIO_PIN_2
#define PWR_GPIO_Port GPIOB
#define ACC_SCL_Pin GPIO_PIN_10
#define ACC_SCL_GPIO_Port GPIOB
#define ACC_SDA_Pin GPIO_PIN_11
#define ACC_SDA_GPIO_Port GPIOB
#define PWRB12_Pin GPIO_PIN_12
#define PWRB12_GPIO_Port GPIOB
#define PIN_6_Pin GPIO_PIN_13
#define PIN_6_GPIO_Port GPIOB
#define DIR_Pin GPIO_PIN_14
#define DIR_GPIO_Port GPIOB
#define ROT_IP_Pin GPIO_PIN_6
#define ROT_IP_GPIO_Port GPIOC
#define MC_LED_Pin GPIO_PIN_11
#define MC_LED_GPIO_Port GPIOA
#define DEBUG_LED_Pin GPIO_PIN_12
#define DEBUG_LED_GPIO_Port GPIOC
#define RTD_LED_Pin GPIO_PIN_2
#define RTD_LED_GPIO_Port GPIOD
#define REG_SCL_Pin GPIO_PIN_6
#define REG_SCL_GPIO_Port GPIOB
#define REG_SDA_Pin GPIO_PIN_7
#define REG_SDA_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
