/**
 * @file    fw_update_bootloader_engine.c
 * @brief   Firmware update validation, installation, and protocol responses.
 *
 * BEGIN checks the image description and erases its pages. DATA writes chunks
 * in order. END verifies the image and writes its stack/reset vectors last.
 * Identical retries reuse the last response without repeating flash writes.
 */

#include "bootloader.h"
#include "fw_update_protocol.h"
#include "stm32f1xx_hal.h"

#include <string.h>

/* Host tests substitute emulated flash for direct memory-mapped readback. */
#ifndef FW_UPDATE_FLASH_POINTER
#define FW_UPDATE_FLASH_POINTER(address) ((const uint8_t*)(uintptr_t)(address))
#endif

#ifndef FW_UPDATE_TARGET_ID
#error "FW_UPDATE_TARGET_ID must be defined by the board target"
#endif

#define FW_UPDATE_BOOTLOADER_VERSION 1U

typedef struct
{
    bool started;     /* BEGIN accepted; flash is unlocked until finish or failure. */
    bool image_ready; /* END accepted; the verified image can now be launched. */
    uint32_t image_size;
    uint32_t next_offset;
    uint32_t expected_crc;
    uint8_t app_vectors[8]; /* Stack pointer and reset handler; written last. */
} UpdateContext;

/* UPDATE STATE AND LAST RESPONSE */
static uint8_t cached_response[64];
static size_t cached_response_length;
static uint32_t cached_session;
static uint32_t cached_sequence;
static FirmwareUpdateMessageType cached_request_type;
static uint16_t cached_request_payload_length;
static uint32_t cached_request_crc;
static bool cached_response_valid;
static UpdateContext update;

/* FUNCTION PROTOTYPES */
static bool SendResponse(const FirmwareUpdateMessage* request,
                         FirmwareUpdateMessageType type,
                         const uint8_t* payload,
                         uint16_t payload_length,
                         FirmwareUpdateBootloaderSendFrame send_frame,
                         void* send_context);
static bool CachedRequestIdsMatch(const FirmwareUpdateMessage* request);
static bool CachedRequestMatches(const FirmwareUpdateMessage* request);
static bool SendAck(const FirmwareUpdateMessage* request,
                    FirmwareUpdateStatus status,
                    uint32_t next_offset,
                    FirmwareUpdateBootloaderSendFrame send_frame,
                    void* send_context);
static void SendBoardInfo(const FirmwareUpdateMessage* request,
                          FirmwareUpdateBootloaderSendFrame send_frame,
                          void* send_context);
static FirmwareUpdateStatus ValidateBeginUpdate(const FirmwareUpdateMessage* request);
static FirmwareUpdateStatus WriteData(const FirmwareUpdateMessage* request);
static FirmwareUpdateStatus FailUpdate(FirmwareUpdateStatus status);
static bool AppVectorsAreValid(void);
static bool FlashImageMatchesCrc(void);
static FirmwareUpdateStatus FinishUpdate(void);
static bool HelloTargetsThisBoard(const FirmwareUpdateMessage* request);
static bool ProcessMessage(const FirmwareUpdateMessage* request,
                           FirmwareUpdateBootloaderSendFrame send_frame,
                           void* send_context);

/**
 * @brief Encodes, caches, and transmits a protocol response.
 */
static bool SendResponse(const FirmwareUpdateMessage* request,
                         FirmwareUpdateMessageType type,
                         const uint8_t* payload,
                         uint16_t payload_length,
                         FirmwareUpdateBootloaderSendFrame send_frame,
                         void* send_context)
{
    cached_response_length = FirmwareUpdateFrameEncode(type,
                                                       request->session_id,
                                                       request->sequence,
                                                       payload,
                                                       payload_length,
                                                       cached_response,
                                                       sizeof(cached_response));
    if (cached_response_length == 0U)
    {
        cached_response_valid = false;
        return false;
    }
    cached_session = request->session_id;
    cached_sequence = request->sequence;
    cached_request_type = request->type;
    cached_request_payload_length = request->payload_length;
    cached_request_crc = FirmwareUpdateCrc32(request->payload, request->payload_length);
    cached_response_valid = true;
    return send_frame(cached_response, cached_response_length, send_context);
}

/**
 * @brief Checks whether a request uses the cached session, sequence, and command type.
 */
static bool CachedRequestIdsMatch(const FirmwareUpdateMessage* request)
{
    return cached_response_valid && (cached_session == request->session_id) &&
           (cached_sequence == request->sequence) && (cached_request_type == request->type);
}

/**
 * @brief Checks the retry's session, sequence, type, length, and payload CRC32.
 */
static bool CachedRequestMatches(const FirmwareUpdateMessage* request)
{
    if (!CachedRequestIdsMatch(request) ||
        (cached_request_payload_length != request->payload_length))
    {
        return false;
    }
    return cached_request_crc == FirmwareUpdateCrc32(request->payload, request->payload_length);
}

/**
 * @brief Sends an ACK or NACK containing status and the expected image offset.
 */
static bool SendAck(const FirmwareUpdateMessage* request,
                    FirmwareUpdateStatus status,
                    uint32_t next_offset,
                    FirmwareUpdateBootloaderSendFrame send_frame,
                    void* send_context)
{
    uint8_t payload[FW_UPDATE_ACK_SIZE];
    payload[0] = (uint8_t)request->type;
    payload[1] = (uint8_t)status;
    FirmwareUpdateWriteBe32(&payload[2], next_offset);
    FirmwareUpdateMessageType response_type =
        (status == FW_UPDATE_STATUS_OK) ? FW_UPDATE_MESSAGE_ACK : FW_UPDATE_MESSAGE_NACK;
    return SendResponse(request, response_type, payload, sizeof(payload), send_frame, send_context);
}

/**
 * @brief Reports this bootloader target’s version, capacity, and capabilities.
 */
static void SendBoardInfo(const FirmwareUpdateMessage* request,
                          FirmwareUpdateBootloaderSendFrame send_frame,
                          void* send_context)
{
    uint8_t payload[FW_UPDATE_BOARD_INFO_SIZE] = {0};
    FirmwareUpdateWriteBe32(&payload[0], FW_UPDATE_TARGET_ID);
    FirmwareUpdateWriteBe32(&payload[4], FW_UPDATE_BOOTLOADER_VERSION);
    /* Installed version is unknown here; the running application reports it. */
    FirmwareUpdateBoardState board_state = FW_UPDATE_BOARD_BOOTLOADER;
    if (update.image_ready)
    {
        board_state = FW_UPDATE_BOARD_PENDING_IMAGE;
    }
    else if (update.started)
    {
        board_state = FW_UPDATE_BOARD_UPDATE_IN_PROGRESS;
    }
    payload[12] = (uint8_t)board_state;
    FirmwareUpdateWriteBe16(&payload[13], FW_UPDATE_MAX_CHUNK_SIZE);
    FirmwareUpdateWriteBe32(&payload[15], BOOTLOADER_APP_MAX_SIZE_BYTES);
    FirmwareUpdateWriteBe32(&payload[19], 0U); /* No application safety-hook capability here. */
    (void)SendResponse(
        request, FW_UPDATE_MESSAGE_BOARD_INFO, payload, sizeof(payload), send_frame, send_context);
}

/**
 * @brief Validates the manifest before starting an application flash update.
 */
static FirmwareUpdateStatus ValidateBeginUpdate(const FirmwareUpdateMessage* request)
{
    if (request->payload_length != FW_UPDATE_BEGIN_UPDATE_SIZE)
    {
        return FW_UPDATE_STATUS_BAD_STATE;
    }
    const uint8_t* payload = request->payload;
    /* BEGIN: board ID, firmware version, image size, and image CRC32. */
    uint32_t target_id = FirmwareUpdateReadBe32(&payload[0]);
    uint32_t image_size = FirmwareUpdateReadBe32(&payload[8]);
    if (target_id != FW_UPDATE_TARGET_ID)
    {
        return FW_UPDATE_STATUS_BAD_TARGET;
    }
    if ((image_size < sizeof(update.app_vectors)) || (image_size > BOOTLOADER_APP_MAX_SIZE_BYTES))
    {
        return FW_UPDATE_STATUS_IMAGE_TOO_LARGE;
    }
    memset(&update, 0, sizeof(update));
    update.image_size = image_size;
    update.expected_crc = FirmwareUpdateReadBe32(&payload[12]);
    if (!BootloaderFlashBeginAppUpdate(update.image_size))
    {
        return FW_UPDATE_STATUS_FLASH_ERROR;
    }
    update.started = true;
    return FW_UPDATE_STATUS_OK;
}

/**
 * @brief Validates a chunk offset and writes the next firmware bytes.
 */
static FirmwareUpdateStatus WriteData(const FirmwareUpdateMessage* request)
{
    if (!update.started || update.image_ready)
    {
        return FW_UPDATE_STATUS_BAD_STATE;
    }
    if (request->payload_length <= FW_UPDATE_DATA_OFFSET_SIZE)
    {
        return FW_UPDATE_STATUS_BAD_OFFSET;
    }
    uint32_t offset = FirmwareUpdateReadBe32(request->payload);
    size_t data_length = request->payload_length - FW_UPDATE_DATA_OFFSET_SIZE;
    const uint8_t* data = &request->payload[FW_UPDATE_DATA_OFFSET_SIZE];
    uint32_t remaining = update.image_size - update.next_offset;
    if ((offset != update.next_offset) || (data_length > FW_UPDATE_MAX_CHUNK_SIZE) ||
        (data_length > remaining))
    {
        return FW_UPDATE_STATUS_BAD_OFFSET;
    }

    /* STM32F1 writes two bytes at a time. Only the image's final byte may
     * need padding, so every earlier chunk must end on a halfword boundary. */
    bool final_chunk = data_length == remaining;
    if (((offset & 1U) != 0U) || (((data_length & 1U) != 0U) && !final_chunk))
    {
        return FW_UPDATE_STATUS_BAD_OFFSET;
    }

    /* Keep the first eight bytes in RAM until the whole image is verified.
     * Without its stack/reset vectors, a partial image cannot be launched. */
    size_t vector_bytes = 0U;
    if (offset < sizeof(update.app_vectors))
    {
        vector_bytes = sizeof(update.app_vectors) - offset;
        if (vector_bytes > data_length)
        {
            vector_bytes = data_length;
        }
        memcpy(&update.app_vectors[offset], data, vector_bytes);
    }

    if ((data_length > vector_bytes) &&
        !BootloaderFlashWrite(BOOTLOADER_APP_START_ADDRESS + offset + vector_bytes,
                              data + vector_bytes,
                              data_length - vector_bytes))
    {
        return FailUpdate(FW_UPDATE_STATUS_FLASH_ERROR);
    }
    update.next_offset += (uint32_t)data_length;
    return FW_UPDATE_STATUS_OK;
}

/**
 * @brief Locks flash and stops the active update before returning its error.
 */
static FirmwareUpdateStatus FailUpdate(FirmwareUpdateStatus status)
{
    BootloaderFlashEndAppUpdate();
    update.started = false;
    return status;
}

/**
 * @brief Checks the proposed application’s initial stack and reset vectors.
 */
static bool AppVectorsAreValid(void)
{
    uint32_t stack_pointer;
    uint32_t reset_vector;
    memcpy(&stack_pointer, &update.app_vectors[0], sizeof(stack_pointer));
    memcpy(&reset_vector, &update.app_vectors[4], sizeof(reset_vector));
    uint32_t reset_address = reset_vector & ~1U;
    return (stack_pointer >= BOOTLOADER_SRAM_START_ADDRESS) &&
           (stack_pointer <= BOOTLOADER_SRAM_END_ADDRESS) && ((stack_pointer & 0x7U) == 0U) &&
           ((reset_vector & 1U) != 0U) && (reset_address >= BOOTLOADER_APP_START_ADDRESS) &&
           (reset_address < (BOOTLOADER_APP_START_ADDRESS + update.image_size));
}

/**
 * @brief Verifies the image CRC32 using withheld vectors and programmed flash bytes.
 */
static bool FlashImageMatchesCrc(void)
{
    uint32_t crc =
        FirmwareUpdateCrc32Update(0xFFFFFFFFU, update.app_vectors, sizeof(update.app_vectors));
    uint32_t offset = sizeof(update.app_vectors);
    while (offset < update.image_size)
    {
        uint32_t remaining = update.image_size - offset;
        size_t block_size =
            remaining > FW_UPDATE_MAX_CHUNK_SIZE ? FW_UPDATE_MAX_CHUNK_SIZE : remaining;
        BootloaderServiceWatchdog();
        crc = FirmwareUpdateCrc32Update(
            crc, FW_UPDATE_FLASH_POINTER(BOOTLOADER_APP_START_ADDRESS + offset), block_size);
        offset += (uint32_t)block_size;
    }
    return (crc ^ 0xFFFFFFFFU) == update.expected_crc;
}

/**
 * @brief Verifies the completed image and writes its launch vectors.
 */
static FirmwareUpdateStatus FinishUpdate(void)
{
    if (!update.started || update.image_ready || (update.next_offset != update.image_size))
    {
        return FW_UPDATE_STATUS_BAD_STATE;
    }
    /* One complete readback CRC32 checks both transferred and programmed bytes.
     * Include the vectors kept in RAM; they stay erased until this succeeds. */
    if (!FlashImageMatchesCrc())
    {
        return FailUpdate(FW_UPDATE_STATUS_BAD_CRC);
    }
    if (!AppVectorsAreValid())
    {
        return FailUpdate(FW_UPDATE_STATUS_FLASH_ERROR);
    }
    /* Commit the stack pointer last. A corrupt reset vector may still point
     * inside the slot, so it must be verified while the stack remains erased. */
    if (!BootloaderFlashWrite(BOOTLOADER_APP_START_ADDRESS + 4U, &update.app_vectors[4], 4U) ||
        !BootloaderFlashWrite(BOOTLOADER_APP_START_ADDRESS, update.app_vectors, 4U))
    {
        return FailUpdate(FW_UPDATE_STATUS_FLASH_ERROR);
    }
    BootloaderFlashEndAppUpdate();
    update.image_ready = BootloaderAppIsValid();
    return update.image_ready ? FW_UPDATE_STATUS_OK : FW_UPDATE_STATUS_FLASH_ERROR;
}

/**
 * @brief Checks whether a HELLO request addresses this firmware target.
 */
static bool HelloTargetsThisBoard(const FirmwareUpdateMessage* request)
{
    return (request->payload_length == 0U) ||
           ((request->payload_length == sizeof(uint32_t)) &&
            (FirmwareUpdateReadBe32(request->payload) == FW_UPDATE_TARGET_ID));
}

/**
 * @brief Handles commands and resends responses to identical retries without repeating writes.
 */
static bool ProcessMessage(const FirmwareUpdateMessage* request,
                           FirmwareUpdateBootloaderSendFrame send_frame,
                           void* send_context)
{
    if (CachedRequestMatches(request))
    {
        /* An ACK may have been lost. Reply again without erasing or writing twice. */
        bool targeted_hello =
            (request->type == FW_UPDATE_MESSAGE_HELLO) && HelloTargetsThisBoard(request);
        bool response_sent = send_frame(cached_response, cached_response_length, send_context);
        if ((request->type == FW_UPDATE_MESSAGE_REBOOT) && update.image_ready && response_sent)
        {
            HAL_Delay(50U);
            BootloaderJumpToApp();
        }
        return targeted_hello;
    }
    if (CachedRequestIdsMatch(request))
    {
        /* Reusing request IDs with different bytes is a conflicting request. */
        (void)SendAck(
            request, FW_UPDATE_STATUS_BAD_STATE, update.next_offset, send_frame, send_context);
        return false;
    }

    FirmwareUpdateStatus status = FW_UPDATE_STATUS_BAD_STATE;
    switch (request->type)
    {
    case FW_UPDATE_MESSAGE_HELLO:
        if (HelloTargetsThisBoard(request))
        {
            SendBoardInfo(request, send_frame, send_context);
            return true;
        }
        status = FW_UPDATE_STATUS_BAD_TARGET;
        break;
    case FW_UPDATE_MESSAGE_ENTER_BOOTLOADER:
        if (request->payload_length == 0U)
        {
            status = FW_UPDATE_STATUS_OK;
        }
        break;
    case FW_UPDATE_MESSAGE_BEGIN_UPDATE:
        status = ValidateBeginUpdate(request);
        break;
    case FW_UPDATE_MESSAGE_DATA:
        status = WriteData(request);
        break;
    case FW_UPDATE_MESSAGE_END_UPDATE:
        if (request->payload_length == 0U)
        {
            status = FinishUpdate();
        }
        break;
    case FW_UPDATE_MESSAGE_REBOOT:
        if ((request->payload_length == 0U) && update.image_ready)
        {
            status = FW_UPDATE_STATUS_OK;
        }
        break;
    case FW_UPDATE_MESSAGE_ABORT:
        if (update.started)
        {
            BootloaderFlashEndAppUpdate();
        }
        memset(&update, 0, sizeof(update));
        status = FW_UPDATE_STATUS_OK;
        break;
    default:
        break;
    }

    /* Each command has one response path. A reboot must wait for its ACK. */
    bool response_sent = SendAck(request, status, update.next_offset, send_frame, send_context);
    if ((request->type == FW_UPDATE_MESSAGE_REBOOT) && (status == FW_UPDATE_STATUS_OK) &&
        response_sent)
    {
        HAL_Delay(50U);
        BootloaderJumpToApp();
    }
    return false;
}

/**
 * @brief Decodes and processes one complete firmware update protocol frame.
 * @param frame Raw protocol frame including its CRC32.
 * @param frame_length Number of encoded bytes.
 * @param send_frame Callback used to transmit protocol responses.
 * @param send_context Caller context passed unchanged to the transmit callback.
 * @return True only for a valid HELLO addressed to this target.
 * @note The return value indicates a targeted HELLO, not overall update success.
 */
bool FirmwareUpdateBootloaderProcessFrame(const uint8_t* frame,
                                          size_t frame_length,
                                          FirmwareUpdateBootloaderSendFrame send_frame,
                                          void* send_context)
{
    if ((frame == NULL) || (send_frame == NULL))
    {
        return false;
    }
    BootloaderServiceWatchdog();
    FirmwareUpdateMessage message;
    if (!FirmwareUpdateFrameDecode(frame, frame_length, &message))
    {
        return false;
    }
    return ProcessMessage(&message, send_frame, send_context);
}
