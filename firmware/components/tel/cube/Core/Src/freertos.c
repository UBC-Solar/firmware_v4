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
#include "cmsis_os2.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "tasks.h"
#include "telemetry_driver.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef StaticTask_t osStaticThreadDef_t;
typedef StaticEventGroup_t osStaticEventGroupDef_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define NUM_USART1_TX_SEMAPHORES        1
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

osSemaphoreId_t usart1_tx_semaphore;

/* Definitions for TasksIMU */
osThreadId_t TasksIMUHandle;
uint32_t TasksIMUBuffer[256];
osStaticThreadDef_t TasksIMUControlBlock;

const osThreadAttr_t TasksIMU_attributes = {
  .name = "TasksIMU",
  .cb_mem = &TasksIMUControlBlock,
  .cb_size = sizeof(TasksIMUControlBlock),
  .stack_mem = &TasksIMUBuffer[0],
  .stack_size = sizeof(TasksIMUBuffer),
  .priority = (osPriority_t) osPriorityLow,
};

/* Definitions for TasksDiagnostics */
osThreadId_t TasksDiagnosticsHandle;
uint32_t TasksDiagnosticsBuffer[128];
osStaticThreadDef_t TasksDiagnosticsControlBlock;

const osThreadAttr_t TasksDiagnostics_attributes = {
  .name = "TasksDiagnostics",
  .cb_mem = &TasksDiagnosticsControlBlock,
  .cb_size = sizeof(TasksDiagnosticsControlBlock),
  .stack_mem = &TasksDiagnosticsBuffer[0],
  .stack_size = sizeof(TasksDiagnosticsBuffer),
  .priority = (osPriority_t) osPriorityHigh,
};

/* Definitions for TasksTimeSinceStartup */
osThreadId_t TasksTimeSinceStartupHandle;
uint32_t TasksTimeSinceStartupBuffer[128];
osStaticThreadDef_t TasksTimeSinceStartupControlBlock;

const osThreadAttr_t TasksTimeSinceStartup_attributes = {
  .name = "TasksTimeSinceStartup",
  .cb_mem = &TasksTimeSinceStartupControlBlock,
  .cb_size = sizeof(TasksTimeSinceStartupControlBlock),
  .stack_mem = &TasksTimeSinceStartupBuffer[0],
  .stack_size = sizeof(TasksTimeSinceStartupBuffer),
  .priority = (osPriority_t) osPriorityLow,
};

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

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
  usart1_tx_semaphore = osSemaphoreNew(NUM_USART1_TX_SEMAPHORES, NUM_USART1_TX_SEMAPHORES, NULL);
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

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */

  /* creation of TasksIMU */
  TasksIMUHandle = osThreadNew(TasksIMU, NULL, &TasksIMU_attributes);
  /* creation of TasksDiagnostics */
  TasksDiagnosticsHandle = osThreadNew(TasksDiagnostics, NULL, &TasksDiagnostics_attributes);
  /* creation of TasksTimeSinceStartup*/
  TasksTimeSinceStartupHandle = osThreadNew(TimeSinceStartup, NULL, &TasksTimeSinceStartup_attributes);

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

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

