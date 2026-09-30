#include "bootloader_boot_request.h"

#include "stm32f1xx_hal.h"

#define FW_UPDATE_BOOT_REQUEST_MAGIC 0x5A7EU
#define FW_UPDATE_TRIAL_ARMED         0xA17EU
#define FW_UPDATE_TRIAL_ARMED_CHECK   0x5E81U
#define FW_UPDATE_TRIAL_LAUNCHED      0x1BADU
#define FW_UPDATE_TRIAL_LAUNCHED_CHECK 0xE452U

#define FW_UPDATE_NON_SOFTWARE_RESET_FLAGS \
    (RCC_CSR_LPWRRSTF | RCC_CSR_WWDGRSTF | RCC_CSR_IWDGRSTF | \
     RCC_CSR_PORRSTF)

typedef enum {
    FW_UPDATE_TRIAL_NONE,
    FW_UPDATE_TRIAL_ARMED_STATE,
    FW_UPDATE_TRIAL_LAUNCHED_STATE,
    FW_UPDATE_TRIAL_INVALID,
} FirmwareUpdateTrialState;

#ifndef FW_UPDATE_BKP_WRITE
#define FW_UPDATE_BKP_WRITE(register_pointer, value) \
    (*(register_pointer) = (uint32_t)(value))
#endif

static void EnableBackupRegisters(void)
{
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_RCC_BKP_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();
}

static FirmwareUpdateTrialState ReadTrialState(void)
{
    EnableBackupRegisters();
    uint16_t marker = (uint16_t)(BKP->DR8 & 0xFFFFU);
    uint16_t check = (uint16_t)(BKP->DR9 & 0xFFFFU);
    if ((marker == 0U) && (check == 0U)) {
        return FW_UPDATE_TRIAL_NONE;
    }
    if ((marker == FW_UPDATE_TRIAL_ARMED) &&
        (check == FW_UPDATE_TRIAL_ARMED_CHECK)) {
        return FW_UPDATE_TRIAL_ARMED_STATE;
    }
    if ((marker == FW_UPDATE_TRIAL_LAUNCHED) &&
        (check == FW_UPDATE_TRIAL_LAUNCHED_CHECK)) {
        return FW_UPDATE_TRIAL_LAUNCHED_STATE;
    }
    return FW_UPDATE_TRIAL_INVALID;
}

static bool WriteTrialState(uint16_t marker, uint16_t check)
{
    EnableBackupRegisters();

    /* Write the check word first when creating/changing a marker and the
     * marker first when clearing it.  A reset between writes leaves an
     * invalid pair, which the bootloader treats as recovery-only. */
    if ((marker == 0U) && (check == 0U)) {
        FW_UPDATE_BKP_WRITE(&BKP->DR8, 0U);
        __DSB();
        FW_UPDATE_BKP_WRITE(&BKP->DR9, 0U);
    } else {
        FW_UPDATE_BKP_WRITE(&BKP->DR9, check);
        __DSB();
        FW_UPDATE_BKP_WRITE(&BKP->DR8, marker);
    }
    __DSB();
    return ((BKP->DR8 & 0xFFFFU) == marker) &&
           ((BKP->DR9 & 0xFFFFU) == check);
}

void FirmwareUpdateRequestBootloader(void)
{
    EnableBackupRegisters();

    /* Reset flags accumulate until RMVF is written.  Clear the application's
     * history immediately before arming the token so the bootloader can prove
     * that the following reset was the requested software reset. */
    __HAL_RCC_CLEAR_RESET_FLAGS();
    BKP->DR10 = FW_UPDATE_BOOT_REQUEST_MAGIC;
    __DSB();
}

bool FirmwareUpdateConsumeBootloaderRequest(void)
{
    EnableBackupRegisters();
    if ((BKP->DR10 & 0xFFFFU) != FW_UPDATE_BOOT_REQUEST_MAGIC) {
        /* The application owns reset-cause diagnostics.  In particular, DRD
         * and STR must still see IWDGRSTF after the bootloader hands off. */
        return false;
    }

    uint32_t reset_flags = RCC->CSR;
    /* STM32F103 commonly sets PINRSTF as a companion to SFTRSTF after
     * NVIC_SystemReset().  SFTRSTF is the positive authorization signal;
     * PINRSTF alone still fails because the software flag is required. */
    bool requested =
        ((reset_flags & RCC_CSR_SFTRSTF) != 0U) &&
        ((reset_flags & FW_UPDATE_NON_SOFTWARE_RESET_FLAGS) == 0U);

    /* Consume even a rejected token.  A token retained through a pin,
     * watchdog, brownout, or power reset must never authorize a later boot. */
    BKP->DR10 = 0U;
    __DSB();
    /* Do not clear RCC reset flags here.  The request path cleared them just
     * before reset, and preserving the observed cause lets the application
     * perform its normal reset diagnostics after either outcome. */
    return requested;
}

bool FirmwareUpdateArmTrialBoot(void)
{
    return WriteTrialState(FW_UPDATE_TRIAL_ARMED,
                           FW_UPDATE_TRIAL_ARMED_CHECK);
}

bool FirmwareUpdatePrepareAppLaunch(void)
{
    FirmwareUpdateTrialState state = ReadTrialState();
    if (state == FW_UPDATE_TRIAL_NONE) {
        return true;
    }
    if (state != FW_UPDATE_TRIAL_ARMED_STATE) {
        return false;
    }
    return WriteTrialState(FW_UPDATE_TRIAL_LAUNCHED,
                           FW_UPDATE_TRIAL_LAUNCHED_CHECK);
}

bool FirmwareUpdateTrialBootRequiresRecovery(void)
{
    FirmwareUpdateTrialState state = ReadTrialState();
    return (state == FW_UPDATE_TRIAL_LAUNCHED_STATE) ||
           (state == FW_UPDATE_TRIAL_INVALID);
}

bool FirmwareUpdateConfirmTrialBoot(void)
{
    FirmwareUpdateTrialState state = ReadTrialState();
    if (state == FW_UPDATE_TRIAL_NONE) {
        return true;
    }
    if (state != FW_UPDATE_TRIAL_LAUNCHED_STATE) {
        return false;
    }
    return WriteTrialState(0U, 0U);
}
