/**
 * @file    diagnostic.c
 * @brief   Diagnostic data management for UBC Solar's DRD Module
 *
 * This file is used to declare and send CAN messages for diagnostic and heartbeat data for the DRD Module.
 *
 * @author  Gregory Bian
 * @date    Feb 4 2026
 */


/*	Includes	*/
#include "CAN_comms.h"
#include "can_driver.h"
#include "diagnostic.h"
#include "drive_state.h"
#include "soc.h"

/* Static Variables */
static uint32_t g_time_since_bootup = 0;
DiagnosticDRD g_diagnostics = {0};

/***************** Diagnostic Setters *****************/

void DiagnosticSetRawADC1(uint16_t raw_adc1){
    g_diagnostics.raw_adc1 = raw_adc1;
}

void DiagnosticSetRawADC2(uint16_t raw_adc2){
    g_diagnostics.raw_adc2 = raw_adc2;
}

void DiagnosticSetMechBrakePressed(bool pressed){
    g_diagnostics.flags.mech_brake_pressed = pressed;
}

void DiagnosticSetRegenEnabled(bool enabled){
    g_diagnostics.flags.regen_enabled = enabled;
}

void DiagnosticSetThrottleADCOutOfRange(bool out_of_range){
    g_diagnostics.flags.throttle_ADC_out_of_range = out_of_range;
}

void DiagnosticSetThrottleADCMismatch(bool mismatch){
    g_diagnostics.flags.throttle_ADC_mismatch = mismatch;
}  

void DiagnosticSetWatchdogReset(bool reset){
    g_diagnostics.flags.watchdog_reset = reset;
}

void DiagnosticSetMotorCommFault(bool fault){
    g_diagnostics.flags.motor_comm_fault = fault;
}   

void DiagnosticSetSpeedTimeout(bool timeout){
    g_diagnostics.cyclic_flags.speed_timeout = timeout;
}

void DiagnosticSetDriveStateTimeout(bool timeout){
    g_diagnostics.cyclic_flags.drive_state_timeout = timeout;
}

void DiagnosticSetSocTimeout(bool timeout){
    g_diagnostics.cyclic_flags.soc_timeout = timeout;
}

void DiagnosticSetVoltageTimeout(bool timeout){
    g_diagnostics.cyclic_flags.voltage_timeout = timeout;
}

void DiagnosticSetCurrentTimeout(bool timeout){
    g_diagnostics.cyclic_flags.current_timeout = timeout;
}


void DiagnosticTimeSinceBootup()
{
    g_time_since_bootup++;
    CAN_comms_Tx_msg_t time_since_bootup_can_tx = {
        .data[0] = (g_time_since_bootup & 0x000000FFU) >> 0,
        .data[1] = (g_time_since_bootup & 0x0000FF00U) >> 8,
        .data[2] = (g_time_since_bootup & 0x00FF0000U) >> 16,
        .data[3] = (g_time_since_bootup & 0xFF000000U) >> 24,
        .header = time_since_bootup_can_header,
    };
    CAN_comms_Add_Tx_message(&time_since_bootup_can_tx);
}

void DiagnosticTransmit(bool from_ISR)
{
    CAN_comms_Tx_msg_t msg;
    msg.header = drd_diagnostic_header;

    msg.data[0] = (g_diagnostics.raw_adc1 & 0xFF);
    msg.data[1] = (g_diagnostics.raw_adc1 >> 8);
    msg.data[2] = (g_diagnostics.raw_adc2 & 0xFF);
    msg.data[3] = (g_diagnostics.raw_adc2 >> 8);
    msg.data[4] = g_diagnostics.flags.all_flags;
    msg.data[5] = DriveStateGetDriveState() & 0xFF;
    msg.data[6] = g_diagnostics.cyclic_flags.cyclic_data_all_flags;
    msg.data[7] = (uint8_t)(SocGetSoc() * 100.0f);

    if (from_ISR)
    {
        CAN_comms_Add_Tx_messageISR(&msg);
    }
    else
    {
        CAN_comms_Add_Tx_message(&msg);
    }
}
