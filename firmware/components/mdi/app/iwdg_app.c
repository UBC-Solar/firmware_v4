/**
 * @file    iwdg_app.c
 * @brief   MDI independent watchdog application logic.
 */
#include "iwdg_app.h"

#include "diagnostic.h"
#include "iwdg_driver.h"

void IwdgAppInit(void)
{
    IwdgDriverInit();
}

void IwdgAppRefresh(void)
{
    IwdgDriverRefresh();
}

void IwdgAppResetHandle(void)
{
    if (IwdgDriverIsReset())
    {
        DiagnosticSetIwdgCrash(true);
    }
}
