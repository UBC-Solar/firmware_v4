#include "bootloader.h"

#include "bootloader_boot_request.h"
#include "stm32f1xx_hal.h"

/*
 * Keep the MSP write and the branch in one naked function.  A normal C
 * function may restore its own stack frame after __set_MSP(), which would
 * read the application's (not the bootloader's) stack and corrupt the
 * handoff.  The AAPCS passes app_stack in r0 and app_reset in r1, so this
 * helper has no compiler-generated prologue/epilogue and cannot touch the
 * stack after installing the application's MSP.
 */
__attribute__((naked, noinline, noreturn))
static void BootloaderBranchToApp(uint32_t app_stack, uint32_t app_reset)
{
    __asm volatile(
        "msr msp, r0\n"
        "cpsie i\n"
        "bx r1\n"
    );
}

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
        (app_reset_address <
         (BOOTLOADER_APP_START_ADDRESS + BOOTLOADER_APP_MAX_SIZE_BYTES));

    return stack_valid && reset_valid;
}

bool BootloaderShouldEnterUpdateMode(void)
{
    if (!BootloaderAppIsValid()) {
        /* An interrupted/blank application must remain recoverable. */
        return true;
    }

    /* Consume/reject the one-shot entry token even when trial recovery also
     * requires us to stay.  This prevents a valid DR10 token from surviving
     * into a later, unrelated software reset. */
    bool board_requests_update = BootloaderBoardStayInBootloader();

    if (SunliteOtaTrialBootRequiresRecovery()) {
        /* The trial image already ran without completing a targeted
         * BOARD_INFO response, or its redundant marker was torn/corrupted. */
        return true;
    }

    /* With a valid application, only a board-approved boot token or an
     * explicit physical recovery hook may enter update mode.  Listening for
     * network/UART requests on every ordinary reset would bypass the safety
     * interlock enforced by the running application. */
    return board_requests_update;
}

void BootloaderPrepareWatchdog(void)
{
    /* DRD/STR can arrive here with IWDG already running across software reset.
     * Extend it to the STM32F1 maximum period before signature verification.
     * Programming these registers does not start a previously stopped IWDG. */
    IWDG->KR = 0x5555U;
    IWDG->PR = 0x06U; /* STM32F1 PR encoding for divider /256. */
    IWDG->RLR = 0x0FFFU;
    uint32_t wait = 100000U;
    while (((IWDG->SR & (IWDG_SR_PVU | IWDG_SR_RVU)) != 0U) &&
           (wait-- > 0U)) {
    }
    BootloaderServiceWatchdog();
}

void BootloaderServiceWatchdog(void)
{
    /* IWDG keeps running across a software reset on STM32F1.  Writing the
     * reload key is harmless when it has not been started and prevents the
     * DRD/STR release watchdog from interrupting a CAN update. */
    IWDG->KR = 0xAAAAU;
}

void BootloaderJumpToApp(void)
{
    /* This check is deliberately centralized: protocol REBOOT, normal boot,
     * and transport idle fallback must all obey the one-launch limit. */
    if (!BootloaderAppIsValid() || !SunliteOtaPrepareAppLaunch()) {
        return;
    }

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

    /* SysTick and PendSV are system exceptions, not NVIC external IRQs. */
    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk | SCB_ICSR_PENDSVCLR_Msk;
    SCB->VTOR = BOOTLOADER_APP_START_ADDRESS;
    __DSB();
    __ISB();

    BootloaderBranchToApp(app_stack, app_reset);
}

void BootloaderRun(void)
{
    BootloaderPrepareWatchdog();
    if (!BootloaderShouldEnterUpdateMode()) {
        /* Re-check the vector immediately before the irreversible handoff. */
        if (BootloaderAppIsValid()) {
            BootloaderJumpToApp();
        }
    }

    BootloaderEnterUpdateMode();
}

__attribute__((weak)) bool BootloaderBoardStayInBootloader(void)
{
    return false;
}

__attribute__((weak)) void BootloaderEnterUpdateMode(void)
{
    while (1) {
        BootloaderServiceWatchdog();
    }
}
