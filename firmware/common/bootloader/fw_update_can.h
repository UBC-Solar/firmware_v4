#ifndef FW_UPDATE_CAN_H
#define FW_UPDATE_CAN_H

#include "stm32f1xx_hal.h"
#include "fw_update_can_transport.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FW_UPDATE_CAN_FILTER_BANK 13U

typedef struct {
    CAN_HandleTypeDef *handle;
    FirmwareUpdateCanTransport transport;
    bool initialized;
} FirmwareUpdateCanLink;

bool FirmwareUpdateCanLinkInit(FirmwareUpdateCanLink *link,
                           CAN_HandleTypeDef *handle,
                           uint8_t local_address,
                           uint8_t peer_address);
bool FirmwareUpdateCanLinkSetPeer(FirmwareUpdateCanLink *link,
                              uint8_t peer_address);
void FirmwareUpdateCanLinkPoll(FirmwareUpdateCanLink *link);
bool FirmwareUpdateCanLinkSendFrame(FirmwareUpdateCanLink *link,
                                const uint8_t *encoded,
                                size_t encoded_length);
bool FirmwareUpdateCanLinkPeekFrame(const FirmwareUpdateCanLink *link,
                                const uint8_t **encoded,
                                size_t *encoded_length);
void FirmwareUpdateCanLinkConsumeFrame(FirmwareUpdateCanLink *link);
bool FirmwareUpdateCanLinkTxBusy(const FirmwareUpdateCanLink *link);
void FirmwareUpdateCanLinkAbort(FirmwareUpdateCanLink *link);

bool FirmwareUpdateCanTargetToNode(uint32_t target_id, uint8_t *node_id);

#endif /* FW_UPDATE_CAN_H */
