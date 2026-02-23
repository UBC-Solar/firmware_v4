/**
 * @file    drive_state.h
 * @brief   Drive state management API for UBC Solar DRD board
 *
 * This header declares the data structures, constants, and function prototypes for the drive state
 * management module. The module implements a finite state machine (FSM) to control the vehicle's
 * drive state, handling inputs from ADC, GPIO, and CAN, and producing outputs for DAC, MDI, and CAN.
 *
 * @author  Tony Chen
 * @date    Jan 28, 2026
 */

#ifndef __DRIVE_STATE_H__
#define __DRIVE_STATE_H__

/* INCLUDES */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/* DRIVE STATE DEFINES */
#define MC_DAC_MAX 1023 // Note: This gets capped by MDI to 920 anyways for safety
#define DRIVE_STATE_FSM_DELAY 25
#define REGEN_DAC_ON 1023
#define REGEN_DAC_OFF 0
#define ACCEL_DAC_OFF 0

#define WHEEL_RADIUS 0.283
#define M_PI 3.14159
#define VELOCITY_THRESHOLD 0.5

/* DRIVE STATE DATA TYPES */
typedef struct
{
    volatile bool brake_on;
    volatile bool regen_on;
    volatile bool cruise_on;
    volatile bool velocity_under_threshold;
    volatile bool next_state_request;
    volatile bool prev_state_request;
    volatile bool eco_mode_on;
} DriveStateFlags;

typedef struct
{
    uint16_t accel_DAC_value;
    uint16_t regen_DAC_value;
    uint8_t motor_control_flags;
} DriveStateMotorControl;

typedef enum
{
    INVALID = (uint8_t)0x00,
    FORWARD = (uint8_t)0x01,
    PARK = (uint8_t)0x02,
    REVERSE = (uint8_t)0x03,
    // CRUISE = (uint8_t) 0x04
} DriveStateStates;

typedef struct {
    DriveStateStates state;
    DriveStateFlags flags;
    uint32_t velocity_kmh;
    uint16_t throttle_dac;
} DriveStateModel;

/* GLOBAL VARIABLES */
extern volatile DriveStateModel g_drive_state_model;


/* FUNCTION PROTOTYPES */

/**
 * @brief Main handler for the drive state finite state machine (FSM).
 *
 * Evaluates inputs and updates the drive state and outputs accordingly.
 */
void DriveStateFsmHandler(void);

/**
 * @brief Handles drive state changes triggered by external interrupts.
 *
 * @param toggle Interrupt toggle value.
 */
void DriveStateInterruptHandler(uint16_t toggle);

/**
 * @brief Handles incoming CAN messages related to vehicle velocity.
 *
 * @param data Pointer to CAN message data.
 */
void VelocityCanMsgHandler(uint8_t* data);

/**
 * @brief Handles incoming CAN messages related to steering.
 *
 * @param data Pointer to CAN message data.
 */
void SteeringCanMsgHandler(uint8_t* data);

#ifdef DEBUG
/**
 * @brief Handles CAN messages requesting a drive state change (debug only).
 *
 * @param data Pointer to CAN message data.
 */
void StateRequestCanMsgHandler(uint8_t* data);
#endif

#endif /* __DRIVE_STATE_H_ */