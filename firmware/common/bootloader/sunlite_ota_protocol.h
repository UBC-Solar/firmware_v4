#ifndef SUNLITE_OTA_PROTOCOL_H
#define SUNLITE_OTA_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SUNLITE_OTA_PROTOCOL_VERSION       1U
#define SUNLITE_OTA_HEADER_SIZE            14U
#define SUNLITE_OTA_FRAME_CRC_SIZE         4U
#define SUNLITE_OTA_BOARD_INFO_SIZE        30U
#define SUNLITE_OTA_ACK_SIZE               6U
#define SUNLITE_OTA_BEGIN_UPDATE_SIZE      116U
#define SUNLITE_OTA_SIGNING_FIELDS_SIZE    52U
#define SUNLITE_OTA_SIGNATURE_SIZE         64U
#define SUNLITE_OTA_SHA256_SIZE            32U
#define SUNLITE_OTA_DATA_OFFSET_SIZE       4U
#define SUNLITE_OTA_MAX_CHUNK_SIZE         1024U
#define SUNLITE_OTA_MAX_RX_PAYLOAD         \
    (SUNLITE_OTA_DATA_OFFSET_SIZE + SUNLITE_OTA_MAX_CHUNK_SIZE)
#define SUNLITE_OTA_MAX_RAW_FRAME          \
    (SUNLITE_OTA_HEADER_SIZE + SUNLITE_OTA_MAX_RX_PAYLOAD + SUNLITE_OTA_FRAME_CRC_SIZE)
#define SUNLITE_OTA_MAX_ENCODED_FRAME      \
    (SUNLITE_OTA_MAX_RAW_FRAME + (SUNLITE_OTA_MAX_RAW_FRAME / 254U) + 2U)

typedef enum {
    SUNLITE_OTA_MESSAGE_HELLO = 1,
    SUNLITE_OTA_MESSAGE_BOARD_INFO = 2,
    SUNLITE_OTA_MESSAGE_ENTER_BOOTLOADER = 3,
    SUNLITE_OTA_MESSAGE_BEGIN_UPDATE = 4,
    SUNLITE_OTA_MESSAGE_DATA = 5,
    SUNLITE_OTA_MESSAGE_END_UPDATE = 6,
    SUNLITE_OTA_MESSAGE_ACK = 7,
    SUNLITE_OTA_MESSAGE_NACK = 8,
    SUNLITE_OTA_MESSAGE_STATUS = 9,
    SUNLITE_OTA_MESSAGE_REBOOT = 10,
    SUNLITE_OTA_MESSAGE_ABORT = 11,
} SunliteOtaMessageType;

typedef enum {
    SUNLITE_OTA_BOARD_APPLICATION = 0,
    SUNLITE_OTA_BOARD_BOOTLOADER = 1,
    SUNLITE_OTA_BOARD_UPDATE_IN_PROGRESS = 2,
    SUNLITE_OTA_BOARD_PENDING_IMAGE = 3,
} SunliteOtaBoardState;

typedef enum {
    SUNLITE_OTA_CAPABILITY_UPDATE_ALLOWED = 1U << 0,
    SUNLITE_OTA_CAPABILITY_ROLLBACK = 1U << 1,
    SUNLITE_OTA_CAPABILITY_SIGNATURE_VERIFICATION = 1U << 2,
    SUNLITE_OTA_CAPABILITY_RESUME = 1U << 3,
} SunliteOtaCapability;

typedef enum {
    SUNLITE_OTA_STATUS_OK = 0,
    SUNLITE_OTA_STATUS_BAD_STATE = 1,
    SUNLITE_OTA_STATUS_BAD_TARGET = 2,
    SUNLITE_OTA_STATUS_BAD_HARDWARE_REVISION = 3,
    SUNLITE_OTA_STATUS_BAD_VERSION = 4,
    SUNLITE_OTA_STATUS_BAD_SIGNATURE = 5,
    SUNLITE_OTA_STATUS_BAD_HASH = 6,
    SUNLITE_OTA_STATUS_BAD_OFFSET = 7,
    SUNLITE_OTA_STATUS_FLASH_ERROR = 8,
    SUNLITE_OTA_STATUS_IMAGE_TOO_LARGE = 9,
    SUNLITE_OTA_STATUS_UNSUPPORTED = 10,
    SUNLITE_OTA_STATUS_UPDATE_NOT_ALLOWED = 11,
    SUNLITE_OTA_STATUS_INTERNAL_ERROR = 12,
} SunliteOtaStatus;

typedef struct {
    SunliteOtaMessageType type;
    uint32_t session_id;
    uint32_t sequence;
    uint16_t payload_length;
    const uint8_t *payload;
} SunliteOtaMessage;

uint16_t SunliteOtaReadBe16(const uint8_t *data);
uint32_t SunliteOtaReadBe32(const uint8_t *data);
void SunliteOtaWriteBe16(uint8_t *data, uint16_t value);
void SunliteOtaWriteBe32(uint8_t *data, uint32_t value);

size_t SunliteOtaFrameEncode(SunliteOtaMessageType type,
                             uint32_t session_id,
                             uint32_t sequence,
                             const uint8_t *payload,
                             uint16_t payload_length,
                             uint8_t *encoded,
                             size_t encoded_capacity);

bool SunliteOtaFrameDecode(const uint8_t *encoded,
                           size_t encoded_length,
                           uint8_t *raw,
                           size_t raw_capacity,
                           SunliteOtaMessage *message);

uint32_t SunliteOtaCrc32(const uint8_t *data, size_t length);

#endif /* SUNLITE_OTA_PROTOCOL_H */
