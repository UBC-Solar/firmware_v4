#include "bootloader.h"

#include "stm32f1xx_hal.h"

int main(void)
{
    HAL_Init();
    BootloaderRun();

    while (1) {
    }
}
