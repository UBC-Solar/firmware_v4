#ifndef FW_UPDATE_CAN_TRANSPORT_H
#define FW_UPDATE_CAN_TRANSPORT_H

#include "fw_update_protocol.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Firmware update over classic CAN uses the ISO-TP normal-fixed 29-bit identifier
 * layout.  The payload transported by this module is the existing COBS-encoded
 * firmware-update UART frame, without its trailing zero delimiter.
 *
 *     0x18DA<destination address><source address>
 *
 * TEL is the diagnostic client/router and uses the conventional tester address
 * 0xF1.  Each remote board receives a distinct eight-bit node address.
 */
#define FW_UPDATE_CAN_ID_BASE                 0x18DA0000U
#define FW_UPDATE_CAN_ID_MASK                 0x1FFF0000U
#define FW_UPDATE_CAN_EXTENDED_ID_MAX         0x1FFFFFFFU
#define FW_UPDATE_CAN_TESTER_ADDRESS          0xF1U
#define FW_UPDATE_CAN_NODE_MDI                 0x10U
#define FW_UPDATE_CAN_NODE_DRD                 0x11U
#define FW_UPDATE_CAN_NODE_STR                 0x12U
#define FW_UPDATE_CAN_NODE_HVC                 0x13U
#define FW_UPDATE_CAN_NODE_MST                 0x14U
#define FW_UPDATE_CAN_FRAME_DATA_SIZE         8U
#define FW_UPDATE_CAN_SINGLE_FRAME_DATA_SIZE  7U
#define FW_UPDATE_CAN_FIRST_FRAME_DATA_SIZE   6U
#define FW_UPDATE_CAN_CONSECUTIVE_DATA_SIZE   7U
#define FW_UPDATE_CAN_MAX_ISOTP_LENGTH        4095U
#define FW_UPDATE_CAN_DEFAULT_TIMEOUT_MS      250U
#define FW_UPDATE_CAN_DEFAULT_BLOCK_SIZE      3U
#define FW_UPDATE_CAN_DEFAULT_STMIN_MS         2U
#define FW_UPDATE_CAN_MAX_WAIT_FRAMES         3U
#define FW_UPDATE_CAN_FRAME_PAD_BYTE          0xAAU
#define FW_UPDATE_CAN_MAX_FRAME_SIZE          \
    (FW_UPDATE_MAX_ENCODED_FRAME - 1U)

typedef bool (*FirmwareUpdateCanSendFunction)(void *user,
                                          uint32_t extended_id,
                                          const uint8_t data[8],
                                          uint8_t dlc);

typedef enum {
    FW_UPDATE_CAN_ERROR_NONE = 0,
    FW_UPDATE_CAN_ERROR_ARGUMENT,
    FW_UPDATE_CAN_ERROR_BUSY,
    FW_UPDATE_CAN_ERROR_FRAME_TOO_LARGE,
    FW_UPDATE_CAN_ERROR_MALFORMED_FRAME,
    FW_UPDATE_CAN_ERROR_UNEXPECTED_FRAME,
    FW_UPDATE_CAN_ERROR_SEQUENCE,
    FW_UPDATE_CAN_ERROR_FLOW_CONTROL_OVERFLOW,
    FW_UPDATE_CAN_ERROR_FLOW_CONTROL_WAIT_LIMIT,
    FW_UPDATE_CAN_ERROR_TIMEOUT,
} FirmwareUpdateCanError;

typedef enum {
    FW_UPDATE_CAN_RX_IGNORED = 0,
    FW_UPDATE_CAN_RX_ACCEPTED,
    FW_UPDATE_CAN_RX_FRAME_COMPLETE,
    FW_UPDATE_CAN_RX_ERROR,
} FirmwareUpdateCanReceiveResult;

typedef enum {
    FW_UPDATE_CAN_TX_IDLE = 0,
    FW_UPDATE_CAN_TX_SEND_SINGLE,
    FW_UPDATE_CAN_TX_SEND_FIRST,
    FW_UPDATE_CAN_TX_WAIT_FLOW_CONTROL,
    FW_UPDATE_CAN_TX_SEND_CONSECUTIVE,
} FirmwareUpdateCanTransmitState;

typedef enum {
    FW_UPDATE_CAN_RX_IDLE = 0,
    FW_UPDATE_CAN_RX_WAIT_CONSECUTIVE,
    FW_UPDATE_CAN_RX_COMPLETE,
} FirmwareUpdateCanReceiveState;

typedef struct {
    uint8_t local_address;
    uint8_t peer_address;
    uint32_t transmit_id;
    uint32_t receive_id;
    FirmwareUpdateCanSendFunction send;
    void *send_user;
    uint32_t timeout_ms;

    FirmwareUpdateCanTransmitState transmit_state;
    uint8_t transmit_buffer[FW_UPDATE_CAN_MAX_FRAME_SIZE];
    uint16_t transmit_length;
    uint16_t transmit_offset;
    uint8_t transmit_sequence;
    uint8_t transmit_block_size;
    uint8_t transmit_block_count;
    uint8_t transmit_stmin_ms;
    uint8_t transmit_wait_count;
    uint32_t transmit_deadline_ms;
    uint32_t transmit_next_frame_ms;

    FirmwareUpdateCanReceiveState receive_state;
    uint8_t receive_buffer[FW_UPDATE_CAN_MAX_FRAME_SIZE];
    uint16_t receive_length;
    uint16_t receive_offset;
    uint8_t receive_sequence;
    uint8_t receive_block_count;
    bool flow_control_pending;
    uint32_t receive_deadline_ms;

    FirmwareUpdateCanError last_error;
} FirmwareUpdateCanTransport;

uint32_t FirmwareUpdateCanAddressedId(uint8_t destination, uint8_t source);
uint32_t FirmwareUpdateCanRequestId(uint8_t target_address);
uint32_t FirmwareUpdateCanResponseId(uint8_t target_address);
bool FirmwareUpdateCanDecodeAddressedId(uint32_t extended_id,
                                    uint8_t *destination,
                                    uint8_t *source);

void FirmwareUpdateCanInit(FirmwareUpdateCanTransport *transport,
                       uint8_t local_address,
                       uint8_t peer_address,
                       FirmwareUpdateCanSendFunction send,
                       void *send_user,
                       uint32_t timeout_ms);

bool FirmwareUpdateCanSetPeer(FirmwareUpdateCanTransport *transport,
                          uint8_t peer_address);

/*
 * Queue one complete firmware-update COBS frame for transmission.  encoded_length may
 * include the UART zero delimiter; it is stripped before CAN transport.  The
 * caller may reuse its input buffer as soon as this function returns.
 */
bool FirmwareUpdateCanSendFrame(FirmwareUpdateCanTransport *transport,
                            const uint8_t *encoded,
                            size_t encoded_length,
                            uint32_t now_ms);

/*
 * Feed one classic-CAN data frame into the transport.  Board adapters should
 * configure the addressed OTA identifier into FIFO1 and call this function
 * from a non-ISR polling context.
 */
FirmwareUpdateCanReceiveResult FirmwareUpdateCanOnFrame(
    FirmwareUpdateCanTransport *transport,
    uint32_t extended_id,
    const uint8_t *data,
    uint8_t dlc,
    uint32_t now_ms);

void FirmwareUpdateCanPoll(FirmwareUpdateCanTransport *transport, uint32_t now_ms);

/* The returned frame omits the UART delimiter and remains valid until consumed. */
bool FirmwareUpdateCanPeekFrame(const FirmwareUpdateCanTransport *transport,
                            const uint8_t **encoded,
                            size_t *encoded_length);
void FirmwareUpdateCanConsumeFrame(FirmwareUpdateCanTransport *transport);

bool FirmwareUpdateCanTxBusy(const FirmwareUpdateCanTransport *transport);
bool FirmwareUpdateCanRxBusy(const FirmwareUpdateCanTransport *transport);
FirmwareUpdateCanError FirmwareUpdateCanLastError(
    const FirmwareUpdateCanTransport *transport);
void FirmwareUpdateCanClearError(FirmwareUpdateCanTransport *transport);
void FirmwareUpdateCanAbort(FirmwareUpdateCanTransport *transport);

#endif /* FW_UPDATE_CAN_TRANSPORT_H */
