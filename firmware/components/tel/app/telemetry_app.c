/**
 * @file    telemetry_app.c
 * @brief   Telemetry application logic for the UBC Solar telemetry system
 *
 * This file contains the implementation of the telemetry application logic for the UBC Solar
 * telemetry system. It handles the transmission and processing of telemetry messages.
 *
 * @author  Tony Chen and Gregory Bian
 * @date    May 22 2026
 */

/* INCLUDES */
#include "telemetry_app.h"
#include "bitops.h"
#include "can_app.h"
#include "car_configs.h"
#include "rtc_driver.h"
#include "stdbool.h"
#include "telemetry_driver.h"
#include "xbee/ipv4.h"
#include "xbee/serial.h"
#include "xbee/tx_status.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

/* DEFINES */
#define NO_PRIORITY 0
#define NON_BLOCKING 0
#define ID_DELIMITER_CHAR '#'
#define CARRIAGE_RETURN_CHAR '\r'
#define NEW_LINE_CHAR '\n'
#define MASK_4_BITS 0xF
#define NUM_FILTERS ((int)(sizeof(filter_whitelist) / sizeof(filter_whitelist[0])))

#define CELLULAR_JSON_BUFFER_SIZE 96U
#define CELLULAR_MILLISECONDS_PER_SECOND 1000U
#define CELLULAR_CAN_ID_MAX 0x1FFFFFFFU
#define HEX_CHARS_PER_BYTE 2U
#define HEX_NIBBLE_BITS 4U
#define CELLULAR_TEST_CAN_ID 1878U
#define CELLULAR_TEST_DATA {1, 2, 3, 4, 5, 6, 7, 8}

/* STATIC VARIABLES */
static TEL_Msg_TypeDef tel_msg = {0};
/* Protected by the same semaphore as the DMA transmit buffer. */
static xbee_dev_t cellular_xbee;
static bool cellular_initialized;
static char cellular_json[CELLULAR_JSON_BUFFER_SIZE];
static char cellular_hex[TEL_DATA_LENGTH * HEX_CHARS_PER_BYTE + 1U];
static const char hex_digits[] = "0123456789abcdef";
static const uint8_t cellular_test_data[] = CELLULAR_TEST_DATA;

/* STATIC FUNCTION PROTOTYPES */
static HAL_StatusTypeDef CellularInitLocked(void);
static int CellularStatus(xbee_dev_t* xbee, const void* frame, uint16_t length, void* context);

/* Digi validates framing/checksums before dispatching these responses. */
const xbee_dispatch_table_entry_t xbee_frame_handlers[] = {
    {XBEE_FRAME_TX_STATUS, 0, CellularStatus, NULL},
    {XBEE_FRAME_MODEM_STATUS, 0, CellularStatus, NULL},
    XBEE_FRAME_TABLE_END
};

/**
 * @brief  Filters CAN IDs based on a whitelist
 * @param  can_id: The CAN ID to filter
 * @retval true if the ID is in the whitelist, false otherwise
 */
static bool filter(uint32_t can_id);
/**
 * @brief  Sets the fields in the telemetry message struct
 * @param  header: Pointer to the CAN header struct
 * @param  data: Pointer to the CAN data
 * @param  tel_msg: Pointer to the telemetry message struct
 * @retval None
 */
static void TelAppSetMsg(CAN_RxHeaderTypeDef* header, uint8_t* data, TEL_Msg_TypeDef* tel_msg);
/**
 * @brief  Sets the fields in the telemetry message struct for transmission
 * @param  header: Pointer to the CAN header struct
 * @param  data: Pointer to the CAN data
 * @param  tel_msg: Pointer to the telemetry message struct
 * @retval None
 */
static void TelAppSetMsg_tx(CAN_TxHeaderTypeDef* header, uint8_t* data, TEL_Msg_TypeDef* tel_msg);
/**
 * @brief  Gets the timestamp for the telemetry message
 * @retval The timestamp as a 64-bit unsigned integer
 */
static uint64_t get_timestamp(void);
/**
 * @brief  Gets the CAN ID from the CAN header struct
 * @param  can_msg_header_ptr: Pointer to the CAN header struct
 * @retval The CAN ID as a 32-bit unsigned integer
 */
static uint32_t get_can_id(CAN_RxHeaderTypeDef* can_msg_header_ptr);
/**
 * @brief  Gets the data length from the CAN header struct
 * @param  DLC: The data length code from the CAN header
 * @retval The data length as an 8-bit unsigned integer
 */
static uint8_t get_data_length(uint32_t DLC);

/* FUNCTION DEFINITIONS */
static bool filter(uint32_t can_id)
{
#ifdef DEBUG
    return true;
#else
    // TODO: Use hashmap for speed?
    for (int i = 0; i < NUM_FILTERS; i++)
    {
        if (filter_whitelist[i].id == can_id)
        {
            if (((++filter_whitelist[i].count) % filter_whitelist[i].mod) == 0)
            {
                return true;
            }
            break; // Found the matching ID; no need to keep looping.
        }
    }
    return false;
#endif
}

void TelAppTransmitMsg(CAN_comms_Rx_msg_t* CAN_comms_Rx_msg)
{
    if (CAN_comms_Rx_msg == NULL)
    {
        return;
    }

    uint32_t can_id = (CAN_comms_Rx_msg->header.IDE == CAN_ID_STD) ? CAN_comms_Rx_msg->header.StdId
                                                                   : CAN_comms_Rx_msg->header.ExtId;
    if (!car_config_can_msg_all && !filter(can_id))
    {
        return;
    }
    if (car_config_cellular)
    {
        /* Remote-request frames contain no data payload for this JSON format. */
        if (CAN_comms_Rx_msg->header.RTR != CAN_RTR_DATA ||
            CellularXbeeSendCAN(can_id, CAN_comms_Rx_msg->data, CAN_comms_Rx_msg->header.DLC) !=
                HAL_OK)
        {
            telemetry_diagnostic.dropped_telemetry_msg++;
        }
        return;
    }
    osSemaphoreAcquire(usart1_tx_semaphore, osWaitForever); // Dont Tx until previous Tx is done
    TelAppSetMsg(&(CAN_comms_Rx_msg->header), CAN_comms_Rx_msg->data, &tel_msg);
    UART_telemetry_transmit((const uint8_t*)&tel_msg, sizeof(tel_msg));
}

// Change name of this to gps, imu send or something???
void TelAppTransmitInternalMsg(CAN_comms_Tx_msg_t* CAN_comms_Tx_msg)
{
    if (CAN_comms_Tx_msg == NULL)
    {
        return;
    }

    uint32_t can_id = (CAN_comms_Tx_msg->header.IDE == CAN_ID_STD) ? CAN_comms_Tx_msg->header.StdId
                                                                   : CAN_comms_Tx_msg->header.ExtId;
    if (!car_config_can_msg_all && !filter(can_id))
    {
        return;
    }
    if (car_config_cellular)
    {
        /* Remote-request frames contain no data payload for this JSON format. */
        if (CAN_comms_Tx_msg->header.RTR != CAN_RTR_DATA ||
            CellularXbeeSendCAN(can_id, CAN_comms_Tx_msg->data, CAN_comms_Tx_msg->header.DLC) !=
                HAL_OK)
        {
            telemetry_diagnostic.dropped_telemetry_msg++;
        }
        return;
    }
    osSemaphoreAcquire(usart1_tx_semaphore, osWaitForever); // Dont Tx until previous Tx is done
    TelAppSetMsg_tx(&(CAN_comms_Tx_msg->header), CAN_comms_Tx_msg->data, &tel_msg);
    UART_telemetry_transmit((const uint8_t*)&tel_msg, sizeof(tel_msg));
}

static void TelAppSetMsg(CAN_RxHeaderTypeDef* header, uint8_t* data, TEL_Msg_TypeDef* tel_msg)
{
    memset(tel_msg, 0, sizeof(TEL_Msg_TypeDef)); // 0 out all 8 bytes data

    tel_msg->timestamp = get_timestamp();
    tel_msg->can_id = get_can_id(header);
    tel_msg->ID_DELIMETER = ID_DELIMITER_CHAR;
    memcpy(tel_msg->data, data, TEL_DATA_LENGTH);
    tel_msg->data_len = get_data_length(header->DLC);
    tel_msg->CARRIAGE_RETURN = CARRIAGE_RETURN_CHAR;
    tel_msg->NEW_LINE = NEW_LINE_CHAR;
}

/**
 * @brief Sets all the fields in the radio message struct
 *
 * @param header The CAN header struct
 * @param data The CAN data
 * @param tel_msg Destination telemetry message.
 */
static void TelAppSetMsg_tx(CAN_TxHeaderTypeDef* header, uint8_t* data, TEL_Msg_TypeDef* tel_msg)
{
    memset(tel_msg, 0, sizeof(TEL_Msg_TypeDef)); // 0 out all 8 bytes data

    tel_msg->timestamp = get_timestamp();
    uint32_t id = (header->IDE == CAN_ID_STD) ? header->StdId : header->ExtId;
    tel_msg->can_id = BITOPS_32BIT_REVERSE(id);
    tel_msg->ID_DELIMETER = ID_DELIMITER_CHAR;
    memcpy(tel_msg->data, data, TEL_DATA_LENGTH);
    tel_msg->data_len = get_data_length(header->DLC);
    tel_msg->CARRIAGE_RETURN = CARRIAGE_RETURN_CHAR;
    tel_msg->NEW_LINE = NEW_LINE_CHAR;
}

/**
 * @brief Getter for the timestamp in the radio message as a 64-bit unsigned integer
 *
 * @return The timestamp as a 64-bit unsigned integer
 */
static uint64_t get_timestamp(void)
{
    DoubleAsUint64 timestamp_union;
    timestamp_union.d = RtcDriverGetTimeStamp();
    return BITOPS_64BIT_REVERSE(timestamp_union.u);
}

static uint32_t get_can_id(CAN_RxHeaderTypeDef* can_msg_header_ptr)
{
    uint32_t can_id = (can_msg_header_ptr->IDE == CAN_ID_STD) ? can_msg_header_ptr->StdId
                                                              : can_msg_header_ptr->ExtId;
    return BITOPS_32BIT_REVERSE(can_id);
}

static uint8_t get_data_length(uint32_t DLC) { return (uint8_t)(DLC & MASK_4_BITS); }

/* Caller holds the UART semaphore, including during first-use initialization. */
static HAL_StatusTypeDef CellularInitLocked(void)
{
    if (!cellular_initialized)
    {
        xbee_serial_t serial = {.baudrate = CELLULAR_BAUD_RATE, .uart = TELEMETRY_UART};
        if (xbee_dev_init(&cellular_xbee, &serial, NULL, NULL) != 0)
        {
            return HAL_ERROR;
        }
        xbee_dev_flowcontrol(&cellular_xbee, false);
        cellular_initialized = true;
    }
    return XBeePlatform_StartRx();
}

HAL_StatusTypeDef CellularXbeeInit(void)
{
    if (!car_config_cellular || osKernelGetState() != osKernelRunning ||
        usart1_tx_semaphore == NULL)
    {
        return HAL_ERROR;
    }
    if (osSemaphoreAcquire(usart1_tx_semaphore, osWaitForever) != osOK)
    {
        return HAL_BUSY;
    }
    HAL_StatusTypeDef status = CellularInitLocked();
    osSemaphoreRelease(usart1_tx_semaphore);
    return status;
}

/* Called by TasksCellular; never waits behind a DMA transmission. */
void CellularXbeePoll(void)
{
    if (!car_config_cellular || usart1_tx_semaphore == NULL ||
        osSemaphoreAcquire(usart1_tx_semaphore, NON_BLOCKING) != osOK)
    {
        return;
    }
    if (CellularInitLocked() == HAL_OK)
    {
        (void)xbee_dev_tick(&cellular_xbee);
    }
    osSemaphoreRelease(usart1_tx_semaphore);
}

static int CellularStatus(xbee_dev_t* xbee, const void* frame, uint16_t length, void* context)
{
    (void)xbee;
    (void)context;
    const uint8_t* bytes = frame;
    if (bytes[0] == XBEE_FRAME_TX_STATUS && length == sizeof(xbee_frame_tx_status_t))
    {
        const xbee_frame_tx_status_t* status = frame;
        telemetry_diagnostic.cellular_last_frame_id = status->frame_id;
        telemetry_diagnostic.cellular_last_tx_status = status->delivery;
        if (status->delivery == XBEE_TX_DELIVERY_SUCCESS)
        {
            telemetry_diagnostic.cellular_modem_tx_success++;
        }
        else
        {
            telemetry_diagnostic.cellular_modem_tx_failures++;
        }
    }
    else if (bytes[0] == XBEE_FRAME_MODEM_STATUS && length == sizeof(xbee_frame_modem_status_t))
    {
        const xbee_frame_modem_status_t* status = frame;
        telemetry_diagnostic.cellular_last_modem_status = status->status;
        telemetry_diagnostic.cellular_modem_status_count++;
    }
    return 0;
}

HAL_StatusTypeDef CellularXbeeSendTest(void)
{
    return CellularXbeeSendCAN(
        CELLULAR_TEST_CAN_ID, cellular_test_data, sizeof(cellular_test_data));
}

HAL_StatusTypeDef CellularXbeeSendCAN(uint32_t can_id, const uint8_t* data, uint32_t dlc)
{
    if (!car_config_cellular || can_id > CELLULAR_CAN_ID_MAX || dlc > TEL_DATA_LENGTH ||
        (dlc != 0 && data == NULL))
    {
        return HAL_ERROR;
    }
    if (osKernelGetState() != osKernelRunning || usart1_tx_semaphore == NULL)
    {
        return HAL_ERROR;
    }
    /* Capture measurement time before waiting for the UART. Same RTC as radio. */
    uint64_t timestamp_ms = (uint64_t)(RtcDriverGetTimeStamp() * CELLULAR_MILLISECONDS_PER_SECOND);
    /* Wait for the previous DMA send before changing any shared buffers. */
    if (osSemaphoreAcquire(usart1_tx_semaphore, osWaitForever) != osOK)
    {
        return HAL_BUSY;
    }

    for (uint32_t i = 0; i < dlc; ++i)
    {
        cellular_hex[i * HEX_CHARS_PER_BYTE] = hex_digits[data[i] >> HEX_NIBBLE_BITS];
        cellular_hex[i * HEX_CHARS_PER_BYTE + 1U] = hex_digits[data[i] & MASK_4_BITS];
    }
    cellular_hex[dlc * HEX_CHARS_PER_BYTE] = '\0';

    int length = snprintf(cellular_json,
                          sizeof(cellular_json),
                          "{\"can_id\":%" PRIu32 ",\"data\":\"%s\",\"ts\":%" PRIu32 ".%03" PRIu32 "}\n",
                          can_id,
                          cellular_hex,
                          (uint32_t)(timestamp_ms / CELLULAR_MILLISECONDS_PER_SECOND),
                          (uint32_t)(timestamp_ms % CELLULAR_MILLISECONDS_PER_SECOND));
    if (length < 0 || (size_t)length >= sizeof(cellular_json))
    {
        osSemaphoreRelease(usart1_tx_semaphore);
        return HAL_ERROR;
    }

    const uint8_t server_ip[] = CELLULAR_SERVER_IP;
    if (CellularInitLocked() != HAL_OK)
    {
        osSemaphoreRelease(usart1_tx_semaphore);
        return HAL_ERROR;
    }

    /* Drain queued statuses even under sustained CAN traffic. */
    (void)xbee_dev_tick(&cellular_xbee);
    xbee_ipv4_envelope_t packet = {.xbee = &cellular_xbee,
                                   .remote_port = CELLULAR_SERVER_PORT,
                                   .protocol = XBEE_IPV4_PROTOCOL_TCP,
                                   .payload = cellular_json,
                                   .length = (uint16_t)length};
    memcpy(&packet.remote_addr_be, server_ip, sizeof(server_ip));

    XBeePlatform_ResetTx();
    int result = xbee_ipv4_envelope_send(&packet);
    if (result < 0)
    {
        osSemaphoreRelease(usart1_tx_semaphore);
        return result == -EBUSY ? HAL_BUSY : HAL_ERROR;
    }
    return XBeePlatform_Transmit();
}
