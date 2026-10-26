/**
 * @file    telemetry_app.c
 * @brief   Telemetry application logic for the UBC Solar telemetry system
 *
 * This file contains the implementation of the telemetry application logic for the UBC Solar telemetry system. 
 * It handles the transmission and processing of telemetry messages.
 *
 * @author  Tony Chen and Gregory Bian
 * @date    May 22 2026
 */

/* INCLUDES */
#include "telemetry_app.h"
#include "telemetry_driver.h"
#include "car_configs.h"
#include "can_app.h"
#include "bitops.h"
#include "stdbool.h"
#include "rtc_driver.h"
#include <string.h>
#include "xbee/ipv4.h"
#include "xbee/serial.h"

/* DEFINES */
#define NO_PRIORITY                                 0
#define NON_BLOCKING                                0
#define ID_DELIMITER_CHAR                           '#'
#define CARRIAGE_RETURN_CHAR                        '\r'
#define NEW_LINE_CHAR                               '\n'
#define MASK_4_BITS                                 0xF
#define NUM_FILTERS                                 ((int)(sizeof(filter_whitelist) / sizeof(filter_whitelist[0])))

/* Cellular test settings. Configure the UART and modem to match (modem AP=1). */
#define CELLULAR_UART                   (&huart2)
#define CELLULAR_BAUD_RATE              9600U
#define CELLULAR_SERVER_IP              {45, 79, 112, 203}
#define CELLULAR_SERVER_PORT            4242U
#define CELLULAR_TX_BUFFER_SIZE         64U
#define CELLULAR_UART_BITS_PER_BYTE     10U /* 8N1: start + 8 data + stop */
#define CELLULAR_TX_TIMEOUT_MARGIN_MS   100U
#define MILLISECONDS_PER_SECOND         1000U
#define CELLULAR_TEST_PAYLOAD           "{\"can_id\":1878,\"data\":\"0102030405060708\"}\n"

/* STATIC VARIABLES */
static TEL_Msg_TypeDef tel_msg = {0};

/* Digi builds the frame here; HAL sends it in one checked write. */
static uint8_t cellular_tx[CELLULAR_TX_BUFFER_SIZE];
static unsigned cellular_tx_used;
static bool cellular_write_error;

/* STATIC FUNCTION PROTOTYPES */
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
        for (int i = 0; i < NUM_FILTERS; i++) {
            if (filter_whitelist[i].id == can_id) {
                if (((++filter_whitelist[i].count) % filter_whitelist[i].mod) == 0) {
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

    uint32_t can_id = (CAN_comms_Rx_msg->header.IDE == CAN_ID_STD) ? CAN_comms_Rx_msg->header.StdId : CAN_comms_Rx_msg->header.ExtId;
    if (!car_config_can_msg_all && !filter(can_id))
    {
        return;
    }
    osSemaphoreAcquire(usart1_tx_semaphore, osWaitForever);   // Dont Tx until previous Tx is done
    TelAppSetMsg(&(CAN_comms_Rx_msg->header), CAN_comms_Rx_msg->data, &tel_msg);
    UART_telemetry_transmit(&tel_msg);
}

// Change name of this to gps, imu send or something???
void TelAppTransmitInternalMsg(CAN_comms_Tx_msg_t* CAN_comms_Tx_msg)
{
    if (CAN_comms_Tx_msg == NULL)
    {
        return;
    }

    uint32_t can_id = (CAN_comms_Tx_msg->header.IDE == CAN_ID_STD) ? CAN_comms_Tx_msg->header.StdId : CAN_comms_Tx_msg->header.ExtId;
    if (!car_config_can_msg_all && !filter(can_id))
    {
        return;
    }
    osSemaphoreAcquire(usart1_tx_semaphore, osWaitForever);   // Dont Tx until previous Tx is done
    TelAppSetMsg_tx(&(CAN_comms_Tx_msg->header), CAN_comms_Tx_msg->data, &tel_msg);
    UART_telemetry_transmit(&tel_msg);
}

static void TelAppSetMsg(CAN_RxHeaderTypeDef* header, uint8_t* data, TEL_Msg_TypeDef* tel_msg)
{
    memset(tel_msg, 0, sizeof(TEL_Msg_TypeDef));           // 0 out all 8 bytes data
    
    tel_msg->timestamp        = get_timestamp();
    tel_msg->can_id           = get_can_id(header);
    tel_msg->ID_DELIMETER     = ID_DELIMITER_CHAR;
    memcpy(tel_msg->data, data, TEL_DATA_LENGTH);
    tel_msg->data_len         = get_data_length(header->DLC);
    tel_msg->CARRIAGE_RETURN  = CARRIAGE_RETURN_CHAR;
    tel_msg->NEW_LINE         = NEW_LINE_CHAR;
}

/**
 * @brief Sets all the fields in the radio message struct
 * 
 * @param header The CAN header struct
 * @param data The CAN data
 */
static void TelAppSetMsg_tx(CAN_TxHeaderTypeDef* header, uint8_t* data, TEL_Msg_TypeDef* tel_msg)
{
    memset(tel_msg, 0, sizeof(TEL_Msg_TypeDef));           // 0 out all 8 bytes data
    
    tel_msg->timestamp        = get_timestamp();
    uint32_t id 				= (header->IDE == CAN_ID_STD) ? header->StdId : header->ExtId; 
    tel_msg->can_id           = BITOPS_32BIT_REVERSE(id);
    tel_msg->ID_DELIMETER     = ID_DELIMITER_CHAR;
    memcpy(tel_msg->data, data, TEL_DATA_LENGTH);
    tel_msg->data_len         = get_data_length(header->DLC);
    tel_msg->CARRIAGE_RETURN  = CARRIAGE_RETURN_CHAR;
    tel_msg->NEW_LINE         = NEW_LINE_CHAR;
}

/**
 * @brief Getter for the timestamp in the radio message as a 64-bit unsigned integer
 * 
 * @return The timestamp as a 64-bit unsigned integer
 */
uint64_t get_timestamp()
{
    DoubleAsUint64 timestamp_union;
    timestamp_union.d = RtcDriverGetTimeStamp();
    return BITOPS_64BIT_REVERSE(timestamp_union.u);
}

static uint32_t get_can_id(CAN_RxHeaderTypeDef* can_msg_header_ptr)
{
    uint32_t can_id = (can_msg_header_ptr->IDE == CAN_ID_STD) ? can_msg_header_ptr->StdId : can_msg_header_ptr->ExtId;
    return BITOPS_32BIT_REVERSE(can_id);
}

static uint8_t get_data_length(uint32_t DLC)
{
    return (uint8_t) (DLC & MASK_4_BITS);
}

HAL_StatusTypeDef CellularTelemetry_SendTest(void)
{
    static xbee_dev_t radio;
    static const char payload[] = CELLULAR_TEST_PAYLOAD;
    const uint8_t server_ip[] = CELLULAR_SERVER_IP;
    xbee_serial_t serial = {
        .baudrate = CELLULAR_BAUD_RATE,
        .uart = CELLULAR_UART
    };

    if (xbee_dev_init(&radio, &serial, NULL, NULL) != 0)
    {
        return HAL_ERROR;
    }
    xbee_dev_flowcontrol(&radio, false);

    xbee_ipv4_envelope_t packet = {
        .xbee = &radio,
        .remote_port = CELLULAR_SERVER_PORT,
        .protocol = XBEE_IPV4_PROTOCOL_TCP,
        .payload = payload,
        .length = sizeof(payload) - 1U /* Exclude the terminating null byte. */
    };
    memcpy(&packet.remote_addr_be, server_ip, sizeof(server_ip));

    cellular_tx_used = 0;
    cellular_write_error = false;
    int result = xbee_ipv4_envelope_send(&packet);
    if (result == -EBUSY)
    {
        return HAL_BUSY;
    }
    if (result < 0 || cellular_write_error || cellular_tx_used == 0)
    {
        return HAL_ERROR;
    }

    /* Round the wire time up to milliseconds, then allow extra HAL overhead. */
    uint32_t wire_bits = cellular_tx_used * CELLULAR_UART_BITS_PER_BYTE;
    uint32_t wire_time_ms = (wire_bits * MILLISECONDS_PER_SECOND +
                            CELLULAR_BAUD_RATE - 1U) / CELLULAR_BAUD_RATE;
    uint32_t timeout_ms = wire_time_ms + CELLULAR_TX_TIMEOUT_MARGIN_MS;
    return HAL_UART_Transmit(CELLULAR_UART, cellular_tx,
                             (uint16_t)cellular_tx_used, timeout_ms);
}

/* Digi serial hooks: validate the UART and collect the frame's pieces. */
bool_t xbee_ser_invalid(xbee_serial_t *serial)
{
    return serial == NULL || serial->uart == NULL;
}

int xbee_ser_open(xbee_serial_t *serial, uint32_t baud_rate)
{
    if (xbee_ser_invalid(serial))
    {
        return -EINVAL;
    }

    /* Cube initializes the hardware; this hook only checks its settings. */
    UART_HandleTypeDef *uart = serial->uart;
    if (baud_rate != CELLULAR_BAUD_RATE || uart->Instance == NULL ||
        uart->Init.BaudRate != baud_rate ||
        uart->Init.WordLength != UART_WORDLENGTH_8B ||
        uart->Init.StopBits != UART_STOPBITS_1 ||
        uart->Init.Parity != UART_PARITY_NONE ||
        uart->Init.Mode != UART_MODE_TX_RX ||
        uart->Init.HwFlowCtl != UART_HWCONTROL_NONE)
    {
        return -EINVAL;
    }
    if (uart->gState != HAL_UART_STATE_READY || uart->RxState != HAL_UART_STATE_READY)
    {
        return -EINVAL;
    }
    return 0;
}

int xbee_ser_write(xbee_serial_t *serial, const void *data, int length)
{
    if (xbee_ser_invalid(serial) || data == NULL || length < 0 ||
        (unsigned)length > sizeof(cellular_tx) - cellular_tx_used)
    {
        cellular_write_error = true;
        return -EINVAL;
    }

    memcpy(cellular_tx + cellular_tx_used, data, (unsigned)length);
    cellular_tx_used += (unsigned)length;
    return length;
}

int xbee_ser_tx_free(xbee_serial_t *serial)
{
    if (xbee_ser_invalid(serial))
    {
        return -EINVAL;
    }
    return (int)(sizeof(cellular_tx) - cellular_tx_used);
}

int xbee_ser_tx_used(xbee_serial_t *serial)
{
    if (xbee_ser_invalid(serial))
    {
        return -EINVAL;
    }
    return (int)cellular_tx_used;
}

/* No hardware flow control; sending disables Digi's CTS check. */
int xbee_ser_flowcontrol(xbee_serial_t *serial, bool_t enabled)
{
    (void)enabled;
    if (xbee_ser_invalid(serial))
    {
        return -EINVAL;
    }
    return 0;
}

int xbee_ser_get_cts(xbee_serial_t *serial)
{
    if (xbee_ser_invalid(serial))
    {
        return -EINVAL;
    }
    return true;
}

uint32_t xbee_millisecond_timer(void)
{
    return HAL_GetTick();
}

uint32_t xbee_seconds_timer(void)
{
    return HAL_GetTick() / MILLISECONDS_PER_SECOND;
}
