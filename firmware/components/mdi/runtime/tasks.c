#include "tasks.h"

#include "can_app.h"
#include "diagnostic.h"
#include "mdi_driver.h"
#include "rtd_driver.h"
#include "main.h"
#include "iwdg_common.h"
#include "stm32f1xx_hal.h"
#include "gpio_driver.h"

void AppMain(void)
{
    CanAppInit();
    RtdDriverInit();
    DiagnosticInit();

    MdiStopMotor();

    DiagnosticSendFlags();

    uint32_t last_diagnostic_tick = HAL_GetTick();
    uint32_t last_iwdg_tick = HAL_GetTick();
    // Half a period out of phase with the diagnostics burst, which already fills all 3 Tx mailboxes
    uint32_t last_commit_hash_tick = last_diagnostic_tick - (MDI_DIAGNOSTICS_DELAY / 2U);

    DiagnosticSetIwdgCrash(IwdgResetHandle());

    for (;;)
    {
          
        uint32_t now = HAL_GetTick();

        if ((uint32_t)(now - last_iwdg_tick) >= IWDG_REFRESH_DELAY){
        IwdgRefresh(&hiwdg);
        last_iwdg_tick = now;
        }

        if ((uint32_t)(now - last_diagnostic_tick) >= MDI_DIAGNOSTICS_DELAY)
        {
            DiagnosticSendTimeSinceBootup();
            DiagnosticSendRtdTemp();
            DiagnosticSendFlags();
            last_diagnostic_tick = now;
        }

        if ((uint32_t)(now - last_commit_hash_tick) >= MDI_DIAGNOSTICS_DELAY)
        {
            DiagnosticSendCommitHash();
            last_commit_hash_tick = now;
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
