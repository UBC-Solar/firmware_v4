/**
 * @file    fw_update_can.c
 * @brief   STM32 CAN adapter and application-side firmware update service.
 *
 * The shared link polls FIFO1 and sends through the STM32 HAL. The application
 * service below answers discovery requests and checks the board's safety hook
 * before resetting into the bootloader.
 */

#include "fw_update_can.h"

#include <string.h>

/* STM32 CAN LINK: SHARED BY THE BOOTLOADER AND APPLICATION */
/* FUNCTION PROTOTYPES */
static bool SendCanFrame(void* user, uint32_t extended_id, const uint8_t data[8], uint8_t dlc);
static bool ConfigureReceiveFilter(FirmwareUpdateCanLink* link);

/**
 * @brief Queues an extended CAN frame while protecting shared transmit mailboxes.
 */
static bool SendCanFrame(void* user, uint32_t extended_id, const uint8_t data[8], uint8_t dlc)
{
    FirmwareUpdateCanLink* link = (FirmwareUpdateCanLink*)user;
    if ((link == NULL) || (link->handle == NULL) || (dlc > 8U))
    {
        return false;
    }

    CAN_TxHeaderTypeDef header = {0};
    uint32_t mailbox = 0U;
    header.ExtId = extended_id;
    header.IDE = CAN_ID_EXT;
    header.RTR = CAN_RTR_DATA;
    header.DLC = dlc;
    uint32_t interrupt_state = __get_PRIMASK();
    __disable_irq();
    bool sent = (HAL_CAN_GetTxMailboxesFreeLevel(link->handle) > 0U) &&
                (HAL_CAN_AddTxMessage(link->handle, &header, (uint8_t*)data, &mailbox) == HAL_OK);
    if (interrupt_state == 0U)
    {
        __enable_irq();
    }
    return sent;
}

/**
 * @brief Routes the exact addressed update identifier to CAN FIFO1.
 */
static bool ConfigureReceiveFilter(FirmwareUpdateCanLink* link)
{
    CAN_FilterTypeDef filter = {0};
    uint32_t id = link->transport.receive_id;
    filter.FilterIdHigh = (uint16_t)(id >> 13U);
    filter.FilterIdLow = (uint16_t)(((id << 3U) & 0xFFF8U) | 0x0004U);
    /* In ID-list mode the mask fields hold the second exact ID.  Duplicate
     * the OTA ID so its 32-bit list match outranks catch-all mask filters. */
    filter.FilterMaskIdHigh = filter.FilterIdHigh;
    filter.FilterMaskIdLow = filter.FilterIdLow;
    filter.FilterFIFOAssignment = CAN_FILTER_FIFO1;
    filter.FilterBank = FW_UPDATE_CAN_FILTER_BANK;
    filter.FilterMode = CAN_FILTERMODE_IDLIST;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterActivation = ENABLE;
    return HAL_CAN_ConfigFilter(link->handle, &filter) == HAL_OK;
}

/**
 * @brief Configures the addressed FIFO1 filter and initializes the STM32 CAN link.
 */
bool FirmwareUpdateCanLinkInit(FirmwareUpdateCanLink* link,
                               CAN_HandleTypeDef* handle,
                               uint8_t local_address,
                               uint8_t peer_address)
{
    if ((link == NULL) || (handle == NULL))
    {
        return false;
    }
    memset(link, 0, sizeof(*link));
    link->handle = handle;
    FirmwareUpdateCanInit(&link->transport,
                          local_address,
                          peer_address,
                          SendCanFrame,
                          link,
                          FW_UPDATE_CAN_DEFAULT_TIMEOUT_MS);
    if (!ConfigureReceiveFilter(link))
    {
        return false;
    }

    HAL_CAN_StateTypeDef state = HAL_CAN_GetState(handle);
    if ((state == HAL_CAN_STATE_READY) && (HAL_CAN_Start(handle) != HAL_OK))
    {
        return false;
    }
    state = HAL_CAN_GetState(handle);
    link->initialized = (state == HAL_CAN_STATE_LISTENING);
    return link->initialized;
}

/**
 * @brief Drains FIFO1 and services transport state from a non-interrupt context.
 */
void FirmwareUpdateCanLinkPoll(FirmwareUpdateCanLink* link)
{
    if ((link == NULL) || !link->initialized)
    {
        return;
    }

    while (HAL_CAN_GetRxFifoFillLevel(link->handle, CAN_RX_FIFO1) > 0U)
    {
        CAN_RxHeaderTypeDef header = {0};
        uint8_t data[8] = {0};
        if (HAL_CAN_GetRxMessage(link->handle, CAN_RX_FIFO1, &header, data) != HAL_OK)
        {
            break;
        }
        if ((header.IDE == CAN_ID_EXT) && (header.RTR == CAN_RTR_DATA) && (header.DLC <= 8U))
        {
            (void)FirmwareUpdateCanOnFrame(
                &link->transport, header.ExtId, data, (uint8_t)header.DLC, HAL_GetTick());
        }
    }
    FirmwareUpdateCanPoll(&link->transport, HAL_GetTick());
}

/**
 * @brief Queues a complete encoded protocol frame for segmented transmission.
 */
bool FirmwareUpdateCanLinkSendFrame(FirmwareUpdateCanLink* link,
                                    const uint8_t* encoded,
                                    size_t encoded_length)
{
    return (link != NULL) && link->initialized &&
           FirmwareUpdateCanSendFrame(&link->transport, encoded, encoded_length, HAL_GetTick());
}

/**
 * @brief Returns a received frame whose buffer remains valid until consumed.
 */
bool FirmwareUpdateCanLinkPeekFrame(const FirmwareUpdateCanLink* link,
                                    const uint8_t** encoded,
                                    size_t* encoded_length)
{
    return (link != NULL) && link->initialized &&
           FirmwareUpdateCanPeekFrame(&link->transport, encoded, encoded_length);
}

/**
 * @brief Releases the current received frame for the next transfer.
 */
void FirmwareUpdateCanLinkConsumeFrame(FirmwareUpdateCanLink* link)
{
    if (link != NULL)
    {
        FirmwareUpdateCanConsumeFrame(&link->transport);
    }
}

/**
 * @brief Reports whether a segmented transmit operation is still pending.
 */
bool FirmwareUpdateCanLinkTxBusy(const FirmwareUpdateCanLink* link)
{
    return (link != NULL) && FirmwareUpdateCanTxBusy(&link->transport);
}

/**
 * @brief Cancels pending transport operations.
 */
void FirmwareUpdateCanLinkAbort(FirmwareUpdateCanLink* link)
{
    if (link != NULL)
    {
        FirmwareUpdateCanAbort(&link->transport);
    }
}

/* APPLICATION DISCOVERY AND BOOTLOADER ENTRY */
#ifndef BOOTLOADER_BUILD
/* Running-application discovery and bootloader entry. */
#include "bootloader.h"
#include "fw_update_protocol.h"

#ifndef FW_UPDATE_TARGET_ID
#error "FW_UPDATE_TARGET_ID must be defined by the board target"
#endif
#ifndef FW_UPDATE_FIRMWARE_VERSION
#error "FW_UPDATE_FIRMWARE_VERSION must be defined by the board target"
#endif
#ifndef FW_UPDATE_CAN_NODE_ID
#error "FW_UPDATE_CAN_NODE_ID must be defined by the board target"
#endif
#ifndef FW_UPDATE_ALLOW_UNSAFE_BENCH_UPDATE
#define FW_UPDATE_ALLOW_UNSAFE_BENCH_UPDATE 0
#endif

#define FW_UPDATE_BOOTLOADER_VERSION 1U
#define FW_UPDATE_RESET_GRACE_MS 250U

static FirmwareUpdateCanLink can_link;
static CAN_HandleTypeDef* can_handle;
static bool reset_after_response;
static uint32_t reset_deadline;

/* FUNCTION PROTOTYPES */
static bool QueueResponse(const FirmwareUpdateMessage* request,
                          FirmwareUpdateMessageType response_type,
                          const uint8_t* payload,
                          uint16_t payload_length);
static bool QueueAck(const FirmwareUpdateMessage* request, FirmwareUpdateStatus status);
static bool QueueBoardInfo(const FirmwareUpdateMessage* request);
static bool HelloTargetsThisBoard(const FirmwareUpdateMessage* request);
static void ProcessRequest(const uint8_t* encoded, size_t encoded_length);

/**
 * @brief Checks the board safe-state interlock; override the weak bench-only default.
 */
__attribute__((weak)) bool FirmwareUpdateCanBoardUpdateAllowed(void)
{
    /* Release builds fail closed until the board overrides this weak hook
     * with a real stopped/isolated-state check. Debug builds can explicitly
     * enable the bench-only fallback through CMake. */
    return FW_UPDATE_ALLOW_UNSAFE_BENCH_UPDATE != 0;
}

/**
 * @brief Encodes and queues an application-side protocol response.
 */
static bool QueueResponse(const FirmwareUpdateMessage* request,
                          FirmwareUpdateMessageType response_type,
                          const uint8_t* payload,
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

/**
 * @brief Queues an ACK or NACK for an application-side request.
 */
static bool QueueAck(const FirmwareUpdateMessage* request, FirmwareUpdateStatus status)
{
    FirmwareUpdateMessageType response_type = FW_UPDATE_MESSAGE_ACK;
    if (status != FW_UPDATE_STATUS_OK)
    {
        response_type = FW_UPDATE_MESSAGE_NACK;
    }
    uint8_t payload[FW_UPDATE_ACK_SIZE];
    payload[0] = (uint8_t)request->type;
    payload[1] = (uint8_t)status;
    FirmwareUpdateWriteBe32(&payload[2], 0U);
    return QueueResponse(request, response_type, payload, sizeof(payload));
}

/**
 * @brief Queues this application target’s version and update capabilities.
 */
static bool QueueBoardInfo(const FirmwareUpdateMessage* request)
{
    bool update_allowed = FirmwareUpdateCanBoardUpdateAllowed();
    uint32_t capabilities = update_allowed ? FW_UPDATE_CAPABILITY_UPDATE_ALLOWED : 0U;

    uint8_t payload[FW_UPDATE_BOARD_INFO_SIZE] = {0};
    FirmwareUpdateWriteBe32(&payload[0], FW_UPDATE_TARGET_ID);
    FirmwareUpdateWriteBe32(&payload[4], FW_UPDATE_BOOTLOADER_VERSION);
    FirmwareUpdateWriteBe32(&payload[8], FW_UPDATE_FIRMWARE_VERSION);
    payload[12] = FW_UPDATE_BOARD_APPLICATION;
    FirmwareUpdateWriteBe16(&payload[13], FW_UPDATE_MAX_CHUNK_SIZE);
    FirmwareUpdateWriteBe32(&payload[15], BOOTLOADER_APP_MAX_SIZE_BYTES);
    FirmwareUpdateWriteBe32(&payload[19], capabilities);
    return QueueResponse(request, FW_UPDATE_MESSAGE_BOARD_INFO, payload, sizeof(payload));
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
 * @brief Handles application discovery and bootloader-entry requests.
 */
static void ProcessRequest(const uint8_t* encoded, size_t encoded_length)
{
    FirmwareUpdateMessage request;
    if (!FirmwareUpdateFrameDecode(encoded, encoded_length, &request))
    {
        return;
    }

    switch (request.type)
    {
    case FW_UPDATE_MESSAGE_HELLO:
        if (HelloTargetsThisBoard(&request))
        {
            (void)QueueBoardInfo(&request);
        }
        else
        {
            (void)QueueAck(&request, FW_UPDATE_STATUS_BAD_TARGET);
        }
        break;
    case FW_UPDATE_MESSAGE_ENTER_BOOTLOADER:
        if (request.payload_length != 0U)
        {
            (void)QueueAck(&request, FW_UPDATE_STATUS_BAD_STATE);
        }
        else if (!FirmwareUpdateCanBoardUpdateAllowed())
        {
            (void)QueueAck(&request, FW_UPDATE_STATUS_UPDATE_NOT_ALLOWED);
        }
        else if (QueueAck(&request, FW_UPDATE_STATUS_OK))
        {
            reset_after_response = true;
            reset_deadline = HAL_GetTick() + FW_UPDATE_RESET_GRACE_MS;
        }
        break;
    case FW_UPDATE_MESSAGE_REBOOT:
        /* The destination may already be running the new image because the
         * bootloader's final ACK was lost. ACK without resetting again; the
         * laptop confirms target ID and firmware version through HELLO. */
        (void)QueueAck(&request,
                       request.payload_length == 0U ? FW_UPDATE_STATUS_OK
                                                    : FW_UPDATE_STATUS_BAD_STATE);
        break;
    default:
        (void)QueueAck(&request, FW_UPDATE_STATUS_BAD_STATE);
        break;
    }
}

/**
 * @brief Initializes the application update link using the board CAN handle.
 */
void FirmwareUpdateCanAppInit(CAN_HandleTypeDef* handle)
{
    can_handle = handle;
    reset_after_response = false;
    reset_deadline = 0U;
    (void)FirmwareUpdateCanLinkInit(
        &can_link, handle, FW_UPDATE_CAN_NODE_ID, FW_UPDATE_CAN_TESTER_ADDRESS);
}

/**
 * @brief Processes update requests and completes a deferred reset.
 */
void FirmwareUpdateCanAppPoll(void)
{
    FirmwareUpdateCanLinkPoll(&can_link);

    const uint8_t* encoded = NULL;
    size_t encoded_length = 0U;
    if (!reset_after_response &&
        FirmwareUpdateCanLinkPeekFrame(&can_link, &encoded, &encoded_length))
    {
        ProcessRequest(encoded, encoded_length);
        FirmwareUpdateCanLinkConsumeFrame(&can_link);
    }

    bool transport_done = !FirmwareUpdateCanLinkTxBusy(&can_link);
    bool mailboxes_drained =
        (can_handle != NULL) && (HAL_CAN_GetTxMailboxesFreeLevel(can_handle) == 3U);
    bool response_grace_expired = (int32_t)(HAL_GetTick() - reset_deadline) >= 0;
    if (reset_after_response && transport_done && (mailboxes_drained || response_grace_expired))
    {
        FirmwareUpdateRequestBootloader();
        HAL_Delay(10U);
        NVIC_SystemReset();
    }
}

#endif /* BOOTLOADER_BUILD */
