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
volatile DriveStateModel g_drive_state_model = {
    .state = PARK,
    .flags = {
        .brake_on = false,
        .regen_on = false,
        .cruise_on = false,
        .velocity_under_threshold = true, // placeholder until MDI testing
        .next_state_request = false,
        .prev_state_request = false,
        .eco_mode_on = false,
    },
    .velocity_kmh = 0,
    .throttle_dac = 0,
};

/* FUNCTION DECLARATIONS */
DriveStateMotorControl ComputeNextCommand(DriveStateModel *model);
DriveStateMotorControl GetMotorCommand(DriveStateModel *model, uint16_t accel_DAC, uint16_t regen_DAC);
uint8_t UpdateMotorCommandFlags(DriveStateModel *model);
void UpdateBrakePedalFlags(DriveStateModel *model);
void ClearDriveStateFlags(DriveStateModel *model);
void ComputeNextState(DriveStateModel *model);
void BreakOnHandler(DriveStateModel *model);
void EcoPowerHandler(DriveStateModel *model);
void VelocityHandler(DriveStateModel *model, uint8_t* data);
void SteeringCanMsgHandler(DriveStateModel *model, uint8_t* data);
#ifdef DEBUG
void StateRequestCanMsgHandler(DriveStateModel *model, uint8_t* data);
#endif
void MotorCommandPackAndSend(DriveStateMotorControl *motor_command, bool isr);
void MotorControlQueryData(void);

/* DRIVE STATE FINITE STATE MACHINE */
void DriveStateFsmHandler()
{
    volatile DriveStateModel *model = &g_drive_state_model;

    UpdateBrakePedalFlags(model);
    DriveStateMotorControl motor_command = ComputeNextCommand(model);
    ComputeNextState(model);

    // TODO: set_cyclic_drive_state(model->state);

    MotorCommandPackAndSend(&motor_command, false);

    // Prints current state
    DEBUG_IO_PRINT("DriveState=%u\r\n", model->state);

    // Prints requested flags
    DEBUG_IO_PRINT("NextStateRequested=%u\r\n", model->flags.next_state_request);
    DEBUG_IO_PRINT("PrevStateRequested=%u\r\n", model->flags.prev_state_request);
    DEBUG_IO_PRINT("BrakeEnabled=%u\r\n", model->flags.brake_on);
    DEBUG_IO_PRINT("EcoModeEnabled=%u\r\n", model->flags.eco_mode_on);

    ClearDriveStateFlags(model);
}

void ComputeNextState(DriveStateModel *model)
{

    bool valid_state_change =
        !(model->flags.next_state_request && model->flags.prev_state_request);

    bool valid_drive_state = model->flags.velocity_under_threshold && valid_state_change;

    if (!valid_drive_state)
    {
        return;
    }

    switch (model->state)
    {
    case PARK:
        if (model->flags.next_state_request)
        {
            model->state = REVERSE;
        }

        else if (model->flags.prev_state_request)
        {
            model->state = FORWARD;
        }
        break;
    case FORWARD:
        if (model->flags.next_state_request)
        {
            model->state = PARK;
        }
        break;
    case REVERSE:
        if (model->flags.prev_state_request)
        {
            model->state = PARK;
        }
        break;
    default:
        model->state = PARK;
        break;
    }
}

DriveStateMotorControl ComputeNextCommand(DriveStateModel *model)
{
    if (model->flags.brake_on || (model->state == PARK))
    {
        return GetMotorCommand(model, ACCEL_DAC_OFF, REGEN_DAC_OFF);
    }

    if (model->state == REVERSE)
    {
        return GetMotorCommand(model, model->throttle_dac, REGEN_DAC_OFF);
    }

    return GetMotorCommand(model, model->throttle_dac, model->flags.regen_on ? REGEN_DAC_ON : REGEN_DAC_OFF);
}

DriveStateMotorControl GetMotorCommand(DriveStateModel *model, uint16_t accel_DAC, uint16_t regen_DAC)
{
    DriveStateMotorControl motor_command;
    motor_command.accel_DAC_value = accel_DAC;
    motor_command.regen_DAC_value = regen_DAC;
    motor_command.motor_control_flags = UpdateMotorCommandFlags(model);

    return motor_command;
}

/* DRIVE STATE DATA COLLECTION */
void UpdateBrakePedalFlags(DriveStateModel *model)
{
    model->flags.brake_on = ReadBrakePin(BRAKE_INPUT_PORT, BRAKE_INPUT_PIN);
    SetBrakeLedPin(BRAKE_LED_PORT, BRAKE_LED_PIN, model->flags.brake_on);

    model->throttle_dac = AcceleratorDriverReadThrottle();

    EcoPowerHandler(model);
}

void ClearDriveStateFlags(DriveStateModel *model)
{
    model->flags.next_state_request = false;
    model->flags.prev_state_request = false;
}

uint8_t UpdateMotorCommandFlags(DriveStateModel *model)
{
    uint8_t flags = 0;
    flags |= ((model->state == REVERSE) ? 0 : 1); // Direction Bit: 0 (REVERSE), 1 (FORWARD/PARK)
    flags |= (model->flags.eco_mode_on ? 1 << 1 : 0);
    return flags;
}

/* SETS DRIVE STATE FLAGS */
void DriveStateInterruptHandler(uint16_t toggle)
{
    volatile DriveStateModel *model = &g_drive_state_model;

    ToggleLedPin(DEBUG_LED0_PORT, DEBUG_LED0_PIN);

    switch (toggle)
    {
    case BRAKE_INPUT_PIN:
        BreakOnHandler(model);
        break;

    case DRIVE_NEXT_PIN:
        model->flags.next_state_request = true;
        break;

    case DRIVE_PREV_PIN:
        model->flags.prev_state_request = true;
        break;
    }
}

void BreakOnHandler(DriveStateModel *model)
{
    model->flags.brake_on = true;
    DriveStateMotorControl motor_command = GetMotorCommand(model, ACCEL_DAC_OFF, REGEN_DAC_OFF);
    MotorCommandPackAndSend(&motor_command, true);
    SetBrakeLedPin(BRAKE_LED_PORT, BRAKE_LED_PIN, 1);
}

void EcoPowerHandler(DriveStateModel *model)
{
    if (!ReadEcoPowerPin(ECO_POWER_PORT, ECO_POWER_PIN))
    {
        model->flags.eco_mode_on = false;
    }
    else
    {
        model->flags.eco_mode_on = true;
    }
}

/* CAN MESSAGE RX HANDLERS */
void VelocityHandler(DriveStateModel *model, uint8_t* data)
{
    uint32_t rpm = (data[4] >> 3) | ((data[5] & 0x7f) << 5);
    float velocity = (WHEEL_RADIUS * 2.0f * M_PI * rpm) / 60.0f;
    model->velocity_kmh = (uint32_t)(velocity * 3.6f);

    if (velocity < VELOCITY_THRESHOLD)
    {
        model->flags.velocity_under_threshold = true;
    }
    else
    {
        model->flags.velocity_under_threshold = false;
    }
}

void SteeringCanMsgHandler(DriveStateModel *model, uint8_t* data)
{ // not configured on STR yet regen is for bit 0 and cruise for bit 1

    model->flags.regen_on = ((data[0] >> 0) & 0x01);
    model->flags.cruise_on = ((data[0] >> 1) & 0x01);
}

#ifdef DEBUG
void StateRequestCanMsgHandler(DriveStateModel *model, uint8_t* data)
{
    int value = data[0];

    switch (value)
    {
    case 0:
        model->flags.next_state_request = true;
        model->flags.prev_state_request = false;
        break;
    case 1:
        model->flags.prev_state_request = true;
        model->flags.next_state_request = false;
        break;
    }
}
#endif

/* CAN DATA TX*/
void MotorCommandPackAndSend(DriveStateMotorControl *motor_command, bool isr)
{
    CAN_comms_Tx_msg_t msg;
    msg.header = drive_control_header;

    uint8_t data[8] = {0};

    uint8_t accel_first_byte = (uint8_t)(motor_command->accel_DAC_value >> 8) & 0xFF;
    uint8_t accel_second_byte = (uint8_t)(motor_command->accel_DAC_value >> 0);
    uint8_t regen_first_byte = (uint8_t)(motor_command->regen_DAC_value >> 8) & 0xFF;
    uint8_t regen_second_byte = (uint8_t)(motor_command->regen_DAC_value >> 0);

    data[0] = accel_first_byte;
    data[1] = accel_second_byte;
    data[2] = regen_first_byte;
    data[3] = regen_second_byte;
    data[4] = motor_command->motor_control_flags;

    memcpy(msg.data, data, CAN_DATA_SIZE);

    if (isr)
    {
        CAN_comms_Add_Tx_messageISR(&msg);
    }
    else
    {
        CAN_comms_Add_Tx_message(&msg);
    }
}

void MotorControlQueryData(void)
{
    CAN_comms_Tx_msg_t msg;

    msg.header = mdu_request_header;
    msg.data[0] = MDU_REQUEST_FRAME;
    CAN_comms_Add_Tx_message(&msg);
}