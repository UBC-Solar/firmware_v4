#include "tasks.h"
#include "CAN_comms.h"
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
    }
}

/* DIAGNOSTICS TASK */
void TasksDiagnostics(void* argument)
{
    (void)argument; // Unused parameter

    for (;;)
    {
        DiagnosticsTransmit();
        osDelay(TIME_SINCE_STARTUP_TASK_DELAY);
    }
}

/* TEL HEARTBEAT TASK */
void TimeSinceStartup(void* argument)
{
    (void)argument; // Unused parameter

    for (;;)
    {
        osDelay(TIME_SINCE_STARTUP_TASK_DELAY); // Delay for specified time
    }
}