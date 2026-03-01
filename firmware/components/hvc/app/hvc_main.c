#include "debug_io.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>

void hvcMain(void)
{
    uint32_t sum = 1;
    DEBUG_IO_PRINT("test,%d\n\r", sum);
    DEBUG_IO_PRINT("Hello from hvcMain0!\r\n");
    DEBUG_IO_PRINT("Hello from hvcMain2!\n\r");
    uint32_t time = HAL_GetTick();
    DEBUG_IO_PRINT("Current time: %d ms\n\r", time);
}
