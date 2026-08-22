#include "sunlite_ota_app.h"

#include "bootloader_boot_request.h"
#include "bootloader_config.h"
#include "can.h"
#include "stm32f1xx_hal.h"
#include "sunlite_ota_can.h"
#include "sunlite_ota_protocol.h"
#include "usart.h"

#include <string.h>

#ifndef SUNLITE_OTA_TARGET_ID
#error "SUNLITE_OTA_TARGET_ID must be defined by the board target"
#endif
#ifndef SUNLITE_OTA_HARDWARE_REVISION
#error "SUNLITE_OTA_HARDWARE_REVISION must be defined by the board target"
#endif
#ifndef SUNLITE_OTA_FIRMWARE_VERSION
#error "SUNLITE_OTA_FIRMWARE_VERSION must be defined by the board target"
#endif
#ifndef SUNLITE_OTA_PUBLIC_KEY_CONFIGURED
#define SUNLITE_OTA_PUBLIC_KEY_CONFIGURED 0
#endif
#ifndef SUNLITE_OTA_SLOT_SIZE_BYTES
#define SUNLITE_OTA_SLOT_SIZE_BYTES BOOTLOADER_APP_MAX_SIZE_BYTES
#endif
#ifndef SUNLITE_OTA_ALLOW_UNSAFE_BENCH_UPDATE
#define SUNLITE_OTA_ALLOW_UNSAFE_BENCH_UPDATE 0
#endif

#define SUNLITE_OTA_BOOTLOADER_VERSION 1U
#define SUNLITE_OTA_RX_RING_SIZE       4096U
#define SUNLITE_OTA_RX_RING_MASK       (SUNLITE_OTA_RX_RING_SIZE - 1U)
#define SUNLITE_OTA_SESSION_TIMEOUT_MS  30000U
#define SUNLITE_OTA_CAN_PROXY_TIMEOUT_MS 7000U

#if (SUNLITE_OTA_RX_RING_SIZE & SUNLITE_OTA_RX_RING_MASK) != 0
#error "SUNLITE_OTA_RX_RING_SIZE must be a power of two"
#endif

static uint8_t encoded_frame[SUNLITE_OTA_MAX_ENCODED_FRAME];
static uint8_t raw_frame[SUNLITE_OTA_MAX_RAW_FRAME];
static size_t encoded_length;
static bool discard_until_delimiter;
static volatile bool ota_session_active;
static uint8_t rx_ring[SUNLITE_OTA_RX_RING_SIZE];
static uint8_t interrupt_rx_byte;
static volatile uint16_t rx_head;
static volatile uint16_t rx_tail;
static volatile bool rx_overflow;
static uint32_t last_session_activity;
static SunliteOtaCanLink can_proxy;
static bool can_proxy_initialized;
static uint32_t routed_session;
static uint32_t routed_target = SUNLITE_OTA_TARGET_ID;
static uint8_t proxy_raw_frame[SUNLITE_OTA_MAX_RAW_FRAME];
static uint8_t proxy_uart_response[64];

__attribute__((weak)) bool SunliteOtaBoardUpdateAllowed(void)
{
    /* Override this hook when TEL receives the real vehicle-level interlock.
     * Debug builds can explicitly enable the bench-only fallback in CMake. */
    return SUNLITE_OTA_ALLOW_UNSAFE_BENCH_UPDATE != 0;
}

__attribute__((weak)) void SunliteOtaBoardTransmitAborted(void)
{
}

__attribute__((weak)) void SunliteOtaBoardYield(void)
{
    HAL_Delay(1U);
}

bool SunliteOtaApplicationSessionActive(void)
{
    return ota_session_active;
}

static bool SendFrame(const SunliteOtaMessage *request,
                      SunliteOtaMessageType type,
                      const uint8_t *payload,
                      uint16_t payload_length)
{
    uint8_t response[64];
    size_t response_length = SunliteOtaFrameEncode(
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

static bool SendAck(const SunliteOtaMessage *request,
                    SunliteOtaStatus status,
                    uint32_t next_offset)
{
    uint8_t payload[SUNLITE_OTA_ACK_SIZE];
    payload[0] = (uint8_t)request->type;
    payload[1] = (uint8_t)status;
    SunliteOtaWriteBe32(&payload[2], next_offset);
    SunliteOtaMessageType response_type = (status == SUNLITE_OTA_STATUS_OK) ?
        SUNLITE_OTA_MESSAGE_ACK : SUNLITE_OTA_MESSAGE_NACK;
    return SendFrame(request, response_type, payload, sizeof(payload));
}

static bool SendBoardInfo(const SunliteOtaMessage *request)
{
    bool update_allowed = SunliteOtaBoardUpdateAllowed();
    uint32_t capabilities = 0U;
    if (update_allowed) {
        capabilities |= SUNLITE_OTA_CAPABILITY_UPDATE_ALLOWED;
    }
#if SUNLITE_OTA_PUBLIC_KEY_CONFIGURED
    capabilities |= SUNLITE_OTA_CAPABILITY_SIGNATURE_VERIFICATION;
#endif

    uint8_t payload[SUNLITE_OTA_BOARD_INFO_SIZE] = {0};
    SunliteOtaWriteBe32(&payload[0], SUNLITE_OTA_TARGET_ID);
    SunliteOtaWriteBe16(&payload[4], SUNLITE_OTA_HARDWARE_REVISION);
    SunliteOtaWriteBe32(&payload[6], SUNLITE_OTA_BOOTLOADER_VERSION);
    SunliteOtaWriteBe32(&payload[10], SUNLITE_OTA_FIRMWARE_VERSION);
    payload[14] = 0U;
    payload[15] = SUNLITE_OTA_BOARD_APPLICATION;
    SunliteOtaWriteBe16(&payload[16], SUNLITE_OTA_MAX_CHUNK_SIZE);
    SunliteOtaWriteBe32(&payload[18], SUNLITE_OTA_SLOT_SIZE_BYTES);
    SunliteOtaWriteBe32(&payload[22], 0U);
    SunliteOtaWriteBe32(&payload[26], capabilities);
    return SendFrame(request,
                     SUNLITE_OTA_MESSAGE_BOARD_INFO,
                     payload,
                     sizeof(payload));
}

static void InitializeCanProxy(void)
{
    if (can_proxy_initialized) {
        return;
    }
    can_proxy_initialized = SunliteOtaCanLinkInit(
        &can_proxy,
        &hcan,
        SUNLITE_OTA_CAN_TESTER_ADDRESS,
        SUNLITE_OTA_CAN_NODE_MDI);
}

static bool ForwardOverCan(const SunliteOtaMessage *request,
                           const uint8_t *request_frame,
                           size_t request_frame_length,
                           uint8_t target_node)
{
    InitializeCanProxy();
    if (!can_proxy_initialized) {
        return false;
    }

    if (!SunliteOtaCanLinkSetPeer(&can_proxy, target_node)) {
        SunliteOtaCanLinkAbort(&can_proxy);
        if (!SunliteOtaCanLinkSetPeer(&can_proxy, target_node)) {
            return false;
        }
    }
    if (!SunliteOtaCanLinkSendFrame(&can_proxy,
                                    request_frame,
                                    request_frame_length)) {
        return false;
    }

    uint32_t deadline = HAL_GetTick() + SUNLITE_OTA_CAN_PROXY_TIMEOUT_MS;
    while ((int32_t)(deadline - HAL_GetTick()) > 0) {
        SunliteOtaCanLinkPoll(&can_proxy);
        const uint8_t *response = NULL;
        size_t response_length = 0U;
        if (SunliteOtaCanLinkPeekFrame(&can_proxy,
                                       &response,
                                       &response_length)) {
            SunliteOtaMessage decoded;
            bool valid = SunliteOtaFrameDecode(response,
                                               response_length,
                                               proxy_raw_frame,
                                               sizeof(proxy_raw_frame),
                                               &decoded) &&
                         (decoded.session_id == request->session_id) &&
                         (decoded.sequence == request->sequence);
            SunliteOtaCanLinkConsumeFrame(&can_proxy);
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
        SunliteOtaBoardYield();
    }
    SunliteOtaCanLinkAbort(&can_proxy);
    last_session_activity = HAL_GetTick();
    return false;
}

static void ProcessMessage(const SunliteOtaMessage *message,
                           const uint8_t *request_frame,
                           size_t request_frame_length)
{
    last_session_activity = HAL_GetTick();
    if (!ota_session_active) {
        ota_session_active = true;
    }

    if (message->type == SUNLITE_OTA_MESSAGE_HELLO) {
        uint32_t requested_target = SUNLITE_OTA_TARGET_ID;
        bool explicitly_targeted = false;
        if (message->payload_length == sizeof(uint32_t)) {
            requested_target = SunliteOtaReadBe32(message->payload);
            explicitly_targeted = requested_target == SUNLITE_OTA_TARGET_ID;
        } else if (message->payload_length != 0U) {
            (void)SendAck(message, SUNLITE_OTA_STATUS_BAD_STATE, 0U);
            return;
        }

        routed_session = message->session_id;
        routed_target = requested_target;
        if (requested_target == SUNLITE_OTA_TARGET_ID) {
            if (SendBoardInfo(message) && explicitly_targeted) {
                /* HAL_UART_Transmit is blocking: success means the complete
                 * targeted BOARD_INFO frame left TEL before confirmation. */
                (void)SunliteOtaConfirmTrialBoot();
            }
            return;
        }

        uint8_t target_node;
        if (!SunliteOtaCanTargetToNode(requested_target, &target_node)) {
            routed_target = SUNLITE_OTA_TARGET_ID;
            (void)SendAck(message, SUNLITE_OTA_STATUS_BAD_TARGET, 0U);
            return;
        }
        if (!ForwardOverCan(message,
                            request_frame,
                            request_frame_length,
                            target_node)) {
            (void)SendAck(message, SUNLITE_OTA_STATUS_INTERNAL_ERROR, 0U);
        }
        return;
    }

    if ((message->session_id == routed_session) &&
        (routed_target != SUNLITE_OTA_TARGET_ID)) {
        if (message->type == SUNLITE_OTA_MESSAGE_ENTER_BOOTLOADER) {
            if (message->payload_length != 0U) {
                (void)SendAck(message, SUNLITE_OTA_STATUS_BAD_STATE, 0U);
                return;
            }
            /* Apply TEL's vehicle-level interlock before any destination
             * application is asked to leave normal operation. The remote
             * board still applies its own local interlock as a second layer. */
            if (!SunliteOtaBoardUpdateAllowed()) {
                (void)SendAck(message,
                              SUNLITE_OTA_STATUS_UPDATE_NOT_ALLOWED,
                              0U);
                return;
            }
        }

        uint8_t target_node;
        if (SunliteOtaCanTargetToNode(routed_target, &target_node) &&
            ForwardOverCan(message,
                           request_frame,
                           request_frame_length,
                           target_node)) {
            return;
        }
        (void)SendAck(message, SUNLITE_OTA_STATUS_INTERNAL_ERROR, 0U);
        return;
    }

    if ((message->type == SUNLITE_OTA_MESSAGE_ENTER_BOOTLOADER) &&
        (message->payload_length == 0U)) {
        if (!SunliteOtaBoardUpdateAllowed()) {
            (void)SendAck(message, SUNLITE_OTA_STATUS_UPDATE_NOT_ALLOWED, 0U);
            return;
        }
        if (SendAck(message, SUNLITE_OTA_STATUS_OK, 0U)) {
            SunliteOtaRequestBootloader();
            HAL_Delay(50U);
            NVIC_SystemReset();
        }
        return;
    }

    /* REBOOT is acknowledged but intentionally not repeated by an already
     * running application. This makes a lost bootloader REBOOT ACK
     * idempotent across the reset boundary; the Pi will next verify this
     * application's target and compiled firmware version with HELLO. */
    if ((message->type == SUNLITE_OTA_MESSAGE_REBOOT) &&
        (message->payload_length == 0U)) {
        (void)SendAck(message, SUNLITE_OTA_STATUS_OK, 0U);
        return;
    }

    (void)SendAck(message, SUNLITE_OTA_STATUS_BAD_STATE, 0U);
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

    SunliteOtaMessage message;
    if (SunliteOtaFrameDecode(encoded_frame,
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

    uint16_t next_head = (uint16_t)((rx_head + 1U) & SUNLITE_OTA_RX_RING_MASK);
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

void SunliteOtaAppPoll(void)
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
        rx_tail = (uint16_t)((rx_tail + 1U) & SUNLITE_OTA_RX_RING_MASK);
        ConsumeByte(byte);
    }

    if (ota_session_active &&
        ((uint32_t)(HAL_GetTick() - last_session_activity) >
         SUNLITE_OTA_SESSION_TIMEOUT_MS)) {
        ota_session_active = false;
        routed_session = 0U;
        routed_target = SUNLITE_OTA_TARGET_ID;
        if (can_proxy_initialized) {
            SunliteOtaCanLinkAbort(&can_proxy);
        }
    }
}
