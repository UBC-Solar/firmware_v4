#ifndef FW_UPDATE_BOOTLOADER_ENGINE_H
#define FW_UPDATE_BOOTLOADER_ENGINE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef bool (*FirmwareUpdateBootloaderSendFrame)(const uint8_t *frame,
                                               size_t frame_length,
                                               void *context);

/* Decode and process one complete COBS-framed Firmware update request.  The frame
 * may include its trailing zero delimiter.  Returns true only for a valid
 * HELLO addressed to this target, which lets a transport implement the boot
 * window without duplicating protocol parsing. */
bool FirmwareUpdateBootloaderProcessFrame(
    const uint8_t *frame,
    size_t frame_length,
    FirmwareUpdateBootloaderSendFrame send_frame,
    void *send_context);

#endif /* FW_UPDATE_BOOTLOADER_ENGINE_H */
