#ifndef SUNLITE_OTA_CAN_TRANSPORT_H
#define SUNLITE_OTA_CAN_TRANSPORT_H

#include "sunlite_ota_protocol.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Sunlite OTA over classic CAN uses the ISO-TP normal-fixed 29-bit identifier
 * layout.  The payload transported by this module is the existing COBS-encoded
 * Sunlite UART frame, without its trailing zero delimiter.
 *
 *     0x18DA<destination address><source address>
 *
 * TEL is the diagnostic client/router and uses the conventional tester address
 * 0xF1.  Each remote board receives a distinct eight-bit node address.
 */
#define SUNLITE_OTA_CAN_ID_BASE                 0x18DA0000U
#define SUNLITE_OTA_CAN_ID_MASK                 0x1FFF0000U
#define SUNLITE_OTA_CAN_EXTENDED_ID_MAX         0x1FFFFFFFU
#define SUNLITE_OTA_CAN_TESTER_ADDRESS          0xF1U
#define SUNLITE_OTA_CAN_NODE_MDI                 0x10U
#define SUNLITE_OTA_CAN_NODE_DRD                 0x11U
#define SUNLITE_OTA_CAN_NODE_STR                 0x12U
#define SUNLITE_OTA_CAN_NODE_HVC                 0x13U
#define SUNLITE_OTA_CAN_NODE_MST                 0x14U
#define SUNLITE_OTA_CAN_FRAME_DATA_SIZE         8U
#define SUNLITE_OTA_CAN_SINGLE_FRAME_DATA_SIZE  7U
#define SUNLITE_OTA_CAN_FIRST_FRAME_DATA_SIZE   6U
#define SUNLITE_OTA_CAN_CONSECUTIVE_DATA_SIZE   7U
#define SUNLITE_OTA_CAN_MAX_ISOTP_LENGTH        4095U
#define SUNLITE_OTA_CAN_DEFAULT_TIMEOUT_MS      250U
#define SUNLITE_OTA_CAN_DEFAULT_BLOCK_SIZE      3U
#define SUNLITE_OTA_CAN_DEFAULT_STMIN_MS         2U
#define SUNLITE_OTA_CAN_MAX_WAIT_FRAMES         3U
#define SUNLITE_OTA_CAN_FRAME_PAD_BYTE          0xAAU
#define SUNLITE_OTA_CAN_MAX_FRAME_SIZE          \
    (SUNLITE_OTA_MAX_ENCODED_FRAME - 1U)

typedef bool (*SunliteOtaCanSendFunction)(void *user,
                                          uint32_t extended_id,
                                          const uint8_t data[8],
                                          uint8_t dlc);

typedef enum {
    SUNLITE_OTA_CAN_ERROR_NONE = 0,
    SUNLITE_OTA_CAN_ERROR_ARGUMENT,
    SUNLITE_OTA_CAN_ERROR_BUSY,
    SUNLITE_OTA_CAN_ERROR_FRAME_TOO_LARGE,
    SUNLITE_OTA_CAN_ERROR_MALFORMED_FRAME,
    SUNLITE_OTA_CAN_ERROR_UNEXPECTED_FRAME,
    SUNLITE_OTA_CAN_ERROR_SEQUENCE,
    SUNLITE_OTA_CAN_ERROR_FLOW_CONTROL_OVERFLOW,
    SUNLITE_OTA_CAN_ERROR_FLOW_CONTROL_WAIT_LIMIT,
    SUNLITE_OTA_CAN_ERROR_TIMEOUT,
} SunliteOtaCanError;

typedef enum {
    SUNLITE_OTA_CAN_RX_IGNORED = 0,
    SUNLITE_OTA_CAN_RX_ACCEPTED,
    SUNLITE_OTA_CAN_RX_FRAME_COMPLETE,
    SUNLITE_OTA_CAN_RX_ERROR,
} SunliteOtaCanReceiveResult;

typedef enum {
    SUNLITE_OTA_CAN_TX_IDLE = 0,
    SUNLITE_OTA_CAN_TX_SEND_SINGLE,
    SUNLITE_OTA_CAN_TX_SEND_FIRST,
    SUNLITE_OTA_CAN_TX_WAIT_FLOW_CONTROL,
    SUNLITE_OTA_CAN_TX_SEND_CONSECUTIVE,
} SunliteOtaCanTransmitState;

typedef enum {
    SUNLITE_OTA_CAN_RX_IDLE = 0,
    SUNLITE_OTA_CAN_RX_WAIT_CONSECUTIVE,
    SUNLITE_OTA_CAN_RX_COMPLETE,
} SunliteOtaCanReceiveState;

typedef struct {
    uint8_t local_address;
    uint8_t peer_address;
    uint32_t transmit_id;
    uint32_t receive_id;
    SunliteOtaCanSendFunction send;
    void *send_user;
    uint32_t timeout_ms;

    SunliteOtaCanTransmitState transmit_state;
    uint8_t transmit_buffer[SUNLITE_OTA_CAN_MAX_FRAME_SIZE];
    uint16_t transmit_length;
    uint16_t transmit_offset;
    uint8_t transmit_sequence;
    uint8_t transmit_block_size;
    uint8_t transmit_block_count;
    uint8_t transmit_stmin_ms;
    uint8_t transmit_wait_count;
    uint32_t transmit_deadline_ms;
    uint32_t transmit_next_frame_ms;

    SunliteOtaCanReceiveState receive_state;
    uint8_t receive_buffer[SUNLITE_OTA_CAN_MAX_FRAME_SIZE];
    uint16_t receive_length;
    uint16_t receive_offset;
    uint8_t receive_sequence;
    uint8_t receive_block_count;
    bool flow_control_pending;
    uint32_t receive_deadline_ms;

    SunliteOtaCanError last_error;
} SunliteOtaCanTransport;

uint32_t SunliteOtaCanAddressedId(uint8_t destination, uint8_t source);
uint32_t SunliteOtaCanRequestId(uint8_t target_address);
uint32_t SunliteOtaCanResponseId(uint8_t target_address);
bool SunliteOtaCanDecodeAddressedId(uint32_t extended_id,
                                    uint8_t *destination,
                                    uint8_t *source);

void SunliteOtaCanInit(SunliteOtaCanTransport *transport,
                       uint8_t local_address,
                       uint8_t peer_address,
                       SunliteOtaCanSendFunction send,
                       void *send_user,
                       uint32_t timeout_ms);

bool SunliteOtaCanSetPeer(SunliteOtaCanTransport *transport,
                          uint8_t peer_address);

/*
 * Queue one complete Sunlite COBS frame for transmission.  encoded_length may
 * include the UART zero delimiter; it is stripped before CAN transport.  The
 * caller may reuse its input buffer as soon as this function returns.
 */
bool SunliteOtaCanSendFrame(SunliteOtaCanTransport *transport,
                            const uint8_t *encoded,
                            size_t encoded_length,
                            uint32_t now_ms);

/*
 * Feed one classic-CAN data frame into the transport.  Board adapters should
 * configure the addressed OTA identifier into FIFO1 and call this function
 * from a non-ISR polling context.
 */
SunliteOtaCanReceiveResult SunliteOtaCanOnFrame(
    SunliteOtaCanTransport *transport,
    uint32_t extended_id,
    const uint8_t *data,
    uint8_t dlc,
    uint32_t now_ms);

void SunliteOtaCanPoll(SunliteOtaCanTransport *transport, uint32_t now_ms);

/* The returned frame omits the UART delimiter and remains valid until consumed. */
bool SunliteOtaCanPeekFrame(const SunliteOtaCanTransport *transport,
                            const uint8_t **encoded,
                            size_t *encoded_length);
void SunliteOtaCanConsumeFrame(SunliteOtaCanTransport *transport);

bool SunliteOtaCanTxBusy(const SunliteOtaCanTransport *transport);
bool SunliteOtaCanRxBusy(const SunliteOtaCanTransport *transport);
SunliteOtaCanError SunliteOtaCanLastError(
    const SunliteOtaCanTransport *transport);
void SunliteOtaCanClearError(SunliteOtaCanTransport *transport);
void SunliteOtaCanAbort(SunliteOtaCanTransport *transport);

#endif /* SUNLITE_OTA_CAN_TRANSPORT_H */
