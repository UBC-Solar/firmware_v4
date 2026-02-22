#ifndef __TASKS_H__
#define __TASKS_H__

#define DEFAULT_TASK_DELAY 			  100
#define TIME_SINCE_STARTUP_TASK_DELAY 1000

/* INCLUDES */
#include "cmsis_os2.h"
#include "drive_state.h"
#include "lcd_app.h"
#include "soc.h"

/* DEFINES */
#define SOC_CALCULATE_ON (0xFF)
#define SOC_CALCULATE_OFF (0x00)
#define CALCULATE_SOC_DELAY (50)

extern osEventFlagsId_t calculate_soc_flagHandle;

/* TASK FUNCTION PROTOTYPES */

/**
 * @brief Drive state task main loop.
 *
 * Manages the drive system state machine and control logic.
 * Executes with at a 1ms period. Task runs indefinitely until system shutdown.
 */
void TasksDriveState(void* argument);
void TasksCalculateSoc(void* argument);

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

/**
 * @brief LCD update task main loop
 *
 * Initialilzes the LCD Driver and manages the LCD pages switch logic
 * Executes with at a 1ms period. Task runs indefinitely until system shutdown.
 */
void TasksLcdUpdate(void *argument);

void TasksDiagnostic(void *argument);

void TasksTimeSinceStartup(void *argument);

#endif //__TASKS_H__