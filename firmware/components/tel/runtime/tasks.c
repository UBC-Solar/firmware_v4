/**
 * @file    tasks.c
 * @brief   FreeRTOS task implementations for TEL board application logic
 *
 * This file contains the implementation of all FreeRTOS tasks for this board component of UBC Solar
 * firmware. Each task represents a concurrent execution thread that runs indefinitely within the
 * real-time operating system.
 */

#include "tasks.h"
#include "CAN_comms.h"
#include "cmsis_os2.h"
#include "main.h"
#include "usart.h"
#include "rtc.h"
#include "telemetry_app.h"
#include "diagnostics.h"

/* IMU TASK */
void TasksIMU(void* argument)
{
    (void)argument; // Unused parameter

    for (;;)
    {
        // TODO: Implement IMU data acquisition and processing
        osDelay(osWaitForever);
    }
}

/* DIAGNOSTICS TASK */
void TasksDiagnostics(void* argument)
{
    (void)argument; // Unused parameter

    for (;;)
    {
        DiagnosticsSendTelFlags();
        osDelay(DIAGNOSTICS_TASK_DELAY);
    }
}

/* TEL HEARTBEAT TASK */
void TimeSinceStartup(void* argument)
{
    (void)argument; // Unused parameter

    for (;;)
    {
        /* Visible application heartbeat: PB14 is TEL's dedicated debug LED.
         * Toggling once per task period produces one second on, one second off. */
        HAL_GPIO_TogglePin(DEBUG_LED_1_GPIO_Port, DEBUG_LED_1_Pin);
        DiagnosticsTimeSinceBootup();
        osDelay(TIME_SINCE_STARTUP_TASK_DELAY); // Delay for specified time
    }
}
