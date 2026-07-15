#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <stdbool.h>
#include <stdint.h>

#include "bootloader_config.h"

bool BootloaderAppIsValid(void);
bool BootloaderShouldEnterUpdateMode(void);

void BootloaderJumpToApp(void);
void BootloaderRun(void);

/*
 * Board or transport layers can override these weak hooks.
 * Keep the common bootloader core transport-agnostic.
 */
bool BootloaderBoardStayInBootloader(void);
bool BootloaderWaitForUpdateRequest(uint32_t timeout_ms);
void BootloaderEnterUpdateMode(void);

#endif /* BOOTLOADER_H */
