/**
 * @file    fault_handler.c
 * @brief   Fault handling implementation for UBC Solar DRD board
 *
 * This file contains the implementation of the fault handling logic for the UBC Solar DRD board.
 * It manages the detection, reporting, and recovery from various fault conditions.
 *
 * @author  Gregory Bian
 * @date    Mar 7 2026
 */

#include "fault_handler.h"
#include <stdint.h>
#include <sys/_intsup.h>
#include "can_driver.h"
#include "cyclic_data_handler.h"

#define GETBIT (value, bit) (((value) >> (bit)) & 0x01)

void FaultHandlerParseMotorFaults(uint8_t* can_rx_data){

}

void FaultHandlerParseECUFaults(uint8_t* can_rx_data){

}

void FaultHandlerParseBatteryFaults(uint8_t* can_rx_data){

}

void FaultHandlerParseTemperatures(uint32_t msg_id, uint8_t* can_rx_data){
    // TODO v4 add mpptD and motor thermistor temperatures when they are added to the CAN messages
    switch (msg_id) {
        case BMS_TEMPERATURES_CAN_ID: {// Battery pack temperatures
            int8_t batt_min_temp = can_rx_data[1];
            int8_t batt_max_temp = can_rx_data[3];
            CyclicDataSetBatteryMinTemperature((uint8_t)batt_min_temp);  
            CyclicDataSetBatteryMaxTemperature((uint8_t)batt_max_temp);
            break;
        }
        case FRAME0: { // Motor controller temperature
            uint8_t mtr_cont_temp = (can_rx_data[3] >> 6) | ((can_rx_data[4] & 0x7) << 2 ); 
            CyclicDataSetMtrContTemperature(mtr_cont_temp);
            break;
        }
        case MPPTA_TEMPERATURE_CAN_ID: {
            float mpptA_temp = can_rx_data[5];
            CyclicDataSetMpptATemperature((uint8_t)mpptA_temp);
            break;
        }
        case MPPTB_TEMPERATURE_CAN_ID: {
            float mpptB_temp = can_rx_data[5];
            CyclicDataSetMpptBTemperature((uint8_t)mpptB_temp);
            break;
        }
        case MPPTC_TEMPERATURE_CAN_ID: {
            float mpptC_temp = can_rx_data[5];
            CyclicDataSetMpptCTemperature((uint8_t)mpptC_temp);
            break;
        }
        default:
            break;
    }
}
