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
#include "tasks.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticTask_t osStaticThreadDef_t;
/* USER CODE BEGIN PTD */
typedef StaticTask_t osStaticThreadDef_t;
typedef StaticEventGroup_t osStaticEventGroupDef_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* Definitions for TasksHexDisplay */
osThreadId_t TasksHexDisplayHandle;
uint32_t TasksHexDisplayBuffer[256];
osStaticThreadDef_t TasksHexDisplayControlBlock;

const osThreadAttr_t TasksHexDisplay_attributes = {
  .name = "TasksHexDisplay",
  .cb_mem = &TasksHexDisplayControlBlock,
  .cb_size = sizeof(TasksHexDisplayControlBlock),
  .stack_mem = &TasksHexDisplayBuffer[0],
  .stack_size = sizeof(TasksHexDisplayBuffer),
  .priority = (osPriority_t) osPriorityNormal,
};

/* Definitions for TasksSteeringOutputs */
osThreadId_t TasksSteeringOutputsHandle;
uint32_t TasksSteeringOutputsBuffer[256];
osStaticThreadDef_t TasksSteeringOutputsControlBlock;

const osThreadAttr_t TasksSteeringOutputs_attributes = {
  .name = "TasksSteeringOutputs",
  .cb_mem = &TasksSteeringOutputsControlBlock,
  .cb_size = sizeof(TasksSteeringOutputsControlBlock),
  .stack_mem = &TasksSteeringOutputsBuffer[0],
  .stack_size = sizeof(TasksSteeringOutputsBuffer),
  .priority = (osPriority_t) osPriorityLow,
};

/* Definitions for TasksDiagnostic */
osThreadId_t TasksDiagnosticHandle;
uint32_t TasksDiagnosticBuffer[128];
osStaticThreadDef_t TasksDiagnosticControlBlock;

const osThreadAttr_t TasksDiagnostic_attributes = {
  .name = "TasksDiagnostic",
  .cb_mem = &TasksDiagnosticControlBlock,
  .cb_size = sizeof(TasksDiagnosticControlBlock),
  .stack_mem = &TasksDiagnosticBuffer[0],
  .stack_size = sizeof(TasksDiagnosticBuffer),
  .priority = (osPriority_t) osPriorityLow,
};

/* Definitions for TasksTimeSinceBootUp */
osThreadId_t TasksTimeSinceBootUpHandle;
uint32_t TasksTimeSinceBootUpBuffer[128];
osStaticThreadDef_t TasksTimeSinceBootUpControlBlock;

const osThreadAttr_t TasksTimeSinceBootUp_attributes = {
  .name = "TasksTimeSinceBootUp",
  .cb_mem = &TasksTimeSinceBootUpControlBlock,
  .cb_size = sizeof(TasksTimeSinceBootUpControlBlock),
  .stack_mem = &TasksTimeSinceBootUpBuffer[0],
  .stack_size = sizeof(TasksTimeSinceBootUpBuffer),
  .priority = (osPriority_t) osPriorityLow,
};

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

  /* Initialization for TasksHexDisplay */
  TasksHexDisplayHandle = osThreadNew(StartHexDisplayTask, NULL, &TasksHexDisplay_attributes);

  /* Initialization for TasksSteeringOutputs */
  TasksSteeringOutputsHandle = osThreadNew(StartSteeringOutputsTask, NULL, &TasksSteeringOutputs_attributes);
  
/* Initialization for TasksDiagnostic */
  TasksDiagnosticHandle = osThreadNew(TasksDiagnostic, NULL, &TasksDiagnostic_attributes);

  /* Initialization for TasksTimeSinceBootUp */
  TasksTimeSinceBootUpHandle = osThreadNew(TimeSinceBootUp, NULL, &TasksTimeSinceBootUp_attributes);

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

