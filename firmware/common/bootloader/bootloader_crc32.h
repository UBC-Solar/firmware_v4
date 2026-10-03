#ifndef BOOTLOADER_CRC32_H
#define BOOTLOADER_CRC32_H

#include <stddef.h>
#include <stdint.h>

#define BOOTLOADER_CRC32_INITIAL 0xFFFFFFFFU

uint32_t BootloaderCrc32Update(uint32_t crc, const uint8_t *data, size_t length);
uint32_t BootloaderCrc32Finalize(uint32_t crc);

#endif /* BOOTLOADER_CRC32_H */
