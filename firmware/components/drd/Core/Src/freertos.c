/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticTask_t osStaticThreadDef_t;
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
uint32_t defaultTaskBuffer[ 128 ];
osStaticThreadDef_t defaultTaskControlBlock;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .cb_mem = &defaultTaskControlBlock,
  .cb_size = sizeof(defaultTaskControlBlock),
  .stack_mem = &defaultTaskBuffer[0],
  .stack_size = sizeof(defaultTaskBuffer),
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for LCDUpdateTask */
osThreadId_t LCDUpdateTaskHandle;
uint32_t LCDUpdateTaskBuffer[ 256 ];
osStaticThreadDef_t LCDUpdateTaskControlBlock;
const osThreadAttr_t LCDUpdateTask_attributes = {
  .name = "LCDUpdateTask",
  .cb_mem = &LCDUpdateTaskControlBlock,
  .cb_size = sizeof(LCDUpdateTaskControlBlock),
  .stack_mem = &LCDUpdateTaskBuffer[0],
  .stack_size = sizeof(LCDUpdateTaskBuffer),
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for DriveStateTask */
osThreadId_t DriveStateTaskHandle;
uint32_t DriveStateTaskBuffer[ 256 ];
osStaticThreadDef_t DriveStateTaskControlBlock;
const osThreadAttr_t DriveStateTask_attributes = {
  .name = "DriveStateTask",
  .cb_mem = &DriveStateTaskControlBlock,
  .cb_size = sizeof(DriveStateTaskControlBlock),
  .stack_mem = &DriveStateTaskBuffer[0],
  .stack_size = sizeof(DriveStateTaskBuffer),
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for TimeSinceStartu */
osThreadId_t TimeSinceStartuHandle;
uint32_t TimeSinceStartuBuffer[ 128 ];
osStaticThreadDef_t TimeSinceStartuControlBlock;
const osThreadAttr_t TimeSinceStartu_attributes = {
  .name = "TimeSinceStartu",
  .cb_mem = &TimeSinceStartuControlBlock,
  .cb_size = sizeof(TimeSinceStartuControlBlock),
  .stack_mem = &TimeSinceStartuBuffer[0],
  .stack_size = sizeof(TimeSinceStartuBuffer),
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for CalculateSocTas */
osThreadId_t CalculateSocTasHandle;
uint32_t CalculateSocTasBuffer[ 512 ];
osStaticThreadDef_t CalculateSocTasControlBlock;
const osThreadAttr_t CalculateSocTas_attributes = {
  .name = "CalculateSocTas",
  .cb_mem = &CalculateSocTasControlBlock,
  .cb_size = sizeof(CalculateSocTasControlBlock),
  .stack_mem = &CalculateSocTasBuffer[0],
  .stack_size = sizeof(CalculateSocTasBuffer),
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for ExtLightsTask */
osThreadId_t ExtLightsTaskHandle;
uint32_t ExtLightsTaskBuffer[ 256 ];
osStaticThreadDef_t ExtLightsTaskControlBlock;
const osThreadAttr_t ExtLightsTask_attributes = {
  .name = "ExtLightsTask",
  .cb_mem = &ExtLightsTaskControlBlock,
  .cb_size = sizeof(ExtLightsTaskControlBlock),
  .stack_mem = &ExtLightsTaskBuffer[0],
  .stack_size = sizeof(ExtLightsTaskBuffer),
  .priority = (osPriority_t) osPriorityLow,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void LCDUpdatetask(void *argument);
void DriveState_task(void *argument);
void TimeSinceStartup_task(void *argument);
void CalculateSoCtask(void *argument);
void ExternalLights_task(void *argument);

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

  /* creation of LCDUpdateTask */
  LCDUpdateTaskHandle = osThreadNew(LCDUpdatetask, NULL, &LCDUpdateTask_attributes);

  /* creation of DriveStateTask */
  DriveStateTaskHandle = osThreadNew(DriveState_task, NULL, &DriveStateTask_attributes);

  /* creation of TimeSinceStartu */
  TimeSinceStartuHandle = osThreadNew(TimeSinceStartup_task, NULL, &TimeSinceStartu_attributes);

  /* creation of CalculateSocTas */
  CalculateSocTasHandle = osThreadNew(CalculateSoCtask, NULL, &CalculateSocTas_attributes);

  /* creation of ExtLightsTask */
  ExtLightsTaskHandle = osThreadNew(ExternalLights_task, NULL, &ExtLightsTask_attributes);

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

/* USER CODE BEGIN Header_LCDUpdatetask */
/**
* @brief Function implementing the LCDUpdateTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_LCDUpdatetask */
void LCDUpdatetask(void *argument)
{
  /* USER CODE BEGIN LCDUpdatetask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END LCDUpdatetask */
}

/* USER CODE BEGIN Header_DriveState_task */
/**
* @brief Function implementing the DriveStateTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_DriveState_task */
void DriveState_task(void *argument)
{
  /* USER CODE BEGIN DriveState_task */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END DriveState_task */
}

/* USER CODE BEGIN Header_TimeSinceStartup_task */
/**
* @brief Function implementing the TimeSinceStartu thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_TimeSinceStartup_task */
void TimeSinceStartup_task(void *argument)
{
  /* USER CODE BEGIN TimeSinceStartup_task */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END TimeSinceStartup_task */
}

/* USER CODE BEGIN Header_CalculateSoCtask */
/**
* @brief Function implementing the CalculateSocTas thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_CalculateSoCtask */
void CalculateSoCtask(void *argument)
{
  /* USER CODE BEGIN CalculateSoCtask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END CalculateSoCtask */
}

/* USER CODE BEGIN Header_ExternalLights_task */
/**
* @brief Function implementing the ExtLightsTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_ExternalLights_task */
void ExternalLights_task(void *argument)
{
  /* USER CODE BEGIN ExternalLights_task */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END ExternalLights_task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

