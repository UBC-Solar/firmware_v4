#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <stdbool.h>
#include <stdint.h>

#include "bootloader_config.h"

bool BootloaderAppIsValid(void);
bool BootloaderShouldEnterUpdateMode(void);

void BootloaderJumpToApp(void);
void BootloaderRun(void);
void BootloaderPrepareWatchdog(void);
void BootloaderServiceWatchdog(void);

/*
 * Board or transport layers can override these weak hooks.
 * Keep the common bootloader core transport-agnostic.
 */
bool BootloaderBoardStayInBootloader(void);
void BootloaderEnterUpdateMode(void);

#endif /* BOOTLOADER_H */
