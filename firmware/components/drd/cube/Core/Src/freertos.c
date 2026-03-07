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
#include "can_driver.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticTask_t osStaticThreadDef_t;
/* USER CODE BEGIN PTD */
typedef StaticTask_t osStaticThreadDef_t;
typedef StaticEventGroup_t osStaticEventGroupDef_t;
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

/* Definitions for TasksLcdUpdate */
osThreadId_t TasksLcdUpdateHandle;
uint32_t TasksLcdUpdateBuffer[256];
osStaticThreadDef_t TasksLcdUpdateControlBlock;

const osThreadAttr_t TasksLcdUpdate_attributes = {
  .name = "TasksLcdUpdate",
  .cb_mem = &TasksLcdUpdateControlBlock,
  .cb_size = sizeof(TasksLcdUpdateControlBlock),
  .stack_mem = &TasksLcdUpdateBuffer[0],
  .stack_size = sizeof(TasksLcdUpdateBuffer),
  .priority = (osPriority_t) osPriorityNormal,
};

/* Definitions for TasksDriveState */
osThreadId_t TasksDriveStateHandle;
uint32_t TasksDriveStateBuffer[256];
osStaticThreadDef_t TasksDriveStateControlBlock;

const osThreadAttr_t TasksDriveState_attributes = {
  .name = "TasksDriveState",
  .cb_mem = &TasksDriveStateControlBlock,
  .cb_size = sizeof(TasksDriveStateControlBlock),
  .stack_mem = &TasksDriveStateBuffer[0],
  .stack_size = sizeof(TasksDriveStateBuffer),
  .priority = (osPriority_t) osPriorityLow,
};

/* Definitions for TasksCalculateSoc */
osThreadId_t TasksCalculateSocHandle;
uint32_t TasksCalculateSocBuffer[512];
osStaticThreadDef_t TasksCalculateSocControlBlock;

const osThreadAttr_t TasksCalculateSoc_attributes = {
  .name = "TasksCalculateSoc",
  .cb_mem = &TasksCalculateSocControlBlock,
  .cb_size = sizeof(TasksCalculateSocControlBlock),
  .stack_mem = &TasksCalculateSocBuffer[0],
  .stack_size = sizeof(TasksCalculateSocBuffer),
  .priority = (osPriority_t) osPriorityLow,
};

osEventFlagsId_t calculate_soc_flagHandle;
osStaticEventGroupDef_t calculate_soc_flagControlBlock;
const osEventFlagsAttr_t calculate_soc_flag_attributes = {
  .name = "calculate_soc_flag",
  .cb_mem = &calculate_soc_flagControlBlock,
  .cb_size = sizeof(calculate_soc_flagControlBlock),
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
  .priority = (osPriority_t) osPriorityNormal,
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

  /* creation of TasksCalculateSoc */
  TasksCalculateSocHandle = osThreadNew(TasksCalculateSoc, NULL, &TasksCalculateSoc_attributes);
  /* creation of TasksDriveState */
  TasksDriveStateHandle = osThreadNew(TasksDriveState, NULL, &TasksDriveState_attributes);
  /* creation of TasksLcdUpdate */
  TasksLcdUpdateHandle = osThreadNew(TasksLcdUpdate, NULL, &TasksLcdUpdate_attributes);
  /* creation of TasksDiagnostic */
  TasksDiagnosticHandle = osThreadNew(TasksDiagnostic, NULL, &TasksDiagnostic_attributes);
  /* creation of TasksTimeSinceStartup */
  TasksTimeSinceStartupHandle = osThreadNew(TasksTimeSinceStartup, NULL, &TasksTimeSinceStartup_attributes);

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
    osDelay(100);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

