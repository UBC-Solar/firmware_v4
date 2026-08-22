#ifndef SUNLITE_OTA_BOOTLOADER_ENGINE_H
#define SUNLITE_OTA_BOOTLOADER_ENGINE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef bool (*SunliteOtaBootloaderSendFrame)(const uint8_t *frame,
                                               size_t frame_length,
                                               void *context);

/* Decode and process one complete COBS-framed Sunlite OTA request.  The frame
 * may include its trailing zero delimiter.  Returns true only for a valid
 * HELLO addressed to this target, which lets a transport implement the boot
 * window without duplicating protocol parsing. */
bool SunliteOtaBootloaderProcessFrame(
    const uint8_t *frame,
    size_t frame_length,
    SunliteOtaBootloaderSendFrame send_frame,
    void *send_context);

#endif /* SUNLITE_OTA_BOOTLOADER_ENGINE_H */
