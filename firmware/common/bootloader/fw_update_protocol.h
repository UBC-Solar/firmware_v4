#ifndef FW_UPDATE_PROTOCOL_H
#define FW_UPDATE_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef FW_UPDATE_ALLOW_UNSIGNED_BENCH
#define FW_UPDATE_ALLOW_UNSIGNED_BENCH 0
#endif
#if FW_UPDATE_ALLOW_UNSIGNED_BENCH && !defined(DEBUG)
#error "Unsigned bench updates require a Debug build"
#endif

#define FW_UPDATE_PROTOCOL_VERSION       1U
#define FW_UPDATE_HEADER_SIZE            14U
#define FW_UPDATE_FRAME_CRC_SIZE         4U
#define FW_UPDATE_BOARD_INFO_SIZE        30U
#define FW_UPDATE_ACK_SIZE               6U
#define FW_UPDATE_BEGIN_UPDATE_SIZE      116U
#define FW_UPDATE_SIGNING_FIELDS_SIZE    52U
#define FW_UPDATE_SIGNATURE_SIZE         64U
#define FW_UPDATE_SHA256_SIZE            32U
#define FW_UPDATE_DATA_OFFSET_SIZE       4U
#define FW_UPDATE_MAX_CHUNK_SIZE         1024U
#define FW_UPDATE_MAX_RX_PAYLOAD         \
    (FW_UPDATE_DATA_OFFSET_SIZE + FW_UPDATE_MAX_CHUNK_SIZE)
#define FW_UPDATE_MAX_RAW_FRAME          \
    (FW_UPDATE_HEADER_SIZE + FW_UPDATE_MAX_RX_PAYLOAD + FW_UPDATE_FRAME_CRC_SIZE)
#define FW_UPDATE_MAX_ENCODED_FRAME      \
    (FW_UPDATE_MAX_RAW_FRAME + (FW_UPDATE_MAX_RAW_FRAME / 254U) + 2U)

typedef enum {
    FW_UPDATE_MESSAGE_HELLO = 1,
    FW_UPDATE_MESSAGE_BOARD_INFO = 2,
    FW_UPDATE_MESSAGE_ENTER_BOOTLOADER = 3,
    FW_UPDATE_MESSAGE_BEGIN_UPDATE = 4,
    FW_UPDATE_MESSAGE_DATA = 5,
    FW_UPDATE_MESSAGE_END_UPDATE = 6,
    FW_UPDATE_MESSAGE_ACK = 7,
    FW_UPDATE_MESSAGE_NACK = 8,
    FW_UPDATE_MESSAGE_STATUS = 9,
    FW_UPDATE_MESSAGE_REBOOT = 10,
    FW_UPDATE_MESSAGE_ABORT = 11,
} FirmwareUpdateMessageType;

typedef enum {
    FW_UPDATE_BOARD_APPLICATION = 0,
    FW_UPDATE_BOARD_BOOTLOADER = 1,
    FW_UPDATE_BOARD_UPDATE_IN_PROGRESS = 2,
    FW_UPDATE_BOARD_PENDING_IMAGE = 3,
} FirmwareUpdateBoardState;

typedef enum {
    FW_UPDATE_CAPABILITY_UPDATE_ALLOWED = 1U << 0,
    FW_UPDATE_CAPABILITY_ROLLBACK = 1U << 1,
    FW_UPDATE_CAPABILITY_SIGNATURE_VERIFICATION = 1U << 2,
    FW_UPDATE_CAPABILITY_RESUME = 1U << 3,
    FW_UPDATE_CAPABILITY_UNSIGNED_BENCH = 1U << 4,
} FirmwareUpdateCapability;

typedef enum {
    FW_UPDATE_STATUS_OK = 0,
    FW_UPDATE_STATUS_BAD_STATE = 1,
    FW_UPDATE_STATUS_BAD_TARGET = 2,
    FW_UPDATE_STATUS_BAD_HARDWARE_REVISION = 3,
    FW_UPDATE_STATUS_BAD_VERSION = 4,
    FW_UPDATE_STATUS_BAD_SIGNATURE = 5,
    FW_UPDATE_STATUS_BAD_HASH = 6,
    FW_UPDATE_STATUS_BAD_OFFSET = 7,
    FW_UPDATE_STATUS_FLASH_ERROR = 8,
    FW_UPDATE_STATUS_IMAGE_TOO_LARGE = 9,
    FW_UPDATE_STATUS_UNSUPPORTED = 10,
    FW_UPDATE_STATUS_UPDATE_NOT_ALLOWED = 11,
    FW_UPDATE_STATUS_INTERNAL_ERROR = 12,
} FirmwareUpdateStatus;

typedef struct {
    FirmwareUpdateMessageType type;
    uint32_t session_id;
    uint32_t sequence;
    uint16_t payload_length;
    const uint8_t *payload;
} FirmwareUpdateMessage;

uint16_t FirmwareUpdateReadBe16(const uint8_t *data);
uint32_t FirmwareUpdateReadBe32(const uint8_t *data);
void FirmwareUpdateWriteBe16(uint8_t *data, uint16_t value);
void FirmwareUpdateWriteBe32(uint8_t *data, uint32_t value);

size_t FirmwareUpdateFrameEncode(FirmwareUpdateMessageType type,
                             uint32_t session_id,
                             uint32_t sequence,
                             const uint8_t *payload,
                             uint16_t payload_length,
                             uint8_t *encoded,
                             size_t encoded_capacity);

bool FirmwareUpdateFrameDecode(const uint8_t *encoded,
                           size_t encoded_length,
                           uint8_t *raw,
                           size_t raw_capacity,
                           FirmwareUpdateMessage *message);

uint32_t FirmwareUpdateCrc32(const uint8_t *data, size_t length);

#endif /* FW_UPDATE_PROTOCOL_H */
