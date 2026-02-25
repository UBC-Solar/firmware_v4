/**
 * @file    tasks.h
 * @brief   FreeRTOS task declarations for DRD board application logic
 *
 * This header declares all FreeRTOS task prototypes for this board component of UBC Solar
 * firmware. Each task represents a concurrent execution thread that runs indefinitely within the
 * real-time operating system.
 *
 * Tasks defined here include the following boards and respective task:
 * - DRD
 *  - TasksDriveState
 *  - TasksCalculateSoc
 *  - TasksLcdUpdate
 *  - TasksExternalLights
 *
 * @author  UBC Solar
 * @date    Feb 4 2026
 */

#ifndef __TASKS_H__
#define __TASKS_H__

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
 * Executes at a 25ms period. Task runs indefinitely until system shutdown.
 * @param argument Pointer to task-specific arguments (unused).
 */
void TasksDriveState(void* argument);

/**
 * @brief State-of-charge calculation task main loop.
 *
 * Calculates battery state-of-charge at a fixed interval.
 * Executes at a 50ms period dependent on task flag. Task runs 
 * indefinitely until system shutdown.
 * @param argument Pointer to task-specific arguments (unused).
 */
void TasksCalculateSoc(void* argument);

/**
 * @brief LCD update task main loop.
 *
 * Initializes the LCD Driver and manages the LCD page switching logic.
 * Executes indefinitely until system shutdown.
 * @param argument Pointer to task-specific arguments (unused).
 */
void TasksLcdUpdate(void *argument);

/**
 * @brief External lights task main loop.
 *
 * Runs the external lights state machine to control turn signals and brake lights.
 * Executes at a 50ms period. Task runs indefinitely until system shutdown.
 * @param argument Pointer to task-specific arguments (unused).
 */
void TasksExternalLights(void* argument);

/**
 * @brief GPIO external interrupt callback handler.
 *
 * Handles external GPIO interrupts and dispatches to appropriate handlers.
 * @param GPIO_Pin The pin number that triggered the interrupt.
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

#endif //__TASKS_H__