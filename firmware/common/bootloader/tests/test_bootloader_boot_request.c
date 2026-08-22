#include "bootloader_boot_request.h"
#include "stm32f1xx_hal.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define BOOT_REQUEST_MAGIC 0x5A7EU
#define TRIAL_ARMED        0xA17EU
#define TRIAL_ARMED_CHECK  0x5E81U
#define TRIAL_LAUNCHED     0x1BADU
#define TRIAL_LAUNCHED_CHECK 0xE452U

BKP_TypeDef test_bkp_registers;
RCC_TypeDef test_rcc_registers;

static unsigned clear_reset_flags_calls;
static unsigned backup_write_calls;
static unsigned fail_backup_write_call;

void TestHalClearResetFlags(void)
{
    clear_reset_flags_calls++;
    test_rcc_registers.CSR = 0U;
}

void TestHalBackupWrite(volatile uint32_t *destination, uint16_t value)
{
    backup_write_calls++;
    if (backup_write_calls != fail_backup_write_call) {
        *destination = value;
    }
}

void HAL_PWR_EnableBkUpAccess(void)
{
}

static void ResetFixture(void)
{
    memset(&test_bkp_registers, 0, sizeof(test_bkp_registers));
    memset(&test_rcc_registers, 0, sizeof(test_rcc_registers));
    clear_reset_flags_calls = 0U;
    backup_write_calls = 0U;
    fail_backup_write_call = 0U;
}

static void TestOneTrialLaunchAndConfirmation(void)
{
    ResetFixture();
    assert(SunliteOtaArmTrialBoot());
    assert((test_bkp_registers.DR8 & 0xFFFFU) == TRIAL_ARMED);
    assert((test_bkp_registers.DR9 & 0xFFFFU) == TRIAL_ARMED_CHECK);
    assert(!SunliteOtaTrialBootRequiresRecovery());

    assert(SunliteOtaPrepareAppLaunch());
    assert((test_bkp_registers.DR8 & 0xFFFFU) == TRIAL_LAUNCHED);
    assert((test_bkp_registers.DR9 & 0xFFFFU) == TRIAL_LAUNCHED_CHECK);
    assert(SunliteOtaTrialBootRequiresRecovery());
    assert(!SunliteOtaPrepareAppLaunch());

    assert(SunliteOtaConfirmTrialBoot());
    assert(test_bkp_registers.DR8 == 0U);
    assert(test_bkp_registers.DR9 == 0U);
    assert(!SunliteOtaTrialBootRequiresRecovery());
    assert(SunliteOtaPrepareAppLaunch());
}

static void TestTornMarkerFailsClosed(void)
{
    ResetFixture();
    test_bkp_registers.DR8 = TRIAL_ARMED;
    test_bkp_registers.DR9 = 0U;
    assert(SunliteOtaTrialBootRequiresRecovery());
    assert(!SunliteOtaPrepareAppLaunch());
    assert(!SunliteOtaConfirmTrialBoot());

    /* A subsequent authenticated update can replace a torn marker. */
    assert(SunliteOtaArmTrialBoot());
    assert(!SunliteOtaTrialBootRequiresRecovery());
}

static void TestTrialWriteReadbackFailureFailsClosed(void)
{
    ResetFixture();
    fail_backup_write_call = 2U;
    assert(!SunliteOtaArmTrialBoot());
    assert(SunliteOtaTrialBootRequiresRecovery());
    assert(!SunliteOtaPrepareAppLaunch());

    ResetFixture();
    assert(SunliteOtaArmTrialBoot());
    assert(SunliteOtaPrepareAppLaunch());
    backup_write_calls = 0U;
    fail_backup_write_call = 2U;
    assert(!SunliteOtaConfirmTrialBoot());
    assert(SunliteOtaTrialBootRequiresRecovery());
}

static void TestBootRequestRequiresSoftwareReset(void)
{
    ResetFixture();
    test_rcc_registers.CSR = RCC_CSR_IWDGRSTF;
    assert(!SunliteOtaConsumeBootloaderRequest());
    assert(test_rcc_registers.CSR == RCC_CSR_IWDGRSTF);
    assert(clear_reset_flags_calls == 0U);

    test_rcc_registers.CSR = RCC_CSR_PORRSTF;
    SunliteOtaRequestBootloader();
    assert(clear_reset_flags_calls == 1U);
    assert((test_bkp_registers.DR10 & 0xFFFFU) == BOOT_REQUEST_MAGIC);

    test_rcc_registers.CSR = RCC_CSR_SFTRSTF | RCC_CSR_PINRSTF;
    assert(SunliteOtaConsumeBootloaderRequest());
    assert(test_bkp_registers.DR10 == 0U);
    assert(test_rcc_registers.CSR ==
           (RCC_CSR_SFTRSTF | RCC_CSR_PINRSTF));
    assert(clear_reset_flags_calls == 1U);

    SunliteOtaRequestBootloader();
    test_rcc_registers.CSR = RCC_CSR_SFTRSTF | RCC_CSR_IWDGRSTF;
    assert(!SunliteOtaConsumeBootloaderRequest());
    assert(test_bkp_registers.DR10 == 0U);
    assert(test_rcc_registers.CSR ==
           (RCC_CSR_SFTRSTF | RCC_CSR_IWDGRSTF));

    SunliteOtaRequestBootloader();
    test_rcc_registers.CSR = RCC_CSR_PINRSTF;
    assert(!SunliteOtaConsumeBootloaderRequest());
    assert(test_bkp_registers.DR10 == 0U);
}

int main(void)
{
    TestOneTrialLaunchAndConfirmation();
    TestTornMarkerFailsClosed();
    TestTrialWriteReadbackFailureFailsClosed();
    TestBootRequestRequiresSoftwareReset();
    puts("Sunlite OTA trial-boot marker tests passed");
    return 0;
}
