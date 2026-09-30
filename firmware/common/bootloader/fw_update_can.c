#include "fw_update_can.h"

#include <string.h>

#define FW_UPDATE_TARGET_MDI 0x4D444920U
#define FW_UPDATE_TARGET_DRD 0x44524420U
#define FW_UPDATE_TARGET_STR 0x53545220U
#define FW_UPDATE_TARGET_HVC 0x48564320U
#define FW_UPDATE_TARGET_MST 0x4D535420U

static bool SendCanFrame(void *user,
                         uint32_t extended_id,
                         const uint8_t data[8],
                         uint8_t dlc)
{
    FirmwareUpdateCanLink *link = (FirmwareUpdateCanLink *)user;
    if ((link == NULL) || (link->handle == NULL) || (dlc > 8U)) {
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
                (HAL_CAN_AddTxMessage(link->handle,
                                     &header,
                                     (uint8_t *)data,
                                     &mailbox) == HAL_OK);
    if (interrupt_state == 0U) {
        __enable_irq();
    }
    return sent;
}

static bool ConfigureReceiveFilter(FirmwareUpdateCanLink *link)
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

bool FirmwareUpdateCanLinkInit(FirmwareUpdateCanLink *link,
                           CAN_HandleTypeDef *handle,
                           uint8_t local_address,
                           uint8_t peer_address)
{
    if ((link == NULL) || (handle == NULL)) {
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
    if (!ConfigureReceiveFilter(link)) {
        return false;
    }

    HAL_CAN_StateTypeDef state = HAL_CAN_GetState(handle);
    if ((state == HAL_CAN_STATE_READY) && (HAL_CAN_Start(handle) != HAL_OK)) {
        return false;
    }
    state = HAL_CAN_GetState(handle);
    link->initialized = (state == HAL_CAN_STATE_LISTENING);
    return link->initialized;
}

bool FirmwareUpdateCanLinkSetPeer(FirmwareUpdateCanLink *link,
                              uint8_t peer_address)
{
    if ((link == NULL) || !link->initialized) {
        return false;
    }
    uint8_t previous_peer = link->transport.peer_address;
    if (!FirmwareUpdateCanSetPeer(&link->transport, peer_address)) {
        return false;
    }
    if (ConfigureReceiveFilter(link)) {
        return true;
    }
    (void)FirmwareUpdateCanSetPeer(&link->transport, previous_peer);
    (void)ConfigureReceiveFilter(link);
    return false;
}

void FirmwareUpdateCanLinkPoll(FirmwareUpdateCanLink *link)
{
    if ((link == NULL) || !link->initialized) {
        return;
    }

    while (HAL_CAN_GetRxFifoFillLevel(link->handle, CAN_RX_FIFO1) > 0U) {
        CAN_RxHeaderTypeDef header = {0};
        uint8_t data[8] = {0};
        if (HAL_CAN_GetRxMessage(link->handle,
                                CAN_RX_FIFO1,
                                &header,
                                data) != HAL_OK) {
            break;
        }
        if ((header.IDE == CAN_ID_EXT) &&
            (header.RTR == CAN_RTR_DATA) &&
            (header.DLC <= 8U)) {
            (void)FirmwareUpdateCanOnFrame(&link->transport,
                                       header.ExtId,
                                       data,
                                       (uint8_t)header.DLC,
                                       HAL_GetTick());
        }
    }
    FirmwareUpdateCanPoll(&link->transport, HAL_GetTick());
}

bool FirmwareUpdateCanLinkSendFrame(FirmwareUpdateCanLink *link,
                                const uint8_t *encoded,
                                size_t encoded_length)
{
    return (link != NULL) && link->initialized &&
           FirmwareUpdateCanSendFrame(&link->transport,
                                  encoded,
                                  encoded_length,
                                  HAL_GetTick());
}

bool FirmwareUpdateCanLinkPeekFrame(const FirmwareUpdateCanLink *link,
                                const uint8_t **encoded,
                                size_t *encoded_length)
{
    return (link != NULL) && link->initialized &&
           FirmwareUpdateCanPeekFrame(&link->transport,
                                  encoded,
                                  encoded_length);
}

void FirmwareUpdateCanLinkConsumeFrame(FirmwareUpdateCanLink *link)
{
    if (link != NULL) {
        FirmwareUpdateCanConsumeFrame(&link->transport);
    }
}

bool FirmwareUpdateCanLinkTxBusy(const FirmwareUpdateCanLink *link)
{
    return (link != NULL) && FirmwareUpdateCanTxBusy(&link->transport);
}

void FirmwareUpdateCanLinkAbort(FirmwareUpdateCanLink *link)
{
    if (link != NULL) {
        FirmwareUpdateCanAbort(&link->transport);
    }
}

bool FirmwareUpdateCanTargetToNode(uint32_t target_id, uint8_t *node_id)
{
    if (node_id == NULL) {
        return false;
    }
    switch (target_id) {
    case FW_UPDATE_TARGET_MDI:
        *node_id = FW_UPDATE_CAN_NODE_MDI;
        return true;
    case FW_UPDATE_TARGET_DRD:
        *node_id = FW_UPDATE_CAN_NODE_DRD;
        return true;
    case FW_UPDATE_TARGET_STR:
        *node_id = FW_UPDATE_CAN_NODE_STR;
        return true;
    case FW_UPDATE_TARGET_HVC:
        *node_id = FW_UPDATE_CAN_NODE_HVC;
        return true;
    case FW_UPDATE_TARGET_MST:
        *node_id = FW_UPDATE_CAN_NODE_MST;
        return true;
    default:
        return false;
    }
}
