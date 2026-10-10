/**
 * @file    fw_update_can.h
 * @brief   STM32 CAN adapter and application-side firmware update service.
 */

#ifndef FW_UPDATE_CAN_H
#define FW_UPDATE_CAN_H

#include "fw_update_protocol.h"
#include "stm32f1xx_hal.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FW_UPDATE_CAN_FILTER_BANK 13U

typedef struct
{
    CAN_HandleTypeDef* handle;
    FirmwareUpdateCanTransport transport;
    bool initialized;
} FirmwareUpdateCanLink;

/**
 * @brief Configures the addressed FIFO1 filter and initializes the STM32 CAN link.
 */
bool FirmwareUpdateCanLinkInit(FirmwareUpdateCanLink* link,
                               CAN_HandleTypeDef* handle,
                               uint8_t local_address,
                               uint8_t peer_address);

/**
 * @brief Drains FIFO1 and services transport state from a non-interrupt context.
 */
void FirmwareUpdateCanLinkPoll(FirmwareUpdateCanLink* link);

/**
 * @brief Queues a complete encoded protocol frame for segmented transmission.
 */
bool FirmwareUpdateCanLinkSendFrame(FirmwareUpdateCanLink* link,
                                    const uint8_t* encoded,
                                    size_t encoded_length);

/**
 * @brief Returns a received frame whose buffer remains valid until consumed.
 */
bool FirmwareUpdateCanLinkPeekFrame(const FirmwareUpdateCanLink* link,
                                    const uint8_t** encoded,
                                    size_t* encoded_length);

/**
 * @brief Releases the current received frame for the next transfer.
 */
void FirmwareUpdateCanLinkConsumeFrame(FirmwareUpdateCanLink* link);

/**
 * @brief Reports whether a segmented transmit operation is still pending.
 */
bool FirmwareUpdateCanLinkTxBusy(const FirmwareUpdateCanLink* link);

/**
 * @brief Cancels pending transport operations.
 */
void FirmwareUpdateCanLinkAbort(FirmwareUpdateCanLink* link);

#ifndef BOOTLOADER_BUILD
/**
 * @brief Initializes the application update link using the board CAN handle.
 */
void FirmwareUpdateCanAppInit(CAN_HandleTypeDef* handle);

/**
 * @brief Processes update requests and completes a deferred reset.
 */
void FirmwareUpdateCanAppPoll(void);

/* Override on a board that has a real safe-state interlock. */
/**
 * @brief Checks the board safe-state interlock; override the weak bench-only default.
 */
bool FirmwareUpdateCanBoardUpdateAllowed(void);
#endif

#endif /* FW_UPDATE_CAN_H */
