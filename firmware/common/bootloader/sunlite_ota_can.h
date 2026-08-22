#ifndef SUNLITE_OTA_CAN_H
#define SUNLITE_OTA_CAN_H

#include "stm32f1xx_hal.h"
#include "sunlite_ota_can_transport.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SUNLITE_OTA_CAN_FILTER_BANK 13U

typedef struct {
    CAN_HandleTypeDef *handle;
    SunliteOtaCanTransport transport;
    bool initialized;
} SunliteOtaCanLink;

bool SunliteOtaCanLinkInit(SunliteOtaCanLink *link,
                           CAN_HandleTypeDef *handle,
                           uint8_t local_address,
                           uint8_t peer_address);
bool SunliteOtaCanLinkSetPeer(SunliteOtaCanLink *link,
                              uint8_t peer_address);
void SunliteOtaCanLinkPoll(SunliteOtaCanLink *link);
bool SunliteOtaCanLinkSendFrame(SunliteOtaCanLink *link,
                                const uint8_t *encoded,
                                size_t encoded_length);
bool SunliteOtaCanLinkPeekFrame(const SunliteOtaCanLink *link,
                                const uint8_t **encoded,
                                size_t *encoded_length);
void SunliteOtaCanLinkConsumeFrame(SunliteOtaCanLink *link);
bool SunliteOtaCanLinkTxBusy(const SunliteOtaCanLink *link);
void SunliteOtaCanLinkAbort(SunliteOtaCanLink *link);

bool SunliteOtaCanTargetToNode(uint32_t target_id, uint8_t *node_id);

#endif /* SUNLITE_OTA_CAN_H */
