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
    IwdgResetHandle();
    CanAppInit();
    RtdDriverInit();
    DiagnosticInit();

    MdiStopMotor();

    DiagnosticSendFlags();

    uint32_t last_diagnostic_tick = HAL_GetTick();

    for (;;)
    {
          
        uint32_t now = HAL_GetTick();

        IwdgRefresh(&hiwdg);

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
