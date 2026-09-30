#include "fw_update_can_app.h"

#include "bootloader_boot_request.h"
#include "fw_update_can.h"
#include "fw_update_protocol.h"

#include <stddef.h>
#include <stdint.h>

#ifndef FW_UPDATE_TARGET_ID
#error "FW_UPDATE_TARGET_ID must be defined by the board target"
#endif
#ifndef FW_UPDATE_HARDWARE_REVISION
#error "FW_UPDATE_HARDWARE_REVISION must be defined by the board target"
#endif
#ifndef FW_UPDATE_FIRMWARE_VERSION
#error "FW_UPDATE_FIRMWARE_VERSION must be defined by the board target"
#endif
#ifndef FW_UPDATE_CAN_NODE_ID
#error "FW_UPDATE_CAN_NODE_ID must be defined by the board target"
#endif
#ifndef FW_UPDATE_PUBLIC_KEY_CONFIGURED
#define FW_UPDATE_PUBLIC_KEY_CONFIGURED 0
#endif
#ifndef FW_UPDATE_SLOT_SIZE_BYTES
#define FW_UPDATE_SLOT_SIZE_BYTES 225280U
#endif
#ifndef FW_UPDATE_ALLOW_UNSAFE_BENCH_UPDATE
#define FW_UPDATE_ALLOW_UNSAFE_BENCH_UPDATE 0
#endif

#define FW_UPDATE_BOOTLOADER_VERSION 1U
#define FW_UPDATE_RESET_GRACE_MS     250U

static FirmwareUpdateCanLink can_link;
static CAN_HandleTypeDef *can_handle;
static bool reset_after_response;
static bool confirm_trial_after_response;
static uint32_t reset_deadline;
static uint8_t protocol_raw[FW_UPDATE_MAX_RAW_FRAME];

__attribute__((weak)) bool FirmwareUpdateCanBoardUpdateAllowed(void)
{
    /* Release builds fail closed until the board overrides this weak hook
     * with a real stopped/isolated-state check. Debug builds can explicitly
     * enable the bench-only fallback through CMake. */
    return FW_UPDATE_ALLOW_UNSAFE_BENCH_UPDATE != 0;
}

static bool QueueResponse(const FirmwareUpdateMessage *request,
                          FirmwareUpdateMessageType response_type,
                          const uint8_t *payload,
                          uint16_t payload_length)
{
    uint8_t response[64];
    size_t response_length = FirmwareUpdateFrameEncode(response_type,
                                                    request->session_id,
                                                    request->sequence,
                                                    payload,
                                                    payload_length,
                                                    response,
                                                    sizeof(response));
    return (response_length > 0U) &&
           FirmwareUpdateCanLinkSendFrame(&can_link, response, response_length);
}

static bool QueueAck(const FirmwareUpdateMessage *request,
                     FirmwareUpdateStatus status)
{
    uint8_t payload[FW_UPDATE_ACK_SIZE];
    payload[0] = (uint8_t)request->type;
    payload[1] = (uint8_t)status;
    FirmwareUpdateWriteBe32(&payload[2], 0U);
    return QueueResponse(request,
                         status == FW_UPDATE_STATUS_OK ?
                             FW_UPDATE_MESSAGE_ACK :
                             FW_UPDATE_MESSAGE_NACK,
                         payload,
                         sizeof(payload));
}

static bool QueueBoardInfo(const FirmwareUpdateMessage *request)
{
    bool update_allowed = FirmwareUpdateCanBoardUpdateAllowed();
    uint32_t capabilities = update_allowed ?
        FW_UPDATE_CAPABILITY_UPDATE_ALLOWED : 0U;
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
    FirmwareUpdateWriteBe32(&payload[10], FW_UPDATE_FIRMWARE_VERSION);
    payload[14] = 0U;
    payload[15] = FW_UPDATE_BOARD_APPLICATION;
    FirmwareUpdateWriteBe16(&payload[16], FW_UPDATE_MAX_CHUNK_SIZE);
    FirmwareUpdateWriteBe32(&payload[18], FW_UPDATE_SLOT_SIZE_BYTES);
    FirmwareUpdateWriteBe32(&payload[22], 0U);
    FirmwareUpdateWriteBe32(&payload[26], capabilities);
    return QueueResponse(request,
                         FW_UPDATE_MESSAGE_BOARD_INFO,
                         payload,
                         sizeof(payload));
}

static bool HelloTargetsThisBoard(const FirmwareUpdateMessage *request)
{
    return (request->payload_length == 0U) ||
           ((request->payload_length == sizeof(uint32_t)) &&
            (FirmwareUpdateReadBe32(request->payload) == FW_UPDATE_TARGET_ID));
}

static void ProcessRequest(const uint8_t *encoded, size_t encoded_length)
{
    FirmwareUpdateMessage request;
    if (!FirmwareUpdateFrameDecode(encoded,
                               encoded_length,
                               protocol_raw,
                               sizeof(protocol_raw),
                               &request)) {
        return;
    }

    switch (request.type) {
    case FW_UPDATE_MESSAGE_HELLO:
        if (HelloTargetsThisBoard(&request)) {
            bool explicitly_targeted =
                (request.payload_length == sizeof(uint32_t)) &&
                (FirmwareUpdateReadBe32(request.payload) ==
                 FW_UPDATE_TARGET_ID);
            if (QueueBoardInfo(&request) && explicitly_targeted) {
                /* Poll completes this only after the segmented response and
                 * the STM32 CAN mailboxes have drained. */
                confirm_trial_after_response = true;
            }
        } else {
            (void)QueueAck(&request, FW_UPDATE_STATUS_BAD_TARGET);
        }
        break;
    case FW_UPDATE_MESSAGE_ENTER_BOOTLOADER:
        if (request.payload_length != 0U) {
            (void)QueueAck(&request, FW_UPDATE_STATUS_BAD_STATE);
        } else if (!FirmwareUpdateCanBoardUpdateAllowed()) {
            (void)QueueAck(&request, FW_UPDATE_STATUS_UPDATE_NOT_ALLOWED);
        } else if (QueueAck(&request, FW_UPDATE_STATUS_OK)) {
            reset_after_response = true;
            reset_deadline = HAL_GetTick() + FW_UPDATE_RESET_GRACE_MS;
        }
        break;
    case FW_UPDATE_MESSAGE_REBOOT:
        /* The destination may already be running the new image because the
         * bootloader's final ACK was lost. ACK without resetting again; TEL
         * and the Pi will confirm target ID and firmware version via HELLO. */
        (void)QueueAck(&request,
                       request.payload_length == 0U ?
                           FW_UPDATE_STATUS_OK : FW_UPDATE_STATUS_BAD_STATE);
        break;
    default:
        (void)QueueAck(&request, FW_UPDATE_STATUS_BAD_STATE);
        break;
    }
}

void FirmwareUpdateCanAppInit(CAN_HandleTypeDef *handle)
{
    can_handle = handle;
    reset_after_response = false;
    confirm_trial_after_response = false;
    reset_deadline = 0U;
    (void)FirmwareUpdateCanLinkInit(&can_link,
                                handle,
                                FW_UPDATE_CAN_NODE_ID,
                                FW_UPDATE_CAN_TESTER_ADDRESS);
}

void FirmwareUpdateCanAppPoll(void)
{
    FirmwareUpdateCanLinkPoll(&can_link);

    const uint8_t *encoded = NULL;
    size_t encoded_length = 0U;
    if (!reset_after_response &&
        FirmwareUpdateCanLinkPeekFrame(&can_link,
                                   &encoded,
                                   &encoded_length)) {
        ProcessRequest(encoded, encoded_length);
        FirmwareUpdateCanLinkConsumeFrame(&can_link);
    }

    bool transport_done = !FirmwareUpdateCanLinkTxBusy(&can_link);
    bool mailboxes_drained =
        (can_handle != NULL) &&
        (HAL_CAN_GetTxMailboxesFreeLevel(can_handle) == 3U);
    bool response_grace_expired =
        (int32_t)(HAL_GetTick() - reset_deadline) >= 0;
    if (confirm_trial_after_response && transport_done) {
        if (FirmwareUpdateCanLastError(&can_link.transport) !=
            FW_UPDATE_CAN_ERROR_NONE) {
            /* A timed-out/aborted response is not a confirmation. */
            confirm_trial_after_response = false;
        } else if (mailboxes_drained) {
            (void)FirmwareUpdateConfirmTrialBoot();
            confirm_trial_after_response = false;
        }
    }
    if (reset_after_response &&
        transport_done &&
        (mailboxes_drained || response_grace_expired)) {
        FirmwareUpdateRequestBootloader();
        HAL_Delay(10U);
        NVIC_SystemReset();
    }
}
