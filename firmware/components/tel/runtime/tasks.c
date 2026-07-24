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
#include "usart.h"
#include "rtc.h"
#include "telemetry_app.h"
#include "diagnostics.h"

/* IMU TASK */
void TasksIMU(void* argument)
{
    (void)argument;

    imu_app_init();

    for (;;)
    {
        imu_app_task();
        osDelay(IMU_TASK_DELAY);
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
        DiagnosticsTimeSinceBootup();
        osDelay(TIME_SINCE_STARTUP_TASK_DELAY); // Delay for specified time
    }
}