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
    assert(FirmwareUpdateArmTrialBoot());
    assert((test_bkp_registers.DR8 & 0xFFFFU) == TRIAL_ARMED);
    assert((test_bkp_registers.DR9 & 0xFFFFU) == TRIAL_ARMED_CHECK);
    assert(!FirmwareUpdateTrialBootRequiresRecovery());

    assert(FirmwareUpdatePrepareAppLaunch());
    assert((test_bkp_registers.DR8 & 0xFFFFU) == TRIAL_LAUNCHED);
    assert((test_bkp_registers.DR9 & 0xFFFFU) == TRIAL_LAUNCHED_CHECK);
    assert(FirmwareUpdateTrialBootRequiresRecovery());
    assert(!FirmwareUpdatePrepareAppLaunch());

    assert(FirmwareUpdateConfirmTrialBoot());
    assert(test_bkp_registers.DR8 == 0U);
    assert(test_bkp_registers.DR9 == 0U);
    assert(!FirmwareUpdateTrialBootRequiresRecovery());
    assert(FirmwareUpdatePrepareAppLaunch());
}

static void TestTornMarkerFailsClosed(void)
{
    ResetFixture();
    test_bkp_registers.DR8 = TRIAL_ARMED;
    test_bkp_registers.DR9 = 0U;
    assert(FirmwareUpdateTrialBootRequiresRecovery());
    assert(!FirmwareUpdatePrepareAppLaunch());
    assert(!FirmwareUpdateConfirmTrialBoot());

    /* A subsequent authenticated update can replace a torn marker. */
    assert(FirmwareUpdateArmTrialBoot());
    assert(!FirmwareUpdateTrialBootRequiresRecovery());
}

static void TestTrialWriteReadbackFailureFailsClosed(void)
{
    ResetFixture();
    fail_backup_write_call = 2U;
    assert(!FirmwareUpdateArmTrialBoot());
    assert(FirmwareUpdateTrialBootRequiresRecovery());
    assert(!FirmwareUpdatePrepareAppLaunch());

    ResetFixture();
    assert(FirmwareUpdateArmTrialBoot());
    assert(FirmwareUpdatePrepareAppLaunch());
    backup_write_calls = 0U;
    fail_backup_write_call = 2U;
    assert(!FirmwareUpdateConfirmTrialBoot());
    assert(FirmwareUpdateTrialBootRequiresRecovery());
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
    assert(test_rcc_registers.CSR ==
           (RCC_CSR_SFTRSTF | RCC_CSR_PINRSTF));
    assert(clear_reset_flags_calls == 1U);

    FirmwareUpdateRequestBootloader();
    test_rcc_registers.CSR = RCC_CSR_SFTRSTF | RCC_CSR_IWDGRSTF;
    assert(!FirmwareUpdateConsumeBootloaderRequest());
    assert(test_bkp_registers.DR10 == 0U);
    assert(test_rcc_registers.CSR ==
           (RCC_CSR_SFTRSTF | RCC_CSR_IWDGRSTF));

    FirmwareUpdateRequestBootloader();
    test_rcc_registers.CSR = RCC_CSR_PINRSTF;
    assert(!FirmwareUpdateConsumeBootloaderRequest());
    assert(test_bkp_registers.DR10 == 0U);
}

int main(void)
{
    TestOneTrialLaunchAndConfirmation();
    TestTornMarkerFailsClosed();
    TestTrialWriteReadbackFailureFailsClosed();
    TestBootRequestRequiresSoftwareReset();
    puts("Firmware update trial-boot marker tests passed");
    return 0;
}
