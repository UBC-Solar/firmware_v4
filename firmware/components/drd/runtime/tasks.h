/**
 * @file    tasks.h
 * @brief   FreeRTOS task declarations for DRD board application logic
 *
 * This header declares all FreeRTOS task prototypes for this board component of UBC Solar
 * firmware. Each task represents a concurrent execution thread that runs indefinitely within the
 * real-time operating system.
 */

#ifndef __TASKS_H__
#define __TASKS_H__

#define DIAGNOSTIC_TASK_DELAY 100
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

/* INCLUDES */
#include "cmsis_os2.h"
#include "drive_state.h"
#include "lcd_app.h"
#include "can_app.h"
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

void TasksDiagnostic(void *argument);

void TasksTimeSinceStartup(void *argument);

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
 * Configures CAN filters and registers the RX callback path, then initializes
 * the CAN communications layer so CAN RX/TX handling is ready before runtime
 * tasks begin normal operation.
 */
void TasksCanInit(void);

#endif //__TASKS_H__