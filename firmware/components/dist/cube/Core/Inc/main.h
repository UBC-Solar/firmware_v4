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
#define ESTOP_ADC_Pin GPIO_PIN_0
#define ESTOP_ADC_GPIO_Port GPIOC
#define SPARE_CTRL_CURRENT_Pin GPIO_PIN_4
#define SPARE_CTRL_CURRENT_GPIO_Port GPIOA
#define SPARE_MUX_CURRENT_Pin GPIO_PIN_5
#define SPARE_MUX_CURRENT_GPIO_Port GPIOA
#define SPARE_CURRENT_Pin GPIO_PIN_6
#define SPARE_CURRENT_GPIO_Port GPIOA
#define MDI_CURRENT_Pin GPIO_PIN_7
#define MDI_CURRENT_GPIO_Port GPIOA
#define DRD_CURRENT_Pin GPIO_PIN_4
#define DRD_CURRENT_GPIO_Port GPIOC
#define SUPP_ADC_Pin GPIO_PIN_1
#define SUPP_ADC_GPIO_Port GPIOB
#define BOOT0_Pin GPIO_PIN_2
#define BOOT0_GPIO_Port GPIOB
#define LED_TOGGLE_Pin GPIO_PIN_10
#define LED_TOGGLE_GPIO_Port GPIOB
#define DRD_FUSE_Pin GPIO_PIN_12
#define DRD_FUSE_GPIO_Port GPIOB
#define MDI_FUSE_Pin GPIO_PIN_13
#define MDI_FUSE_GPIO_Port GPIOB
#define SPARE_CTRL_FUSE_Pin GPIO_PIN_14
#define SPARE_CTRL_FUSE_GPIO_Port GPIOB
#define SPARE_FUSE_Pin GPIO_PIN_15
#define SPARE_FUSE_GPIO_Port GPIOB
#define SPARE_MUX_FUSE_Pin GPIO_PIN_6
#define SPARE_MUX_FUSE_GPIO_Port GPIOC
#define DEBUG_Pin GPIO_PIN_9
#define DEBUG_GPIO_Port GPIOC
#define SPARE_MUX_CTRL_Pin GPIO_PIN_8
#define SPARE_MUX_CTRL_GPIO_Port GPIOA
#define SPARE_CTRL_Pin GPIO_PIN_10
#define SPARE_CTRL_GPIO_Port GPIOA
#define MDI_CTRL_Pin GPIO_PIN_11
#define MDI_CTRL_GPIO_Port GPIOA
#define DRD_CTRL_Pin GPIO_PIN_12
#define DRD_CTRL_GPIO_Port GPIOA
#define MUX_STATUS_Pin GPIO_PIN_5
#define MUX_STATUS_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
