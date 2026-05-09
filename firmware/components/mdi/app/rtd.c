#include "rtd.h"
#include "can_app.h"
#include "diagnostic.h"
#include "mdi_driver.h"
#include "main.h"

void AppMain(void)
{
    CanAppInit();
    g_mdi_diagnostic_flags.raw = 0;
    CanAppSendDiagnosticFlags();

    MdiStopMotor();
    g_mdi_last_command_tick = HAL_GetTick();
    uint32_t last_diagnostic_tick = HAL_GetTick();

    for (;;)
    {
        uint32_t now = HAL_GetTick();

        if ((uint32_t)(now - last_diagnostic_tick) >= MDI_DIAGNOSTICS_DELAY)
        {
            CanAppSendTimeSinceBootup();
            CanAppSendDiagnosticFlags();
            last_diagnostic_tick = now;
        }

        if ((uint32_t)(now - g_mdi_last_command_tick) >= MDI_MAX_TIMEOUT_VALUE)
        {
            MdiStopMotor();
        }

        if (g_mdi_motor_command_received)
        {
            MdiSetMotorCommand(&g_mdi_motor_command);
        }
    }
}