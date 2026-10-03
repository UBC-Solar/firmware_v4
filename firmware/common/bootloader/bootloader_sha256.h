#ifndef BOOTLOADER_SHA256_H
#define BOOTLOADER_SHA256_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t state[8];
    uint64_t total_bytes;
    uint8_t buffer[64];
    size_t buffer_length;
} BootloaderSha256Context;

void BootloaderSha256Init(BootloaderSha256Context *context);
void BootloaderSha256Update(BootloaderSha256Context *context,
                            const uint8_t *data,
                            size_t length);
void BootloaderSha256Final(BootloaderSha256Context *context, uint8_t digest[32]);

#endif /* BOOTLOADER_SHA256_H */
