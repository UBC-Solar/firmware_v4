#ifndef BOOTLOADER_BOOT_REQUEST_H
#define BOOTLOADER_BOOT_REQUEST_H

#include <stdbool.h>

void SunliteOtaRequestBootloader(void);
bool SunliteOtaConsumeBootloaderRequest(void);

/*
 * A newly committed image gets one trial launch.  The marker is kept in a
 * redundant pair of STM32 backup registers so a reset between either write is
 * detected and fails closed.  Applications must confirm only after their
 * targeted BOARD_INFO response has actually been transmitted.
 */
bool SunliteOtaArmTrialBoot(void);
bool SunliteOtaPrepareAppLaunch(void);
bool SunliteOtaTrialBootRequiresRecovery(void);
bool SunliteOtaConfirmTrialBoot(void);

#endif /* BOOTLOADER_BOOT_REQUEST_H */
