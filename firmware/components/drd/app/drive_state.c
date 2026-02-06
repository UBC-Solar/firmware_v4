/**
 *  @file drive_state.c
 *  @brief Handles the drive state for the car. Takes ADC, GPIO and CAN inputs and
 *         outputs DAC values, MDI flags, and CAN data messages
 * 
 *  @author Tony Chen
 *  @date Jan 28, 2026
 */

#include "drive_state.h"

/* DEFINES */

/* GLOBAL VARIABLES */
uint16_t g_throttle_DAC = 0;

volatile drive_state_t g_drive_state = PARK;
volatile drive_flags_t g_drive_flags;

/* DRIVE STATE FINITE STATE MACHINE */

void drive_state_fsm_handler() {

    update_drive_flags();

    drive_state_t next_drive_state = compute_next_state(g_drive_state, g_drive_flags);
    g_drive_state = next_drive_state;

    motor_control_t motor_command = compute_next_command(g_drive_state, g_drive_flags);

    // set_cyclic_drive_state(g_drive_state);

    clear_drive_flags(g_drive_flags);
}

drive_state_t compute_next_state(drive_state_t drive_state, drive_flags_t drive_flags) {

    const bool allow_state_change = drive_flags.brake_on && drive_flags.velocity_under_threshold;

    const int requests = (int)drive_flags.forward_request + (int)drive_flags.reverse_request + (int)drive_flags.park_request;

    if (requests > 1 || !allow_state_change) return drive_state;

    switch (drive_state) {
        case PARK:
            if (drive_flags.forward_request) drive_state = FORWARD;
            if (drive_flags.reverse_request) drive_state = REVERSE;
        return drive_state;

        case FORWARD:
            if (drive_flags.park_request) drive_state = PARK;
            if (drive_flags.reverse_request) drive_state = REVERSE;
        return drive_state;

        case REVERSE:
            if (drive_flags.park_request) drive_state = PARK;
            if (drive_flags.forward_request) drive_state = FORWARD;
        return drive_state;

        default:
            return PARK;
    }
}

motor_control_t compute_next_command(drive_state_t drive_state, drive_flags_t drive_flags) {

    if (drive_flags.brake_on || drive_flags.park_request) return get_motor_command(ACCEL_DAC_OFF, REGEN_DAC_OFF);

    if (drive_state == REVERSE) {
        return get_motor_command(g_throttle_DAC, REGEN_DAC_OFF);
    }

    return get_motor_command(g_throttle_DAC, drive_flags.regen_on ? REGEN_DAC_ON : REGEN_DAC_OFF);
}

motor_control_t get_motor_command() {
    // do later
}

/* DRIVE STATE DATA COLLECTION */

void update_drive_flags(void) {

}

void clear_drive_flags(drive_flags_t drive_flags) {
    g_drive_flags.park_request = false;
    g_drive_flags.forward_request = false;
    g_drive_flags.reverse_request = false;
}