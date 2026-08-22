#include "sunlite_ota_can_app.h"

#include "bootloader_boot_request.h"
#include "sunlite_ota_can.h"
#include "sunlite_ota_protocol.h"

#include <stddef.h>
#include <stdint.h>

#ifndef SUNLITE_OTA_TARGET_ID
#error "SUNLITE_OTA_TARGET_ID must be defined by the board target"
#endif
#ifndef SUNLITE_OTA_HARDWARE_REVISION
#error "SUNLITE_OTA_HARDWARE_REVISION must be defined by the board target"
#endif
#ifndef SUNLITE_OTA_FIRMWARE_VERSION
#error "SUNLITE_OTA_FIRMWARE_VERSION must be defined by the board target"
#endif
#ifndef SUNLITE_OTA_CAN_NODE_ID
#error "SUNLITE_OTA_CAN_NODE_ID must be defined by the board target"
#endif
#ifndef SUNLITE_OTA_PUBLIC_KEY_CONFIGURED
#define SUNLITE_OTA_PUBLIC_KEY_CONFIGURED 0
#endif
#ifndef SUNLITE_OTA_SLOT_SIZE_BYTES
#define SUNLITE_OTA_SLOT_SIZE_BYTES 225280U
#endif
#ifndef SUNLITE_OTA_ALLOW_UNSAFE_BENCH_UPDATE
#define SUNLITE_OTA_ALLOW_UNSAFE_BENCH_UPDATE 0
#endif

#define SUNLITE_OTA_BOOTLOADER_VERSION 1U
#define SUNLITE_OTA_RESET_GRACE_MS     250U

static SunliteOtaCanLink can_link;
static CAN_HandleTypeDef *can_handle;
static bool reset_after_response;
static bool confirm_trial_after_response;
static uint32_t reset_deadline;
static uint8_t protocol_raw[SUNLITE_OTA_MAX_RAW_FRAME];

__attribute__((weak)) bool SunliteOtaCanBoardUpdateAllowed(void)
{
    /* Release builds fail closed until the board overrides this weak hook
     * with a real stopped/isolated-state check. Debug builds can explicitly
     * enable the bench-only fallback through CMake. */
    return SUNLITE_OTA_ALLOW_UNSAFE_BENCH_UPDATE != 0;
}

static bool QueueResponse(const SunliteOtaMessage *request,
                          SunliteOtaMessageType response_type,
                          const uint8_t *payload,
                          uint16_t payload_length)
{
    uint8_t response[64];
    size_t response_length = SunliteOtaFrameEncode(response_type,
                                                    request->session_id,
                                                    request->sequence,
                                                    payload,
                                                    payload_length,
                                                    response,
                                                    sizeof(response));
    return (response_length > 0U) &&
           SunliteOtaCanLinkSendFrame(&can_link, response, response_length);
}

static bool QueueAck(const SunliteOtaMessage *request,
                     SunliteOtaStatus status)
{
    uint8_t payload[SUNLITE_OTA_ACK_SIZE];
    payload[0] = (uint8_t)request->type;
    payload[1] = (uint8_t)status;
    SunliteOtaWriteBe32(&payload[2], 0U);
    return QueueResponse(request,
                         status == SUNLITE_OTA_STATUS_OK ?
                             SUNLITE_OTA_MESSAGE_ACK :
                             SUNLITE_OTA_MESSAGE_NACK,
                         payload,
                         sizeof(payload));
}

static bool QueueBoardInfo(const SunliteOtaMessage *request)
{
    bool update_allowed = SunliteOtaCanBoardUpdateAllowed();
    uint32_t capabilities = update_allowed ?
        SUNLITE_OTA_CAPABILITY_UPDATE_ALLOWED : 0U;
#if SUNLITE_OTA_PUBLIC_KEY_CONFIGURED
    capabilities |= SUNLITE_OTA_CAPABILITY_SIGNATURE_VERIFICATION;
#endif

    uint8_t payload[SUNLITE_OTA_BOARD_INFO_SIZE] = {0};
    SunliteOtaWriteBe32(&payload[0], SUNLITE_OTA_TARGET_ID);
    SunliteOtaWriteBe16(&payload[4], SUNLITE_OTA_HARDWARE_REVISION);
    SunliteOtaWriteBe32(&payload[6], SUNLITE_OTA_BOOTLOADER_VERSION);
    SunliteOtaWriteBe32(&payload[10], SUNLITE_OTA_FIRMWARE_VERSION);
    payload[14] = 0U;
    payload[15] = SUNLITE_OTA_BOARD_APPLICATION;
    SunliteOtaWriteBe16(&payload[16], SUNLITE_OTA_MAX_CHUNK_SIZE);
    SunliteOtaWriteBe32(&payload[18], SUNLITE_OTA_SLOT_SIZE_BYTES);
    SunliteOtaWriteBe32(&payload[22], 0U);
    SunliteOtaWriteBe32(&payload[26], capabilities);
    return QueueResponse(request,
                         SUNLITE_OTA_MESSAGE_BOARD_INFO,
                         payload,
                         sizeof(payload));
}

static bool HelloTargetsThisBoard(const SunliteOtaMessage *request)
{
    return (request->payload_length == 0U) ||
           ((request->payload_length == sizeof(uint32_t)) &&
            (SunliteOtaReadBe32(request->payload) == SUNLITE_OTA_TARGET_ID));
}

static void ProcessRequest(const uint8_t *encoded, size_t encoded_length)
{
    SunliteOtaMessage request;
    if (!SunliteOtaFrameDecode(encoded,
                               encoded_length,
                               protocol_raw,
                               sizeof(protocol_raw),
                               &request)) {
        return;
    }

    switch (request.type) {
    case SUNLITE_OTA_MESSAGE_HELLO:
        if (HelloTargetsThisBoard(&request)) {
            bool explicitly_targeted =
                (request.payload_length == sizeof(uint32_t)) &&
                (SunliteOtaReadBe32(request.payload) ==
                 SUNLITE_OTA_TARGET_ID);
            if (QueueBoardInfo(&request) && explicitly_targeted) {
                /* Poll completes this only after the segmented response and
                 * the STM32 CAN mailboxes have drained. */
                confirm_trial_after_response = true;
            }
        } else {
            (void)QueueAck(&request, SUNLITE_OTA_STATUS_BAD_TARGET);
        }
        break;
    case SUNLITE_OTA_MESSAGE_ENTER_BOOTLOADER:
        if (request.payload_length != 0U) {
            (void)QueueAck(&request, SUNLITE_OTA_STATUS_BAD_STATE);
        } else if (!SunliteOtaCanBoardUpdateAllowed()) {
            (void)QueueAck(&request, SUNLITE_OTA_STATUS_UPDATE_NOT_ALLOWED);
        } else if (QueueAck(&request, SUNLITE_OTA_STATUS_OK)) {
            reset_after_response = true;
            reset_deadline = HAL_GetTick() + SUNLITE_OTA_RESET_GRACE_MS;
        }
        break;
    case SUNLITE_OTA_MESSAGE_REBOOT:
        /* The destination may already be running the new image because the
         * bootloader's final ACK was lost. ACK without resetting again; TEL
         * and the Pi will confirm target ID and firmware version via HELLO. */
        (void)QueueAck(&request,
                       request.payload_length == 0U ?
                           SUNLITE_OTA_STATUS_OK : SUNLITE_OTA_STATUS_BAD_STATE);
        break;
    default:
        (void)QueueAck(&request, SUNLITE_OTA_STATUS_BAD_STATE);
        break;
    }
}

void SunliteOtaCanAppInit(CAN_HandleTypeDef *handle)
{
    can_handle = handle;
    reset_after_response = false;
    confirm_trial_after_response = false;
    reset_deadline = 0U;
    (void)SunliteOtaCanLinkInit(&can_link,
                                handle,
                                SUNLITE_OTA_CAN_NODE_ID,
                                SUNLITE_OTA_CAN_TESTER_ADDRESS);
}

void SunliteOtaCanAppPoll(void)
{
    SunliteOtaCanLinkPoll(&can_link);

    const uint8_t *encoded = NULL;
    size_t encoded_length = 0U;
    if (!reset_after_response &&
        SunliteOtaCanLinkPeekFrame(&can_link,
                                   &encoded,
                                   &encoded_length)) {
        ProcessRequest(encoded, encoded_length);
        SunliteOtaCanLinkConsumeFrame(&can_link);
    }

    bool transport_done = !SunliteOtaCanLinkTxBusy(&can_link);
    bool mailboxes_drained =
        (can_handle != NULL) &&
        (HAL_CAN_GetTxMailboxesFreeLevel(can_handle) == 3U);
    bool response_grace_expired =
        (int32_t)(HAL_GetTick() - reset_deadline) >= 0;
    if (confirm_trial_after_response && transport_done) {
        if (SunliteOtaCanLastError(&can_link.transport) !=
            SUNLITE_OTA_CAN_ERROR_NONE) {
            /* A timed-out/aborted response is not a confirmation. */
            confirm_trial_after_response = false;
        } else if (mailboxes_drained) {
            (void)SunliteOtaConfirmTrialBoot();
            confirm_trial_after_response = false;
        }
    }
    if (reset_after_response &&
        transport_done &&
        (mailboxes_drained || response_grace_expired)) {
        SunliteOtaRequestBootloader();
        HAL_Delay(10U);
        NVIC_SystemReset();
    }
}
