#include "bootloader_boot_request.h"

#include "stm32f1xx_hal.h"

#define SUNLITE_OTA_BOOT_REQUEST_MAGIC 0x5A7EU
#define SUNLITE_OTA_TRIAL_ARMED         0xA17EU
#define SUNLITE_OTA_TRIAL_ARMED_CHECK   0x5E81U
#define SUNLITE_OTA_TRIAL_LAUNCHED      0x1BADU
#define SUNLITE_OTA_TRIAL_LAUNCHED_CHECK 0xE452U

#define SUNLITE_OTA_NON_SOFTWARE_RESET_FLAGS \
    (RCC_CSR_LPWRRSTF | RCC_CSR_WWDGRSTF | RCC_CSR_IWDGRSTF | \
     RCC_CSR_PORRSTF)

typedef enum {
    SUNLITE_OTA_TRIAL_NONE,
    SUNLITE_OTA_TRIAL_ARMED_STATE,
    SUNLITE_OTA_TRIAL_LAUNCHED_STATE,
    SUNLITE_OTA_TRIAL_INVALID,
} SunliteOtaTrialState;

#ifndef SUNLITE_OTA_BKP_WRITE
#define SUNLITE_OTA_BKP_WRITE(register_pointer, value) \
    (*(register_pointer) = (uint32_t)(value))
#endif

static void EnableBackupRegisters(void)
{
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_RCC_BKP_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();
}

static SunliteOtaTrialState ReadTrialState(void)
{
    EnableBackupRegisters();
    uint16_t marker = (uint16_t)(BKP->DR8 & 0xFFFFU);
    uint16_t check = (uint16_t)(BKP->DR9 & 0xFFFFU);
    if ((marker == 0U) && (check == 0U)) {
        return SUNLITE_OTA_TRIAL_NONE;
    }
    if ((marker == SUNLITE_OTA_TRIAL_ARMED) &&
        (check == SUNLITE_OTA_TRIAL_ARMED_CHECK)) {
        return SUNLITE_OTA_TRIAL_ARMED_STATE;
    }
    if ((marker == SUNLITE_OTA_TRIAL_LAUNCHED) &&
        (check == SUNLITE_OTA_TRIAL_LAUNCHED_CHECK)) {
        return SUNLITE_OTA_TRIAL_LAUNCHED_STATE;
    }
    return SUNLITE_OTA_TRIAL_INVALID;
}

static bool WriteTrialState(uint16_t marker, uint16_t check)
{
    EnableBackupRegisters();

    /* Write the check word first when creating/changing a marker and the
     * marker first when clearing it.  A reset between writes leaves an
     * invalid pair, which the bootloader treats as recovery-only. */
    if ((marker == 0U) && (check == 0U)) {
        SUNLITE_OTA_BKP_WRITE(&BKP->DR8, 0U);
        __DSB();
        SUNLITE_OTA_BKP_WRITE(&BKP->DR9, 0U);
    } else {
        SUNLITE_OTA_BKP_WRITE(&BKP->DR9, check);
        __DSB();
        SUNLITE_OTA_BKP_WRITE(&BKP->DR8, marker);
    }
    __DSB();
    return ((BKP->DR8 & 0xFFFFU) == marker) &&
           ((BKP->DR9 & 0xFFFFU) == check);
}

void SunliteOtaRequestBootloader(void)
{
    EnableBackupRegisters();

    /* Reset flags accumulate until RMVF is written.  Clear the application's
     * history immediately before arming the token so the bootloader can prove
     * that the following reset was the requested software reset. */
    __HAL_RCC_CLEAR_RESET_FLAGS();
    BKP->DR10 = SUNLITE_OTA_BOOT_REQUEST_MAGIC;
    __DSB();
}

bool SunliteOtaConsumeBootloaderRequest(void)
{
    EnableBackupRegisters();
    if ((BKP->DR10 & 0xFFFFU) != SUNLITE_OTA_BOOT_REQUEST_MAGIC) {
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
        ((reset_flags & SUNLITE_OTA_NON_SOFTWARE_RESET_FLAGS) == 0U);

    /* Consume even a rejected token.  A token retained through a pin,
     * watchdog, brownout, or power reset must never authorize a later boot. */
    BKP->DR10 = 0U;
    __DSB();
    /* Do not clear RCC reset flags here.  The request path cleared them just
     * before reset, and preserving the observed cause lets the application
     * perform its normal reset diagnostics after either outcome. */
    return requested;
}

bool SunliteOtaArmTrialBoot(void)
{
    return WriteTrialState(SUNLITE_OTA_TRIAL_ARMED,
                           SUNLITE_OTA_TRIAL_ARMED_CHECK);
}

bool SunliteOtaPrepareAppLaunch(void)
{
    SunliteOtaTrialState state = ReadTrialState();
    if (state == SUNLITE_OTA_TRIAL_NONE) {
        return true;
    }
    if (state != SUNLITE_OTA_TRIAL_ARMED_STATE) {
        return false;
    }
    return WriteTrialState(SUNLITE_OTA_TRIAL_LAUNCHED,
                           SUNLITE_OTA_TRIAL_LAUNCHED_CHECK);
}

bool SunliteOtaTrialBootRequiresRecovery(void)
{
    SunliteOtaTrialState state = ReadTrialState();
    return (state == SUNLITE_OTA_TRIAL_LAUNCHED_STATE) ||
           (state == SUNLITE_OTA_TRIAL_INVALID);
}

bool SunliteOtaConfirmTrialBoot(void)
{
    SunliteOtaTrialState state = ReadTrialState();
    if (state == SUNLITE_OTA_TRIAL_NONE) {
        return true;
    }
    if (state != SUNLITE_OTA_TRIAL_LAUNCHED_STATE) {
        return false;
    }
    return WriteTrialState(0U, 0U);
}
