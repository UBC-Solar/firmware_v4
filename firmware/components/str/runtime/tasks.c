#include "tasks.h"

#include <stdint.h>

#include "can_app.h"
#include "hex_driver.h"
#include "hex_app.h"
#include "main.h"

enum
{
    STR_UI_PERIOD_MS = 100U,
};

static void StrInit(void)
{
    CanAppInit();
}

void AppMain(void)
{
    StrInit();
    HexDisplayInit();

    uint8_t count = 0U;

    for (;;)
    {
        HexDisplayWriteDecimal(count);
        HAL_Delay(STR_UI_PERIOD_MS);
        count++;
        if (count == 99U) {
            count = 0U;
        }
    }
}