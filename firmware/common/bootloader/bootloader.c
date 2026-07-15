#include "bootloader.h"

#include "stm32f1xx_hal.h"

typedef void (*app_entry_t)(void);

bool BootloaderAppIsValid(void)
{
    uint32_t app_stack = *(uint32_t *)BOOTLOADER_APP_START_ADDRESS;
    uint32_t app_reset = *(uint32_t *)(BOOTLOADER_APP_START_ADDRESS + 4U);
    uint32_t app_reset_address = app_reset & ~1U;

    bool stack_valid =
        (app_stack >= BOOTLOADER_SRAM_START_ADDRESS) &&
        (app_stack <= BOOTLOADER_SRAM_END_ADDRESS) &&
        ((app_stack & 0x7U) == 0U);

    bool reset_valid =
        ((app_reset & 0x1U) != 0U) &&
        (app_reset_address >= BOOTLOADER_APP_START_ADDRESS) &&
        (app_reset_address < BOOTLOADER_FLASH_END_ADDRESS);

    return stack_valid && reset_valid;
}

bool BootloaderShouldEnterUpdateMode(void)
{
    if (!BootloaderAppIsValid()) {
        return true;
    }

    if (BootloaderBoardStayInBootloader()) {
        return true;
    }

    return BootloaderWaitForUpdateRequest(BOOTLOADER_UPDATE_WINDOW_MS);
}

void BootloaderJumpToApp(void)
{
    uint32_t app_stack = *(uint32_t *)BOOTLOADER_APP_START_ADDRESS;
    uint32_t app_reset = *(uint32_t *)(BOOTLOADER_APP_START_ADDRESS + 4U);

    HAL_DeInit();

    __disable_irq();

    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    for (int i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }

    SCB->VTOR = BOOTLOADER_APP_START_ADDRESS;
    __DSB();
    __ISB();

    __set_MSP(app_stack);

    __enable_irq();

    app_entry_t app_entry = (app_entry_t)app_reset;
    app_entry();
}

void BootloaderRun(void)
{
    if (!BootloaderShouldEnterUpdateMode()) {
        BootloaderJumpToApp();
    }

    BootloaderEnterUpdateMode();
}

__attribute__((weak)) bool BootloaderBoardStayInBootloader(void)
{
    return false;
}

__attribute__((weak)) bool BootloaderWaitForUpdateRequest(uint32_t timeout_ms)
{
    (void)timeout_ms;
    return false;
}

__attribute__((weak)) void BootloaderEnterUpdateMode(void)
{
    while (1) {
    }
}
