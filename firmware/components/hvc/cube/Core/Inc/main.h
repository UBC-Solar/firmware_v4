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

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define GPIO1_Pin GPIO_PIN_0
#define GPIO1_GPIO_Port GPIOC
#define GPIO2_Pin GPIO_PIN_1
#define GPIO2_GPIO_Port GPIOC
#define IMD_GPIO_IN_Pin GPIO_PIN_2
#define IMD_GPIO_IN_GPIO_Port GPIOC
#define IMD_GPIO_IN_EXTI_IRQn EXTI2_IRQn
#define THERMISTOR_Pin GPIO_PIN_1
#define THERMISTOR_GPIO_Port GPIOA
#define MOTOR_PRECHARGE_Pin GPIO_PIN_4
#define MOTOR_PRECHARGE_GPIO_Port GPIOA
#define MPPT_PRECHARGE_Pin GPIO_PIN_5
#define MPPT_PRECHARGE_GPIO_Port GPIOA
#define MASTERBOARD_FAULT_Pin GPIO_PIN_6
#define MASTERBOARD_FAULT_GPIO_Port GPIOA
#define MASTERBOARD_FAULT_EXTI_IRQn EXTI9_5_IRQn
#define SUPP_SENSE_Pin GPIO_PIN_7
#define SUPP_SENSE_GPIO_Port GPIOA
#define ESTOP_Pin GPIO_PIN_4
#define ESTOP_GPIO_Port GPIOC
#define ESTOP_EXTI_IRQn EXTI4_IRQn
#define DCDC_ACTIVE_Pin GPIO_PIN_5
#define DCDC_ACTIVE_GPIO_Port GPIOC
#define DCDC_ACTIVE_EXTI_IRQn EXTI9_5_IRQn
#define SUPP_ACTIVE_Pin GPIO_PIN_0
#define SUPP_ACTIVE_GPIO_Port GPIOB
#define SUPP_ACTIVE_EXTI_IRQn EXTI0_IRQn
#define LV_CURR_SENSE_Pin GPIO_PIN_1
#define LV_CURR_SENSE_GPIO_Port GPIOB
#define BOOT1_Pin GPIO_PIN_2
#define BOOT1_GPIO_Port GPIOB
#define POS_CTRL_Pin GPIO_PIN_10
#define POS_CTRL_GPIO_Port GPIOB
#define LLIM_CTRL_Pin GPIO_PIN_11
#define LLIM_CTRL_GPIO_Port GPIOB
#define MOTOR_PC_CTRL_Pin GPIO_PIN_12
#define MOTOR_PC_CTRL_GPIO_Port GPIOB
#define HLIM_CTRL_Pin GPIO_PIN_13
#define HLIM_CTRL_GPIO_Port GPIOB
#define MPPT_PC_CTRL_Pin GPIO_PIN_14
#define MPPT_PC_CTRL_GPIO_Port GPIOB
#define DISCHARGE_TOGGLE_OFF_Pin GPIO_PIN_15
#define DISCHARGE_TOGGLE_OFF_GPIO_Port GPIOB
#define NEG_CTRL_Pin GPIO_PIN_6
#define NEG_CTRL_GPIO_Port GPIOC
#define ESTOP_LED_Pin GPIO_PIN_7
#define ESTOP_LED_GPIO_Port GPIOC
#define FAULT_LED_Pin GPIO_PIN_8
#define FAULT_LED_GPIO_Port GPIOC
#define SUPP_LOW_LED_Pin GPIO_PIN_9
#define SUPP_LOW_LED_GPIO_Port GPIOC
#define DEBUG_LED_Pin GPIO_PIN_8
#define DEBUG_LED_GPIO_Port GPIOA
#define MPPT_CTRL_Pin GPIO_PIN_9
#define MPPT_CTRL_GPIO_Port GPIOA
#define DIST_CTRL_Pin GPIO_PIN_10
#define DIST_CTRL_GPIO_Port GPIOA
#define HV_CURRENT_ALERT_Pin GPIO_PIN_10
#define HV_CURRENT_ALERT_GPIO_Port GPIOC
#define HV_CURRENT_ALERT_EXTI_IRQn EXTI15_10_IRQn
#define IMD_CTRL_Pin GPIO_PIN_11
#define IMD_CTRL_GPIO_Port GPIOC
#define FAN_PWM_Pin GPIO_PIN_8
#define FAN_PWM_GPIO_Port GPIOB
#define FAN_CTRL_Pin GPIO_PIN_9
#define FAN_CTRL_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
