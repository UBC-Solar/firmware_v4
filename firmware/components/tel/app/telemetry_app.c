#include "telemetry_app.h"
#include "telemetry_driver.h"
#include "can_app.h"
#include "bitops.h"
#include "stdbool.h"
#include "rtc_driver.h"
#include <string.h>

#define NO_PRIORITY                                 0
#define NON_BLOCKING                                0
#define ID_DELIMITER_CHAR                           '#'
#define CARRIAGE_RETURN_CHAR                        '\r'
#define NEW_LINE_CHAR                               '\n'
#define MASK_4_BITS                                 0xF
#define NUM_FILTERS                                 ((int)(sizeof(filter_whitelist) / sizeof(filter_whitelist[0])))

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

TEL_Msg_TypeDef tel_msg = {0};
void TEL_transmit_msg(CAN_comms_Rx_msg_t* CAN_comms_Rx_msg)
{
    // Not filtered yet??? TODO
    osSemaphoreAcquire(usart1_tx_semaphore, osWaitForever);   // Dont Tx until previous Tx is done
    set_tel_msg(&(CAN_comms_Rx_msg->header), CAN_comms_Rx_msg->data, &tel_msg);
    UART_telemetry_transmit(&tel_msg);
}

// Change name of this to gps, imu send or something???
void TEL_transmit_msg_tx(CAN_comms_Tx_msg_t* CAN_comms_Tx_msg)
{
    osSemaphoreAcquire(usart1_tx_semaphore, osWaitForever);   // Dont Tx until previous Tx is done
    set_tel_msg_tx(&(CAN_comms_Tx_msg->header), CAN_comms_Tx_msg->data, &tel_msg);
    UART_telemetry_transmit(&tel_msg);
}

/**
 * @brief Sets all the fields in the radio message struct
 * 
 * @param header The CAN header struct
 * @param data The CAN data
 */
void set_tel_msg(CAN_RxHeaderTypeDef* header, uint8_t* data, TEL_Msg_TypeDef* tel_msg)
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
void set_tel_msg_tx(CAN_TxHeaderTypeDef* header, uint8_t* data, TEL_Msg_TypeDef* tel_msg)
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

/**
 * @brief Getter for CAN ID inside the CAN header struct
 * 
 * @param can_msg_header_ptr The CAN header struct
 * 
 * @return The CAN ID as a 32-bit unsigned integer to account for both standard and extended IDs
 */
uint32_t get_can_id(CAN_RxHeaderTypeDef* can_msg_header_ptr)
{
    uint32_t can_id = (can_msg_header_ptr->IDE == CAN_ID_STD) ? can_msg_header_ptr->StdId : can_msg_header_ptr->ExtId;
    return BITOPS_32BIT_REVERSE(can_id);
}

/**
 * @brief Gets the data length in the radio message
 * 
 * @param DLC The data length code from the CAN header
 * 
 * @return The data length as an 8-bit unsigned integer but only the 4 least significant bits are used
 */
uint8_t get_data_length(uint32_t DLC)
{
    return (uint8_t) (DLC & MASK_4_BITS);
}