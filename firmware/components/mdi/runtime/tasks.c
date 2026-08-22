#include "tasks.h"

#include "can_app.h"
#include "can_driver.h"
#include "diagnostic.h"
#include "mdi_driver.h"
#include "rtd_driver.h"
#include "main.h"
#include "sunlite_ota_can_app.h"

void AppMain(void)
{
    CanAppInit();
    SunliteOtaCanAppInit(&hcan);
    RtdDriverInit();
    DiagnosticInit();
    DiagnosticSendFlags();

    MdiStopMotor();

    uint32_t last_diagnostic_tick = HAL_GetTick();

    for (;;)
    {
        SunliteOtaCanAppPoll();
        uint32_t now = HAL_GetTick();

        if ((uint32_t)(now - last_diagnostic_tick) >= MDI_DIAGNOSTICS_DELAY)
        {
            DiagnosticSendTimeSinceBootup();
            DiagnosticSendRtdTemp();
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
