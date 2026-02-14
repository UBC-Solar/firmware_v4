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

/* FUNCTION DECLARATIONS */
drive_state_t compute_next_state(drive_state_t drive_state, drive_flags_t drive_flags);
motor_control_t compute_next_command(drive_state_t drive_state, drive_flags_t drive_flags);
motor_control_t get_motor_command(uint16_t accel_DAC, uint16_t regen_DAC);
void update_drive_flags(void);
void clear_drive_flags(void);
void break_on_handler(void);
void eco_power_handler(void);

/* GLOBAL VARIABLES */
uint16_t g_throttle_DAC = 0;

volatile drive_state_t g_drive_state = PARK;
volatile drive_flags_t g_drive_flags;

/* DRIVE STATE FINITE STATE MACHINE */

void drive_state_fsm_handler()
{
    update_drive_flags();

    drive_state_t next_drive_state = compute_next_state(g_drive_state, g_drive_flags);
    g_drive_state = next_drive_state;

    motor_control_t motor_command = compute_next_command(g_drive_state, g_drive_flags);

    // TODO: set_cyclic_drive_state(g_drive_state);

    clear_drive_flags();
}

drive_state_t compute_next_state(drive_state_t drive_state, drive_flags_t drive_flags)
{

    const bool allow_state_change = drive_flags.brake_on && drive_flags.velocity_under_threshold;

    const int requests = (int)drive_flags.next_state_request + (int)drive_flags.prev_state_request; // is this needed with a switch?

    if (requests > 1 || !allow_state_change)
        return drive_state;

    /* HANDLE CYCLE LOGIC HERE */

    // PARK -> FORWARD -> REVERSE

    switch (drive_state)
    {
    case PARK:
        if (drive_flags.next_state_request)
            drive_state = FORWARD;
        if (drive_flags.prev_state_request)
            drive_state = PARK;
        return drive_state;

    case FORWARD:
        if (drive_flags.next_state_request)
            drive_state = REVERSE;
        if (drive_flags.prev_state_request)
            drive_state = PARK;
        return drive_state;

    case REVERSE:
        if (drive_flags.next_state_request)
            drive_state = REVERSE;
        if (drive_flags.prev_state_request)
            drive_state = FORWARD;
        return drive_state;

    default:
        return PARK;
    }
}

motor_control_t compute_next_command(drive_state_t drive_state, drive_flags_t drive_flags)
{

    if (drive_flags.brake_on || (drive_state == PARK))
        return get_motor_command(ACCEL_DAC_OFF, REGEN_DAC_OFF);

    if (drive_state == REVERSE)
    {
        return get_motor_command(g_throttle_DAC, REGEN_DAC_OFF);
    }

    return get_motor_command(g_throttle_DAC, drive_flags.regen_on ? REGEN_DAC_ON : REGEN_DAC_OFF);
}

motor_control_t get_motor_command(uint16_t accel_DAC, uint16_t regen_DAC)
{
    motor_control_t motor_command;
    motor_command.accel_DAC_value = accel_DAC;
    motor_command.regen_DAC_value = regen_DAC;

    return motor_command;
}

/* DRIVE STATE DATA COLLECTION */
void update_drive_flags(void)
{
    bool brake_pressed = gpio_read_pin(BRAKE_INPUT_PORT, BRAKE_INPUT_PIN); // adjust
    g_drive_flags.brake_on = brake_pressed;

    g_throttle_DAC = adc_driver_read_throttle();
}

void clear_drive_flags(void)
{
    g_drive_flags.next_state_request = false;
    g_drive_flags.prev_state_request = false;
}

/* SETS DRIVE STATE FLAGS */
void drive_state_interrupt_handler(uint16_t toggle)
{
    gpio_toggle_pin(DEBUG_LED0_PORT, DEBUG_LED0_PIN);

    switch (toggle)
    {
    case BRAKE_INPUT_PIN:
        break_on_handler();
        break;

    case DRIVE_NEXT_PIN:
        g_drive_flags.next_state_request = true;
        break;

    case DRIVE_PREV_PIN:
        g_drive_flags.prev_state_request = true;
        break;

    case ECO_POWER_PIN:
        eco_power_handler();
        break;
    }
}

void break_on_handler() {
    g_drive_flags.brake_on = true;
    motor_control_t motor_command = get_motor_command(ACCEL_DAC_OFF, REGEN_DAC_OFF);
}

void eco_power_handler(void)
{
    if(!gpio_read_pin(ECO_POWER_PORT, ECO_POWER_PIN)) {
        g_drive_flags.eco_mode_on = false;
    } else {
        g_drive_flags.eco_mode_on = true;
    }
}
 
/* CAN MESSAGE HANDLERS */

void velocity_handler(uint8_t* data) {
    uint32_t rpm = (data[4] >> 3) | ((data[5] & 0x7f) << 5);
    float velocity = (WHEEL_RADIUS * 2.0f * M_PI * rpm) / 60.0f;
    g_velocity_kmh = velocity * 3.6f;

    if (velocity < VELOCITY_THRESHOLD) {
        g_drive_flags.velocity_under_threshold = true;
    } else {
        g_drive_flags.velocity_under_threshold = false;
    }
}

void steering_can_msg_handler(uint8_t *data) { // not configured on STR yet regen is for bit 0 and cruise for bit 1

    g_drive_flags.regen_on = ((data[0] >> 0) & 0x01);
    g_drive_flags.cruise_on = ((data[0] >> 1) & 0x01);
}

#ifdef DEBUG
void state_request_can_msg_handler(uint8_t* data) {
    int value = data[0];

    switch (value) {
        case 0:
            g_drive_flags.next_state_request = true;
        break;
        case 1:
            g_drive_flags.prev_state_request = true;
        break;
    }
}
#endif