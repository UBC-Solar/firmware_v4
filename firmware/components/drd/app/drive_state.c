/**
 *  @file  drive_state.c
 *  @brief Handles the drive state for the car. Takes ADC, GPIO and CAN inputs and
 *         outputs DAC values, MDI flags, and CAN data messages
 *
 *  @author Tony Chen
 *  @date Jan 28, 2026
 */

/* INCLUDES */
#include <string.h>

#include "CAN_comms.h"
#include "accel_driver.h"
#include "can_driver.h"
#include "debug_io.h"
#include "drive_state.h"
#include "gpio_driver.h"

/* GLOBAL VARIABLES */
volatile DriveStateStates g_drive_state = PARK;
volatile DriveStateFlags g_drive_flags = {
    .brake_on = false,
    .regen_on = false,
    .cruise_on = false,
    .velocity_under_threshold = true, // placeholder until MDI testing
    .next_state_request = false,
    .prev_state_request = false,
    .eco_mode_on = false,
};
volatile uint32_t g_velocity_kmh = 0;
volatile uint16_t g_throttle_dac = 0;

/* FUNCTION DECLARATIONS */
DriveStateMotorControl ComputeNextCommand(void);
DriveStateMotorControl GetMotorCommand(uint16_t accel_DAC, uint16_t regen_DAC);
uint8_t UpdateMotorCommandFlags(void);
void UpdateBrakePedalFlags(void);
void ClearDriveStateFlags(void);
void ComputeNextState(void);
void BreakOnHandler(void);
void EcoPowerHandler(void);
void VelocityHandler(uint8_t* data);
// void SteeringCanMsgHandler(uint8_t* data);
// void VechicleStateCANRxHandler(uint32_t msg_id, uint8_t* data);
#ifdef DEBUG
void StateRequestCanMsgHandler(uint8_t* data);
#endif
// void MotorCommandPackAndSend(DriveStateMotorControl *motor_command, bool isr);
// void MotorControlQueryData(void);

/* DRIVE STATE FINITE STATE MACHINE */
void DriveStateFsmHandler()
{
    UpdateBrakePedalFlags();
    DriveStateMotorControl motor_command = ComputeNextCommand();
    ComputeNextState();

    // TODO: set_cyclic_drive_state(g_drive_state);

    // MotorCommandPackAndSend(&motor_command, false);

    // Prints current state
    DEBUG_IO_PRINT("DriveState=%u\r\n", g_drive_state);

    // Prints requested flags
    DEBUG_IO_PRINT("NextStateRequested=%u\r\n", g_drive_flags.next_state_request);
    DEBUG_IO_PRINT("PrevStateRequested=%u\r\n", g_drive_flags.prev_state_request);
    DEBUG_IO_PRINT("BrakeEnabled=%u\r\n", g_drive_flags.brake_on);
    DEBUG_IO_PRINT("EcoModeEnabled=%u\r\n", g_drive_flags.eco_mode_on);
}

void ComputeNextState(void)
{

    bool valid_state_change = !(g_drive_flags.next_state_request && g_drive_flags.prev_state_request);

    bool valid_drive_state = !(g_drive_flags.velocity_under_threshold) && valid_state_change;

    if (!g_drive_flags.velocity_under_threshold)
    {
        return;
    }

    switch (g_drive_state)
    {
    case PARK:
        if (g_drive_flags.next_state_request)
        {
            g_drive_state = REVERSE;
        }

        else if (g_drive_flags.prev_state_request)
        {
            g_drive_state = FORWARD;
        }
        break;
    case FORWARD:
        if (g_drive_flags.next_state_request)
        {
            g_drive_state = PARK;
        }
        break;
    case REVERSE:
        if (g_drive_flags.prev_state_request)
        {
            g_drive_state = PARK;
        }
        break;
    default:
        g_drive_state = PARK;
        break;
    }
}

DriveStateMotorControl ComputeNextCommand(void)
{
    if (g_drive_flags.brake_on || (g_drive_state == PARK))
    {
        return GetMotorCommand(ACCEL_DAC_OFF, REGEN_DAC_OFF);
    }

    if (g_drive_state == REVERSE)
    {
        return GetMotorCommand(g_throttle_dac, REGEN_DAC_OFF);
    }

    return GetMotorCommand(g_throttle_dac, g_drive_flags.regen_on ? REGEN_DAC_ON : REGEN_DAC_OFF);
}

DriveStateMotorControl GetMotorCommand(uint16_t accel_DAC, uint16_t regen_DAC)
{
    DriveStateMotorControl motor_command;
    motor_command.accel_DAC_value = accel_DAC;
    motor_command.regen_DAC_value = regen_DAC;
    //motor_command.motor_control_flags = UpdateMotorCommandFlags();

    return motor_command;
}

/* DRIVE STATE DATA COLLECTION */
void UpdateBrakePedalFlags(void)
{
    if (g_drive_flags.brake_on && ReadBrakePin(BRAKE_INPUT_PORT, BRAKE_INPUT_PIN)) {
        g_drive_flags.brake_on = false;
    }

    g_throttle_dac = AcceleratorDriverReadThrottle();
}

void ClearDriveStateFlags(void)
{
    g_drive_flags.next_state_request = false;
    g_drive_flags.prev_state_request = false;
}

uint8_t UpdateMotorCommandFlags(void)
{
    uint8_t flags = 0;
    flags |= ((g_drive_state == REVERSE) ? 0 : 1); // Direction Bit: 0 (REVERSE), 1 (FORWARD/PARK)
    flags |= (g_drive_flags.eco_mode_on ? 1 << 1 : 0);
    return flags;
}

/* SETS DRIVE STATE FLAGS */
void DriveStateInterruptHandler(uint16_t toggle)
{
    ToggleLedPin(DEBUG_LED0_PORT, DEBUG_LED0_PIN);

    switch (toggle)
    {
    case BRAKE_INPUT_PIN:
        BreakOnHandler();
        break;

    case DRIVE_NEXT_PIN:
        g_drive_flags.next_state_request = true;
        break;

    case DRIVE_PREV_PIN:
        g_drive_flags.prev_state_request = true;
        break;

    case ECO_POWER_PIN:
        EcoPowerHandler();
        break;
    }
}

void BreakOnHandler()
{
    g_drive_flags.brake_on = true;
    DriveStateMotorControl motor_command = GetMotorCommand(ACCEL_DAC_OFF, REGEN_DAC_OFF);
    // MotorCommandPackAndSend(&motor_command, true);
    ToggleBrakeLedPin(BRAKE_LED_PORT, BRAKE_LED_PIN);
}

void EcoPowerHandler(void)
{
    if (!ReadEcoPowerPin(ECO_POWER_PORT, ECO_POWER_PIN))
    {
        g_drive_flags.eco_mode_on = false;
    }
    else
    {
        g_drive_flags.eco_mode_on = true;
    }
}

/* CAN MESSAGE RX HANDLERS */
void VelocityHandler(uint8_t* data)
{
    uint32_t rpm = (data[4] >> 3) | ((data[5] & 0x7f) << 5);
    float velocity = (WHEEL_RADIUS * 2.0f * M_PI * rpm) / 60.0f;
    g_velocity_kmh = velocity * 3.6f;

    if (velocity < VELOCITY_THRESHOLD)
    {
        g_drive_flags.velocity_under_threshold = true;
    }
    else
    {
        g_drive_flags.velocity_under_threshold = false;
    }
}

void SteeringCanMsgHandler(uint8_t* data)
{ // not configured on STR yet regen is for bit 0 and cruise for bit 1

    g_drive_flags.regen_on = ((data[0] >> 0) & 0x01);
    g_drive_flags.cruise_on = ((data[0] >> 1) & 0x01);
}

#ifdef DEBUG
void StateRequestCanMsgHandler(uint8_t* data)
{
    int value = data[0];

    switch (value)
    {
    case 0:
        g_drive_flags.next_state_request = true;
        g_drive_flags.prev_state_request = false;
        ComputeNextStateHandler();
        ClearDriveStateFlags();
        break;
    case 1:
        g_drive_flags.prev_state_request = true;
        g_drive_flags.next_state_request = false;
        ComputeNextStateHandler();
        ClearDriveStateFlags();
        break;
    }
}
#endif

/* CAN DATA RX */
// void VechicleStateCANRxHandler(uint32_t msg_id, uint8_t* data)
// {

//     switch (msg_id)
//     {
//     case FRAME0:
//         VelocityHandler(data);
//         break;
//     case STR_CAN_MSG_ID:
//         SteeringCanMsgHandler(data);
//         break;

// #ifdef DEBUG
//     case 0x500:
//         StateRequestCanMsgHandler(data);
//         break;
// #endif
//     }
// }

/* CAN DATA TX*/
// void MotorCommandPackAndSend(DriveStateMotorControl *motor_command, bool isr)
// {
//     CAN_comms_Tx_msg_t msg;
//     msg.header = drive_control_header;

//     uint8_t data[8] = {0};

//     uint8_t accel_first_byte = (uint8_t)(motor_command->accel_DAC_value >> 8) & 0xFF;
//     uint8_t accel_second_byte = (uint8_t)(motor_command->accel_DAC_value >> 0);
//     uint8_t regen_first_byte = (uint8_t)(motor_command->regen_DAC_value >> 8) & 0xFF;
//     uint8_t regen_second_byte = (uint8_t)(motor_command->regen_DAC_value >> 0);

//     data[0] = accel_first_byte;
//     data[1] = accel_second_byte;
//     data[2] = regen_first_byte;
//     data[3] = regen_second_byte;
//     data[4] = motor_command->motor_control_flags;

//     memcpy(msg.data, data, CAN_DATA_SIZE);

//     if (isr)
//     {
//         CAN_comms_Add_Tx_messageISR(&msg);
//     }
//     else
//     {
//         CAN_comms_Add_Tx_message(&msg);
//     }
// }

// void MotorControlQueryData(void)
// {
//     CAN_comms_Tx_msg_t msg;

//     msg.header = mdu_request_header;
//     msg.data[0] = MDU_REQUEST_FRAME;
//     CAN_comms_Add_Tx_message(&msg);
// }