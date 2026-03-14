/**
 * @file    fault_handler.c
 * @brief   Fault handling implementation for UBC Solar DRD board
 *
 * This file contains the implementation of the fault handling logic for the UBC Solar DRD board.
 * It manages the detection, reporting, and recovery from various fault conditions.
 *
 * See CAN ID table for any questions regarding how faults and warnings are detected from CAN messages:
 *
 * @author  Gregory Bian
 * @date    Mar 7 2026
 */

#include "fault_handler.h"
#include "debug_io.h"
#include "lcd_handler.h"
#include <stdint.h>
#include <sys/_intsup.h>
#include <sys/select.h>
#include "can_driver.h"
#include "cyclic_data_handler.h"
#include "fault_handler_driver.h"
#include <string.h>

// TODO: Make library with these macros
#define GETBIT(value, bit) (((value) >> (bit)) & 0x01)

static bool g_last_pack_current_sign = 0;
static bool g_battery_fault = false;
static bool g_ecu_fault = false;
static bool g_pack_voltage_fault = false;
static bool g_motor_fault = false;

void FaultHandlerFlashLED() {
    while(g_battery_fault || g_ecu_fault || g_pack_voltage_fault || g_motor_fault) {
        FaultHandlerDriverFlashDebug();
    }
}

void FaultHandlerEStop(uint8_t* can_rx_data) {
    bool estop = GETBIT(can_rx_data[5], 5);
    FaultHandlerDriverEStop(estop);
}

void FaultHandlerParseBatteryFaults(uint8_t* can_rx_data){    
    // TODO: v4 figure out what supplo is(v4 thing)
    LcdHandlerSetBatterySlaveBoardCommFault(GETBIT(can_rx_data[0], 0));
    LcdHandlerSetBMSSelfTestFault(GETBIT(can_rx_data[0], 1));
    LcdHandlerSetBatteryOvertemp(GETBIT(can_rx_data[0], 2));
    LcdHandlerSetBatteryUndervoltFault(GETBIT(can_rx_data[0], 3));
    LcdHandlerSetBatteryVoltageHigh(GETBIT(can_rx_data[0], 4));

    // Charging overcurrent fault if pack current is negative
    bool charge_overcurrent = GETBIT(can_rx_data[0], 6) && g_last_pack_current_sign;
    // Discharging overcurrent fault if pack current is positive
    bool discharge_overcurrent = GETBIT(can_rx_data[0], 6) && !g_last_pack_current_sign;
    LcdHandlerSetBatteryChargeOvercurrentFault(charge_overcurrent);
    LcdHandlerSetBatteryDischargeOvercurrentFault(discharge_overcurrent);

    g_battery_fault = 
        charge_overcurrent || discharge_overcurrent || 
        GETBIT(can_rx_data[0], 0) || GETBIT(can_rx_data[0], 1) || 
        GETBIT(can_rx_data[0], 2) || GETBIT(can_rx_data[0], 3) || 
        GETBIT(can_rx_data[0], 4);
    // General battery fault flag is set if any of the specific battery faults are set
    LcdHandlerSetBatteryFault(g_battery_fault);
}
void FaultHandlerParseECUFaults(uint8_t* can_rx_data){
    // MSB of pack current is the sign bit, so we save it to determine if the overcurrent fault is a charge or discharge fault in the battery fault handler
    g_last_pack_current_sign = GETBIT(can_rx_data[1],7);
    LcdHandlerSetBatteryResetFromWatchdogFault(GETBIT(can_rx_data[5], 4));
    g_ecu_fault = GETBIT(can_rx_data[5], 4);
}

void FaultHandlerParsePackVoltageFaults(uint8_t* can_rx_data){
    uint16_t pack_voltage = (can_rx_data[1] << 8) | can_rx_data[0];
    pack_voltage = pack_voltage / PACK_VOLTAGE_DIVISOR;
    if(pack_voltage >= MAX_PACK_VOLTAGE)
    {
        LcdHandlerSetBatteryOvervoltFault(true);
        LcdHandlerSetBatteryUndervoltFault(false);
        g_pack_voltage_fault = true;
    }
    else if(pack_voltage <= MIN_PACK_VOLTAGE)
    {
        LcdHandlerSetBatteryOvervoltFault(false);
        LcdHandlerSetBatteryUndervoltFault(true);
        g_pack_voltage_fault = true;
    }
    else
    {
        LcdHandlerSetBatteryOvervoltFault(false);
        LcdHandlerSetBatteryUndervoltFault(false);
        g_pack_voltage_fault = false;
    }
}

void FaultHandlerParseMotorFaults(uint8_t* can_rx_data){
    LcdHandlerSetMotorFetThermistorError(GETBIT(can_rx_data[0], 3));
    LcdHandlerSetMotorOvercurrentFault(GETBIT(can_rx_data[2], 1));
    LcdHandlerSetMotorOvervoltageFault(GETBIT(can_rx_data[2], 3));
    LcdHandlerSetMotorSystemFault(GETBIT(can_rx_data[3], 0));

    g_motor_fault = GETBIT(can_rx_data[0], 3) || GETBIT(can_rx_data[2], 1) 
                || GETBIT(can_rx_data[2], 3) || GETBIT(can_rx_data[3], 0);
}

void FaultHandlerParseBatteryWarnings(uint8_t* can_rx_data) {
    LcdHandlerSetLowVoltWarning(GETBIT(can_rx_data[1], 5));
    LcdHandlerSetHighVoltWarning(GETBIT(can_rx_data[1], 6));
    LcdHandlerSetLowTempWarning(GETBIT(can_rx_data[1], 7));
    LcdHandlerSetHighTempWarning(GETBIT(can_rx_data[2], 0));
    LcdHandlerSetNoEcuMessageWarning(GETBIT(can_rx_data[2], 2));
}

void FaultHandlerParseECUWarnings(uint8_t* can_rx_data) {
    LcdHandlerSetPackOverdischargeWarning(GETBIT(can_rx_data[5], 0));
    LcdHandlerSetPackOverchargeWarning(GETBIT(can_rx_data[5], 1));
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
            mtr_cont_temp *= 5;
            CyclicDataSetMtrContTemperature(mtr_cont_temp);
            break;
        }
        case MPPTA_TEMPERATURE_CAN_ID: {
            float mpptA_temp;
            memcpy(&mpptA_temp, &can_rx_data[4], sizeof(float));
            CyclicDataSetMpptATemperature((uint8_t)mpptA_temp);
            break;
        }
        case MPPTB_TEMPERATURE_CAN_ID: {
            float mpptB_temp;
            memcpy(&mpptB_temp, &can_rx_data[4], sizeof(float));
            CyclicDataSetMpptBTemperature((uint8_t)mpptB_temp);
            break;
        }
        case MPPTC_TEMPERATURE_CAN_ID: {
            float mpptC_temp;
            memcpy(&mpptC_temp, &can_rx_data[4], sizeof(float));
            CyclicDataSetMpptCTemperature((uint8_t)mpptC_temp);
            break;
        }
        default:
            break;
    }
}