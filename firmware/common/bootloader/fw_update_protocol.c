/**
 * @file    fw_update_protocol.c
 * @brief   CAN update transport, message framing, and integrity checks.
 *
 * Raw messages carry a header, payload, and CRC32. ISO-TP supplies message
 * boundaries and splits each message into eight-byte CAN frames.
 */

#include "fw_update_protocol.h"

#include <string.h>

#define FW_UPDATE_MAGIC_0 'S'
#define FW_UPDATE_MAGIC_1 'U'

/* MESSAGE ENCODING AND CRC */
/**
 * @brief Reads a 16-bit integer from two bytes in network byte order.
 */
uint16_t FirmwareUpdateReadBe16(const uint8_t* data)
{
    return ((uint16_t)data[0] << 8U) | (uint16_t)data[1];
}

/**
 * @brief Reads a 32-bit integer from four bytes in network byte order.
 */
uint32_t FirmwareUpdateReadBe32(const uint8_t* data)
{
    return ((uint32_t)data[0] << 24U) | ((uint32_t)data[1] << 16U) | ((uint32_t)data[2] << 8U) |
           (uint32_t)data[3];
}

/**
 * @brief Writes a 16-bit integer as two bytes in network byte order.
 */
void FirmwareUpdateWriteBe16(uint8_t* data, uint16_t value)
{
    data[0] = (uint8_t)(value >> 8U);
    data[1] = (uint8_t)value;
}

/**
 * @brief Writes a 32-bit integer as four bytes in network byte order.
 */
void FirmwareUpdateWriteBe32(uint8_t* data, uint32_t value)
{
    data[0] = (uint8_t)(value >> 24U);
    data[1] = (uint8_t)(value >> 16U);
    data[2] = (uint8_t)(value >> 8U);
    data[3] = (uint8_t)value;
}

/**
 * @brief Updates a running reflected IEEE CRC32 with input bytes.
 */
uint32_t FirmwareUpdateCrc32Update(uint32_t crc, const uint8_t* data, size_t length)
{
    for (size_t index = 0U; index < length; index++)
    {
        crc ^= data[index];
        for (uint32_t bit = 0U; bit < 8U; bit++)
        {
            crc = ((crc & 1U) != 0U) ? ((crc >> 1U) ^ 0xEDB88320U) : (crc >> 1U);
        }
    }
    return crc;
}

/**
 * @brief Computes the reflected IEEE CRC32 used for frames and firmware images.
 */
uint32_t FirmwareUpdateCrc32(const uint8_t* data, size_t length)
{
    return FirmwareUpdateCrc32Update(0xFFFFFFFFU, data, length) ^ 0xFFFFFFFFU;
}

/**
 * @brief Writes a raw message header, payload, and CRC32 into the output buffer.
 * @return Frame length, or zero for invalid arguments or insufficient capacity.
 */
size_t FirmwareUpdateFrameEncode(FirmwareUpdateMessageType type,
                                 uint32_t session_id,
                                 uint32_t sequence,
                                 const uint8_t* payload,
                                 uint16_t payload_length,
                                 uint8_t* frame,
                                 size_t capacity)
{
    size_t body_length = FW_UPDATE_HEADER_SIZE + payload_length;
    size_t frame_length = body_length + FW_UPDATE_FRAME_CRC_SIZE;
    if ((frame == NULL) || (payload_length > FW_UPDATE_MAX_RX_PAYLOAD) ||
        ((payload_length > 0U) && (payload == NULL)) || (frame_length > capacity))
    {
        return 0U;
    }
    frame[0] = FW_UPDATE_MAGIC_0;
    frame[1] = FW_UPDATE_MAGIC_1;
    frame[2] = FW_UPDATE_PROTOCOL_VERSION;
    frame[3] = (uint8_t)type;
    FirmwareUpdateWriteBe32(&frame[4], session_id);
    FirmwareUpdateWriteBe32(&frame[8], sequence);
    FirmwareUpdateWriteBe16(&frame[12], payload_length);
    if (payload_length > 0U)
    {
        memcpy(&frame[FW_UPDATE_HEADER_SIZE], payload, payload_length);
    }
    FirmwareUpdateWriteBe32(&frame[body_length], FirmwareUpdateCrc32(frame, body_length));
    return frame_length;
}

/**
 * @brief Checks a raw message's header, length, and CRC32 without copying its payload.
 * @note The message payload refers to frame; keep that buffer until processing finishes.
 */
bool FirmwareUpdateFrameDecode(const uint8_t* frame,
                               size_t frame_length,
                               FirmwareUpdateMessage* message)
{
    if ((frame == NULL) || (message == NULL) ||
        (frame_length < FW_UPDATE_HEADER_SIZE + FW_UPDATE_FRAME_CRC_SIZE) ||
        (frame_length > FW_UPDATE_MAX_FRAME_SIZE))
    {
        return false;
    }
    size_t body_length = frame_length - FW_UPDATE_FRAME_CRC_SIZE;
    if ((frame[0] != FW_UPDATE_MAGIC_0) || (frame[1] != FW_UPDATE_MAGIC_1) ||
        (frame[2] != FW_UPDATE_PROTOCOL_VERSION) || (frame[3] < FW_UPDATE_MESSAGE_HELLO) ||
        (frame[3] > FW_UPDATE_MESSAGE_ABORT) ||
        (FirmwareUpdateReadBe16(&frame[12]) != body_length - FW_UPDATE_HEADER_SIZE) ||
        (FirmwareUpdateReadBe32(&frame[body_length]) != FirmwareUpdateCrc32(frame, body_length)))
    {
        return false;
    }
    message->type = (FirmwareUpdateMessageType)frame[3];
    message->session_id = FirmwareUpdateReadBe32(&frame[4]);
    message->sequence = FirmwareUpdateReadBe32(&frame[8]);
    message->payload_length = FirmwareUpdateReadBe16(&frame[12]);
    message->payload = &frame[FW_UPDATE_HEADER_SIZE];
    return true;
}

/* CAN TRANSPORT: SEGMENTATION, FLOW CONTROL, AND TIMEOUTS */
/* The first byte's high nibble selects the ISO-TP frame type (PCI).
 * The low nibble holds a length, sequence number, or flow-control status. */
#define PCI_TYPE_MASK 0xF0U
#define PCI_SINGLE_FRAME 0x00U
#define PCI_FIRST_FRAME 0x10U
#define PCI_CONSECUTIVE_FRAME 0x20U
#define PCI_FLOW_CONTROL 0x30U
#define FLOW_STATUS_MASK 0x0FU
#define FLOW_STATUS_CONTINUE 0x00U
#define FLOW_STATUS_WAIT 0x01U
#define FLOW_STATUS_OVERFLOW 0x02U

#if FW_UPDATE_CAN_MAX_FRAME_SIZE > FW_UPDATE_CAN_MAX_ISOTP_LENGTH
#error "Firmware update frames exceed the classic ISO-TP 12-bit length field"
#endif

/* FUNCTION PROTOTYPES */
static bool DeadlineReached(uint32_t now_ms, uint32_t deadline_ms);
static uint32_t TimeoutDeadline(const FirmwareUpdateCanTransport* transport, uint32_t now_ms);
static void ResetTransmit(FirmwareUpdateCanTransport* transport);
static void ResetReceive(FirmwareUpdateCanTransport* transport);
static void FailTransmit(FirmwareUpdateCanTransport* transport, FirmwareUpdateCanError error);
static FirmwareUpdateCanReceiveResult FailReceive(FirmwareUpdateCanTransport* transport,
                                                  FirmwareUpdateCanError error);
static void PadCanFrame(uint8_t frame[FW_UPDATE_CAN_FRAME_DATA_SIZE]);
static bool SendTransportFrame(FirmwareUpdateCanTransport* transport,
                               const uint8_t data[FW_UPDATE_CAN_FRAME_DATA_SIZE]);
static FirmwareUpdateCanReceiveResult CompleteReceive(FirmwareUpdateCanTransport* transport);
static uint8_t DecodeStminMs(uint8_t encoded);
static FirmwareUpdateCanReceiveResult
HandleSingleFrame(FirmwareUpdateCanTransport* transport, const uint8_t* data, uint8_t dlc);
static void SendOverflowFlowControl(FirmwareUpdateCanTransport* transport);
static FirmwareUpdateCanReceiveResult HandleFirstFrame(FirmwareUpdateCanTransport* transport,
                                                       const uint8_t* data,
                                                       uint8_t dlc,
                                                       uint32_t now_ms);
static FirmwareUpdateCanReceiveResult HandleConsecutiveFrame(FirmwareUpdateCanTransport* transport,
                                                             const uint8_t* data,
                                                             uint8_t dlc,
                                                             uint32_t now_ms);
static FirmwareUpdateCanReceiveResult HandleFlowControl(FirmwareUpdateCanTransport* transport,
                                                        const uint8_t* data,
                                                        uint8_t dlc,
                                                        uint32_t now_ms);
static void PollFlowControl(FirmwareUpdateCanTransport* transport, uint32_t now_ms);
static void PollSingleFrame(FirmwareUpdateCanTransport* transport);
static void PollFirstFrame(FirmwareUpdateCanTransport* transport, uint32_t now_ms);
static void PollConsecutiveFrame(FirmwareUpdateCanTransport* transport, uint32_t now_ms);

/**
 * @brief Compares millisecond deadlines across tick-counter wraparound.
 */
static bool DeadlineReached(uint32_t now_ms, uint32_t deadline_ms)
{
    return (int32_t)(now_ms - deadline_ms) >= 0;
}

/**
 * @brief Calculates the next transport timeout deadline.
 */
static uint32_t TimeoutDeadline(const FirmwareUpdateCanTransport* transport, uint32_t now_ms)
{
    return now_ms + transport->timeout_ms;
}

/**
 * @brief Clears the active transmit transfer state.
 */
static void ResetTransmit(FirmwareUpdateCanTransport* transport)
{
    transport->transmit_state = FW_UPDATE_CAN_TX_IDLE;
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

/**
 * @brief Clears receive reassembly and any completed frame.
 */
static void ResetReceive(FirmwareUpdateCanTransport* transport)
{
    transport->receive_state = FW_UPDATE_CAN_RX_IDLE;
    transport->receive_length = 0U;
    transport->receive_offset = 0U;
    transport->receive_sequence = 0U;
    transport->receive_block_count = 0U;
    transport->flow_control_pending = false;
    transport->receive_deadline_ms = 0U;
}

/**
 * @brief Records a transport error and clears the failed transmit transfer.
 */
static void FailTransmit(FirmwareUpdateCanTransport* transport, FirmwareUpdateCanError error)
{
    ResetTransmit(transport);
    transport->last_error = error;
}

/**
 * @brief Records a transport error and clears the failed receive transfer.
 */
static FirmwareUpdateCanReceiveResult FailReceive(FirmwareUpdateCanTransport* transport,
                                                  FirmwareUpdateCanError error)
{
    ResetReceive(transport);
    transport->last_error = error;
    return FW_UPDATE_CAN_RX_ERROR;
}

/**
 * @brief Initializes unused CAN payload bytes with the transport padding value.
 */
static void PadCanFrame(uint8_t frame[FW_UPDATE_CAN_FRAME_DATA_SIZE])
{
    memset(frame, FW_UPDATE_CAN_FRAME_PAD_BYTE, FW_UPDATE_CAN_FRAME_DATA_SIZE);
}

/**
 * @brief Sends a CAN payload through the configured transport callback.
 */
static bool SendTransportFrame(FirmwareUpdateCanTransport* transport,
                               const uint8_t data[FW_UPDATE_CAN_FRAME_DATA_SIZE])
{
    return transport->send(
        transport->send_user, transport->transmit_id, data, FW_UPDATE_CAN_FRAME_DATA_SIZE);
}

/**
 * @brief Marks a fully reassembled protocol frame as ready for consumption.
 */
static FirmwareUpdateCanReceiveResult CompleteReceive(FirmwareUpdateCanTransport* transport)
{
    transport->receive_state = FW_UPDATE_CAN_RX_COMPLETE;
    transport->receive_deadline_ms = 0U;
    transport->flow_control_pending = false;
    return FW_UPDATE_CAN_RX_FRAME_COMPLETE;
}

/**
 * @brief Converts ISO-TP separation-time encoding to millisecond pacing.
 */
static uint8_t DecodeStminMs(uint8_t encoded)
{
    if (encoded <= 0x7FU)
    {
        return encoded;
    }
    if ((encoded >= 0xF1U) && (encoded <= 0xF9U))
    {
        /* HAL_GetTick() has millisecond resolution, so round 100-900 us up. */
        return 1U;
    }
    /* Reserved values are treated conservatively as zero delay. */
    return 0U;
}

/**
 * @brief Builds a 29-bit CAN identifier from destination and source addresses.
 */
uint32_t FirmwareUpdateCanAddressedId(uint8_t destination, uint8_t source)
{
    return FW_UPDATE_CAN_ID_BASE | ((uint32_t)destination << 8U) | (uint32_t)source;
}

/**
 * @brief Initializes transport buffers, addresses, callback, and timeout settings.
 */
void FirmwareUpdateCanInit(FirmwareUpdateCanTransport* transport,
                           uint8_t local_address,
                           uint8_t peer_address,
                           FirmwareUpdateCanSendFunction send,
                           void* send_user,
                           uint32_t timeout_ms)
{
    if (transport == NULL)
    {
        return;
    }
    memset(transport, 0, sizeof(*transport));
    transport->transmit_id = FirmwareUpdateCanAddressedId(peer_address, local_address);
    transport->receive_id = FirmwareUpdateCanAddressedId(local_address, peer_address);
    transport->send = send;
    transport->send_user = send_user;
    transport->timeout_ms = (timeout_ms == 0U) ? FW_UPDATE_CAN_DEFAULT_TIMEOUT_MS : timeout_ms;
    if (send == NULL)
    {
        transport->last_error = FW_UPDATE_CAN_ERROR_ARGUMENT;
    }
}

/**
 * @brief Copies and queues a complete encoded protocol frame for transmission.
 */
bool FirmwareUpdateCanSendFrame(FirmwareUpdateCanTransport* transport,
                                const uint8_t* encoded,
                                size_t encoded_length,
                                uint32_t now_ms)
{
    if ((transport == NULL) || (transport->send == NULL) || (encoded == NULL))
    {
        if (transport != NULL)
        {
            transport->last_error = FW_UPDATE_CAN_ERROR_ARGUMENT;
        }
        return false;
    }
    if ((encoded_length == 0U) || (encoded_length > sizeof(transport->transmit_buffer)) ||
        (encoded_length > FW_UPDATE_CAN_MAX_ISOTP_LENGTH))
    {
        transport->last_error = FW_UPDATE_CAN_ERROR_FRAME_TOO_LARGE;
        return false;
    }
    if (transport->transmit_state != FW_UPDATE_CAN_TX_IDLE)
    {
        transport->last_error = FW_UPDATE_CAN_ERROR_BUSY;
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
    transport->transmit_state = (encoded_length <= FW_UPDATE_CAN_SINGLE_FRAME_DATA_SIZE)
                                    ? FW_UPDATE_CAN_TX_SEND_SINGLE
                                    : FW_UPDATE_CAN_TX_SEND_FIRST;
    transport->last_error = FW_UPDATE_CAN_ERROR_NONE;
    FirmwareUpdateCanPoll(transport, now_ms);

    return transport->transmit_state != FW_UPDATE_CAN_TX_IDLE ||
           transport->last_error == FW_UPDATE_CAN_ERROR_NONE;
}

/**
 * @brief Validates and receives a complete single-frame CAN payload.
 */
static FirmwareUpdateCanReceiveResult
HandleSingleFrame(FirmwareUpdateCanTransport* transport, const uint8_t* data, uint8_t dlc)
{
    uint8_t length = data[0] & 0x0FU;
    if ((length == 0U) || (length > FW_UPDATE_CAN_SINGLE_FRAME_DATA_SIZE) ||
        ((uint8_t)(length + 1U) > dlc))
    {
        return FailReceive(transport, FW_UPDATE_CAN_ERROR_MALFORMED_FRAME);
    }
    if (transport->receive_state == FW_UPDATE_CAN_RX_COMPLETE)
    {
        transport->last_error = FW_UPDATE_CAN_ERROR_BUSY;
        return FW_UPDATE_CAN_RX_ERROR;
    }
    ResetReceive(transport);
    memcpy(transport->receive_buffer, &data[1], length);
    transport->receive_length = length;
    transport->receive_offset = length;
    return CompleteReceive(transport);
}

/**
 * @brief Reports that the incoming transfer exceeds receive capacity.
 */
static void SendOverflowFlowControl(FirmwareUpdateCanTransport* transport)
{
    uint8_t frame[FW_UPDATE_CAN_FRAME_DATA_SIZE];
    PadCanFrame(frame);
    frame[0] = PCI_FLOW_CONTROL | FLOW_STATUS_OVERFLOW;
    frame[1] = 0U;
    frame[2] = 0U;
    (void)SendTransportFrame(transport, frame);
}

/**
 * @brief Starts multi-frame reassembly and requests sender flow control.
 */
static FirmwareUpdateCanReceiveResult HandleFirstFrame(FirmwareUpdateCanTransport* transport,
                                                       const uint8_t* data,
                                                       uint8_t dlc,
                                                       uint32_t now_ms)
{
    if (dlc != FW_UPDATE_CAN_FRAME_DATA_SIZE)
    {
        return FailReceive(transport, FW_UPDATE_CAN_ERROR_MALFORMED_FRAME);
    }
    uint16_t length = (uint16_t)(((uint16_t)(data[0] & 0x0FU) << 8U) | data[1]);
    if (length <= FW_UPDATE_CAN_SINGLE_FRAME_DATA_SIZE)
    {
        return FailReceive(transport, FW_UPDATE_CAN_ERROR_MALFORMED_FRAME);
    }
    if ((length > FW_UPDATE_CAN_MAX_FRAME_SIZE) || (length > FW_UPDATE_CAN_MAX_ISOTP_LENGTH))
    {
        ResetReceive(transport);
        SendOverflowFlowControl(transport);
        transport->last_error = FW_UPDATE_CAN_ERROR_FRAME_TOO_LARGE;
        return FW_UPDATE_CAN_RX_ERROR;
    }
    if (transport->receive_state == FW_UPDATE_CAN_RX_COMPLETE)
    {
        transport->last_error = FW_UPDATE_CAN_ERROR_BUSY;
        return FW_UPDATE_CAN_RX_ERROR;
    }

    /* A fresh first frame cleanly restarts an incomplete outer request. */
    ResetReceive(transport);
    memcpy(transport->receive_buffer, &data[2], FW_UPDATE_CAN_FIRST_FRAME_DATA_SIZE);
    transport->receive_length = length;
    transport->receive_offset = FW_UPDATE_CAN_FIRST_FRAME_DATA_SIZE;
    transport->receive_sequence = 1U;
    transport->receive_block_count = 0U;
    transport->receive_state = FW_UPDATE_CAN_RX_WAIT_CONSECUTIVE;
    transport->flow_control_pending = true;
    transport->receive_deadline_ms = TimeoutDeadline(transport, now_ms);
    FirmwareUpdateCanPoll(transport, now_ms);
    return FW_UPDATE_CAN_RX_ACCEPTED;
}

/**
 * @brief Checks sequence order and appends the next receive segment.
 */
static FirmwareUpdateCanReceiveResult HandleConsecutiveFrame(FirmwareUpdateCanTransport* transport,
                                                             const uint8_t* data,
                                                             uint8_t dlc,
                                                             uint32_t now_ms)
{
    if ((transport->receive_state != FW_UPDATE_CAN_RX_WAIT_CONSECUTIVE) ||
        transport->flow_control_pending)
    {
        return FailReceive(transport, FW_UPDATE_CAN_ERROR_UNEXPECTED_FRAME);
    }
    if ((data[0] & 0x0FU) != transport->receive_sequence)
    {
        return FailReceive(transport, FW_UPDATE_CAN_ERROR_SEQUENCE);
    }

    uint16_t remaining = transport->receive_length - transport->receive_offset;
    uint8_t copy_length = (remaining > FW_UPDATE_CAN_CONSECUTIVE_DATA_SIZE)
                              ? FW_UPDATE_CAN_CONSECUTIVE_DATA_SIZE
                              : (uint8_t)remaining;
    if ((dlc < 2U) || ((uint8_t)(dlc - 1U) < copy_length))
    {
        return FailReceive(transport, FW_UPDATE_CAN_ERROR_MALFORMED_FRAME);
    }

    memcpy(&transport->receive_buffer[transport->receive_offset], &data[1], copy_length);
    transport->receive_offset += copy_length;
    transport->receive_sequence = (uint8_t)((transport->receive_sequence + 1U) & 0x0FU);
    transport->receive_block_count++;
    transport->receive_deadline_ms = TimeoutDeadline(transport, now_ms);

    if (transport->receive_offset == transport->receive_length)
    {
        return CompleteReceive(transport);
    }
    if (transport->receive_block_count >= FW_UPDATE_CAN_DEFAULT_BLOCK_SIZE)
    {
        transport->receive_block_count = 0U;
        transport->flow_control_pending = true;
        FirmwareUpdateCanPoll(transport, now_ms);
    }
    return FW_UPDATE_CAN_RX_ACCEPTED;
}

/**
 * @brief Applies the peer’s block size, pacing, and flow-control status.
 */
static FirmwareUpdateCanReceiveResult HandleFlowControl(FirmwareUpdateCanTransport* transport,
                                                        const uint8_t* data,
                                                        uint8_t dlc,
                                                        uint32_t now_ms)
{
    if (dlc < 3U)
    {
        FailTransmit(transport, FW_UPDATE_CAN_ERROR_MALFORMED_FRAME);
        return FW_UPDATE_CAN_RX_ERROR;
    }
    if (transport->transmit_state != FW_UPDATE_CAN_TX_WAIT_FLOW_CONTROL)
    {
        transport->last_error = FW_UPDATE_CAN_ERROR_UNEXPECTED_FRAME;
        return FW_UPDATE_CAN_RX_ERROR;
    }

    switch (data[0] & FLOW_STATUS_MASK)
    {
    case FLOW_STATUS_CONTINUE:
        transport->transmit_block_size = data[1];
        transport->transmit_block_count = 0U;
        transport->transmit_stmin_ms = DecodeStminMs(data[2]);
        transport->transmit_wait_count = 0U;
        transport->transmit_next_frame_ms = now_ms + transport->transmit_stmin_ms;
        transport->transmit_deadline_ms = TimeoutDeadline(transport, now_ms);
        transport->transmit_state = FW_UPDATE_CAN_TX_SEND_CONSECUTIVE;
        return FW_UPDATE_CAN_RX_ACCEPTED;
    case FLOW_STATUS_WAIT:
        transport->transmit_wait_count++;
        if (transport->transmit_wait_count > FW_UPDATE_CAN_MAX_WAIT_FRAMES)
        {
            FailTransmit(transport, FW_UPDATE_CAN_ERROR_FLOW_CONTROL_WAIT_LIMIT);
            return FW_UPDATE_CAN_RX_ERROR;
        }
        transport->transmit_deadline_ms = TimeoutDeadline(transport, now_ms);
        return FW_UPDATE_CAN_RX_ACCEPTED;
    case FLOW_STATUS_OVERFLOW:
        FailTransmit(transport, FW_UPDATE_CAN_ERROR_FLOW_CONTROL_OVERFLOW);
        return FW_UPDATE_CAN_RX_ERROR;
    default:
        FailTransmit(transport, FW_UPDATE_CAN_ERROR_MALFORMED_FRAME);
        return FW_UPDATE_CAN_RX_ERROR;
    }
}

/**
 * @brief Processes one addressed classic-CAN data frame and advances reassembly.
 */
FirmwareUpdateCanReceiveResult FirmwareUpdateCanOnFrame(FirmwareUpdateCanTransport* transport,
                                                        uint32_t extended_id,
                                                        const uint8_t* data,
                                                        uint8_t dlc,
                                                        uint32_t now_ms)
{
    if (transport == NULL)
    {
        return FW_UPDATE_CAN_RX_ERROR;
    }
    if (extended_id != transport->receive_id)
    {
        return FW_UPDATE_CAN_RX_IGNORED;
    }
    if ((data == NULL) || (dlc == 0U) || (dlc > FW_UPDATE_CAN_FRAME_DATA_SIZE))
    {
        transport->last_error = FW_UPDATE_CAN_ERROR_ARGUMENT;
        return FW_UPDATE_CAN_RX_ERROR;
    }

    switch (data[0] & PCI_TYPE_MASK)
    {
    case PCI_SINGLE_FRAME:
        return HandleSingleFrame(transport, data, dlc);
    case PCI_FIRST_FRAME:
        return HandleFirstFrame(transport, data, dlc, now_ms);
    case PCI_CONSECUTIVE_FRAME:
        return HandleConsecutiveFrame(transport, data, dlc, now_ms);
    case PCI_FLOW_CONTROL:
        return HandleFlowControl(transport, data, dlc, now_ms);
    default:
        return FailReceive(transport, FW_UPDATE_CAN_ERROR_MALFORMED_FRAME);
    }
}

/**
 * @brief Sends pending receive flow control when a mailbox becomes available.
 */
static void PollFlowControl(FirmwareUpdateCanTransport* transport, uint32_t now_ms)
{
    if (!transport->flow_control_pending)
    {
        return;
    }
    uint8_t frame[FW_UPDATE_CAN_FRAME_DATA_SIZE];
    PadCanFrame(frame);
    frame[0] = PCI_FLOW_CONTROL | FLOW_STATUS_CONTINUE;
    frame[1] = FW_UPDATE_CAN_DEFAULT_BLOCK_SIZE;
    frame[2] = FW_UPDATE_CAN_DEFAULT_STMIN_MS;

    /* Clear first so a synchronous test adapter can feed the first CF back. */
    transport->flow_control_pending = false;
    if (!SendTransportFrame(transport, frame))
    {
        transport->flow_control_pending = true;
    }
    else
    {
        transport->receive_deadline_ms = TimeoutDeadline(transport, now_ms);
    }
}

/**
 * @brief Transmits a queued payload that fits in one CAN frame.
 */
static void PollSingleFrame(FirmwareUpdateCanTransport* transport)
{
    uint8_t frame[FW_UPDATE_CAN_FRAME_DATA_SIZE];
    PadCanFrame(frame);
    frame[0] = (uint8_t)transport->transmit_length;
    memcpy(&frame[1], transport->transmit_buffer, transport->transmit_length);

    transport->transmit_state = FW_UPDATE_CAN_TX_IDLE;
    if (SendTransportFrame(transport, frame))
    {
        transport->transmit_length = 0U;
        transport->transmit_offset = 0U;
    }
    else
    {
        transport->transmit_state = FW_UPDATE_CAN_TX_SEND_SINGLE;
    }
}

/**
 * @brief Starts a segmented transmit transfer with its length and first bytes.
 */
static void PollFirstFrame(FirmwareUpdateCanTransport* transport, uint32_t now_ms)
{
    uint8_t frame[FW_UPDATE_CAN_FRAME_DATA_SIZE];
    PadCanFrame(frame);
    frame[0] = PCI_FIRST_FRAME | (uint8_t)(transport->transmit_length >> 8U);
    frame[1] = (uint8_t)transport->transmit_length;
    memcpy(&frame[2], transport->transmit_buffer, FW_UPDATE_CAN_FIRST_FRAME_DATA_SIZE);

    transport->transmit_offset = FW_UPDATE_CAN_FIRST_FRAME_DATA_SIZE;
    transport->transmit_state = FW_UPDATE_CAN_TX_WAIT_FLOW_CONTROL;
    if (!SendTransportFrame(transport, frame))
    {
        transport->transmit_offset = 0U;
        transport->transmit_state = FW_UPDATE_CAN_TX_SEND_FIRST;
    }
    else
    {
        transport->transmit_deadline_ms = TimeoutDeadline(transport, now_ms);
    }
}

/**
 * @brief Transmits the next segment while respecting peer pacing and block limits.
 */
static void PollConsecutiveFrame(FirmwareUpdateCanTransport* transport, uint32_t now_ms)
{
    if (!DeadlineReached(now_ms, transport->transmit_next_frame_ms))
    {
        return;
    }
    uint16_t remaining = transport->transmit_length - transport->transmit_offset;
    uint8_t copy_length = (remaining > FW_UPDATE_CAN_CONSECUTIVE_DATA_SIZE)
                              ? FW_UPDATE_CAN_CONSECUTIVE_DATA_SIZE
                              : (uint8_t)remaining;
    uint8_t frame[FW_UPDATE_CAN_FRAME_DATA_SIZE];
    PadCanFrame(frame);
    frame[0] = PCI_CONSECUTIVE_FRAME | (transport->transmit_sequence & 0x0FU);
    memcpy(&frame[1], &transport->transmit_buffer[transport->transmit_offset], copy_length);

    uint16_t old_offset = transport->transmit_offset;
    uint8_t old_sequence = transport->transmit_sequence;
    uint8_t old_block_count = transport->transmit_block_count;
    uint32_t old_deadline_ms = transport->transmit_deadline_ms;
    uint32_t old_next_frame_ms = transport->transmit_next_frame_ms;
    FirmwareUpdateCanTransmitState old_state = transport->transmit_state;

    transport->transmit_offset += copy_length;
    transport->transmit_sequence = (uint8_t)((transport->transmit_sequence + 1U) & 0x0FU);
    transport->transmit_block_count++;
    transport->transmit_deadline_ms = TimeoutDeadline(transport, now_ms);
    transport->transmit_next_frame_ms = now_ms + transport->transmit_stmin_ms;
    if (transport->transmit_offset == transport->transmit_length)
    {
        transport->transmit_state = FW_UPDATE_CAN_TX_IDLE;
    }
    else if ((transport->transmit_block_size != 0U) &&
             (transport->transmit_block_count >= transport->transmit_block_size))
    {
        transport->transmit_block_count = 0U;
        transport->transmit_state = FW_UPDATE_CAN_TX_WAIT_FLOW_CONTROL;
    }

    if (!SendTransportFrame(transport, frame))
    {
        transport->transmit_offset = old_offset;
        transport->transmit_sequence = old_sequence;
        transport->transmit_block_count = old_block_count;
        transport->transmit_deadline_ms = old_deadline_ms;
        transport->transmit_next_frame_ms = old_next_frame_ms;
        transport->transmit_state = old_state;
        return;
    }
    if (transport->transmit_state == FW_UPDATE_CAN_TX_IDLE)
    {
        transport->transmit_length = 0U;
        transport->transmit_offset = 0U;
    }
}

/**
 * @brief Services transmit flow control, frame pacing, and receive timeouts.
 */
void FirmwareUpdateCanPoll(FirmwareUpdateCanTransport* transport, uint32_t now_ms)
{
    if ((transport == NULL) || (transport->send == NULL))
    {
        return;
    }

    if ((transport->receive_state == FW_UPDATE_CAN_RX_WAIT_CONSECUTIVE) &&
        DeadlineReached(now_ms, transport->receive_deadline_ms))
    {
        (void)FailReceive(transport, FW_UPDATE_CAN_ERROR_TIMEOUT);
    }
    if ((transport->transmit_state != FW_UPDATE_CAN_TX_IDLE) &&
        DeadlineReached(now_ms, transport->transmit_deadline_ms))
    {
        FailTransmit(transport, FW_UPDATE_CAN_ERROR_TIMEOUT);
    }

    PollFlowControl(transport, now_ms);

    switch (transport->transmit_state)
    {
    case FW_UPDATE_CAN_TX_SEND_SINGLE:
        PollSingleFrame(transport);
        break;
    case FW_UPDATE_CAN_TX_SEND_FIRST:
        PollFirstFrame(transport, now_ms);
        break;
    case FW_UPDATE_CAN_TX_SEND_CONSECUTIVE:
        PollConsecutiveFrame(transport, now_ms);
        break;
    case FW_UPDATE_CAN_TX_IDLE:
    case FW_UPDATE_CAN_TX_WAIT_FLOW_CONTROL:
    default:
        break;
    }
}

/**
 * @brief Exposes the completed receive buffer without consuming it.
 */
bool FirmwareUpdateCanPeekFrame(const FirmwareUpdateCanTransport* transport,
                                const uint8_t** encoded,
                                size_t* encoded_length)
{
    if ((transport == NULL) || (encoded == NULL) || (encoded_length == NULL) ||
        (transport->receive_state != FW_UPDATE_CAN_RX_COMPLETE))
    {
        return false;
    }
    *encoded = transport->receive_buffer;
    *encoded_length = transport->receive_length;
    return true;
}

/**
 * @brief Releases a completed receive frame.
 */
void FirmwareUpdateCanConsumeFrame(FirmwareUpdateCanTransport* transport)
{
    if ((transport != NULL) && (transport->receive_state == FW_UPDATE_CAN_RX_COMPLETE))
    {
        ResetReceive(transport);
    }
}

/**
 * @brief Reports whether a transmit transfer is active.
 */
bool FirmwareUpdateCanTxBusy(const FirmwareUpdateCanTransport* transport)
{
    return (transport != NULL) && (transport->transmit_state != FW_UPDATE_CAN_TX_IDLE);
}

/**
 * @brief Cancels transfer state and discards buffered frame data.
 */
void FirmwareUpdateCanAbort(FirmwareUpdateCanTransport* transport)
{
    if (transport == NULL)
    {
        return;
    }
    ResetTransmit(transport);
    ResetReceive(transport);
}
