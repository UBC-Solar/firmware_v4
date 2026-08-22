#include "sunlite_ota_can_transport.h"

#include <string.h>

#define PCI_TYPE_MASK              0xF0U
#define PCI_SINGLE_FRAME           0x00U
#define PCI_FIRST_FRAME            0x10U
#define PCI_CONSECUTIVE_FRAME      0x20U
#define PCI_FLOW_CONTROL           0x30U
#define FLOW_STATUS_MASK           0x0FU
#define FLOW_STATUS_CONTINUE       0x00U
#define FLOW_STATUS_WAIT           0x01U
#define FLOW_STATUS_OVERFLOW       0x02U

#if SUNLITE_OTA_CAN_MAX_FRAME_SIZE > SUNLITE_OTA_CAN_MAX_ISOTP_LENGTH
#error "Sunlite OTA frames exceed the classic ISO-TP 12-bit length field"
#endif

static bool TimeReached(uint32_t now_ms, uint32_t deadline_ms)
{
    return (int32_t)(now_ms - deadline_ms) >= 0;
}

static uint32_t TimeoutDeadline(const SunliteOtaCanTransport *transport,
                                uint32_t now_ms)
{
    return now_ms + transport->timeout_ms;
}

static void ResetTransmit(SunliteOtaCanTransport *transport)
{
    transport->transmit_state = SUNLITE_OTA_CAN_TX_IDLE;
    transport->transmit_length = 0U;
    transport->transmit_offset = 0U;
    transport->transmit_sequence = 0U;
    transport->transmit_block_size = 0U;
    transport->transmit_block_count = 0U;
    transport->transmit_stmin_ms = 0U;
    transport->transmit_wait_count = 0U;
    transport->transmit_deadline_ms = 0U;
    transport->transmit_next_frame_ms = 0U;
}

static void ResetReceive(SunliteOtaCanTransport *transport)
{
    transport->receive_state = SUNLITE_OTA_CAN_RX_IDLE;
    transport->receive_length = 0U;
    transport->receive_offset = 0U;
    transport->receive_sequence = 0U;
    transport->receive_block_count = 0U;
    transport->flow_control_pending = false;
    transport->receive_deadline_ms = 0U;
}

static void FailTransmit(SunliteOtaCanTransport *transport,
                         SunliteOtaCanError error)
{
    ResetTransmit(transport);
    transport->last_error = error;
}

static SunliteOtaCanReceiveResult FailReceive(
    SunliteOtaCanTransport *transport,
    SunliteOtaCanError error)
{
    ResetReceive(transport);
    transport->last_error = error;
    return SUNLITE_OTA_CAN_RX_ERROR;
}

static void PadCanFrame(uint8_t frame[SUNLITE_OTA_CAN_FRAME_DATA_SIZE])
{
    memset(frame,
           SUNLITE_OTA_CAN_FRAME_PAD_BYTE,
           SUNLITE_OTA_CAN_FRAME_DATA_SIZE);
}

static bool Emit(SunliteOtaCanTransport *transport,
                 const uint8_t data[SUNLITE_OTA_CAN_FRAME_DATA_SIZE])
{
    return transport->send(transport->send_user,
                           transport->transmit_id,
                           data,
                           SUNLITE_OTA_CAN_FRAME_DATA_SIZE);
}

static bool FrameHasNoDelimiter(const uint8_t *encoded, size_t length)
{
    for (size_t index = 0U; index < length; index++) {
        if (encoded[index] == 0U) {
            return false;
        }
    }
    return true;
}

static SunliteOtaCanReceiveResult CompleteReceive(
    SunliteOtaCanTransport *transport)
{
    if (!FrameHasNoDelimiter(transport->receive_buffer,
                             transport->receive_length)) {
        return FailReceive(transport,
                           SUNLITE_OTA_CAN_ERROR_MALFORMED_FRAME);
    }
    transport->receive_state = SUNLITE_OTA_CAN_RX_COMPLETE;
    transport->receive_deadline_ms = 0U;
    transport->flow_control_pending = false;
    return SUNLITE_OTA_CAN_RX_FRAME_COMPLETE;
}

static uint8_t DecodeStminMs(uint8_t encoded)
{
    if (encoded <= 0x7FU) {
        return encoded;
    }
    if ((encoded >= 0xF1U) && (encoded <= 0xF9U)) {
        /* HAL_GetTick() has millisecond resolution, so round 100-900 us up. */
        return 1U;
    }
    /* Reserved values are treated conservatively as zero delay. */
    return 0U;
}

uint32_t SunliteOtaCanAddressedId(uint8_t destination, uint8_t source)
{
    return SUNLITE_OTA_CAN_ID_BASE |
           ((uint32_t)destination << 8U) |
           (uint32_t)source;
}

uint32_t SunliteOtaCanRequestId(uint8_t target_address)
{
    return SunliteOtaCanAddressedId(target_address,
                                    SUNLITE_OTA_CAN_TESTER_ADDRESS);
}

uint32_t SunliteOtaCanResponseId(uint8_t target_address)
{
    return SunliteOtaCanAddressedId(SUNLITE_OTA_CAN_TESTER_ADDRESS,
                                    target_address);
}

bool SunliteOtaCanDecodeAddressedId(uint32_t extended_id,
                                    uint8_t *destination,
                                    uint8_t *source)
{
    if ((extended_id > SUNLITE_OTA_CAN_EXTENDED_ID_MAX) ||
        ((extended_id & SUNLITE_OTA_CAN_ID_MASK) != SUNLITE_OTA_CAN_ID_BASE)) {
        return false;
    }
    if (destination != NULL) {
        *destination = (uint8_t)(extended_id >> 8U);
    }
    if (source != NULL) {
        *source = (uint8_t)extended_id;
    }
    return true;
}

void SunliteOtaCanInit(SunliteOtaCanTransport *transport,
                       uint8_t local_address,
                       uint8_t peer_address,
                       SunliteOtaCanSendFunction send,
                       void *send_user,
                       uint32_t timeout_ms)
{
    if (transport == NULL) {
        return;
    }
    memset(transport, 0, sizeof(*transport));
    transport->local_address = local_address;
    transport->peer_address = peer_address;
    transport->transmit_id = SunliteOtaCanAddressedId(peer_address,
                                                      local_address);
    transport->receive_id = SunliteOtaCanAddressedId(local_address,
                                                     peer_address);
    transport->send = send;
    transport->send_user = send_user;
    transport->timeout_ms = (timeout_ms == 0U) ?
        SUNLITE_OTA_CAN_DEFAULT_TIMEOUT_MS : timeout_ms;
    if (send == NULL) {
        transport->last_error = SUNLITE_OTA_CAN_ERROR_ARGUMENT;
    }
}

bool SunliteOtaCanSetPeer(SunliteOtaCanTransport *transport,
                          uint8_t peer_address)
{
    if ((transport == NULL) || (transport->send == NULL)) {
        if (transport != NULL) {
            transport->last_error = SUNLITE_OTA_CAN_ERROR_ARGUMENT;
        }
        return false;
    }
    if ((transport->transmit_state != SUNLITE_OTA_CAN_TX_IDLE) ||
        (transport->receive_state != SUNLITE_OTA_CAN_RX_IDLE)) {
        transport->last_error = SUNLITE_OTA_CAN_ERROR_BUSY;
        return false;
    }
    transport->peer_address = peer_address;
    transport->transmit_id = SunliteOtaCanAddressedId(peer_address,
                                                      transport->local_address);
    transport->receive_id = SunliteOtaCanAddressedId(transport->local_address,
                                                     peer_address);
    return true;
}

bool SunliteOtaCanSendFrame(SunliteOtaCanTransport *transport,
                            const uint8_t *encoded,
                            size_t encoded_length,
                            uint32_t now_ms)
{
    if ((transport == NULL) || (transport->send == NULL) ||
        (encoded == NULL)) {
        if (transport != NULL) {
            transport->last_error = SUNLITE_OTA_CAN_ERROR_ARGUMENT;
        }
        return false;
    }
    if ((encoded_length > 0U) && (encoded[encoded_length - 1U] == 0U)) {
        encoded_length--;
    }
    if ((encoded_length == 0U) ||
        (encoded_length > SUNLITE_OTA_CAN_MAX_FRAME_SIZE) ||
        (encoded_length > SUNLITE_OTA_CAN_MAX_ISOTP_LENGTH)) {
        transport->last_error = SUNLITE_OTA_CAN_ERROR_FRAME_TOO_LARGE;
        return false;
    }
    if (!FrameHasNoDelimiter(encoded, encoded_length)) {
        transport->last_error = SUNLITE_OTA_CAN_ERROR_MALFORMED_FRAME;
        return false;
    }
    if (transport->transmit_state != SUNLITE_OTA_CAN_TX_IDLE) {
        transport->last_error = SUNLITE_OTA_CAN_ERROR_BUSY;
        return false;
    }

    memcpy(transport->transmit_buffer, encoded, encoded_length);
    transport->transmit_length = (uint16_t)encoded_length;
    transport->transmit_offset = 0U;
    transport->transmit_sequence = 1U;
    transport->transmit_block_size = 0U;
    transport->transmit_block_count = 0U;
    transport->transmit_stmin_ms = 0U;
    transport->transmit_wait_count = 0U;
    transport->transmit_deadline_ms = TimeoutDeadline(transport, now_ms);
    transport->transmit_next_frame_ms = now_ms;
    transport->transmit_state =
        (encoded_length <= SUNLITE_OTA_CAN_SINGLE_FRAME_DATA_SIZE) ?
        SUNLITE_OTA_CAN_TX_SEND_SINGLE : SUNLITE_OTA_CAN_TX_SEND_FIRST;
    transport->last_error = SUNLITE_OTA_CAN_ERROR_NONE;
    SunliteOtaCanPoll(transport, now_ms);
    return transport->transmit_state != SUNLITE_OTA_CAN_TX_IDLE ||
           transport->last_error == SUNLITE_OTA_CAN_ERROR_NONE;
}

static SunliteOtaCanReceiveResult HandleSingleFrame(
    SunliteOtaCanTransport *transport,
    const uint8_t *data,
    uint8_t dlc)
{
    uint8_t length = data[0] & 0x0FU;
    if ((length == 0U) ||
        (length > SUNLITE_OTA_CAN_SINGLE_FRAME_DATA_SIZE) ||
        ((uint8_t)(length + 1U) > dlc)) {
        return FailReceive(transport,
                           SUNLITE_OTA_CAN_ERROR_MALFORMED_FRAME);
    }
    if (transport->receive_state == SUNLITE_OTA_CAN_RX_COMPLETE) {
        transport->last_error = SUNLITE_OTA_CAN_ERROR_BUSY;
        return SUNLITE_OTA_CAN_RX_ERROR;
    }
    ResetReceive(transport);
    memcpy(transport->receive_buffer, &data[1], length);
    transport->receive_length = length;
    transport->receive_offset = length;
    return CompleteReceive(transport);
}

static void SendOverflowFlowControl(SunliteOtaCanTransport *transport)
{
    uint8_t frame[SUNLITE_OTA_CAN_FRAME_DATA_SIZE];
    PadCanFrame(frame);
    frame[0] = PCI_FLOW_CONTROL | FLOW_STATUS_OVERFLOW;
    frame[1] = 0U;
    frame[2] = 0U;
    (void)Emit(transport, frame);
}

static SunliteOtaCanReceiveResult HandleFirstFrame(
    SunliteOtaCanTransport *transport,
    const uint8_t *data,
    uint8_t dlc,
    uint32_t now_ms)
{
    if (dlc != SUNLITE_OTA_CAN_FRAME_DATA_SIZE) {
        return FailReceive(transport,
                           SUNLITE_OTA_CAN_ERROR_MALFORMED_FRAME);
    }
    uint16_t length = (uint16_t)(((uint16_t)(data[0] & 0x0FU) << 8U) |
                                 data[1]);
    if (length <= SUNLITE_OTA_CAN_SINGLE_FRAME_DATA_SIZE) {
        return FailReceive(transport,
                           SUNLITE_OTA_CAN_ERROR_MALFORMED_FRAME);
    }
    if ((length > SUNLITE_OTA_CAN_MAX_FRAME_SIZE) ||
        (length > SUNLITE_OTA_CAN_MAX_ISOTP_LENGTH)) {
        ResetReceive(transport);
        SendOverflowFlowControl(transport);
        transport->last_error = SUNLITE_OTA_CAN_ERROR_FRAME_TOO_LARGE;
        return SUNLITE_OTA_CAN_RX_ERROR;
    }
    if (transport->receive_state == SUNLITE_OTA_CAN_RX_COMPLETE) {
        transport->last_error = SUNLITE_OTA_CAN_ERROR_BUSY;
        return SUNLITE_OTA_CAN_RX_ERROR;
    }

    /* A fresh first frame cleanly restarts an incomplete outer request. */
    ResetReceive(transport);
    memcpy(transport->receive_buffer,
           &data[2],
           SUNLITE_OTA_CAN_FIRST_FRAME_DATA_SIZE);
    transport->receive_length = length;
    transport->receive_offset = SUNLITE_OTA_CAN_FIRST_FRAME_DATA_SIZE;
    transport->receive_sequence = 1U;
    transport->receive_block_count = 0U;
    transport->receive_state = SUNLITE_OTA_CAN_RX_WAIT_CONSECUTIVE;
    transport->flow_control_pending = true;
    transport->receive_deadline_ms = TimeoutDeadline(transport, now_ms);
    SunliteOtaCanPoll(transport, now_ms);
    return SUNLITE_OTA_CAN_RX_ACCEPTED;
}

static SunliteOtaCanReceiveResult HandleConsecutiveFrame(
    SunliteOtaCanTransport *transport,
    const uint8_t *data,
    uint8_t dlc,
    uint32_t now_ms)
{
    if ((transport->receive_state != SUNLITE_OTA_CAN_RX_WAIT_CONSECUTIVE) ||
        transport->flow_control_pending) {
        return FailReceive(transport,
                           SUNLITE_OTA_CAN_ERROR_UNEXPECTED_FRAME);
    }
    if ((data[0] & 0x0FU) != transport->receive_sequence) {
        return FailReceive(transport, SUNLITE_OTA_CAN_ERROR_SEQUENCE);
    }

    uint16_t remaining = transport->receive_length - transport->receive_offset;
    uint8_t copy_length = (remaining > SUNLITE_OTA_CAN_CONSECUTIVE_DATA_SIZE) ?
        SUNLITE_OTA_CAN_CONSECUTIVE_DATA_SIZE : (uint8_t)remaining;
    if ((dlc < 2U) || ((uint8_t)(dlc - 1U) < copy_length)) {
        return FailReceive(transport,
                           SUNLITE_OTA_CAN_ERROR_MALFORMED_FRAME);
    }

    memcpy(&transport->receive_buffer[transport->receive_offset],
           &data[1],
           copy_length);
    transport->receive_offset += copy_length;
    transport->receive_sequence =
        (uint8_t)((transport->receive_sequence + 1U) & 0x0FU);
    transport->receive_block_count++;
    transport->receive_deadline_ms = TimeoutDeadline(transport, now_ms);

    if (transport->receive_offset == transport->receive_length) {
        return CompleteReceive(transport);
    }
    if (transport->receive_block_count >=
        SUNLITE_OTA_CAN_DEFAULT_BLOCK_SIZE) {
        transport->receive_block_count = 0U;
        transport->flow_control_pending = true;
        SunliteOtaCanPoll(transport, now_ms);
    }
    return SUNLITE_OTA_CAN_RX_ACCEPTED;
}

static SunliteOtaCanReceiveResult HandleFlowControl(
    SunliteOtaCanTransport *transport,
    const uint8_t *data,
    uint8_t dlc,
    uint32_t now_ms)
{
    if (dlc < 3U) {
        FailTransmit(transport, SUNLITE_OTA_CAN_ERROR_MALFORMED_FRAME);
        return SUNLITE_OTA_CAN_RX_ERROR;
    }
    if (transport->transmit_state !=
        SUNLITE_OTA_CAN_TX_WAIT_FLOW_CONTROL) {
        transport->last_error = SUNLITE_OTA_CAN_ERROR_UNEXPECTED_FRAME;
        return SUNLITE_OTA_CAN_RX_ERROR;
    }

    switch (data[0] & FLOW_STATUS_MASK) {
    case FLOW_STATUS_CONTINUE:
        transport->transmit_block_size = data[1];
        transport->transmit_block_count = 0U;
        transport->transmit_stmin_ms = DecodeStminMs(data[2]);
        transport->transmit_wait_count = 0U;
        transport->transmit_next_frame_ms =
            now_ms + transport->transmit_stmin_ms;
        transport->transmit_deadline_ms = TimeoutDeadline(transport, now_ms);
        transport->transmit_state =
            SUNLITE_OTA_CAN_TX_SEND_CONSECUTIVE;
        return SUNLITE_OTA_CAN_RX_ACCEPTED;
    case FLOW_STATUS_WAIT:
        transport->transmit_wait_count++;
        if (transport->transmit_wait_count >
            SUNLITE_OTA_CAN_MAX_WAIT_FRAMES) {
            FailTransmit(
                transport,
                SUNLITE_OTA_CAN_ERROR_FLOW_CONTROL_WAIT_LIMIT);
            return SUNLITE_OTA_CAN_RX_ERROR;
        }
        transport->transmit_deadline_ms = TimeoutDeadline(transport, now_ms);
        return SUNLITE_OTA_CAN_RX_ACCEPTED;
    case FLOW_STATUS_OVERFLOW:
        FailTransmit(transport,
                     SUNLITE_OTA_CAN_ERROR_FLOW_CONTROL_OVERFLOW);
        return SUNLITE_OTA_CAN_RX_ERROR;
    default:
        FailTransmit(transport, SUNLITE_OTA_CAN_ERROR_MALFORMED_FRAME);
        return SUNLITE_OTA_CAN_RX_ERROR;
    }
}

SunliteOtaCanReceiveResult SunliteOtaCanOnFrame(
    SunliteOtaCanTransport *transport,
    uint32_t extended_id,
    const uint8_t *data,
    uint8_t dlc,
    uint32_t now_ms)
{
    if (transport == NULL) {
        return SUNLITE_OTA_CAN_RX_ERROR;
    }
    if (extended_id != transport->receive_id) {
        return SUNLITE_OTA_CAN_RX_IGNORED;
    }
    if ((data == NULL) || (dlc == 0U) ||
        (dlc > SUNLITE_OTA_CAN_FRAME_DATA_SIZE)) {
        transport->last_error = SUNLITE_OTA_CAN_ERROR_ARGUMENT;
        return SUNLITE_OTA_CAN_RX_ERROR;
    }

    switch (data[0] & PCI_TYPE_MASK) {
    case PCI_SINGLE_FRAME:
        return HandleSingleFrame(transport, data, dlc);
    case PCI_FIRST_FRAME:
        return HandleFirstFrame(transport, data, dlc, now_ms);
    case PCI_CONSECUTIVE_FRAME:
        return HandleConsecutiveFrame(transport, data, dlc, now_ms);
    case PCI_FLOW_CONTROL:
        return HandleFlowControl(transport, data, dlc, now_ms);
    default:
        return FailReceive(transport,
                           SUNLITE_OTA_CAN_ERROR_MALFORMED_FRAME);
    }
}

static void PollFlowControl(SunliteOtaCanTransport *transport,
                            uint32_t now_ms)
{
    if (!transport->flow_control_pending) {
        return;
    }
    uint8_t frame[SUNLITE_OTA_CAN_FRAME_DATA_SIZE];
    PadCanFrame(frame);
    frame[0] = PCI_FLOW_CONTROL | FLOW_STATUS_CONTINUE;
    frame[1] = SUNLITE_OTA_CAN_DEFAULT_BLOCK_SIZE;
    frame[2] = SUNLITE_OTA_CAN_DEFAULT_STMIN_MS;

    /* Clear first so a synchronous test adapter can feed the first CF back. */
    transport->flow_control_pending = false;
    if (!Emit(transport, frame)) {
        transport->flow_control_pending = true;
    } else {
        transport->receive_deadline_ms = TimeoutDeadline(transport, now_ms);
    }
}

static void PollSingleFrame(SunliteOtaCanTransport *transport)
{
    uint8_t frame[SUNLITE_OTA_CAN_FRAME_DATA_SIZE];
    PadCanFrame(frame);
    frame[0] = (uint8_t)transport->transmit_length;
    memcpy(&frame[1],
           transport->transmit_buffer,
           transport->transmit_length);

    transport->transmit_state = SUNLITE_OTA_CAN_TX_IDLE;
    if (Emit(transport, frame)) {
        transport->transmit_length = 0U;
        transport->transmit_offset = 0U;
    } else {
        transport->transmit_state = SUNLITE_OTA_CAN_TX_SEND_SINGLE;
    }
}

static void PollFirstFrame(SunliteOtaCanTransport *transport,
                           uint32_t now_ms)
{
    uint8_t frame[SUNLITE_OTA_CAN_FRAME_DATA_SIZE];
    PadCanFrame(frame);
    frame[0] = PCI_FIRST_FRAME |
        (uint8_t)(transport->transmit_length >> 8U);
    frame[1] = (uint8_t)transport->transmit_length;
    memcpy(&frame[2],
           transport->transmit_buffer,
           SUNLITE_OTA_CAN_FIRST_FRAME_DATA_SIZE);

    transport->transmit_offset = SUNLITE_OTA_CAN_FIRST_FRAME_DATA_SIZE;
    transport->transmit_state = SUNLITE_OTA_CAN_TX_WAIT_FLOW_CONTROL;
    if (!Emit(transport, frame)) {
        transport->transmit_offset = 0U;
        transport->transmit_state = SUNLITE_OTA_CAN_TX_SEND_FIRST;
    } else {
        transport->transmit_deadline_ms = TimeoutDeadline(transport, now_ms);
    }
}

static void PollConsecutiveFrame(SunliteOtaCanTransport *transport,
                                 uint32_t now_ms)
{
    if (!TimeReached(now_ms, transport->transmit_next_frame_ms)) {
        return;
    }
    uint16_t remaining =
        transport->transmit_length - transport->transmit_offset;
    uint8_t copy_length =
        (remaining > SUNLITE_OTA_CAN_CONSECUTIVE_DATA_SIZE) ?
        SUNLITE_OTA_CAN_CONSECUTIVE_DATA_SIZE : (uint8_t)remaining;
    uint8_t frame[SUNLITE_OTA_CAN_FRAME_DATA_SIZE];
    PadCanFrame(frame);
    frame[0] = PCI_CONSECUTIVE_FRAME |
        (transport->transmit_sequence & 0x0FU);
    memcpy(&frame[1],
           &transport->transmit_buffer[transport->transmit_offset],
           copy_length);

    uint16_t old_offset = transport->transmit_offset;
    uint8_t old_sequence = transport->transmit_sequence;
    uint8_t old_block_count = transport->transmit_block_count;
    uint32_t old_deadline_ms = transport->transmit_deadline_ms;
    uint32_t old_next_frame_ms = transport->transmit_next_frame_ms;
    SunliteOtaCanTransmitState old_state = transport->transmit_state;

    transport->transmit_offset += copy_length;
    transport->transmit_sequence =
        (uint8_t)((transport->transmit_sequence + 1U) & 0x0FU);
    transport->transmit_block_count++;
    transport->transmit_deadline_ms = TimeoutDeadline(transport, now_ms);
    transport->transmit_next_frame_ms =
        now_ms + transport->transmit_stmin_ms;
    if (transport->transmit_offset == transport->transmit_length) {
        transport->transmit_state = SUNLITE_OTA_CAN_TX_IDLE;
    } else if ((transport->transmit_block_size != 0U) &&
               (transport->transmit_block_count >=
                transport->transmit_block_size)) {
        transport->transmit_block_count = 0U;
        transport->transmit_state =
            SUNLITE_OTA_CAN_TX_WAIT_FLOW_CONTROL;
    }

    if (!Emit(transport, frame)) {
        transport->transmit_offset = old_offset;
        transport->transmit_sequence = old_sequence;
        transport->transmit_block_count = old_block_count;
        transport->transmit_deadline_ms = old_deadline_ms;
        transport->transmit_next_frame_ms = old_next_frame_ms;
        transport->transmit_state = old_state;
        return;
    }
    if (transport->transmit_state == SUNLITE_OTA_CAN_TX_IDLE) {
        transport->transmit_length = 0U;
        transport->transmit_offset = 0U;
    }
}

void SunliteOtaCanPoll(SunliteOtaCanTransport *transport, uint32_t now_ms)
{
    if ((transport == NULL) || (transport->send == NULL)) {
        return;
    }

    if ((transport->receive_state ==
         SUNLITE_OTA_CAN_RX_WAIT_CONSECUTIVE) &&
        TimeReached(now_ms, transport->receive_deadline_ms)) {
        (void)FailReceive(transport, SUNLITE_OTA_CAN_ERROR_TIMEOUT);
    }
    if ((transport->transmit_state != SUNLITE_OTA_CAN_TX_IDLE) &&
        TimeReached(now_ms, transport->transmit_deadline_ms)) {
        FailTransmit(transport, SUNLITE_OTA_CAN_ERROR_TIMEOUT);
    }

    PollFlowControl(transport, now_ms);

    switch (transport->transmit_state) {
    case SUNLITE_OTA_CAN_TX_SEND_SINGLE:
        PollSingleFrame(transport);
        break;
    case SUNLITE_OTA_CAN_TX_SEND_FIRST:
        PollFirstFrame(transport, now_ms);
        break;
    case SUNLITE_OTA_CAN_TX_SEND_CONSECUTIVE:
        PollConsecutiveFrame(transport, now_ms);
        break;
    case SUNLITE_OTA_CAN_TX_IDLE:
    case SUNLITE_OTA_CAN_TX_WAIT_FLOW_CONTROL:
    default:
        break;
    }
}

bool SunliteOtaCanPeekFrame(const SunliteOtaCanTransport *transport,
                            const uint8_t **encoded,
                            size_t *encoded_length)
{
    if ((transport == NULL) || (encoded == NULL) ||
        (encoded_length == NULL) ||
        (transport->receive_state != SUNLITE_OTA_CAN_RX_COMPLETE)) {
        return false;
    }
    *encoded = transport->receive_buffer;
    *encoded_length = transport->receive_length;
    return true;
}

void SunliteOtaCanConsumeFrame(SunliteOtaCanTransport *transport)
{
    if ((transport != NULL) &&
        (transport->receive_state == SUNLITE_OTA_CAN_RX_COMPLETE)) {
        ResetReceive(transport);
    }
}

bool SunliteOtaCanTxBusy(const SunliteOtaCanTransport *transport)
{
    return (transport != NULL) &&
           (transport->transmit_state != SUNLITE_OTA_CAN_TX_IDLE);
}

bool SunliteOtaCanRxBusy(const SunliteOtaCanTransport *transport)
{
    return (transport != NULL) &&
           (transport->receive_state != SUNLITE_OTA_CAN_RX_IDLE);
}

SunliteOtaCanError SunliteOtaCanLastError(
    const SunliteOtaCanTransport *transport)
{
    return (transport == NULL) ? SUNLITE_OTA_CAN_ERROR_ARGUMENT :
        transport->last_error;
}

void SunliteOtaCanClearError(SunliteOtaCanTransport *transport)
{
    if (transport != NULL) {
        transport->last_error = SUNLITE_OTA_CAN_ERROR_NONE;
    }
}

void SunliteOtaCanAbort(SunliteOtaCanTransport *transport)
{
    if (transport == NULL) {
        return;
    }
    ResetTransmit(transport);
    ResetReceive(transport);
}
