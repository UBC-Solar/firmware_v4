#include "bootloader.h"
#include "stm32f1xx_hal.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define BOOT_REQUEST_MAGIC 0x5A7EU
BKP_TypeDef test_bkp_registers;
RCC_TypeDef test_rcc_registers;

static unsigned clear_reset_flags_calls;

void TestHalClearResetFlags(void)
{
    clear_reset_flags_calls++;
    test_rcc_registers.CSR = 0U;
}

void HAL_PWR_EnableBkUpAccess(void) {}

static void ResetFixture(void)
{
    memset(&test_bkp_registers, 0, sizeof(test_bkp_registers));
    memset(&test_rcc_registers, 0, sizeof(test_rcc_registers));
    clear_reset_flags_calls = 0U;
}

static void TestBootRequestRequiresSoftwareReset(void)
{
    ResetFixture();
    test_rcc_registers.CSR = RCC_CSR_IWDGRSTF;
    assert(!FirmwareUpdateConsumeBootloaderRequest());
    assert(test_rcc_registers.CSR == RCC_CSR_IWDGRSTF);
    assert(clear_reset_flags_calls == 0U);

    test_rcc_registers.CSR = RCC_CSR_PORRSTF;
    FirmwareUpdateRequestBootloader();
    assert(clear_reset_flags_calls == 1U);
    assert((test_bkp_registers.DR10 & 0xFFFFU) == BOOT_REQUEST_MAGIC);

    test_rcc_registers.CSR = RCC_CSR_SFTRSTF | RCC_CSR_PINRSTF;
    assert(FirmwareUpdateConsumeBootloaderRequest());
    assert(test_bkp_registers.DR10 == 0U);
    assert(test_rcc_registers.CSR == (RCC_CSR_SFTRSTF | RCC_CSR_PINRSTF));
    assert(clear_reset_flags_calls == 1U);

    FirmwareUpdateRequestBootloader();
    test_rcc_registers.CSR = RCC_CSR_SFTRSTF | RCC_CSR_IWDGRSTF;
    assert(!FirmwareUpdateConsumeBootloaderRequest());
    assert(test_bkp_registers.DR10 == 0U);
    assert(test_rcc_registers.CSR == (RCC_CSR_SFTRSTF | RCC_CSR_IWDGRSTF));

    FirmwareUpdateRequestBootloader();
    test_rcc_registers.CSR = RCC_CSR_PINRSTF;
    assert(!FirmwareUpdateConsumeBootloaderRequest());
    assert(test_bkp_registers.DR10 == 0U);
}

int main(void)
{
    TestBootRequestRequiresSoftwareReset();
    puts("Firmware update boot-entry request tests passed");
    return 0;
}
