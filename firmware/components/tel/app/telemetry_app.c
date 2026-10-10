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

/* DEFINES */
#define NO_PRIORITY                                 0
#define NON_BLOCKING                                0
#define ID_DELIMITER_CHAR                           '#'
#define CARRIAGE_RETURN_CHAR                        '\r'
#define NEW_LINE_CHAR                               '\n'
#define MASK_4_BITS                                 0xF
#define NUM_FILTERS                                 ((int)(sizeof(filter_whitelist) / sizeof(filter_whitelist[0])))
#define CELL_LINE_MAX_LENGTH                        64U     // longest line is 51: 20 digit time, extended ID, 8 data bytes
#define CELL_STD_ID_HEX_DIGITS                      3
#define CELL_EXT_ID_HEX_DIGITS                      8
#define BITS_PER_HEX_DIGIT                          4
#define MILLISECONDS_PER_SECOND                     1000.0

/* STATIC VARIABLES */
static TEL_Msg_TypeDef tel_msg = {0};
static char cell_line[CELL_LINE_MAX_LENGTH];
static const char hex_digits[] = "0123456789ABCDEF";

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
 * @brief  Formats a CAN message as one cellular telemetry line (format in telemetry_app.h)
 * @param  can_id: The CAN ID, not bit-reversed
 * @param  ide: CAN_ID_STD or CAN_ID_EXT
 * @param  dlc: The data length code from the CAN header
 * @param  data: Pointer to the CAN data
 * @param  line: Buffer of at least CELL_LINE_MAX_LENGTH characters
 * @retval Number of characters written, including the trailing newline
 */
static uint16_t TelAppSetCellLine(uint32_t can_id, uint32_t ide, uint32_t dlc, const uint8_t* data, char* line);
/**
 * @brief  Writes an unsigned integer as decimal digits
 * @param  out: Where to write the digits
 * @param  value: The value to write
 * @retval Pointer to the character after the last digit written
 */
static char* TelAppWriteDecimal(char* out, uint64_t value);
/**
 * @brief  Writes the low 4 * num_digits bits of a value as uppercase hex digits
 * @param  out: Where to write the digits
 * @param  value: The value to write
 * @param  num_digits: Number of hex digits to write
 * @retval Pointer to the character after the last digit written
 */
static char* TelAppWriteHex(char* out, uint32_t value, int num_digits);
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
    if (car_config_cellular)
    {
        uint16_t len = TelAppSetCellLine(can_id, CAN_comms_Rx_msg->header.IDE, CAN_comms_Rx_msg->header.DLC, CAN_comms_Rx_msg->data, cell_line);
        UART_telemetry_transmit((uint8_t*)cell_line, len);
    }
    else
    {
        TelAppSetMsg(&(CAN_comms_Rx_msg->header), CAN_comms_Rx_msg->data, &tel_msg);
        UART_telemetry_transmit((uint8_t*)&tel_msg, sizeof(TEL_Msg_TypeDef));
    }
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
    if (car_config_cellular)
    {
        uint16_t len = TelAppSetCellLine(can_id, CAN_comms_Tx_msg->header.IDE, CAN_comms_Tx_msg->header.DLC, CAN_comms_Tx_msg->data, cell_line);
        UART_telemetry_transmit((uint8_t*)cell_line, len);
    }
    else
    {
        TelAppSetMsg_tx(&(CAN_comms_Tx_msg->header), CAN_comms_Tx_msg->data, &tel_msg);
        UART_telemetry_transmit((uint8_t*)&tel_msg, sizeof(TEL_Msg_TypeDef));
    }
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

static uint16_t TelAppSetCellLine(uint32_t can_id, uint32_t ide, uint32_t dlc, const uint8_t* data, char* line)
{
    char* p = line;
    uint8_t data_len = get_data_length(dlc);
    if (data_len > TEL_DATA_LENGTH)
    {
        data_len = TEL_DATA_LENGTH;
    }

    *p++ = '@';
    p = TelAppWriteDecimal(p, (uint64_t)(RtcDriverGetTimeStamp() * MILLISECONDS_PER_SECOND + 0.5));
    *p++ = ',';
    p = TelAppWriteHex(p, can_id, (ide == CAN_ID_STD) ? CELL_STD_ID_HEX_DIGITS : CELL_EXT_ID_HEX_DIGITS);
    *p++ = ID_DELIMITER_CHAR;
    for (uint8_t i = 0; i < data_len; i++)
    {
        p = TelAppWriteHex(p, data[i], 2);
    }

    uint8_t checksum = 0;
    for (const char* c = line + 1; c < p; c++)
    {
        checksum ^= (uint8_t)*c;
    }
    *p++ = '*';
    p = TelAppWriteHex(p, checksum, 2);
    *p++ = NEW_LINE_CHAR;

    return (uint16_t)(p - line);
}

static char* TelAppWriteDecimal(char* out, uint64_t value)
{
    char digits[20];    // UINT64_MAX has 20 digits
    int num_digits = 0;

    do
    {
        digits[num_digits++] = (char)('0' + (value % 10));
        value /= 10;
    } while (value > 0);

    while (num_digits > 0)
    {
        *out++ = digits[--num_digits];
    }
    return out;
}

static char* TelAppWriteHex(char* out, uint32_t value, int num_digits)
{
    for (int shift = (num_digits - 1) * BITS_PER_HEX_DIGIT; shift >= 0; shift -= BITS_PER_HEX_DIGIT)
    {
        *out++ = hex_digits[(value >> shift) & MASK_4_BITS];
    }
    return out;
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