#include "fw_update_bootloader_engine.h"

#include "bootloader.h"
#include "bootloader_boot_request.h"
#include "bootloader_flash.h"
#include "bootloader_metadata.h"
#include "bootloader_sha256.h"
#include "monocypher-ed25519.h"
#include "stm32f1xx_hal.h"
#include "fw_update_protocol.h"
#include "fw_update_public_key.h"

#include <string.h>

#ifndef FW_UPDATE_TARGET_ID
#error "FW_UPDATE_TARGET_ID must be defined by the board target"
#endif
#ifndef FW_UPDATE_HARDWARE_REVISION
#error "FW_UPDATE_HARDWARE_REVISION must be defined by the board target"
#endif
#ifndef FW_UPDATE_PUBLIC_KEY_CONFIGURED
#define FW_UPDATE_PUBLIC_KEY_CONFIGURED 0
#endif

#define FW_UPDATE_BOOTLOADER_VERSION   1U
#define FW_UPDATE_SIGNING_DOMAIN_SIZE 24U

typedef struct {
    bool started;
    bool image_ready;
    uint32_t firmware_version;
    uint32_t image_size;
    uint32_t next_offset;
    uint8_t expected_sha256[FW_UPDATE_SHA256_SIZE];
    uint8_t vector_prefix[8];
    BootloaderSha256Context sha256;
} UpdateContext;

static uint8_t raw_frame[FW_UPDATE_MAX_RAW_FRAME];
static uint8_t cached_response[64];
static size_t cached_response_length;
static uint32_t cached_session;
static uint32_t cached_sequence;
static FirmwareUpdateMessageType cached_request_type;
static uint16_t cached_request_payload_length;
static uint8_t cached_request_payload[FW_UPDATE_MAX_RX_PAYLOAD];
static bool cached_response_valid;
static UpdateContext update;

#if FW_UPDATE_PUBLIC_KEY_CONFIGURED
static const uint8_t signing_domain[FW_UPDATE_SIGNING_DOMAIN_SIZE] = {
    'S', 'U', 'N', 'L', 'I', 'T', 'E', '-', 'O', 'T', 'A', '-',
    'M', 'A', 'N', 'I', 'F', 'E', 'S', 'T', '-', 'V', '1', 0,
};
#endif

static uint32_t InstalledFirmwareVersion(void)
{
    uint32_t firmware_version = 0U;
    uint32_t image_size = 0U;
    uint8_t digest[FW_UPDATE_SHA256_SIZE];
    (void)BootloaderMetadataRead(&firmware_version, &image_size, digest);
    return firmware_version;
}

static bool SendResponse(const FirmwareUpdateMessage *request,
                         FirmwareUpdateMessageType type,
                         const uint8_t *payload,
                         uint16_t payload_length,
                         FirmwareUpdateBootloaderSendFrame send_frame,
                         void *send_context)
{
    cached_response_length = FirmwareUpdateFrameEncode(
        type,
        request->session_id,
        request->sequence,
        payload,
        payload_length,
        cached_response,
        sizeof(cached_response));
    if (cached_response_length == 0U) {
        cached_response_valid = false;
        return false;
    }
    cached_session = request->session_id;
    cached_sequence = request->sequence;
    cached_request_type = request->type;
    cached_request_payload_length = request->payload_length;
    if (request->payload_length > 0U) {
        memcpy(cached_request_payload,
               request->payload,
               request->payload_length);
    }
    cached_response_valid = true;
    return send_frame(cached_response, cached_response_length, send_context);
}

static bool CachedTupleMatches(const FirmwareUpdateMessage *request)
{
    return cached_response_valid &&
           (cached_session == request->session_id) &&
           (cached_sequence == request->sequence) &&
           (cached_request_type == request->type);
}

static bool CachedRequestMatches(const FirmwareUpdateMessage *request)
{
    return CachedTupleMatches(request) &&
           (cached_request_payload_length == request->payload_length) &&
           ((request->payload_length == 0U) ||
            (memcmp(cached_request_payload,
                    request->payload,
                    request->payload_length) == 0));
}

static bool ResendCachedResponse(FirmwareUpdateBootloaderSendFrame send_frame,
                                 void *send_context)
{
    return send_frame(cached_response, cached_response_length, send_context);
}

static bool SendAck(const FirmwareUpdateMessage *request,
                    FirmwareUpdateStatus status,
                    uint32_t next_offset,
                    FirmwareUpdateBootloaderSendFrame send_frame,
                    void *send_context)
{
    uint8_t payload[FW_UPDATE_ACK_SIZE];
    payload[0] = (uint8_t)request->type;
    payload[1] = (uint8_t)status;
    FirmwareUpdateWriteBe32(&payload[2], next_offset);
    FirmwareUpdateMessageType response_type = (status == FW_UPDATE_STATUS_OK) ?
        FW_UPDATE_MESSAGE_ACK : FW_UPDATE_MESSAGE_NACK;
    return SendResponse(request,
                        response_type,
                        payload,
                        sizeof(payload),
                        send_frame,
                        send_context);
}

static void SendBoardInfo(const FirmwareUpdateMessage *request,
                          FirmwareUpdateBootloaderSendFrame send_frame,
                          void *send_context)
{
    uint32_t capabilities = 0U;
#if FW_UPDATE_PUBLIC_KEY_CONFIGURED
    capabilities |= FW_UPDATE_CAPABILITY_SIGNATURE_VERIFICATION;
#endif
#if FW_UPDATE_ALLOW_UNSIGNED_BENCH
    capabilities |= FW_UPDATE_CAPABILITY_UNSIGNED_BENCH;
#endif
    uint8_t payload[FW_UPDATE_BOARD_INFO_SIZE] = {0};
    FirmwareUpdateWriteBe32(&payload[0], FW_UPDATE_TARGET_ID);
    FirmwareUpdateWriteBe16(&payload[4], FW_UPDATE_HARDWARE_REVISION);
    FirmwareUpdateWriteBe32(&payload[6], FW_UPDATE_BOOTLOADER_VERSION);
    uint32_t reported_version = BootloaderAppIsValid() ?
        InstalledFirmwareVersion() : 0U;
    FirmwareUpdateWriteBe32(&payload[10], reported_version);
    payload[14] = 0U;
    payload[15] = update.image_ready ?
        FW_UPDATE_BOARD_PENDING_IMAGE :
        (update.started ? FW_UPDATE_BOARD_UPDATE_IN_PROGRESS :
                          FW_UPDATE_BOARD_BOOTLOADER);
    FirmwareUpdateWriteBe16(&payload[16], FW_UPDATE_MAX_CHUNK_SIZE);
    FirmwareUpdateWriteBe32(&payload[18], BOOTLOADER_APP_MAX_SIZE_BYTES);
    FirmwareUpdateWriteBe32(&payload[22], 0U);
    FirmwareUpdateWriteBe32(&payload[26], capabilities);
    (void)SendResponse(request,
                       FW_UPDATE_MESSAGE_BOARD_INFO,
                       payload,
                       sizeof(payload),
                       send_frame,
                       send_context);
}

static FirmwareUpdateStatus ValidateBeginUpdate(const FirmwareUpdateMessage *request)
{
    if (request->payload_length != FW_UPDATE_BEGIN_UPDATE_SIZE) {
        return FW_UPDATE_STATUS_BAD_STATE;
    }
    const uint8_t *payload = request->payload;
    uint32_t target_id = FirmwareUpdateReadBe32(&payload[0]);
    uint16_t hardware_min = FirmwareUpdateReadBe16(&payload[4]);
    uint16_t hardware_max = FirmwareUpdateReadBe16(&payload[6]);
    uint32_t firmware_version = FirmwareUpdateReadBe32(&payload[8]);
    uint32_t image_size = FirmwareUpdateReadBe32(&payload[12]);
    uint32_t minimum_bootloader = FirmwareUpdateReadBe32(&payload[16]);

    if (target_id != FW_UPDATE_TARGET_ID) {
        return FW_UPDATE_STATUS_BAD_TARGET;
    }
    if ((FW_UPDATE_HARDWARE_REVISION < hardware_min) ||
        (FW_UPDATE_HARDWARE_REVISION > hardware_max)) {
        return FW_UPDATE_STATUS_BAD_HARDWARE_REVISION;
    }
    if (minimum_bootloader > FW_UPDATE_BOOTLOADER_VERSION) {
        return FW_UPDATE_STATUS_UNSUPPORTED;
    }
    if ((image_size < sizeof(update.vector_prefix)) ||
        (image_size > BOOTLOADER_APP_MAX_SIZE_BYTES)) {
        return FW_UPDATE_STATUS_IMAGE_TOO_LARGE;
    }
    uint32_t current_version = InstalledFirmwareVersion();
    if ((BootloaderAppIsValid() && (firmware_version <= current_version)) ||
        (!BootloaderAppIsValid() && (firmware_version < current_version))) {
        return FW_UPDATE_STATUS_BAD_VERSION;
    }
#if FW_UPDATE_ALLOW_UNSIGNED_BENCH
    bool unsigned_manifest = true;
    for (size_t i = FW_UPDATE_SIGNING_FIELDS_SIZE; i < FW_UPDATE_BEGIN_UPDATE_SIZE; ++i) {
        unsigned_manifest = unsigned_manifest && (payload[i] == 0U);
    }
    if (!unsigned_manifest) {
#endif
#if !FW_UPDATE_PUBLIC_KEY_CONFIGURED
    return FW_UPDATE_STATUS_UNSUPPORTED;
#else
    uint8_t signed_message[FW_UPDATE_SIGNING_DOMAIN_SIZE +
                           FW_UPDATE_SIGNING_FIELDS_SIZE];
    memcpy(signed_message, signing_domain, sizeof(signing_domain));
    memcpy(&signed_message[sizeof(signing_domain)],
           payload,
           FW_UPDATE_SIGNING_FIELDS_SIZE);
    if (crypto_ed25519_check(&payload[FW_UPDATE_SIGNING_FIELDS_SIZE],
                             fw_update_public_key,
                             signed_message,
                             sizeof(signed_message)) != 0) {
        return FW_UPDATE_STATUS_BAD_SIGNATURE;
    }
#endif

#if FW_UPDATE_ALLOW_UNSIGNED_BENCH
    }
#endif

    memset(&update, 0, sizeof(update));
    update.firmware_version = firmware_version;
    update.image_size = image_size;
    memcpy(update.expected_sha256, &payload[20], sizeof(update.expected_sha256));
    if (!BootloaderFlashBeginAppUpdate(update.image_size)) {
        return FW_UPDATE_STATUS_FLASH_ERROR;
    }
    BootloaderSha256Init(&update.sha256);
    update.started = true;
    return FW_UPDATE_STATUS_OK;
}

static FirmwareUpdateStatus WriteData(const FirmwareUpdateMessage *request)
{
    if (!update.started || update.image_ready) {
        return FW_UPDATE_STATUS_BAD_STATE;
    }
    if (request->payload_length <= FW_UPDATE_DATA_OFFSET_SIZE) {
        return FW_UPDATE_STATUS_BAD_OFFSET;
    }
    uint32_t offset = FirmwareUpdateReadBe32(request->payload);
    size_t data_length = request->payload_length - FW_UPDATE_DATA_OFFSET_SIZE;
    const uint8_t *data = &request->payload[FW_UPDATE_DATA_OFFSET_SIZE];
    bool final_chunk = (data_length <= update.image_size - update.next_offset) &&
                       (offset + data_length == update.image_size);
    if (((offset & 1U) != 0U) ||
        (((data_length & 1U) != 0U) && !final_chunk) ||
        (offset != update.next_offset) ||
        (data_length > FW_UPDATE_MAX_CHUNK_SIZE) ||
        (data_length > update.image_size - update.next_offset)) {
        return FW_UPDATE_STATUS_BAD_OFFSET;
    }

    static uint8_t flash_data[FW_UPDATE_MAX_CHUNK_SIZE];
    memcpy(flash_data, data, data_length);
    for (size_t index = 0U; index < data_length; index++) {
        uint32_t image_offset = offset + (uint32_t)index;
        if (image_offset < sizeof(update.vector_prefix)) {
            update.vector_prefix[image_offset] = data[index];
            flash_data[index] = 0xFFU;
        }
    }

    BootloaderSha256Update(&update.sha256, data, data_length);
    if (!BootloaderFlashWrite(BOOTLOADER_APP_START_ADDRESS + offset,
                              flash_data,
                              data_length)) {
        BootloaderFlashEndAppUpdate();
        update.started = false;
        return FW_UPDATE_STATUS_FLASH_ERROR;
    }
    update.next_offset += (uint32_t)data_length;
    return FW_UPDATE_STATUS_OK;
}

static bool VectorPrefixIsValid(void)
{
    uint32_t stack_pointer;
    uint32_t reset_vector;
    memcpy(&stack_pointer, &update.vector_prefix[0], sizeof(stack_pointer));
    memcpy(&reset_vector, &update.vector_prefix[4], sizeof(reset_vector));
    uint32_t reset_address = reset_vector & ~1U;
    return (stack_pointer >= BOOTLOADER_SRAM_START_ADDRESS) &&
           (stack_pointer <= BOOTLOADER_SRAM_END_ADDRESS) &&
           ((stack_pointer & 0x7U) == 0U) &&
           ((reset_vector & 1U) != 0U) &&
           (reset_address >= BOOTLOADER_APP_START_ADDRESS) &&
           (reset_address <
            (BOOTLOADER_APP_START_ADDRESS + update.image_size));
}

static bool FlashImageMatchesDigest(
    const uint8_t expected_digest[FW_UPDATE_SHA256_SIZE])
{
    BootloaderSha256Context readback;
    uint8_t digest[FW_UPDATE_SHA256_SIZE];
    BootloaderSha256Init(&readback);

    /* The vector prefix is intentionally still 0xFF in flash at this point;
     * hash the authenticated bytes retained in RAM, then hash the programmed
     * application body directly from flash. */
    BootloaderSha256Update(&readback,
                           update.vector_prefix,
                           sizeof(update.vector_prefix));
    uint32_t offset = sizeof(update.vector_prefix);
    while (offset < update.image_size) {
        uint32_t remaining = update.image_size - offset;
        size_t block_size = remaining > FW_UPDATE_MAX_CHUNK_SIZE ?
            FW_UPDATE_MAX_CHUNK_SIZE : (size_t)remaining;
        const uint8_t *flash =
            (const uint8_t *)(uintptr_t)(BOOTLOADER_APP_START_ADDRESS + offset);
        BootloaderServiceWatchdog();
        BootloaderSha256Update(&readback, flash, block_size);
        offset += (uint32_t)block_size;
    }
    BootloaderSha256Final(&readback, digest);
    return memcmp(digest, expected_digest, sizeof(digest)) == 0;
}

static FirmwareUpdateStatus FinishUpdate(void)
{
    if (!update.started || update.image_ready ||
        (update.next_offset != update.image_size)) {
        return FW_UPDATE_STATUS_BAD_STATE;
    }
    uint8_t digest[FW_UPDATE_SHA256_SIZE];
    BootloaderSha256Final(&update.sha256, digest);
    if (memcmp(digest, update.expected_sha256, sizeof(digest)) != 0) {
        BootloaderFlashEndAppUpdate();
        update.started = false;
        return FW_UPDATE_STATUS_BAD_HASH;
    }
    if (!VectorPrefixIsValid()) {
        BootloaderFlashEndAppUpdate();
        update.started = false;
        return FW_UPDATE_STATUS_FLASH_ERROR;
    }
    if (!FlashImageMatchesDigest(digest)) {
        BootloaderFlashEndAppUpdate();
        update.started = false;
        return FW_UPDATE_STATUS_BAD_HASH;
    }
    if (!BootloaderMetadataWrite(update.firmware_version,
                                 update.image_size,
                                 digest)) {
        BootloaderFlashEndAppUpdate();
        update.started = false;
        return FW_UPDATE_STATUS_FLASH_ERROR;
    }

    /* Arm before committing the vector prefix.  A reset at any later point
     * therefore either sees an invalid app or a valid app with exactly one
     * permitted trial launch; there is no vector-valid/untracked gap. */
    if (!FirmwareUpdateArmTrialBoot() ||
        !BootloaderFlashWrite(BOOTLOADER_APP_START_ADDRESS,
                              update.vector_prefix,
                              sizeof(update.vector_prefix))) {
        BootloaderFlashEndAppUpdate();
        update.started = false;
        return FW_UPDATE_STATUS_FLASH_ERROR;
    }
    BootloaderFlashEndAppUpdate();
    update.image_ready = BootloaderAppIsValid();
    return update.image_ready ? FW_UPDATE_STATUS_OK :
                                FW_UPDATE_STATUS_FLASH_ERROR;
}

static bool HelloTargetsThisBoard(const FirmwareUpdateMessage *request)
{
    return (request->payload_length == 0U) ||
           ((request->payload_length == sizeof(uint32_t)) &&
            (FirmwareUpdateReadBe32(request->payload) == FW_UPDATE_TARGET_ID));
}

static bool ProcessMessage(const FirmwareUpdateMessage *request,
                           FirmwareUpdateBootloaderSendFrame send_frame,
                           void *send_context)
{
    if (CachedRequestMatches(request)) {
        bool response_sent = ResendCachedResponse(send_frame, send_context);
        if ((request->type == FW_UPDATE_MESSAGE_REBOOT) &&
            update.image_ready && response_sent) {
            HAL_Delay(50U);
            BootloaderJumpToApp();
        }
        return (request->type == FW_UPDATE_MESSAGE_HELLO) &&
               HelloTargetsThisBoard(request);
    }
    if (CachedTupleMatches(request)) {
        (void)SendAck(request,
                      FW_UPDATE_STATUS_BAD_STATE,
                      update.next_offset,
                      send_frame,
                      send_context);
        return false;
    }

    switch (request->type) {
    case FW_UPDATE_MESSAGE_HELLO:
        if (HelloTargetsThisBoard(request)) {
            SendBoardInfo(request, send_frame, send_context);
            return true;
        }
        (void)SendAck(request,
                      FW_UPDATE_STATUS_BAD_TARGET,
                      update.next_offset,
                      send_frame,
                      send_context);
        break;
    case FW_UPDATE_MESSAGE_ENTER_BOOTLOADER:
        (void)SendAck(request,
                      request->payload_length == 0U ?
                          FW_UPDATE_STATUS_OK : FW_UPDATE_STATUS_BAD_STATE,
                      update.next_offset,
                      send_frame,
                      send_context);
        break;
    case FW_UPDATE_MESSAGE_BEGIN_UPDATE: {
        FirmwareUpdateStatus status = ValidateBeginUpdate(request);
        (void)SendAck(request, status, update.next_offset,
                      send_frame, send_context);
        break;
    }
    case FW_UPDATE_MESSAGE_DATA: {
        FirmwareUpdateStatus status = WriteData(request);
        (void)SendAck(request, status, update.next_offset,
                      send_frame, send_context);
        break;
    }
    case FW_UPDATE_MESSAGE_END_UPDATE: {
        FirmwareUpdateStatus status = (request->payload_length == 0U) ?
            FinishUpdate() : FW_UPDATE_STATUS_BAD_STATE;
        (void)SendAck(request, status, update.next_offset,
                      send_frame, send_context);
        break;
    }
    case FW_UPDATE_MESSAGE_REBOOT:
        if ((request->payload_length != 0U) || !update.image_ready) {
            (void)SendAck(request, FW_UPDATE_STATUS_BAD_STATE,
                          update.next_offset, send_frame, send_context);
        } else if (SendAck(request, FW_UPDATE_STATUS_OK,
                           update.next_offset, send_frame, send_context)) {
            HAL_Delay(50U);
            BootloaderJumpToApp();
        }
        break;
    case FW_UPDATE_MESSAGE_ABORT:
        if (update.started) {
            BootloaderFlashEndAppUpdate();
        }
        memset(&update, 0, sizeof(update));
        (void)SendAck(request, FW_UPDATE_STATUS_OK, 0U,
                      send_frame, send_context);
        break;
    default:
        (void)SendAck(request, FW_UPDATE_STATUS_BAD_STATE,
                      update.next_offset, send_frame, send_context);
        break;
    }
    return false;
}

bool FirmwareUpdateBootloaderProcessFrame(
    const uint8_t *frame,
    size_t frame_length,
    FirmwareUpdateBootloaderSendFrame send_frame,
    void *send_context)
{
    if ((frame == NULL) || (send_frame == NULL)) {
        return false;
    }
    BootloaderServiceWatchdog();
    FirmwareUpdateMessage message;
    if (!FirmwareUpdateFrameDecode(frame,
                               frame_length,
                               raw_frame,
                               sizeof(raw_frame),
                               &message)) {
        return false;
    }
    return ProcessMessage(&message, send_frame, send_context);
}
