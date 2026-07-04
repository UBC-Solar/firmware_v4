#include "main.h"
#include <stdint.h>

#define APP_START_ADDRESS   0x08008000U
#define SRAM_START          0x20000000U
#define SRAM_SIZE           (20U * 1024U)
#define SRAM_END            (SRAM_START + SRAM_SIZE)

typedef void (*app_entry_t)(void);

static int app_is_valid(void)
{
    uint32_t app_stack = *(uint32_t *)APP_START_ADDRESS;
    uint32_t app_reset = *(uint32_t *)(APP_START_ADDRESS + 4);

    int stack_valid = (app_stack >= SRAM_START) && (app_stack <= SRAM_END);
    int reset_valid = (app_reset >= APP_START_ADDRESS) && (app_reset < 0x08020000U);

    return stack_valid && reset_valid;
}

static void jump_to_app(void)
{
    uint32_t app_stack = *(uint32_t *)APP_START_ADDRESS;
    uint32_t app_reset = *(uint32_t *)(APP_START_ADDRESS + 4);

    HAL_DeInit();

    __disable_irq();

    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    for (int i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }

    SCB->VTOR = APP_START_ADDRESS;

    __set_MSP(app_stack);

    app_entry_t app_entry = (app_entry_t)app_reset;
    app_entry();
}