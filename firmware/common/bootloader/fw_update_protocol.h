/**
 * @file    fw_update_protocol.h
 * @brief   CAN update transport, message framing, and integrity checks.
 */

#ifndef FW_UPDATE_PROTOCOL_H
#define FW_UPDATE_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* MESSAGE FORMAT AND BYTE ENCODING */
#define FW_UPDATE_PROTOCOL_VERSION 3U
#define FW_UPDATE_HEADER_SIZE 14U
#define FW_UPDATE_FRAME_CRC_SIZE 4U
#define FW_UPDATE_BOARD_INFO_SIZE 23U
#define FW_UPDATE_ACK_SIZE 6U
#define FW_UPDATE_BEGIN_UPDATE_SIZE 16U
#define FW_UPDATE_DATA_OFFSET_SIZE 4U
#define FW_UPDATE_MAX_CHUNK_SIZE 1024U
#define FW_UPDATE_MAX_RX_PAYLOAD (FW_UPDATE_DATA_OFFSET_SIZE + FW_UPDATE_MAX_CHUNK_SIZE)
#define FW_UPDATE_MAX_FRAME_SIZE                                                                   \
    (FW_UPDATE_HEADER_SIZE + FW_UPDATE_MAX_RX_PAYLOAD + FW_UPDATE_FRAME_CRC_SIZE)

typedef enum
{
    FW_UPDATE_MESSAGE_HELLO = 1,
    FW_UPDATE_MESSAGE_BOARD_INFO = 2,
    FW_UPDATE_MESSAGE_ENTER_BOOTLOADER = 3,
    FW_UPDATE_MESSAGE_BEGIN_UPDATE = 4,
    FW_UPDATE_MESSAGE_DATA = 5,
    FW_UPDATE_MESSAGE_END_UPDATE = 6,
    FW_UPDATE_MESSAGE_ACK = 7,
    FW_UPDATE_MESSAGE_NACK = 8,
    FW_UPDATE_MESSAGE_REBOOT = 10,
    FW_UPDATE_MESSAGE_ABORT = 11,
} FirmwareUpdateMessageType;

typedef enum
{
    FW_UPDATE_BOARD_APPLICATION = 0,
    FW_UPDATE_BOARD_BOOTLOADER = 1,
    FW_UPDATE_BOARD_UPDATE_IN_PROGRESS = 2,
    FW_UPDATE_BOARD_PENDING_IMAGE = 3,
} FirmwareUpdateBoardState;

typedef enum
{
    FW_UPDATE_CAPABILITY_UPDATE_ALLOWED = 1U << 0,
} FirmwareUpdateCapability;

typedef enum
{
    FW_UPDATE_STATUS_OK = 0,
    FW_UPDATE_STATUS_BAD_STATE = 1,
    FW_UPDATE_STATUS_BAD_TARGET = 2,
    FW_UPDATE_STATUS_BAD_CRC = 3,
    FW_UPDATE_STATUS_BAD_OFFSET = 4,
    FW_UPDATE_STATUS_FLASH_ERROR = 5,
    FW_UPDATE_STATUS_IMAGE_TOO_LARGE = 6,
    FW_UPDATE_STATUS_UPDATE_NOT_ALLOWED = 7,
} FirmwareUpdateStatus;

typedef struct
{
    FirmwareUpdateMessageType type;
    uint32_t session_id;
    uint32_t sequence;
    uint16_t payload_length;
    const uint8_t* payload;
} FirmwareUpdateMessage;

/**
 * @brief Reads a 16-bit integer from two bytes in network byte order.
 */
uint16_t FirmwareUpdateReadBe16(const uint8_t* data);

/**
 * @brief Reads a 32-bit integer from four bytes in network byte order.
 */
uint32_t FirmwareUpdateReadBe32(const uint8_t* data);

/**
 * @brief Writes a 16-bit integer as two bytes in network byte order.
 */
void FirmwareUpdateWriteBe16(uint8_t* data, uint16_t value);

/**
 * @brief Writes a 32-bit integer as four bytes in network byte order.
 */
void FirmwareUpdateWriteBe32(uint8_t* data, uint32_t value);

/**
 * @brief Writes a raw message header, payload, and CRC32 into the output buffer.
 */
size_t FirmwareUpdateFrameEncode(FirmwareUpdateMessageType type,
                                 uint32_t session_id,
                                 uint32_t sequence,
                                 const uint8_t* payload,
                                 uint16_t payload_length,
                                 uint8_t* encoded,
                                 size_t encoded_capacity);

/**
 * @brief Checks a raw frame and returns a payload view valid until the frame buffer is reused.
 */
bool FirmwareUpdateFrameDecode(const uint8_t* encoded,
                               size_t encoded_length,
                               FirmwareUpdateMessage* message);

/**
 * @brief Adds bytes to a running CRC32 (start with 0xFFFFFFFF, finish with XOR 0xFFFFFFFF).
 */
uint32_t FirmwareUpdateCrc32Update(uint32_t crc, const uint8_t* data, size_t length);

/**
 * @brief Computes the reflected IEEE CRC32 used for frames and firmware images.
 */
uint32_t FirmwareUpdateCrc32(const uint8_t* data, size_t length);

/* CAN TRANSPORT: SEGMENTATION AND FLOW CONTROL */
/*
 * Firmware update over classic CAN uses the ISO-TP normal-fixed 29-bit identifier
 * layout. Its payload is one raw firmware-update message, including CRC32.
 * ISO-TP lengths supply boundaries; zero bytes need no escaping.
 *
 *     0x18DA<destination address><source address>
 *
 * The laptop uses tester address 0xF1; each board has its own node address.
 */
#define FW_UPDATE_CAN_ID_BASE 0x18DA0000U
#define FW_UPDATE_CAN_TESTER_ADDRESS 0xF1U
#define FW_UPDATE_CAN_NODE_MDI 0x10U
#define FW_UPDATE_CAN_NODE_DRD 0x11U
#define FW_UPDATE_CAN_NODE_STR 0x12U
#define FW_UPDATE_CAN_FRAME_DATA_SIZE 8U
#define FW_UPDATE_CAN_SINGLE_FRAME_DATA_SIZE 7U
#define FW_UPDATE_CAN_FIRST_FRAME_DATA_SIZE 6U
#define FW_UPDATE_CAN_CONSECUTIVE_DATA_SIZE 7U
#define FW_UPDATE_CAN_MAX_ISOTP_LENGTH 4095U
#define FW_UPDATE_CAN_DEFAULT_TIMEOUT_MS 250U
#define FW_UPDATE_CAN_DEFAULT_BLOCK_SIZE 3U
#define FW_UPDATE_CAN_DEFAULT_STMIN_MS 2U
#define FW_UPDATE_CAN_MAX_WAIT_FRAMES 3U
#define FW_UPDATE_CAN_FRAME_PAD_BYTE 0xAAU
#define FW_UPDATE_CAN_MAX_FRAME_SIZE FW_UPDATE_MAX_FRAME_SIZE

/* Boards transmit only ACK/NACK/BOARD_INFO; host transport tests override this. */
#ifndef FW_UPDATE_CAN_TX_FRAME_SIZE
#define FW_UPDATE_CAN_TX_FRAME_SIZE 64U
#endif

typedef bool (*FirmwareUpdateCanSendFunction)(void* user,
                                              uint32_t extended_id,
                                              const uint8_t data[8],
                                              uint8_t dlc);

typedef enum
{
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

typedef enum
{
    FW_UPDATE_CAN_RX_IGNORED = 0,
    FW_UPDATE_CAN_RX_ACCEPTED,
    FW_UPDATE_CAN_RX_FRAME_COMPLETE,
    FW_UPDATE_CAN_RX_ERROR,
} FirmwareUpdateCanReceiveResult;

typedef enum
{
    FW_UPDATE_CAN_TX_IDLE = 0,
    FW_UPDATE_CAN_TX_SEND_SINGLE,
    FW_UPDATE_CAN_TX_SEND_FIRST,
    FW_UPDATE_CAN_TX_WAIT_FLOW_CONTROL,
    FW_UPDATE_CAN_TX_SEND_CONSECUTIVE,
} FirmwareUpdateCanTransmitState;

typedef enum
{
    FW_UPDATE_CAN_RX_IDLE = 0,
    FW_UPDATE_CAN_RX_WAIT_CONSECUTIVE,
    FW_UPDATE_CAN_RX_COMPLETE,
} FirmwareUpdateCanReceiveState;

typedef struct
{
    /* Addressing and the board-specific CAN send callback. */
    uint32_t transmit_id;
    uint32_t receive_id;
    FirmwareUpdateCanSendFunction send;
    void* send_user;
    uint32_t timeout_ms;

    /* Outgoing message progress; flow control limits blocks and frame spacing. */
    FirmwareUpdateCanTransmitState transmit_state;
    uint8_t transmit_buffer[FW_UPDATE_CAN_TX_FRAME_SIZE];
    uint16_t transmit_length;
    uint16_t transmit_offset;
    uint8_t transmit_sequence;
    uint8_t transmit_block_size;
    uint8_t transmit_block_count;
    uint8_t transmit_stmin_ms; /* Minimum separation time between CAN frames. */
    uint8_t transmit_wait_count;
    uint32_t transmit_deadline_ms;
    uint32_t transmit_next_frame_ms;

    /* Incoming message reassembly; COMPLETE holds the buffer until consumed. */
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

/**
 * @brief Builds a 29-bit CAN identifier from destination and source addresses.
 */
uint32_t FirmwareUpdateCanAddressedId(uint8_t destination, uint8_t source);

/**
 * @brief Initializes transport buffers, addresses, callback, and timeout settings.
 */
void FirmwareUpdateCanInit(FirmwareUpdateCanTransport* transport,
                           uint8_t local_address,
                           uint8_t peer_address,
                           FirmwareUpdateCanSendFunction send,
                           void* send_user,
                           uint32_t timeout_ms);

/*
 * Queue one complete raw firmware-update message for transmission. The caller
 * may reuse its input buffer as soon as this function returns.
 */
/**
 * @brief Copies and queues a complete encoded protocol frame for transmission.
 */
bool FirmwareUpdateCanSendFrame(FirmwareUpdateCanTransport* transport,
                                const uint8_t* encoded,
                                size_t encoded_length,
                                uint32_t now_ms);

/*
 * Feed one classic-CAN data frame into the transport.  Board adapters should
 * configure the addressed OTA identifier into FIFO1 and call this function
 * from a non-ISR polling context.
 */
/**
 * @brief Processes one addressed classic-CAN data frame and advances reassembly.
 */
FirmwareUpdateCanReceiveResult FirmwareUpdateCanOnFrame(FirmwareUpdateCanTransport* transport,
                                                        uint32_t extended_id,
                                                        const uint8_t* data,
                                                        uint8_t dlc,
                                                        uint32_t now_ms);

/**
 * @brief Services transmit flow control, frame pacing, and receive timeouts.
 */
void FirmwareUpdateCanPoll(FirmwareUpdateCanTransport* transport, uint32_t now_ms);

/* The returned frame remains valid until consumed. */
/**
 * @brief Exposes the completed receive buffer without consuming it.
 */
bool FirmwareUpdateCanPeekFrame(const FirmwareUpdateCanTransport* transport,
                                const uint8_t** encoded,
                                size_t* encoded_length);

/**
 * @brief Releases a completed receive frame.
 */
void FirmwareUpdateCanConsumeFrame(FirmwareUpdateCanTransport* transport);

/**
 * @brief Reports whether a transmit transfer is active.
 */
bool FirmwareUpdateCanTxBusy(const FirmwareUpdateCanTransport* transport);

/**
 * @brief Cancels transfer state and discards buffered frame data.
 */
void FirmwareUpdateCanAbort(FirmwareUpdateCanTransport* transport);

#endif /* FW_UPDATE_PROTOCOL_H */
