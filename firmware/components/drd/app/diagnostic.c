/**
 * @file    diagnostic.c
 * @brief   Diagnostic data management for UBC Solar's DRD Module
 *
 * This file is used to declare and send CAN messages for diagnostic and heartbeat data for the DRD Module.
 *
 * @author  Gregory Bian
 * @date    Feb 4 2026
 */


/*	INCLUDES	*/
#include "CAN_comms.h"
#include "can_driver.h"
#include "diagnostic.h"
#include "iwdg_driver.h"
#include "lcd_app.h"
#include "drive_state.h"
#include "soc.h"

/*	GLOBAL VARIABLES	*/
static uint32_t time_since_bootup = 0;
DiagnosticDRD diagnostics = {0};


uint16_t DiagnosticGetRawADC1() {
    return diagnostics.raw_adc1;
}
uint16_t DiagnosticGetRawADC2() {
    return diagnostics.raw_adc2;
}
bool DiagnosticGetMechBrakePressed() {
    return diagnostics.flags.mech_brake_pressed;
}
bool DiagnosticGetRegenEnabled() {
    return diagnostics.flags.regen_enabled;
}
bool DiagnosticGetThrottleADCOutOfRange() {
    return diagnostics.flags.throttle_ADC_out_of_range;
}
bool DiagnosticGetThrottleADCMismatch() {
    return diagnostics.flags.throttle_ADC_mismatch;
}
bool DiagnosticGetMotorCommFault() {
    return diagnostics.flags.motor_comm_fault;
}
bool DiagnosticGetSpeedTimeout() {
    return diagnostics.cyclic_flags.speed_timeout;
}
bool DiagnosticGetDriveStateTimeout() {
    return diagnostics.cyclic_flags.drive_state_timeout;
}
bool DiagnosticGetSocTimeout() {
    return diagnostics.cyclic_flags.soc_timeout;
}
bool DiagnosticGetVoltageTimeout() {
    return diagnostics.cyclic_flags.voltage_timeout;
}
bool DiagnosticGetCurrentTimeout() {
    return diagnostics.cyclic_flags.current_timeout;
}
DiagnosticDRD DiagnosticGetAllDiagnostics() {
    return diagnostics;
}

void DiagnosticSetRawADC1(uint16_t raw_adc1){
    diagnostics.raw_adc1 = raw_adc1;
}

void DiagnosticSetRawADC2(uint16_t raw_adc2){
    diagnostics.raw_adc2 = raw_adc2;
}

void DiagnosticSetMechBrakePressed(bool pressed){
    diagnostics.flags.mech_brake_pressed = pressed;
}

void DiagnosticSetRegenEnabled(bool enabled){
    diagnostics.flags.regen_enabled = enabled;
}

void DiagnosticSetThrottleADCOutOfRange(bool out_of_range){
    diagnostics.flags.throttle_ADC_out_of_range = out_of_range;
}

void DiagnosticSetThrottleADCMismatch(bool mismatch){
    diagnostics.flags.throttle_ADC_mismatch = mismatch;
}  

void DiagnosticSetWatchdogReset(bool reset){
    diagnostics.flags.watchdog_reset = reset;
}

void DiagnosticSetMotorCommFault(bool fault){
    diagnostics.flags.motor_comm_fault = fault;
}   

void DiagnosticSetSpeedTimeout(bool timeout){
    diagnostics.cyclic_flags.speed_timeout = timeout;
}

void DiagnosticSetDriveStateTimeout(bool timeout){
    diagnostics.cyclic_flags.drive_state_timeout = timeout;
}

void DiagnosticSetSocTimeout(bool timeout){
    diagnostics.cyclic_flags.soc_timeout = timeout;
}

void DiagnosticSetVoltageTimeout(bool timeout){
    diagnostics.cyclic_flags.voltage_timeout = timeout;
}

void DiagnosticSetCurrentTimeout(bool timeout){
    diagnostics.cyclic_flags.current_timeout = timeout;
}

/**
 * @brief  Sends the time since bootup via CAN
 * @retval None
 */
void DiagnosticTimeSinceBootup()
{
    time_since_bootup++;
    CAN_comms_Tx_msg_t time_since_bootup_can_tx = {
        .data[0] = (time_since_bootup & 0x000000FFU) >> 0,
        .data[1] = (time_since_bootup & 0x0000FF00U) >> 8,
        .data[2] = (time_since_bootup & 0x00FF0000U) >> 16,
        .data[3] = (time_since_bootup & 0xFF000000U) >> 24,
        .header = time_since_bootup_can_header,
    };
    CAN_comms_Add_Tx_message(&time_since_bootup_can_tx);
}

/*	@brief Transmits DRD Diagnostic Messages over CAN
 *
 */
void DiagnosticTransmit(bool from_ISR)
{
    CAN_comms_Tx_msg_t msg;
    msg.header = drd_diagnostic_header;


    // TODO: Update the data bytes with the appropriate diagnostic data
    msg.data[0] = (diagnostics.raw_adc1 & 0xFF);
    msg.data[1] = (diagnostics.raw_adc1 >> 8);
    msg.data[2] = (diagnostics.raw_adc2 & 0xFF);
    msg.data[3] = (diagnostics.raw_adc2 >> 8);
    msg.data[4] = diagnostics.flags.all_flags;
    msg.data[5] = g_drive_state_model.state & 0xFF;
    msg.data[6] = diagnostics.cyclic_flags.cyclic_data_all_flags;
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
