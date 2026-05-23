#include "superloop.h"

#include <stdint.h>

#include "can_app.h"
#include "main.h"

enum
{
    STR_UI_PERIOD_MS = 10U,
};

static void StrInit(void)
{
    CanAppInit();
}

static void StrRunUi(void)
{
    CanAppTransmitNextPage();
}

void AppMain(void)
{
    StrInit();

    for (;;)
    {
        StrRunUi();
        HAL_Delay(STR_UI_PERIOD_MS);
    }
}