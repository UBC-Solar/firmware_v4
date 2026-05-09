#ifndef __TELEMETRY__APP__H__
#define __TELEMETRY__APP__H__

#include "can.h"
#include "telemetry_driver.h"
#include "CAN_comms.h"
#include <stdint.h>

void TEL_transmit_msg(CAN_comms_Rx_msg_t* CAN_comms_Rx_msg);
void TEL_transmit_msg_tx(CAN_comms_Tx_msg_t* CAN_comms_Tx_msg);
void set_tel_msg(CAN_RxHeaderTypeDef* header, uint8_t* data, TEL_Msg_TypeDef* tel_msg);
void set_tel_msg_tx(CAN_TxHeaderTypeDef* header, uint8_t* data, TEL_Msg_TypeDef* tel_msg);
uint64_t get_timestamp(void);
uint32_t get_can_id(CAN_RxHeaderTypeDef* can_msg_header_ptr);
uint8_t get_data_length(uint32_t DLC);

#endif /* __TELEMETRY__APP__H__ */