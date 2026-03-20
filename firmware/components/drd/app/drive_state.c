/**
 * @file    drive_state.c
 * @brief   Drive state management implementation for UBC Solar DRD board
 *
 * This file implements the drive state logic for the car, handling ADC, GPIO, and CAN inputs,
 * and producing DAC values, MDI flags, and CAN data messages. It manages the drive state finite
 * state machine (FSM), state transitions, and related control logic.
 *
 * @author  Tony Chen
 * @date    Jan 28, 2026
 */

/* INCLUDES */
#include <string.h>

#include "CAN_comms.h"
#include "can_app.h"
#include "accel_driver.h"
#include "can_driver.h"
#include "debug_io.h"
#include "drive_state.h"
#include "cyclic_data_handler.h"
#include "diagnostic.h"
#include "gpio_driver.h"

/* GLOBAL VARIABLES */
volatile DriveStateCtx g_drive_state_ctx = {
    .state = PARK,
    .flags = {
        .brake_on = false,
        .regen_on = true,
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
/**
 * @brief Computes the next motor control command based on the current drive state model.
 * @param model Pointer to the drive state model.
 * @return The computed motor control command.
 */
static DriveStateMotorControl ComputeNextCommand(DriveStateCtx *ctx);
/**
 * @brief Returns a motor command using the drive state model and DAC values for acceleration and regeneration.
 * @param model Pointer to the drive state model.
 * @param accel_DAC DAC value for acceleration.
 * @param regen_DAC DAC value for regeneration.
 * @return The motor control command.
 */
static DriveStateMotorControl GetMotorCommand(DriveStateCtx *ctx, uint16_t accel_DAC, uint16_t regen_DAC);
/**
 * @brief Updates and returns the status flags for the motor command.
 * @param model Pointer to the drive state model.
 * @return Updated status flags.
 */
static uint8_t UpdateMotorCommandFlags(DriveStateCtx *ctx);
/**
 * @brief Updates the brake pedal status flags in the drive state model.
 * @param model Pointer to the drive state model.
 */
static void UpdatePedalFlags(DriveStateCtx *ctx);
/**
 * @brief Clears all drive state flags in the model.
 * @param model Pointer to the drive state model.
 */
static void ClearDriveStateFlags(DriveStateCtx *ctx);
/**
 * @brief Computes the next state for the drive state machine.
 * @param model Pointer to the drive state model.
 */
static void ComputeNextState(DriveStateCtx *ctx);
/**
 * @brief Handles logic when the brake is engaged.
 * @param model Pointer to the drive state model.
 */
static void BreakOnHandler(DriveStateCtx *ctx);
/**
 * @brief Handles logic for eco power mode.
 * @param model Pointer to the drive state model.
 */
static void EcoPowerHandler(DriveStateCtx *ctx);

/* DRIVE STATE FINITE STATE MACHINE */
void DriveStateFsmHandler()
{
    volatile DriveStateCtx *v_ctx = &g_drive_state_ctx;
    DriveStateCtx *ctx = (DriveStateCtx *)v_ctx;

    UpdatePedalFlags(ctx);
    DriveStateMotorControl motor_command = ComputeNextCommand(ctx);
    ComputeNextState(ctx);

    CyclicDataSetDriveState(g_drive_state_ctx.state);

    MotorCommandPackAndSend(&motor_command, false);

    // Prints current state
    DEBUG_IO_PRINT("DriveState=%u\r\n", ctx->state);

    // Prints requested flags
    DEBUG_IO_PRINT("NextStateRequested=%u\r\n", ctx->flags.next_state_request);
    DEBUG_IO_PRINT("PrevStateRequested=%u\r\n", ctx->flags.prev_state_request);
    DEBUG_IO_PRINT("BrakeEnabled=%u\r\n", ctx->flags.brake_on);
    DEBUG_IO_PRINT("EcoModeEnabled=%u\r\n", ctx->flags.eco_mode_on);

    ClearDriveStateFlags(ctx);
}

static void ComputeNextState(DriveStateCtx *ctx)
{

    bool valid_state_change =
        !(ctx->flags.next_state_request && ctx->flags.prev_state_request);

    bool valid_drive_state = ctx->flags.velocity_under_threshold && ctx->flags.brake_on && valid_state_change;

    if (!valid_drive_state)
    {
        return;
    }

    switch (ctx->state)
    {
    case PARK:
        if (ctx->flags.next_state_request)
        {
            ctx->state = FORWARD;
        }

        else if (ctx->flags.prev_state_request)
        {
            ctx->state = REVERSE;
        }
        break;
    case FORWARD:
        if (ctx->flags.prev_state_request)
        {
            ctx->state = PARK;
        }
        break;
    case REVERSE:
        if (ctx->flags.next_state_request)
        {
            ctx->state = PARK;
        }
        break;
    default:
        ctx->state = PARK;
        break;
    }
}

static DriveStateMotorControl ComputeNextCommand(DriveStateCtx *ctx)
{
    if (ctx->flags.brake_on || (ctx->state == PARK))
    {
        return GetMotorCommand(ctx, ACCEL_DAC_OFF, REGEN_DAC_OFF);
    }

    if (ctx->state == REVERSE)
    {
        return GetMotorCommand(ctx, ctx->throttle_dac, REGEN_DAC_OFF);
    }

    return GetMotorCommand(ctx, ctx->throttle_dac, ctx->flags.regen_on ? REGEN_DAC_ON : REGEN_DAC_OFF);
}

static DriveStateMotorControl GetMotorCommand(DriveStateCtx *ctx, uint16_t accel_DAC, uint16_t regen_DAC)
{
    DriveStateMotorControl motor_command;
    motor_command.accel_DAC_value = accel_DAC;
    motor_command.regen_DAC_value = regen_DAC;
    motor_command.motor_control_flags = UpdateMotorCommandFlags(ctx);

    return motor_command;
}

/* SETS DRIVE STATE FLAGS */
static void UpdatePedalFlags(DriveStateCtx *ctx)
{
    ctx->flags.brake_on = ReadBrakePin(BRAKE_INPUT_PORT, BRAKE_INPUT_PIN);
    SetBrakeLedPin(BRAKE_LED_PORT, BRAKE_LED_PIN, ctx->flags.brake_on);
    DiagnosticSetMechBrakePressed(ctx->flags.brake_on);

    ctx->throttle_dac = AccelDriverReadThrottle();

    EcoPowerHandler(ctx);
}

static void ClearDriveStateFlags(DriveStateCtx *ctx)
{
    ctx->flags.next_state_request = false;
    ctx->flags.prev_state_request = false;
}

static uint8_t UpdateMotorCommandFlags(DriveStateCtx *ctx)
{
    uint8_t flags = 0;
    flags |= ((ctx->state == REVERSE) ? 0 : 1); // Direction Bit: 0 (REVERSE), 1 (FORWARD/PARK)
    flags |= (ctx->flags.eco_mode_on ? 1 << 1 : 0);
    return flags;
}

void DriveStateInterruptHandler(uint16_t toggle)
{
    volatile DriveStateCtx *v_ctx = &g_drive_state_ctx;
    DriveStateCtx *ctx = (DriveStateCtx *)v_ctx;

    ToggleLedPin(DEBUG_LED0_PORT, DEBUG_LED0_PIN);

    switch (toggle)
    {
    case BRAKE_INPUT_PIN:
        BreakOnHandler(ctx);
        break;

    case DRIVE_NEXT_PIN:
        ctx->flags.next_state_request = true;
        break;

    case DRIVE_PREV_PIN:
        ctx->flags.prev_state_request = true;
        break;
    }
}

static void BreakOnHandler(DriveStateCtx *ctx)
{
    ctx->flags.brake_on = true;
    DriveStateMotorControl motor_command = GetMotorCommand(ctx, ACCEL_DAC_OFF, REGEN_DAC_OFF);
    MotorCommandPackAndSend(&motor_command, true);
    SetBrakeLedPin(BRAKE_LED_PORT, BRAKE_LED_PIN, 1);
}

static void EcoPowerHandler(DriveStateCtx *ctx)
{
    if (!ReadEcoPowerPin(ECO_POWER_PORT, ECO_POWER_PIN))
    {
        ctx->flags.eco_mode_on = false;
    }
    else
    {
        ctx->flags.eco_mode_on = true;
    }
}

/* CAN MESSAGE RX HANDLERS */
void DriveStateVelocityCanMsgHandler(uint8_t* data)
{
    uint32_t rpm = (data[4] >> 3) | ((data[5] & 0x7f) << 5);
    float velocity = (WHEEL_RADIUS * 2.0f * M_PI * rpm) / 60.0f;
    g_drive_state_ctx.velocity_kmh = (uint32_t)(velocity * 3.6f);
    CyclicDataSetSpeed(g_drive_state_ctx.velocity_kmh);

    if (velocity < VELOCITY_THRESHOLD)
    {
        g_drive_state_ctx.flags.velocity_under_threshold = true;
    }
    else
    {
        g_drive_state_ctx.flags.velocity_under_threshold = false;
    }
}

void DriveStateSteeringCanMsgHandler(uint8_t* data)
{ // not configured on STR yet regen is for bit 0 and cruise for bit 1

    g_drive_state_ctx.flags.regen_on = ((data[0] >> 0) & 0x01);
    g_drive_state_ctx.flags.cruise_on = ((data[0] >> 1) & 0x01);
    DiagnosticSetRegenEnabled(g_drive_state_ctx.flags.regen_on);

}

#ifdef DEBUG
void StateRequestCanMsgHandler(uint8_t* data)
{
    int value = data[0];

    switch (value)
    {
    case 0:
        g_drive_state_ctx.flags.next_state_request = true;
        g_drive_state_ctx.flags.prev_state_request = false;
        break;
    case 1:
        g_drive_state_ctx.flags.prev_state_request = true;
        g_drive_state_ctx.flags.next_state_request = false;
        break;
    }
}
#endif

DriveStateStates DriveStateGetDriveState(void)
{
    return g_drive_state_ctx.state;
}

volatile bool DriveStateGetDriveMode(){
    return g_drive_state_ctx.flags.eco_mode_on;
}