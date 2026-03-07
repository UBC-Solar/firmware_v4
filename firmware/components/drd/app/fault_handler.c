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

#define GETBIT (value, bit) (((value) >> (bit)) & 0x01)

void FaultHandlerParseMotorFaults(uint8_t* can_rx_data){
    
}

void FaultHandlerParseECUFaults(uint8_t* can_rx_data){

}

void FaultHandlerParseBatteryFaults(uint8_t* can_rx_data){

}

void FaultHandlerParseTemperatures(uint8_t* can_rx_data){

}
 // get values of faults from CAN

 // get temperatures

 //