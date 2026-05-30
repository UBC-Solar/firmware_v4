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
        DiagnosticsTimeSinceBootup();
        osDelay(TIME_SINCE_STARTUP_TASK_DELAY); // Delay for specified time
    }
}