/**
 *  @file  drive_state.c
 *  @brief Handles the drive state for the car. Takes ADC, GPIO and CAN inputs and
 *         outputs DAC values, MDI flags, and CAN data messages
 *
 *  @author Tony Chen
 *  @date Jan 28, 2026
 */

#include "drive_state.h"
#include "adc_driver.h"
#include "debug_io.h"
#include "gpio_driver.h"
// #include "CAN_comms.h"

/* FUNCTION DECLARATIONS */
typedef struct
{
    DriveStateFlags flags;
    uint16_t throttle_DAC;
} DriveStateInputs;

DriveStateStates ComputeNextState(const DriveStateFlags* flags);
DriveStateMotorControl ComputeNextCommand(const DriveStateFlags* flags, uint16_t throttle_DAC);
DriveStateMotorControl GetMotorCommand(uint16_t accel_DAC, uint16_t regen_DAC);
DriveStateInputs UpdateDriveFlags(void);
void ClearDriveFlags(void);
void BreakOnHandler(void);
void EcoPowerHandler(void);

/* GLOBAL VARIABLES */
volatile DriveStateStates g_drive_state = PARK;
volatile DriveStateFlags g_drive_flags = {0};

/* DRIVE STATE FINITE STATE MACHINE */
void DriveStateFsmHandler()
{
    DriveStateInputs inputs = UpdateDriveFlags();

    DriveStateStates next_drive_state = ComputeNextState(&inputs.flags);

    DriveStateMotorControl motor_command = ComputeNextCommand(&inputs.flags, inputs.throttle_DAC);

    // TODO: set_cyclic_drive_state(g_drive_state);

    // Prints current state
    DEBUG_IO_PRINT("DriveState=%u\r\n", g_drive_state);

    // Prints requested flags
    DEBUG_IO_PRINT("NextStateRequested=%u\r\n", inputs.flags.next_state_request);
    DEBUG_IO_PRINT("PrevStateRequested=%u\r\n", inputs.flags.prev_state_request);
    DEBUG_IO_PRINT("BrakeEnabled=%u\r\n", inputs.flags.brake_on);
    DEBUG_IO_PRINT("EcoModeEnabled=%u\r\n", inputs.flags.eco_mode_on);

    ClearDriveFlags();
}

DriveStateStates ComputeNextState(const DriveStateFlags* flags)
{

    const bool allow_state_change = flags->brake_on && flags->velocity_under_threshold;

    const bool invalid_requests = flags->next_state_request && flags->prev_state_request;

    if (invalid_requests || !allow_state_change)
    {
        return g_drive_state;
    }

    /* HANDLE CYCLE LOGIC HERE */

    // FORWARD -> PARK -> REVERSE
    switch (g_drive_state)
    {
    case PARK:
        if (flags->next_state_request)
            g_drive_state = REVERSE;
        if (flags->prev_state_request)
            g_drive_state = FORWARD;
        return g_drive_state;

    case FORWARD:
        if (flags->next_state_request)
            g_drive_state = PARK;
        if (flags->prev_state_request)
            g_drive_state = FORWARD;
        return g_drive_state;

    case REVERSE:
        if (flags->next_state_request)
            g_drive_state = REVERSE;
        if (flags->prev_state_request)
            g_drive_state = PARK;
        return g_drive_state;

    default:
        return PARK;
    }
}

DriveStateMotorControl ComputeNextCommand(const DriveStateFlags* flags, uint16_t throttle_DAC)
{

    if (flags->brake_on || (g_drive_state == PARK))
    {
        return GetMotorCommand(ACCEL_DAC_OFF, REGEN_DAC_OFF);
    }

    if (g_drive_state == REVERSE)
    {
        return GetMotorCommand(throttle_DAC, REGEN_DAC_OFF);
    }

    return GetMotorCommand(throttle_DAC, flags->regen_on ? REGEN_DAC_ON : REGEN_DAC_OFF);
}

DriveStateMotorControl GetMotorCommand(uint16_t accel_DAC, uint16_t regen_DAC)
{
    DriveStateMotorControl motor_command;
    motor_command.accel_DAC_value = accel_DAC;
    motor_command.regen_DAC_value = regen_DAC;

    return motor_command;
}

/* DRIVE STATE DATA COLLECTION */
DriveStateInputs UpdateDriveFlags(void)
{
    bool brake_pressed = ReadBrakePin(BRAKE_INPUT_PORT, BRAKE_INPUT_PIN);

    DriveStateInputs inputs = {
        .flags = g_drive_flags,
        .throttle_DAC = 0,
    };

    inputs.flags.brake_on = brake_pressed;
    inputs.throttle_DAC = AcceleratorDriverReadThrottle();

    return inputs;
}

void ClearDriveFlags(void)
{
    g_drive_flags.next_state_request = false;
    g_drive_flags.prev_state_request = false;
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
}

void EcoPowerHandler(void)
{
    if (!ReadEcoPin(ECO_POWER_PORT, ECO_POWER_PIN))
    {
        g_drive_flags.eco_mode_on = false;
    }
    else
    {
        g_drive_flags.eco_mode_on = true;
    }
}

/* CAN MESSAGE INPUT HANDLERS */

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

// static uint16_t x;
// void MotorCommandPackAndSend(DriveStateMotorControl* motor_command, bool isr) {
//     CAN_comms_Tx_msg_t msg;
//     msg.header = drive_control_header;

//     uint8_t data[8] = {0};

// }

#ifdef DEBUG
void StateRequestCanMsgHandler(uint8_t* data)
{
    int value = data[0];

    switch (value)
    {
    case 0:
        g_drive_flags.next_state_request = true;
        break;
    case 1:
        g_drive_flags.prev_state_request = true;
        break;
    }
}
#endif