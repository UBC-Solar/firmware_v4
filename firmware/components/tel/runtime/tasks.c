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
#include "iwdg_common.h"
#include "cmsis_os2.h"
#include "stm32f1xx_hal.h"
#include "usart.h"
#include "rtc.h"
#include "telemetry_app.h"
#include "diagnostics.h"
#include "imu_app.h"
#include "canload.h"
#include "gps_app.h"

/* CAN Load task */
void TasksCanload(void *argument)
{
    (void)argument;

    for(;;)
    {
        CanloadUpdateSlidingWindow();
        CAN_tx_canload_msg();
        osDelay(CANLOAD_TASK_DELAY);  
    }
}

/* IMU TASK */
void TasksIMU(void* argument)
{
    (void)argument;

    ImuAppInit();

    for (;;)
    {
        ImuAppTask();
        osDelay(IMU_TASK_DELAY);
    }
}

/* GPS TASK */
void TasksGPS(void* argument)
{
    (void)argument; // Unused parameter

    GpsAppInit();

    for (;;)
    {
        GpsAppTask();
        osDelay(GPS_TASK_DELAY);
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

void TasksIWDG(void *argument)
{
    IwdgResetHandle();

    for (;;)
    {
        // Refresh the watchdog PPtimer to prevent reset and transmit diagnostics over CAN
        IwdgRefresh(&hiwdg);
        osDelay(IWDG_TASK_DELAY);
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
