#ifndef BOOTLOADER_FLASH_H
#define BOOTLOADER_FLASH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool BootloaderFlashBeginAppUpdate(uint32_t image_size);
bool BootloaderFlashWrite(uint32_t address, const uint8_t *data, size_t length);
void BootloaderFlashEndAppUpdate(void);

#endif /* BOOTLOADER_FLASH_H */
