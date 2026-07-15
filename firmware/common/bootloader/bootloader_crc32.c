#include "bootloader_crc32.h"

uint32_t BootloaderCrc32Update(uint32_t crc, const uint8_t *data, size_t length)
{
    for (size_t index = 0; index < length; index++) {
        crc ^= data[index];

        for (uint32_t bit = 0; bit < 8U; bit++) {
            if ((crc & 1U) != 0U) {
                crc = (crc >> 1U) ^ 0xEDB88320U;
            } else {
                crc >>= 1U;
            }
        }
    }

    return crc;
}

uint32_t BootloaderCrc32Finalize(uint32_t crc)
{
    return crc ^ 0xFFFFFFFFU;
}
