/**
 *  @file  drive_state.c
 *  @brief Handles the drive state for the car. Takes ADC, GPIO and CAN inputs and
 *         outputs DAC values, MDI flags, and CAN data messages
 *
 *  @author Tony Chen
 *  @date Jan 28, 2026
 */

/* INCLUDES */
#include "drive_state.h"
#include "adc_driver.h"
#include "debug_io.h"
#include "gpio_driver.h"
// #include "CAN_comms.h"

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
// DriveStateStates ComputeNextState(const DriveStateFlags* flags);
DriveStateMotorControl ComputeNextCommand(void);
DriveStateMotorControl GetMotorCommand(uint16_t accel_DAC, uint16_t regen_DAC);
void UpdateBrakePedalFlags(void);
void ClearDriveStateFlags(void);
void ComputeNextStateHandler(void);
void BreakOnHandler(void);
void EcoPowerHandler(void);

/* DRIVE STATE FINITE STATE MACHINE */
void DriveStateFsmHandler()
{
    UpdateBrakePedalFlags();

    DriveStateMotorControl motor_command = ComputeNextCommand();

    // TODO: set_cyclic_drive_state(g_drive_state);

    // Prints current state
    DEBUG_IO_PRINT("DriveState=%u\r\n", g_drive_state);

    // Prints requested flags
    DEBUG_IO_PRINT("NextStateRequested=%u\r\n", g_drive_flags.next_state_request);
    DEBUG_IO_PRINT("PrevStateRequested=%u\r\n", g_drive_flags.prev_state_request);
    DEBUG_IO_PRINT("BrakeEnabled=%u\r\n", g_drive_flags.brake_on);
    DEBUG_IO_PRINT("EcoModeEnabled=%u\r\n", g_drive_flags.eco_mode_on);
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

    return motor_command;
}

/* DRIVE STATE DATA COLLECTION */
void UpdateBrakePedalFlags(void)
{
    bool brake_pressed = !ReadBrakePin(BRAKE_INPUT_PORT, BRAKE_INPUT_PIN);
    g_drive_flags.brake_on = brake_pressed;
    g_throttle_dac = AcceleratorDriverReadThrottle();
}

void ClearDriveStateFlags(void)
{
    g_drive_flags.next_state_request = false;
    g_drive_flags.prev_state_request = false;
}

/* SETS DRIVE STATE FLAGS */
void DriveStateInterruptHandler(uint16_t toggle)
{
    static uint32_t last_toggle = 0;
    uint32_t current_toggle = HAL_GetTick();

    ToggleLedPin(DEBUG_LED0_PORT, DEBUG_LED0_PIN);

    switch (toggle)
    {
    case BRAKE_INPUT_PIN:
        BreakOnHandler();
        break;

    case DRIVE_NEXT_PIN:
        if ((current_toggle - last_toggle) < 100)
            return; // lock
        last_toggle = current_toggle;
        g_drive_flags.next_state_request = true;
        g_drive_flags.prev_state_request = false;
        ComputeNextStateHandler();
        ClearDriveStateFlags();
        break;

    case DRIVE_PREV_PIN:
        if ((current_toggle - last_toggle) < 100)
            return; // lock
        last_toggle = current_toggle;
        g_drive_flags.prev_state_request = true;
        g_drive_flags.next_state_request = false;
        ComputeNextStateHandler();
        ClearDriveStateFlags();
        break;

    case ECO_POWER_PIN:
        EcoPowerHandler();
        break;
    }
}

void ComputeNextStateHandler(void)
{

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

void BreakOnHandler()
{
    g_drive_flags.brake_on = true;
    DriveStateMotorControl motor_command = GetMotorCommand(ACCEL_DAC_OFF, REGEN_DAC_OFF);
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

// #ifdef DEBUG
// void StateRequestCanMsgHandler(uint8_t* data)
// {
//     int value = data[0];

//     switch (value)
//     {
//     case 0:
//         g_drive_flags.next_state_request = true;
//         break;
//     case 1:
//         g_drive_flags.prev_state_request = true;
//         break;
//     }
// }
// #endif