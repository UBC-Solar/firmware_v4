#include "fw_update_app.h"

#include "bootloader_boot_request.h"
#include "bootloader_config.h"
#include "can.h"
#include "stm32f1xx_hal.h"
#include "fw_update_can.h"
#include "fw_update_protocol.h"
#include "usart.h"

#include <string.h>

#ifndef FW_UPDATE_TARGET_ID
#error "FW_UPDATE_TARGET_ID must be defined by the board target"
#endif
#ifndef FW_UPDATE_HARDWARE_REVISION
#error "FW_UPDATE_HARDWARE_REVISION must be defined by the board target"
#endif
#ifndef FW_UPDATE_FIRMWARE_VERSION
#error "FW_UPDATE_FIRMWARE_VERSION must be defined by the board target"
#endif
#ifndef FW_UPDATE_PUBLIC_KEY_CONFIGURED
#define FW_UPDATE_PUBLIC_KEY_CONFIGURED 0
#endif
#ifndef FW_UPDATE_SLOT_SIZE_BYTES
#define FW_UPDATE_SLOT_SIZE_BYTES BOOTLOADER_APP_MAX_SIZE_BYTES
#endif
#ifndef FW_UPDATE_ALLOW_UNSAFE_BENCH_UPDATE
#define FW_UPDATE_ALLOW_UNSAFE_BENCH_UPDATE 0
#endif

#define FW_UPDATE_BOOTLOADER_VERSION 1U
#define FW_UPDATE_RX_RING_SIZE       4096U
#define FW_UPDATE_RX_RING_MASK       (FW_UPDATE_RX_RING_SIZE - 1U)
#define FW_UPDATE_SESSION_TIMEOUT_MS  30000U
#define FW_UPDATE_CAN_PROXY_TIMEOUT_MS 7000U

#if (FW_UPDATE_RX_RING_SIZE & FW_UPDATE_RX_RING_MASK) != 0
#error "FW_UPDATE_RX_RING_SIZE must be a power of two"
#endif

static uint8_t encoded_frame[FW_UPDATE_MAX_ENCODED_FRAME];
static uint8_t raw_frame[FW_UPDATE_MAX_RAW_FRAME];
static size_t encoded_length;
static bool discard_until_delimiter;
static volatile bool ota_session_active;
static uint8_t rx_ring[FW_UPDATE_RX_RING_SIZE];
static uint8_t interrupt_rx_byte;
static volatile uint16_t rx_head;
static volatile uint16_t rx_tail;
static volatile bool rx_overflow;
static uint32_t last_session_activity;
static FirmwareUpdateCanLink can_proxy;
static bool can_proxy_initialized;
static uint32_t routed_session;
static uint32_t routed_target = FW_UPDATE_TARGET_ID;
static uint8_t proxy_raw_frame[FW_UPDATE_MAX_RAW_FRAME];
static uint8_t proxy_uart_response[64];

__attribute__((weak)) bool FirmwareUpdateBoardUpdateAllowed(void)
{
    /* Override this hook when TEL receives the real vehicle-level interlock.
     * Debug builds can explicitly enable the bench-only fallback in CMake. */
    return FW_UPDATE_ALLOW_UNSAFE_BENCH_UPDATE != 0;
}

__attribute__((weak)) void FirmwareUpdateBoardTransmitAborted(void)
{
}

__attribute__((weak)) void FirmwareUpdateBoardYield(void)
{
    HAL_Delay(1U);
}

bool FirmwareUpdateApplicationSessionActive(void)
{
    return ota_session_active;
}

static bool SendFrame(const FirmwareUpdateMessage *request,
                      FirmwareUpdateMessageType type,
                      const uint8_t *payload,
                      uint16_t payload_length)
{
    uint8_t response[64];
    size_t response_length = FirmwareUpdateFrameEncode(
        type,
        request->session_id,
        request->sequence,
        payload,
        payload_length,
        response,
        sizeof(response));
    return (response_length > 0U) &&
           (HAL_UART_Transmit(&huart5,
                              response,
                              (uint16_t)response_length,
                              1000U) == HAL_OK);
}

static bool SendAck(const FirmwareUpdateMessage *request,
                    FirmwareUpdateStatus status,
                    uint32_t next_offset)
{
    uint8_t payload[FW_UPDATE_ACK_SIZE];
    payload[0] = (uint8_t)request->type;
    payload[1] = (uint8_t)status;
    FirmwareUpdateWriteBe32(&payload[2], next_offset);
    FirmwareUpdateMessageType response_type = (status == FW_UPDATE_STATUS_OK) ?
        FW_UPDATE_MESSAGE_ACK : FW_UPDATE_MESSAGE_NACK;
    return SendFrame(request, response_type, payload, sizeof(payload));
}

static bool SendBoardInfo(const FirmwareUpdateMessage *request)
{
    bool update_allowed = FirmwareUpdateBoardUpdateAllowed();
    uint32_t capabilities = 0U;
    if (update_allowed) {
        capabilities |= FW_UPDATE_CAPABILITY_UPDATE_ALLOWED;
    }
#if FW_UPDATE_PUBLIC_KEY_CONFIGURED
    capabilities |= FW_UPDATE_CAPABILITY_SIGNATURE_VERIFICATION;
#endif

    uint8_t payload[FW_UPDATE_BOARD_INFO_SIZE] = {0};
    FirmwareUpdateWriteBe32(&payload[0], FW_UPDATE_TARGET_ID);
    FirmwareUpdateWriteBe16(&payload[4], FW_UPDATE_HARDWARE_REVISION);
    FirmwareUpdateWriteBe32(&payload[6], FW_UPDATE_BOOTLOADER_VERSION);
    FirmwareUpdateWriteBe32(&payload[10], FW_UPDATE_FIRMWARE_VERSION);
    payload[14] = 0U;
    payload[15] = FW_UPDATE_BOARD_APPLICATION;
    FirmwareUpdateWriteBe16(&payload[16], FW_UPDATE_MAX_CHUNK_SIZE);
    FirmwareUpdateWriteBe32(&payload[18], FW_UPDATE_SLOT_SIZE_BYTES);
    FirmwareUpdateWriteBe32(&payload[22], 0U);
    FirmwareUpdateWriteBe32(&payload[26], capabilities);
    return SendFrame(request,
                     FW_UPDATE_MESSAGE_BOARD_INFO,
                     payload,
                     sizeof(payload));
}

static void InitializeCanProxy(void)
{
    if (can_proxy_initialized) {
        return;
    }
    can_proxy_initialized = FirmwareUpdateCanLinkInit(
        &can_proxy,
        &hcan,
        FW_UPDATE_CAN_TESTER_ADDRESS,
        FW_UPDATE_CAN_NODE_MDI);
}

static bool ForwardOverCan(const FirmwareUpdateMessage *request,
                           const uint8_t *request_frame,
                           size_t request_frame_length,
                           uint8_t target_node)
{
    InitializeCanProxy();
    if (!can_proxy_initialized) {
        return false;
    }

    if (!FirmwareUpdateCanLinkSetPeer(&can_proxy, target_node)) {
        FirmwareUpdateCanLinkAbort(&can_proxy);
        if (!FirmwareUpdateCanLinkSetPeer(&can_proxy, target_node)) {
            return false;
        }
    }
    if (!FirmwareUpdateCanLinkSendFrame(&can_proxy,
                                    request_frame,
                                    request_frame_length)) {
        return false;
    }

    uint32_t deadline = HAL_GetTick() + FW_UPDATE_CAN_PROXY_TIMEOUT_MS;
    while ((int32_t)(deadline - HAL_GetTick()) > 0) {
        FirmwareUpdateCanLinkPoll(&can_proxy);
        const uint8_t *response = NULL;
        size_t response_length = 0U;
        if (FirmwareUpdateCanLinkPeekFrame(&can_proxy,
                                       &response,
                                       &response_length)) {
            FirmwareUpdateMessage decoded;
            bool valid = FirmwareUpdateFrameDecode(response,
                                               response_length,
                                               proxy_raw_frame,
                                               sizeof(proxy_raw_frame),
                                               &decoded) &&
                         (decoded.session_id == request->session_id) &&
                         (decoded.sequence == request->sequence);
            FirmwareUpdateCanLinkConsumeFrame(&can_proxy);
            last_session_activity = HAL_GetTick();
            /* A response from an earlier timed-out request can still be in
             * FIFO1. Consume it and keep waiting instead of turning a stale
             * frame into a terminal INTERNAL_ERROR for the current request. */
            if (!valid || (response_length >= sizeof(proxy_uart_response))) {
                continue;
            }
            memcpy(proxy_uart_response, response, response_length);
            proxy_uart_response[response_length] = 0U;
            return HAL_UART_Transmit(&huart5,
                                     proxy_uart_response,
                                     (uint16_t)(response_length + 1U),
                                     1000U) == HAL_OK;
        }
        FirmwareUpdateBoardYield();
    }
    FirmwareUpdateCanLinkAbort(&can_proxy);
    last_session_activity = HAL_GetTick();
    return false;
}

static void ProcessMessage(const FirmwareUpdateMessage *message,
                           const uint8_t *request_frame,
                           size_t request_frame_length)
{
    last_session_activity = HAL_GetTick();
    if (!ota_session_active) {
        ota_session_active = true;
    }

    if (message->type == FW_UPDATE_MESSAGE_HELLO) {
        uint32_t requested_target = FW_UPDATE_TARGET_ID;
        bool explicitly_targeted = false;
        if (message->payload_length == sizeof(uint32_t)) {
            requested_target = FirmwareUpdateReadBe32(message->payload);
            explicitly_targeted = requested_target == FW_UPDATE_TARGET_ID;
        } else if (message->payload_length != 0U) {
            (void)SendAck(message, FW_UPDATE_STATUS_BAD_STATE, 0U);
            return;
        }

        routed_session = message->session_id;
        routed_target = requested_target;
        if (requested_target == FW_UPDATE_TARGET_ID) {
            if (SendBoardInfo(message) && explicitly_targeted) {
                /* HAL_UART_Transmit is blocking: success means the complete
                 * targeted BOARD_INFO frame left TEL before confirmation. */
                (void)FirmwareUpdateConfirmTrialBoot();
            }
            return;
        }

        uint8_t target_node;
        if (!FirmwareUpdateCanTargetToNode(requested_target, &target_node)) {
            routed_target = FW_UPDATE_TARGET_ID;
            (void)SendAck(message, FW_UPDATE_STATUS_BAD_TARGET, 0U);
            return;
        }
        if (!ForwardOverCan(message,
                            request_frame,
                            request_frame_length,
                            target_node)) {
            (void)SendAck(message, FW_UPDATE_STATUS_INTERNAL_ERROR, 0U);
        }
        return;
    }

    if ((message->session_id == routed_session) &&
        (routed_target != FW_UPDATE_TARGET_ID)) {
        if (message->type == FW_UPDATE_MESSAGE_ENTER_BOOTLOADER) {
            if (message->payload_length != 0U) {
                (void)SendAck(message, FW_UPDATE_STATUS_BAD_STATE, 0U);
                return;
            }
            /* Apply TEL's vehicle-level interlock before any destination
             * application is asked to leave normal operation. The remote
             * board still applies its own local interlock as a second layer. */
            if (!FirmwareUpdateBoardUpdateAllowed()) {
                (void)SendAck(message,
                              FW_UPDATE_STATUS_UPDATE_NOT_ALLOWED,
                              0U);
                return;
            }
        }

        uint8_t target_node;
        if (FirmwareUpdateCanTargetToNode(routed_target, &target_node) &&
            ForwardOverCan(message,
                           request_frame,
                           request_frame_length,
                           target_node)) {
            return;
        }
        (void)SendAck(message, FW_UPDATE_STATUS_INTERNAL_ERROR, 0U);
        return;
    }

    if ((message->type == FW_UPDATE_MESSAGE_ENTER_BOOTLOADER) &&
        (message->payload_length == 0U)) {
        if (!FirmwareUpdateBoardUpdateAllowed()) {
            (void)SendAck(message, FW_UPDATE_STATUS_UPDATE_NOT_ALLOWED, 0U);
            return;
        }
        if (SendAck(message, FW_UPDATE_STATUS_OK, 0U)) {
            FirmwareUpdateRequestBootloader();
            HAL_Delay(50U);
            NVIC_SystemReset();
        }
        return;
    }

    /* REBOOT is acknowledged but intentionally not repeated by an already
     * running application. This makes a lost bootloader REBOOT ACK
     * idempotent across the reset boundary; the Pi will next verify this
     * application's target and compiled firmware version with HELLO. */
    if ((message->type == FW_UPDATE_MESSAGE_REBOOT) &&
        (message->payload_length == 0U)) {
        (void)SendAck(message, FW_UPDATE_STATUS_OK, 0U);
        return;
    }

    (void)SendAck(message, FW_UPDATE_STATUS_BAD_STATE, 0U);
}

static void ConsumeByte(uint8_t byte)
{
    if (byte != 0U) {
        if (discard_until_delimiter) {
            return;
        }
        if (encoded_length >= sizeof(encoded_frame)) {
            encoded_length = 0U;
            discard_until_delimiter = true;
            return;
        }
        encoded_frame[encoded_length++] = byte;
        return;
    }

    if (discard_until_delimiter) {
        discard_until_delimiter = false;
        encoded_length = 0U;
        return;
    }
    if (encoded_length == 0U) {
        return;
    }

    FirmwareUpdateMessage message;
    if (FirmwareUpdateFrameDecode(encoded_frame,
                              encoded_length,
                              raw_frame,
                              sizeof(raw_frame),
                              &message)) {
        ProcessMessage(&message, encoded_frame, encoded_length);
    }
    encoded_length = 0U;
}

static void ArmInterruptReceive(void)
{
    if (huart5.RxState == HAL_UART_STATE_READY) {
        (void)HAL_UART_Receive_IT(&huart5, &interrupt_rx_byte, 1U);
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *uart)
{
    if (uart->Instance != UART5) {
        return;
    }

    uint16_t next_head = (uint16_t)((rx_head + 1U) & FW_UPDATE_RX_RING_MASK);
    if (next_head == rx_tail) {
        rx_overflow = true;
    } else {
        rx_ring[rx_head] = interrupt_rx_byte;
        rx_head = next_head;
    }
    (void)HAL_UART_Receive_IT(&huart5, &interrupt_rx_byte, 1U);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *uart)
{
    if (uart->Instance == UART5) {
        rx_overflow = true;
        ArmInterruptReceive();
    }
}

void FirmwareUpdateAppPoll(void)
{
    InitializeCanProxy();
    ArmInterruptReceive();

    if (rx_overflow) {
        uint32_t interrupt_state = __get_PRIMASK();
        __disable_irq();
        rx_tail = rx_head;
        rx_overflow = false;
        if (interrupt_state == 0U) {
            __enable_irq();
        }
        encoded_length = 0U;
        discard_until_delimiter = true;
    }

    while (rx_tail != rx_head) {
        uint8_t byte = rx_ring[rx_tail];
        rx_tail = (uint16_t)((rx_tail + 1U) & FW_UPDATE_RX_RING_MASK);
        ConsumeByte(byte);
    }

    if (ota_session_active &&
        ((uint32_t)(HAL_GetTick() - last_session_activity) >
         FW_UPDATE_SESSION_TIMEOUT_MS)) {
        ota_session_active = false;
        routed_session = 0U;
        routed_target = FW_UPDATE_TARGET_ID;
        if (can_proxy_initialized) {
            FirmwareUpdateCanLinkAbort(&can_proxy);
        }
    }
}
