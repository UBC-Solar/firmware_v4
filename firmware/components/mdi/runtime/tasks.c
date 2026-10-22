#include "tasks.h"

#include "can_app.h"
#include "diagnostic.h"
#include "mcu_health_driver.h"
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
    McuHealthDriverInit();
    DiagnosticInit();

    MdiStopMotor();

    DiagnosticSendFlags();

    uint32_t last_diagnostic_tick = HAL_GetTick();
    uint32_t last_iwdg_tick = HAL_GetTick();

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
            DiagnosticSendMcuHealth();
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
