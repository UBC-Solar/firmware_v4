/**
 * @file    bootloader_boot_request.c
 * @brief   One-shot bootloader entry requests across software reset.
 *
 * DR10 requests update mode after the application approves flashing. The token
 * is consumed once and accepted only after the requested software reset.
 */

#include "bootloader.h"

#include "stm32f1xx_hal.h"

#define FW_UPDATE_BOOT_REQUEST_MAGIC 0x5A7EU
#define FW_UPDATE_NON_SOFTWARE_RESET_FLAGS                                                         \
    (RCC_CSR_LPWRRSTF | RCC_CSR_WWDGRSTF | RCC_CSR_IWDGRSTF | RCC_CSR_PORRSTF)

/* FUNCTION PROTOTYPES */
static void EnableBackupRegisters(void);

/**
 * @brief Enables access to the retained boot-request register.
 */
static void EnableBackupRegisters(void)
{
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_RCC_BKP_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();
}

/**
 * @brief Arms a one-shot update request in the backup registers.
 * @note The caller must trigger a software reset after arming the request.
 */
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

/**
 * @brief Validates and clears a pending bootloader-entry token.
 * @return True only for a matching token with an authorized software-reset cause.
 */
bool FirmwareUpdateConsumeBootloaderRequest(void)
{
    EnableBackupRegisters();
    if ((BKP->DR10 & 0xFFFFU) != FW_UPDATE_BOOT_REQUEST_MAGIC)
    {
        /* The application owns reset-cause diagnostics.  In particular, DRD
         * and STR must still see IWDGRSTF after the bootloader hands off. */
        return false;
    }

    uint32_t reset_flags = RCC->CSR;
    /* STM32F103 commonly sets PINRSTF as a companion to SFTRSTF after
     * NVIC_SystemReset().  SFTRSTF is the positive authorization signal;
     * PINRSTF alone still fails because the software flag is required. */
    bool requested = ((reset_flags & RCC_CSR_SFTRSTF) != 0U) &&
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
