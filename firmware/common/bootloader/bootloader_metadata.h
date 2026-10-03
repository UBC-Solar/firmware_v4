#ifndef BOOTLOADER_METADATA_H
#define BOOTLOADER_METADATA_H

#include <stdbool.h>
#include <stdint.h>

bool BootloaderMetadataRead(uint32_t *firmware_version,
                            uint32_t *image_size,
                            uint8_t image_sha256[32]);
bool BootloaderMetadataWrite(uint32_t firmware_version,
                             uint32_t image_size,
                             const uint8_t image_sha256[32]);

#endif /* BOOTLOADER_METADATA_H */
