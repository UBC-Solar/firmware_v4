/**
 *  @file  drive_state.c
 *  @brief Handles the drive state for the car. Takes ADC, GPIO and CAN inputs and
 *         outputs DAC values, MDI flags, and CAN data messages
 *
 *  @author Tony Chen
 *  @date Jan 28, 2026
 */

#include "drive_state.h"

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

    // set_cyclic_drive_state(g_drive_state);

    clear_drive_flags(); // why clear
}

drive_state_t compute_next_state(drive_state_t drive_state, drive_flags_t drive_flags)
{

    const bool allow_state_change = drive_flags.brake_on && drive_flags.velocity_under_threshold;

    const int requests = (int)drive_flags.forward_request + (int)drive_flags.reverse_request +
                         (int)drive_flags.park_request;

    if (requests > 1 || !allow_state_change)
        return drive_state;

    switch (drive_state)
    {
    case PARK:
        if (drive_flags.forward_request)
            drive_state = FORWARD;
        if (drive_flags.reverse_request)
            drive_state = REVERSE;
        return drive_state;

    case FORWARD:
        if (drive_flags.park_request)
            drive_state = PARK;
        if (drive_flags.reverse_request)
            drive_state = REVERSE;
        return drive_state;

    case REVERSE:
        if (drive_flags.park_request)
            drive_state = PARK;
        if (drive_flags.forward_request)
            drive_state = FORWARD;
        return drive_state;

    default:
        return PARK;
    }
}

motor_control_t compute_next_command(drive_state_t drive_state, drive_flags_t drive_flags)
{

    if (drive_flags.brake_on || drive_flags.park_request)
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
}

void clear_drive_flags(void)
{
    g_drive_flags.park_request = false;
    g_drive_flags.forward_request = false;
    g_drive_flags.reverse_request = false;
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
        next_state_handler(g_drive_state);
        break;

    case DRIVE_PREV_PIN:
        prev_state_handler(g_drive_state);
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
// PARK -> FORWARD -> REVERSE -> PARK
void next_state_handler(drive_state_t drive_state)
{

    switch (drive_state)
    {
    case PARK:
        g_drive_flags.forward_request = true;
        break;

    case FORWARD:
        g_drive_flags.reverse_request = true;
        break;

    case REVERSE:
        g_drive_flags.park_request = true; // cap at reverse to remove rotation
        break;
    }
}
// PARK -> REVERSE -> FORWARD -> PARK
void prev_state_handler(drive_state_t drive_state)
{

    switch (drive_state)
    {
    case PARK:
        g_drive_flags.reverse_request = true; // cap at park to remove rotation
        break;

    case FORWARD:
        g_drive_flags.park_request = true;
        break;

    case REVERSE:
        g_drive_flags.forward_request = true;
        break;
    }
}

void eco_power_handler(void)
{
    if(!gpio_read_pin(ECO_POWER_PORT, ECO_POWER_PIN)) {
        g_drive_flags.eco_mode_on = false;
    } else {
        g_drive_flags.eco_mode_on = true;
    }
}

/* ACCELERATION READINGS */
void get_acceleration_readings(void)
{
    uint16_t adc1 = adc_read_accel_1();
    uint16_t adc2 = adc_read_accel_2();

    if (!accel_validity(adc1, adc2)) {
        g_throttle_DAC = 0;
        return;
    }

    normalize_adc_values(adc1, adc2);
}

void normalize_adc_values(uint16_t adc1, uint16_t adc2) {

    (void)adc2; // use adc2?

    if (adc1 <= LOWEST_ADC) {
        g_throttle_DAC = 1023;
        return;
    }

    if (adc1 >= HIGHEST_ADC) {
        g_throttle_DAC = 0;
        return;
    }

    uint32_t range = (uint32_t)(HIGHEST_ADC - LOWEST_ADC);
    uint32_t value = (uint32_t)(adc1 - LOWEST_ADC);
    uint32_t scaled = (uint32_t)((value * 1023) / range);

    uint16_t raw_throttle_DAC = (uint16_t)(1023 - scaled);

    g_throttle_DAC = ((raw_throttle_DAC < 0) ? 0 : raw_throttle_DAC);
}

uint16_t convert_to_dac(uint16_t adc) {
    adc = MIN(MAX(adc, ADC_NO_THROTTLE_MAX), ADC_FULL_THROTTLE_MIN);
    return ((adc - ADC_NO_THROTTLE_MAX) * MC_DAC_MAX) / (ADC_FULL_THROTTLE_MIN - ADC_NO_THROTTLE_MAX);
}

bool accel_validity(uint16_t adc1, uint16_t adc2) {
    if ((adc1 < ADC_LOWER_DEADZONE) || (adc1 > ADC_UPPER_DEADZONE) ||
        (adc2 < ADC_LOWER_DEADZONE) || (adc2 > ADC_UPPER_DEADZONE)) {
        return false;
    }

    if (abs(adc1 - adc2) > ADC_MAX_DIFFERENCE) {
        return false;
    }

    return true;
}

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
            g_drive_flags.park_request = true;
        break;
        case 1:
            g_drive_flags.reverse_request = true;
        break;
        case 2:
            g_drive_flags.forward_request = true;
        break;
    }
}
#endif