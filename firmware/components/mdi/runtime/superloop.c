/**
 * @file    superloop.c
 * @brief   MDI superloop implementation.
 */
#include "superloop.h"

#include "can_app.h"
#include "diagnostic.h"
#include "iwdg_app.h"
#include "mdi_app.h"
#include "main.h"

void AppMain(void)
{
    CanAppInit();
    DiagnosticInit();
    IwdgAppInit();
    IwdgAppResetHandle();
    DiagnosticSendFlags();

    MdiAppStopMotor();

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
            MdiAppStopMotor();
        }

        MdiMotorCommand command;
        if (CanAppTryGetMotorCommand(&command))
        {
            MdiAppSetMotorCommand(&command);
        }
    }
}
