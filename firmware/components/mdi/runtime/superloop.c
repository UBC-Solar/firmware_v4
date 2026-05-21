#include "superloop.h"

#include "can_app.h"
#include "diagnostic.h"
#include "iwdg_app.h"
#include "mdi_driver.h"
#include "main.h"

void AppMain(void)
{
    CanAppInit();
    DiagnosticInit();
    IwdgAppInit();
    IwdgAppResetHandle();
    DiagnosticSendFlags();

    MdiStopMotor();

    uint32_t last_diagnostic_tick = HAL_GetTick();

    for (;;)
    {
        uint32_t now = HAL_GetTick();

        IwdgAppRefresh();

        if ((uint32_t)(now - last_diagnostic_tick) >= MDI_DIAGNOSTICS_DELAY)
        {
            DiagnosticSendTimeSinceBootup();
            DiagnosticSendFlags();
            last_diagnostic_tick = now;
        }

        if ((uint32_t)(now - CanAppGetLastCommandTick()) >= MDI_MAX_TIMEOUT_VALUE)
        {
            MdiStopMotor();
        }

        MdiMotorCommand command;
        if (CanAppTryGetMotorCommand(&command))
        {
            MdiSetMotorCommand(&command);
        }
    }
}
