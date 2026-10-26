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
#define CELLULAR_SERVER_IP              {45, 79, 112, 203}
#define CELLULAR_SERVER_PORT            4242U
#define CELLULAR_TEST_PAYLOAD           "{\"can_id\":1878,\"data\":\"0102030405060708\"}\n"

/* STATIC VARIABLES */
static TEL_Msg_TypeDef tel_msg = {0};

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
    UART_telemetry_transmit((const uint8_t *)&tel_msg, sizeof(tel_msg));
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
    UART_telemetry_transmit((const uint8_t *)&tel_msg, sizeof(tel_msg));
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

HAL_StatusTypeDef CellularXbeeInit(void)
{
    if (!car_config_cellular)
    {
        return HAL_ERROR;
    }

    if (osKernelGetState() != osKernelRunning || usart1_tx_semaphore == NULL)
    {
        return HAL_ERROR;
    }
    
    if (osSemaphoreAcquire(usart1_tx_semaphore, NON_BLOCKING) != osOK)
    {
        return HAL_BUSY;
    }

    static xbee_dev_t radio;
    static const char payload[] = CELLULAR_TEST_PAYLOAD;
    const uint8_t server_ip[] = CELLULAR_SERVER_IP;

    xbee_serial_t serial = {
        .baudrate = CELLULAR_BAUD_RATE,
        .uart = TELEMETRY_UART
    };

    xbee_ipv4_envelope_t packet = {
        .xbee = &radio,
        .remote_port = CELLULAR_SERVER_PORT,
        .protocol = XBEE_IPV4_PROTOCOL_TCP,
        .payload = payload,
        .length = sizeof(payload) - 1U // Exclude the terminating null byte.
    };

    if (xbee_dev_init(&radio, &serial, NULL, NULL) != 0)
    {
        osSemaphoreRelease(usart1_tx_semaphore);
        return HAL_ERROR;
    }

    xbee_dev_flowcontrol(&radio, false);

    memcpy(&packet.remote_addr_be, server_ip, sizeof(server_ip));

    XBeePlatform_ResetTx();
    int result = xbee_ipv4_envelope_send(&packet);

    if (result < 0) // Error
    {
        osSemaphoreRelease(usart1_tx_semaphore);
        return (result == -EBUSY ? HAL_BUSY : HAL_ERROR);
    }

    return XBeePlatform_Transmit();
}
