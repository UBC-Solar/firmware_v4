/**
 * @file    lcd_app.c
 * @brief   LCD application logic for the UBC Solar DRD board
 *
 * This file contains the implementation of the LCD application logic for the 5 current pages
 * of the DRD board. It handles updating the LCD display with vehicle state information, faults,
 * warnings, and other relevant data. It also manages page changes and the formatting of displayed
 * text fields.
 *
 * @author  Gregory Bian
 * @date    Feb 4 2026
 */

#include "lcd_app.h"
#include <stdint.h>
#include <string.h>
#include "cyclic_data_handler.h"
#include "lcd_driver.h"
#include <stdio.h>
// #include "soc.h"

/*--------------------------------------------------------------------------
  Internal Types & Variables
--------------------------------------------------------------------------*/

/* Static variables to store old bounding boxes for updating text fields */
static LcdDriverBoundingBox old_bb_speed = {0, 0, 0, 0};
static LcdDriverBoundingBox old_bb_drive_state = {0, 0, 0, 0};
static LcdDriverBoundingBox old_bb_drive_mode = {0, 0, 0, 0};
static LcdDriverBoundingBox old_bb_soc = {0, 0, 0, 0};
static LcdDriverBoundingBox old_bb_fault_indicator = {0, 0, 0, 0};
static LcdDriverBoundingBox old_bb_warning_indicator = {0, 0, 0, 0};

static char g_faults[8][10] = {0};
static char g_warning_char[8][10] = {0};
static uint8_t g_prev_fault_count = 0;
static uint8_t g_prev_warning_count = 0;

/*--------------------------------------------------------------------------
  PAGE 1 (DRIVE PAGE) FUNCTIONS
--------------------------------------------------------------------------*/

/**
 * @brief Displays the speed on the LCD drive page.
 *
 * @param speed The speed value to display.
 * @param units The speed units (LCD_SPEED_UNITS_MPH or LCD_SPEED_UNITS_KPH).
 */
void LcdAppDisplaySpeedDrivePage(volatile uint32_t* speed, volatile uint8_t units)
{
    char speed_str[12];

    /* Clear the previous speed and unit areas */
    LcdDriverClearBoundingBox(LCD_APP_SPEED_THREEDIGIT_X, old_bb_speed.y1 + 10, LCD_DRIVER_BOTTOM_RIGHT_X, old_bb_speed.y2);

    if (speed == NULL)
    { // Stale speed data
        sprintf(speed_str, "XX");
        old_bb_speed = LcdDriverDrawText(speed_str, LCD_APP_SPEED_TWODIGIT_X + 10, LCD_APP_SPEED_Y + 10, LCD_APP_SPEED_NULL_FONT, LCD_APP_SPEED_SPACING + 10);
    }
    else if (*speed < 10)
    { // Single digit speed
        sprintf(speed_str, "%01lu", (unsigned long)*speed);
        old_bb_speed = LcdDriverDrawText(speed_str, LCD_APP_SPEED_ONEDIGIT_X, LCD_APP_SPEED_Y, LCD_APP_SPEED_FONT, LCD_APP_SPEED_SPACING);
    }
    else if (*speed < 100)
    { // Double digit second
        sprintf(speed_str, "%02lu", (unsigned long)*speed);
        old_bb_speed = LcdDriverDrawText(speed_str, LCD_APP_SPEED_TWODIGIT_X, LCD_APP_SPEED_Y, LCD_APP_SPEED_FONT, LCD_APP_SPEED_SPACING);
    }
    else
    {
        sprintf(speed_str, "%03lu", (unsigned long)*speed);
        old_bb_speed = LcdDriverDrawText(speed_str, LCD_APP_SPEED_THREEDIGIT_X, LCD_APP_SPEED_Y, LCD_APP_SPEED_FONT, LCD_APP_SPEED_SPACING);
    }

    /* Draw the speed units */
    switch (units)
    {
    case LCD_APP_KPH:
        LcdDriverDrawText("kph", LCD_APP_SPEED_X + LCD_APP_SPEED_UNIT_KPH_X, LCD_APP_SPEED_UNIT_Y, LCD_APP_SPEED_UNITS_FONT, LCD_APP_SPEED_UNITS_SPACING);
        break;
    case LCD_APP_MPH:
        LcdDriverDrawText("mph", LCD_APP_SPEED_X + LCD_APP_SPEED_UNIT_MPH_X, LCD_APP_SPEED_UNIT_Y, LCD_APP_SPEED_UNITS_FONT, LCD_APP_SPEED_UNITS_SPACING);
        break;
    default:
        LcdDriverDrawText("xxx", LCD_APP_SPEED_X + LCD_APP_SPEED_UNIT_MPH_X, LCD_APP_SPEED_UNIT_Y, LCD_APP_SPEED_UNITS_FONT, LCD_APP_SPEED_UNITS_SPACING);
        break;
    }

    LcdDriverRefresh();
}

/**
 * @brief Displays the state of charge (SOC) on the LCD drive page.
 *
 * @param soc The state of charge (in percent).
 */
void LcdAppDisplaySocDrivePage(volatile uint32_t* soc)
{
    char soc_str[12];

    LcdDriverClearBoundingBox(
        old_bb_soc.x1, old_bb_soc.y1, old_bb_soc.x2, old_bb_soc.y2 - LCD_APP_SOC_SPACING);

    // Check for stale data and display "--" if so.
    if (soc == NULL)
    {
        sprintf(soc_str, "--");
        old_bb_soc = LcdDriverDrawText(soc_str, LCD_APP_SOC_TWODIGIT_X, LCD_APP_SOC_Y, LCD_APP_SOC_FONT, LCD_APP_SOC_SPACING);
    }
    else if (*soc < 10)
    {
        sprintf(soc_str, "%01lu", (unsigned long)*soc);
        old_bb_soc = LcdDriverDrawText(soc_str, LCD_APP_SOC_ONEDIGIT_X, LCD_APP_SOC_Y, LCD_APP_SOC_FONT, LCD_APP_SOC_SPACING);
    }
    else if (*soc < 100)
    {
        sprintf(soc_str, "%02lu", (unsigned long)*soc);
        old_bb_soc = LcdDriverDrawText(soc_str, LCD_APP_SOC_TWODIGIT_X, LCD_APP_SOC_Y, LCD_APP_SOC_FONT, LCD_APP_SOC_SPACING);
    }
    else
    {
        sprintf(soc_str, "%03lu", (unsigned long)*soc);
        old_bb_soc = LcdDriverDrawText(soc_str, LCD_APP_SOC_THREEDIGIT_X, LCD_APP_SOC_Y, LCD_APP_SOC_FONT, LCD_APP_SOC_SPACING);
    }

    LcdDriverDrawChar(LCD_APP_SOC_UNITS, LCD_APP_SOC_UNITS_X, LCD_APP_SOC_Y, LCD_APP_SOC_UNITS_FONT);

    LcdDriverRefresh();
}

/**
 * @brief Displays an E for ECO mode and P for POWER mode
 *
 * @param drive_mode The drive mode
 */
void LcdAppDisplayDriveModeDrivePage(volatile uint8_t drive_mode)
{
    LcdDriverClearBoundingBox(old_bb_drive_mode.x1, old_bb_drive_mode.y1 + 4, old_bb_drive_mode.x2, old_bb_drive_mode.y2 - 2);

    // Drive mode is valid, display the corresponding symbol.
    switch (drive_mode)
    {
    case LCD_APP_DRIVE_MODE_ECO:
        old_bb_drive_mode = LcdDriverDrawChar(LCD_APP_ECO_SYMBOL, LCD_APP_ECO_MODE_X, LCD_APP_ECO_MODE_Y, LCD_APP_ECO_MODE_FONT);
        break;
    case LCD_APP_DRIVE_MODE_POWER:
        old_bb_drive_mode = LcdDriverDrawChar(LCD_APP_POWER_SYMBOL, LCD_APP_POWER_MODE_X, LCD_APP_POWER_MODE_Y, LCD_APP_POWER_MODE_FONT);
        break;
    default:
        old_bb_drive_mode = LcdDriverDrawChar(LCD_APP_ERROR_SYMBOL, LCD_APP_ECO_MODE_X, LCD_APP_ECO_MODE_Y, LCD_APP_ECO_MODE_FONT);
        break;
    }
    LcdDriverRefresh();
}

/**
 * @brief Displays the drive state on the LCD drive page.
 *
 * @param state The drive state (e.g., FORWARD_STATE, PARK_STATE, REVERSE_STATE).
 */
void LcdAppDisplayDriveStateDrivePage(volatile DriveStateStates* state)
{
    char state_str[2] = {LCD_APP_ERROR_SYMBOL, '\0'}; // Default to error symbol.

    if (state == NULL)
    { // Stale data for drive state
        sprintf(state_str, "-");
    }
    else
    {
        switch (*state)
        {
        case FORWARD:
            state_str[0] = LCD_APP_FORWARD_SYMBOL;
            break;
        case PARK:
            state_str[0] = LCD_APP_PARK_SYMBOL;
            break;
        case REVERSE:
            state_str[0] = LCD_APP_REVERSE_SYMBOL;
            break;
        default:
            state_str[0] = LCD_APP_ERROR_SYMBOL;
            break;
        }
    }

    LcdDriverClearBoundingBox(
        LCD_APP_STATE_X, LCD_APP_STATE_Y, old_bb_drive_state.x2, LCD_DRIVER_BOTTOM_RIGHT_Y);
    old_bb_drive_state = LcdDriverDrawText(state_str, LCD_APP_STATE_X, LCD_APP_STATE_Y, LCD_APP_STATE_FONT, LCD_APP_STATE_SPACING);

    LcdDriverRefresh();
}

/**
 * @brief Displays a fault indicator on the LCD Drive Page
 *
 * @param fault_indicator A general indicator to signal a fault to prompt the driver to change pages
 */
void LcdAppDisplayFaultIndicator(LcdAppBattFaults* batt_faults, LcdAppMotorFaults* motor_faults)
{
    LcdDriverClearBoundingBox(old_bb_fault_indicator.x1,old_bb_fault_indicator.y1,old_bb_fault_indicator.x2,old_bb_fault_indicator.y2);

    uint8_t fault_count = LcdAppCheckFaults(batt_faults, motor_faults);

    // Check if there is an existing fault
    if (fault_count > 0)
    {
        old_bb_fault_indicator = LcdDriverDrawChar(LCD_APP_FAULT_SYMBOL, LCD_APP_FAULT_X, LCD_APP_FAULT_Y, LCD_APP_FAULT_SYMBOL_FONT);
    }
    LcdDriverRefresh();
}

/**
 * @brief Displays a warning indicator on the LCD Drive Page
 *
 * @param warning_indicator A general indicator to signal a warning to prompt the driver to change
 * pages
 */
void LcdAppDisplayWarningIndicator(LcdAppWarnings* warnings)
{
    LcdDriverClearBoundingBox(old_bb_warning_indicator.x1,old_bb_warning_indicator.y1,old_bb_warning_indicator.x2,old_bb_warning_indicator.y2);

    uint8_t warning_count = LcdAppCheckWarnings(warnings);

    // Check if there is an existing warning
    if (warning_count > 0)
    {
        old_bb_warning_indicator = LcdDriverDrawChar(LCD_APP_WARNING_SYMBOL, LCD_APP_WARNING_X, LCD_APP_WARNING_Y, LCD_APP_WARNING_SYMBOL_FONT);
    }
    LcdDriverRefresh();
}

/*--------------------------------------------------------------------------
  PAGE 2 (FAULT PAGE) FUNCTIONS
--------------------------------------------------------------------------*/

uint8_t LcdAppCheckFaults(LcdAppBattFaults* batt_faults, LcdAppMotorFaults* motor_faults)
{
    // This clears the faults so we parse faults every time
    memset(g_faults, 0, sizeof(g_faults));  

    // Stores amount of fault
    uint8_t fault_count = 0;

    if (batt_faults->battery_fault)
    {
        sprintf(g_faults[fault_count], "%s", LCD_APP_BATT_FLT_CHARS);
        fault_count++;
    }
    if(batt_faults->selftest_fault)
    {
        sprintf(g_faults[fault_count], "%s", LCD_APP_BATT_SELFTEST_FLT_CHARS);
        fault_count++;
    }
    if (batt_faults->supp_lo)
    {
        sprintf(g_faults[fault_count], "%s", LCD_APP_BATT_SUPPLO_FLT_CHARS);
        fault_count++;
    }
    if (batt_faults->voltage_high)
    {
        sprintf(g_faults[fault_count], "%s", LCD_APP_BATT_VOLTHIGH_FLT_CHARS);
        fault_count++;
    }
    if (batt_faults->voltage_low)
    {
        sprintf(g_faults[fault_count], "%s", LCD_APP_BATT_VOLTLOW_FLT_CHARS);
        fault_count++;
    }
    if (batt_faults->slave_board_comm_fault)
    {
        sprintf(g_faults[fault_count], "%s", LCD_APP_BATT_SLAVE_COMM_FLT_CHARS);
        fault_count++;
    }
    if (batt_faults->overvolt_fault)
    {
        sprintf(g_faults[fault_count], "%s", LCD_APP_BATT_OVERVOLT_FLT_CHARS);
        fault_count++;
    }
    if (batt_faults->undervolt_fault)
    {
        sprintf(g_faults[fault_count], "%s", LCD_APP_BATT_UNDERVOLT_FLT_CHARS);
        fault_count++;
    }
    if (batt_faults->overtemp_fault)
    {
        sprintf(g_faults[fault_count], "%s", LCD_APP_BATT_OVERTEMP_FLT_CHARS);
        fault_count++;
    }
    if (batt_faults->charge_overcurrent_fault)
    {
        sprintf(g_faults[fault_count], "%s", LCD_APP_BATT_CHARGE_OC_FLT_CHARS);
        fault_count++;
    }
    if (batt_faults->discharge_overcurrent_fault)
    {
        sprintf(g_faults[fault_count], "%s", LCD_APP_BATT_DISCHARGE_OC_FLT_CHARS);
        fault_count++;
    }
    if (batt_faults->reset_from_watchdog)
    {
        sprintf(g_faults[fault_count], "%s", LCD_APP_BATT_RST_FROM_WATCH_FLT_CHARS);
        fault_count++;
    }
    if (motor_faults->motor_system_error)
    {
        sprintf(g_faults[fault_count], "%s", LCD_APP_MTR_SYSTEM_FLT_CHARS);
        fault_count++;
    }
    if (motor_faults->overcurrent_fault)
    {
        sprintf(g_faults[fault_count], "%s", LCD_APP_MTR_OVERCURR_FLT_CHARS);
        fault_count++;
    }
    if (motor_faults->overvoltage_fault)
    {
        sprintf(g_faults[fault_count], "%s", LCD_APP_MTR_OVERVOLT_FLT_CHARS);
        fault_count++;
    }
    if (motor_faults->fet_thermistor_error)
    {
        sprintf(g_faults[fault_count], "%s", LCD_APP_MTR_OVERTEMP_FLT_CHARS);
        fault_count++;
    }
    if (motor_faults->motor_comm_fault)
    {
        sprintf(g_faults[fault_count], "%s", LCD_APP_MTR_COMM_FLT_CHARS);
        fault_count++;
    }
    if (motor_faults->throttle_adc_outofrange)
    {
        sprintf(g_faults[fault_count], "%s", LCD_APP_MTR_THROT_ADC_OOR_FLT_CHARS);
        fault_count++;
    }
    if (motor_faults->throttle_adc_mismatch)
    {
        sprintf(g_faults[fault_count], "%s", LCD_APP_MTR_THROT_ADC_MISMATCH_FLT_CHARS);
        fault_count++;
    }

    return fault_count;
}

/**
 * @brief Dynamically displays battery and motor faults on the LCD
 *
 * @param batt_faults The battery faults to be displayed on the LCD
 * @param motor_faults The motor faults to be displayed on the LCD
 */
void LcdAppDisplayFaults(LcdAppBattFaults* batt_faults, LcdAppMotorFaults* motor_faults)
{
    uint8_t fault_count = LcdAppCheckFaults(batt_faults, motor_faults);

    // This forces the screen to clear given no faults (clear doesn't update dirty pages)
    if(fault_count == 0 || fault_count < g_prev_fault_count){
        LcdDriverForceClearBoundingBox(0, LCD_APP_FAULT_FOUR_Y1, LCD_DRIVER_BOTTOM_RIGHT_X, LCD_DRIVER_BOTTOM_RIGHT_Y);
    } else {
        // clear normally
        LcdDriverClearBoundingBox(0, LCD_APP_FAULT_FOUR_Y1, LCD_DRIVER_BOTTOM_RIGHT_X, LCD_DRIVER_BOTTOM_RIGHT_Y);
    }

    g_prev_fault_count = fault_count;

    LcdDriverDrawText(LCD_APP_FAULT_LABEL_CHARS, LCD_APP_FAULT_LABEL_X, LCD_APP_FAULT_LABEL_Y, LCD_APP_FAULT_LABEL_FONT, LCD_APP_FAULT_SPACING);
    for (uint8_t i = 0; i < LCD_APP_FAULT_LABEL_UNDERLINE_X; i++)
    {
        LcdDriverSetPixel(i, LCD_APP_FAULT_LABEL_UNDERLINE_Y, 1);
    }
    if (fault_count <= 3)
    {
        LcdDriverDrawText(g_faults[0], LCD_APP_FAULT_FOUR_X1, LCD_APP_FAULT_FOUR_Y1, LCD_APP_FAULT_FOUR_FONT, LCD_APP_FAULT_SPACING);
        LcdDriverDrawText(g_faults[1], LCD_APP_FAULT_FOUR_X2, LCD_APP_FAULT_FOUR_Y2, LCD_APP_FAULT_FOUR_FONT, LCD_APP_FAULT_SPACING);
        LcdDriverDrawText(g_faults[2], LCD_APP_FAULT_FOUR_X3, LCD_APP_FAULT_FOUR_Y3, LCD_APP_FAULT_FOUR_FONT, LCD_APP_FAULT_SPACING);
    }
    else if (fault_count <= 8)
    {
        LcdDriverDrawText(g_faults[0], LCD_APP_FAULT_EIGHT_X1, LCD_APP_FAULT_EIGHT_Y1, LCD_APP_FAULT_EIGHT_FONT, LCD_APP_FAULT_SPACING);
        LcdDriverDrawText(g_faults[1], LCD_APP_FAULT_EIGHT_X2, LCD_APP_FAULT_EIGHT_Y2, LCD_APP_FAULT_EIGHT_FONT, LCD_APP_FAULT_SPACING);
        LcdDriverDrawText(g_faults[2], LCD_APP_FAULT_EIGHT_X3, LCD_APP_FAULT_EIGHT_Y3, LCD_APP_FAULT_EIGHT_FONT, LCD_APP_FAULT_SPACING);
        LcdDriverDrawText(g_faults[3], LCD_APP_FAULT_EIGHT_X4, LCD_APP_FAULT_EIGHT_Y4, LCD_APP_FAULT_EIGHT_FONT, LCD_APP_FAULT_SPACING);
        LcdDriverDrawText(g_faults[4], LCD_APP_FAULT_EIGHT_X5, LCD_APP_FAULT_EIGHT_Y5, LCD_APP_FAULT_EIGHT_FONT, LCD_APP_FAULT_SPACING);
        LcdDriverDrawText(g_faults[5], LCD_APP_FAULT_EIGHT_X6, LCD_APP_FAULT_EIGHT_Y6, LCD_APP_FAULT_EIGHT_FONT, LCD_APP_FAULT_SPACING);
        LcdDriverDrawText(g_faults[6], LCD_APP_FAULT_EIGHT_X7, LCD_APP_FAULT_EIGHT_Y7, LCD_APP_FAULT_EIGHT_FONT, LCD_APP_FAULT_SPACING);
        LcdDriverDrawText(g_faults[7], LCD_APP_FAULT_EIGHT_X8, LCD_APP_FAULT_EIGHT_Y8, LCD_APP_FAULT_EIGHT_FONT, LCD_APP_FAULT_SPACING);
    }

    LcdDriverRefresh();
}

/*--------------------------------------------------------------------------
  PAGE 3 (WARNING PAGE) FUNCTIONS
--------------------------------------------------------------------------*/

uint8_t LcdAppCheckWarnings(LcdAppWarnings* warnings)
{
    // This clears the faults so we parse faults every time
    memset(g_warning_char, 0, sizeof(g_warning_char));

    // Tracks warnings
    uint8_t warning_count = 0;

    if (warnings->high_temp_warning)
    {
        sprintf(g_warning_char[warning_count], "%s", LCD_APP_HIGHTEMP_WARN_CHARS);
        warning_count++;
    }
    if (warnings->high_volt_warning)
    {
        sprintf(g_warning_char[warning_count], "%s", LCD_APP_HIGHVOLT_WARN_CHARS);
        warning_count++;
    }
    if (warnings->low_temp_warning)
    {
        sprintf(g_warning_char[warning_count], "%s", LCD_APP_LOWTEMP_WARN_CHARS);
        warning_count++;
    }
    if (warnings->low_volt_warning)
    {
        sprintf(g_warning_char[warning_count], "%s", LCD_APP_LOWVOLT_WARN_CHARS);
        warning_count++;
    }
    if (warnings->no_ecu_message)
    {
        sprintf(g_warning_char[warning_count], "%s", LCD_APP_NOMSG_WARN_CHARS);
        warning_count++;
    }
    if (warnings->pack_overcharge)
    {
        sprintf(g_warning_char[warning_count], "%s", LCD_APP_PACK_OC_WARN_CHARS);
        warning_count++;
    }
    if (warnings->pack_overdischarge)
    {
        sprintf(g_warning_char[warning_count], "%s", LCD_APP_PACK_OD_WARN_CHARS);
        warning_count++;
    }

    return warning_count;
}


void LcdAppDisplayWarnings(LcdAppWarnings* warnings)
{
    uint8_t warning_count = LcdAppCheckWarnings(warnings);

    // This forces the screen to clear given no warnings (clear doesn't update dirty pages)
    if(warning_count == 0 || g_prev_warning_count < warning_count){
        LcdDriverForceClearBoundingBox( 0, LCD_APP_WARNING_FOUR_Y1, LCD_DRIVER_BOTTOM_RIGHT_X, LCD_DRIVER_BOTTOM_RIGHT_Y);
    } else {
        // clear normally
        LcdDriverClearBoundingBox( 0, LCD_APP_WARNING_FOUR_Y1, LCD_DRIVER_BOTTOM_RIGHT_X, LCD_DRIVER_BOTTOM_RIGHT_Y);
    }

    g_prev_warning_count = warning_count;

    LcdDriverDrawText(LCD_APP_WARNING_LABEL_CHARS, LCD_APP_WARNING_LABEL_X, LCD_APP_WARNING_LABEL_Y, LCD_APP_WARNING_LABEL_FONT, LCD_APP_WARNING_SPACING);
    for (uint8_t i = 0; i < LCD_APP_WARNING_LABEL_UNDERLINE_X; i++)
    {
        LcdDriverSetPixel(i, LCD_APP_WARNING_LABEL_UNDERLINE_Y, 1);
    }
    if (warning_count <= 3)
    {
        LcdDriverDrawText(g_warning_char[0], LCD_APP_WARNING_FOUR_X1, LCD_APP_WARNING_FOUR_Y1, LCD_APP_WARNING_FOUR_FONT, LCD_APP_WARNING_SPACING);
        LcdDriverDrawText(g_warning_char[1], LCD_APP_WARNING_FOUR_X2, LCD_APP_WARNING_FOUR_Y2, LCD_APP_WARNING_FOUR_FONT, LCD_APP_WARNING_SPACING);
        LcdDriverDrawText(g_warning_char[2], LCD_APP_WARNING_FOUR_X3, LCD_APP_WARNING_FOUR_Y3, LCD_APP_WARNING_FOUR_FONT, LCD_APP_WARNING_SPACING);
    }
    else if (warning_count <= 8)
    {
        LcdDriverDrawText(g_warning_char[0], LCD_APP_WARNING_EIGHT_X1, LCD_APP_WARNING_EIGHT_Y1, LCD_APP_WARNING_EIGHT_FONT, LCD_APP_WARNING_SPACING);
        LcdDriverDrawText(g_warning_char[1], LCD_APP_WARNING_EIGHT_X2, LCD_APP_WARNING_EIGHT_Y2, LCD_APP_WARNING_EIGHT_FONT, LCD_APP_WARNING_SPACING);
        LcdDriverDrawText(g_warning_char[2], LCD_APP_WARNING_EIGHT_X3, LCD_APP_WARNING_EIGHT_Y3, LCD_APP_WARNING_EIGHT_FONT, LCD_APP_WARNING_SPACING);
        LcdDriverDrawText(g_warning_char[3], LCD_APP_WARNING_EIGHT_X4, LCD_APP_WARNING_EIGHT_Y4, LCD_APP_WARNING_EIGHT_FONT, LCD_APP_WARNING_SPACING);
        LcdDriverDrawText(g_warning_char[4], LCD_APP_WARNING_EIGHT_X5, LCD_APP_WARNING_EIGHT_Y5, LCD_APP_WARNING_EIGHT_FONT, LCD_APP_WARNING_SPACING);
        LcdDriverDrawText(g_warning_char[5], LCD_APP_WARNING_EIGHT_X6, LCD_APP_WARNING_EIGHT_Y6, LCD_APP_WARNING_EIGHT_FONT, LCD_APP_WARNING_SPACING);
        LcdDriverDrawText(g_warning_char[6], LCD_APP_WARNING_EIGHT_X7, LCD_APP_WARNING_EIGHT_Y7, LCD_APP_WARNING_EIGHT_FONT, LCD_APP_WARNING_SPACING);
        LcdDriverDrawText(g_warning_char[7], LCD_APP_WARNING_EIGHT_X8, LCD_APP_WARNING_EIGHT_Y8, LCD_APP_WARNING_EIGHT_FONT, LCD_APP_WARNING_SPACING);
    }

    LcdDriverRefresh();
}

/*--------------------------------------------------------------------------
  PAGE 4 (TEMPERATURE PAGE) FUNCTIONS
--------------------------------------------------------------------------*/

void LcdAppDisplayTemperature(LcdAppTemperature temperature_data)
{
    // Stores a Bounding Box used for changing temp symbol position
    LcdDriverBoundingBox bb = {0};

    // Variables to hold values based on what temperature is being displayed
    char temp_label[8];
    char temp_str[4];
    uint8_t temp_x;
    uint8_t temp_y;
    uint8_t temp_shift;

    // Set variables based on what temperature is displayed and clear appropriate area of screen
    switch (temperature_data.temp_label)
    {
    case MPPTA:
        sprintf(temp_label, "%s", LCD_APP_MPPT_A_CHARS);
        temp_x = LCD_APP_MPPT_A_X;
        temp_y = LCD_APP_MPPT_A_Y;
        temp_shift = LCD_APP_TEMP_MPPT_OFFSET;
        LcdDriverClearBoundingBox(
            LCD_APP_TEMP_MPPT_OFFSET, LCD_APP_MPPT_A_Y, LCD_APP_BATT_MAX_X - 2, LCD_APP_MPPT_B_Y);
        break;
    case MPPTB:
        sprintf(temp_label, "%s", LCD_APP_MPPT_B_CHARS);
        temp_x = LCD_APP_MPPT_B_X;
        temp_y = LCD_APP_MPPT_B_Y;
        temp_shift = LCD_APP_TEMP_MPPT_OFFSET;
        LcdDriverClearBoundingBox(
            LCD_APP_TEMP_MPPT_OFFSET, LCD_APP_MPPT_B_Y, LCD_APP_BATT_MAX_X - 2, LCD_APP_MPPT_C_Y);
        break;
    case MPPTC:
        sprintf(temp_label, "%s", LCD_APP_MPPT_C_CHARS);
        temp_x = LCD_APP_MPPT_C_X;
        temp_y = LCD_APP_MPPT_C_Y;
        temp_shift = LCD_APP_TEMP_MPPT_OFFSET;
        LcdDriverClearBoundingBox(
            LCD_APP_TEMP_MPPT_OFFSET, LCD_APP_MPPT_C_Y, LCD_APP_BATT_MAX_X - 2, LCD_APP_MPPT_D_Y);
        break;
    case MPPTD:
        sprintf(temp_label, "%s", LCD_APP_MPPT_D_CHARS);
        temp_x = LCD_APP_MPPT_D_X;
        temp_y = LCD_APP_MPPT_D_Y;
        temp_shift = LCD_APP_TEMP_MPPT_OFFSET;
        LcdDriverClearBoundingBox(LCD_APP_TEMP_MPPT_OFFSET, LCD_APP_MPPT_D_Y, LCD_APP_BATT_MAX_X - 2, LCD_DRIVER_BOTTOM_RIGHT_Y);
        break;
    case BATT_MAX:
        sprintf(temp_label, "%s", LCD_APP_BATT_MAX_CHARS);
        temp_x = LCD_APP_BATT_MAX_X;
        temp_y = LCD_APP_BATT_MAX_Y;
        temp_shift = LCD_APP_TEMP_BATT_OFFSET;
        LcdDriverClearBoundingBox(LCD_APP_BATT_MAX_X + LCD_APP_TEMP_BATT_OFFSET, 0, LCD_DRIVER_BOTTOM_RIGHT_X, LCD_APP_BATT_MIN_Y);
        break;
    case BATT_MIN:
        sprintf(temp_label, "%s", LCD_APP_BATT_MIN_CHARS);
        temp_x = LCD_APP_BATT_MIN_X;
        temp_y = LCD_APP_BATT_MIN_Y;
        temp_shift = LCD_APP_TEMP_BATT_OFFSET;
        LcdDriverClearBoundingBox(LCD_APP_BATT_MAX_X + LCD_APP_TEMP_BATT_OFFSET,LCD_APP_BATT_MIN_Y, LCD_DRIVER_BOTTOM_RIGHT_X, LCD_APP_MTR_CONT_Y);
        break;
    case MOTOR_CONT:
        sprintf(temp_label, "%s", LCD_APP_MTR_CONT_CHARS);
        temp_x = LCD_APP_MTR_CONT_X;
        temp_y = LCD_APP_MTR_CONT_Y;
        temp_shift = LCD_APP_TEMP_MTR_OFFSET;
        LcdDriverClearBoundingBox(LCD_APP_MTR_CONT_X + LCD_APP_TEMP_MTR_OFFSET,LCD_APP_MTR_CONT_Y,LCD_DRIVER_BOTTOM_RIGHT_X,LCD_APP_MTR_THERM_Y);
        break;
    case MOTOR_THERM:
        sprintf(temp_label, "%s", LCD_APP_MTR_THERM_CHARS);
        temp_x = LCD_APP_MTR_THERM_X;
        temp_y = LCD_APP_MTR_THERM_Y;
        temp_shift = LCD_APP_TEMP_MTR_OFFSET;
        LcdDriverClearBoundingBox(LCD_APP_MTR_CONT_X + LCD_APP_TEMP_MTR_OFFSET,LCD_APP_MTR_THERM_Y,LCD_DRIVER_BOTTOM_RIGHT_X,LCD_DRIVER_BOTTOM_RIGHT_Y);
        break;
    default:
        return;
    }

    LcdDriverDrawText(temp_label, temp_x, temp_y, LCD_APP_TEMP_LABEL_FONT, LCD_APP_TEMP_SPACING);

    // Check digits in temperature data received
    if (temperature_data.temperature == NULL)
    { // temperature not read
        sprintf(temp_str, "--");
        bb = LcdDriverDrawText(temp_str, temp_x + temp_shift, temp_y, LCD_APP_TEMP_FONT, LCD_APP_TEMP_SPACING);
    }
    else if (*temperature_data.temperature < 10)
    { // Single digit temperature
        sprintf(temp_str, "%01lu", (unsigned long)*temperature_data.temperature);
        bb = LcdDriverDrawText(temp_str, temp_x + temp_shift, temp_y, LCD_APP_TEMP_FONT, LCD_APP_TEMP_SPACING);
    }
    else if (*temperature_data.temperature < 100)
    { // Double digit temperature
        sprintf(temp_str, "%02lu", (unsigned long)*temperature_data.temperature);
        bb = LcdDriverDrawText(temp_str, temp_x + temp_shift, temp_y, LCD_APP_TEMP_FONT, LCD_APP_TEMP_SPACING);
    }
    else
    { // Triple digit
        sprintf(temp_str, "%03lu", (unsigned long)*temperature_data.temperature);
        bb = LcdDriverDrawText(temp_str, temp_x + temp_shift, temp_y, LCD_APP_TEMP_FONT, LCD_APP_TEMP_SPACING);
    }

    // Draws the Degrees Celsius symbol according to the position of the bounding box
    LcdDriverDrawChar(LCD_APP_TEMP_DEGREES_SYMBOL, bb.x2 + LCD_APP_TEMP_DEGREES_OFFSET_X, temp_y - LCD_APP_TEMP_DEGREES_OFFSET_Y, LCD_APP_TEMP_DEGREES_FONT);
    LcdDriverDrawChar(LCD_APP_TEMP_UNITS, bb.x2 + LCD_APP_TEMP_UNITS_OFFSET, temp_y, LCD_APP_TEMP_UNITS_FONT);

    LcdDriverRefresh();
}

/*--------------------------------------------------------------------------
  PAGE 5 (DEBUG PAGE) FUNCTIONS
--------------------------------------------------------------------------*/

void LcdAppDisplayPowerBar(volatile int16_t* pack_current, volatile uint16_t* pack_voltage)
{
    /* Clear the drawing area (including extra space for the center line) */
    LcdDriverClearBoundingBox(
        LCD_APP_BAR_LEFT, LCD_APP_BAR_TOP, LCD_APP_BAR_RIGHT, LCD_APP_BAR_BOTTOM + 3);

    /* Draw the outline of the power bar */
    LcdDriverDrawRectangle(
        LCD_APP_BAR_LEFT, LCD_APP_BAR_TOP, LCD_APP_BAR_RIGHT, LCD_APP_BAR_BOTTOM, 1);

    /* If either of voltage or current equals NULL, we display a cross over the bar*/
    if (pack_current == NULL || pack_voltage == NULL)
    {
        int bar_width = LCD_APP_BAR_RIGHT - LCD_APP_BAR_LEFT;
        int bar_height = LCD_APP_BAR_BOTTOM - LCD_APP_BAR_TOP;
        for (int i = 0; i <= bar_width; i++)
        {
            int x = LCD_APP_BAR_LEFT + i;
            int y = LCD_APP_BAR_TOP + (i * bar_height) / bar_width;
            LcdDriverSetPixel(x, y, 1);
        }
        for (int i = 0; i <= bar_width; i++)
        {
            int x = LCD_APP_BAR_RIGHT - i;
            int y = LCD_APP_BAR_TOP + (i * bar_height) / bar_width;
            LcdDriverSetPixel(x, y, 1);
        }
        LcdDriverRefresh();
        return;
    }
    else
    {
        float power = (float)*pack_current * (float)*pack_voltage;
        int fill_pixels = 0;

        if (power > 0)
        {
            float ratio = power / LCD_APP_MAX_POSITIVE_POWER;
            if (ratio > 1.0f)
                ratio = 1.0f;
            int total_pixels_right = LCD_APP_BAR_RIGHT - LCD_APP_CENTER_X;
            fill_pixels = (int)(ratio * total_pixels_right);
            for (int y = LCD_APP_BAR_TOP + 1; y < LCD_APP_BAR_BOTTOM; y++)
            {
                for (int x = LCD_APP_CENTER_X + 1; x <= LCD_APP_CENTER_X + fill_pixels; x++)
                {
                    LcdDriverSetPixel(x, y, 1);
                }
            }
        }
        else if (power < 0)
        {
            float ratio = (-power) / LCD_APP_MAX_NEGATIVE_POWER;
            if (ratio > 1.0f)
                ratio = 1.0f;
            int total_pixels_left = LCD_APP_CENTER_X - LCD_APP_BAR_LEFT;
            fill_pixels = (int)(ratio * total_pixels_left);
            for (int y = LCD_APP_BAR_TOP + 1; y < LCD_APP_BAR_BOTTOM; y++)
            {
                for (int x = LCD_APP_CENTER_X - 1; x >= LCD_APP_CENTER_X - fill_pixels; x--)
                {
                    LcdDriverSetPixel(x, y, 1);
                }
            }
        }

        /* Redraw the center line extending 3 pixels below the bar */
        for (int y = LCD_APP_BAR_TOP; y <= LCD_APP_BAR_BOTTOM + 3; y++)
        {
            LcdDriverSetPixel(LCD_APP_CENTER_X, y, 1);
        }
        LcdDriverRefresh();
    }
}

void LcdAppDisplaySpeedDebugPage(volatile uint32_t* speed, volatile uint8_t units)
{
    char speed_str[12];

    /* Clear the previous speed and unit areas */
    LcdDriverClearBoundingBox(LCD_APP_DEBUG_SPEED_THREEDIGIT_X,old_bb_speed.y1,LCD_DRIVER_BOTTOM_RIGHT_X,old_bb_speed.y2 - 3);

    if (speed == NULL)
    { // Stale speed data
        sprintf(speed_str, "XX");
        old_bb_speed = LcdDriverDrawText(speed_str, LCD_APP_DEBUG_SPEED_TWODIGIT_X + 10, LCD_APP_DEBUG_SPEED_Y + 10, LCD_APP_DEBUG_SPEED_FONT, LCD_APP_DEBUG_SPEED_SPACING + 10);
    }
    else if (*speed < 10)
    { // Single digit speed
        sprintf(speed_str, "%01lu", (unsigned long)*speed);
        old_bb_speed = LcdDriverDrawText(speed_str, LCD_APP_DEBUG_SPEED_ONEDIGIT_X, LCD_APP_DEBUG_SPEED_Y, LCD_APP_DEBUG_SPEED_FONT, LCD_APP_DEBUG_SPEED_SPACING);
    }
    else if (*speed < 100)
    { // Double digit second
        sprintf(speed_str, "%02lu", (unsigned long)*speed);
        old_bb_speed = LcdDriverDrawText(speed_str, LCD_APP_DEBUG_SPEED_TWODIGIT_X, LCD_APP_DEBUG_SPEED_Y, LCD_APP_DEBUG_SPEED_FONT, LCD_APP_DEBUG_SPEED_SPACING);
    }
    else
    {
        sprintf(speed_str, "%03lu", (unsigned long)*speed);
        old_bb_speed = LcdDriverDrawText(speed_str, LCD_APP_DEBUG_SPEED_THREEDIGIT_X, LCD_APP_DEBUG_SPEED_Y, LCD_APP_DEBUG_SPEED_FONT, LCD_APP_DEBUG_SPEED_SPACING);
    }

    /* Draw the speed units */
    switch (units)
    {
    case LCD_APP_KPH:
        LcdDriverDrawText("kph", LCD_APP_DEBUG_SPEED_X + LCD_APP_DEBUG_SPEED_UNIT_KPH_X, LCD_APP_DEBUG_SPEED_UNIT_Y, LCD_APP_DEBUG_SPEED_UNITS_FONT, LCD_APP_DEBUG_SPEED_UNITS_SPACING);
        break;
    case LCD_APP_MPH:
        LcdDriverDrawText("mph", LCD_APP_DEBUG_SPEED_X + LCD_APP_DEBUG_SPEED_UNIT_MPH_X, LCD_APP_DEBUG_SPEED_UNIT_Y, LCD_APP_DEBUG_SPEED_UNITS_FONT, LCD_APP_DEBUG_SPEED_UNITS_SPACING);
        break;
    default:
        LcdDriverDrawText("xxx", LCD_APP_DEBUG_SPEED_X + LCD_APP_DEBUG_SPEED_UNIT_MPH_X, LCD_APP_DEBUG_SPEED_UNIT_Y, LCD_APP_DEBUG_SPEED_UNITS_FONT, LCD_APP_DEBUG_SPEED_UNITS_SPACING);
        break;
    }

    LcdDriverRefresh();
}

void LcdAppDisplaySocDebugPage(volatile uint32_t* soc)
{
    char soc_str[12];

    LcdDriverClearBoundingBox(old_bb_soc.x1, old_bb_soc.y1, old_bb_soc.x2, old_bb_soc.y2 - 5);

    // Check for stale data and display "--" if so.
    if (soc == NULL)
    {
        sprintf(soc_str, "--");
        old_bb_soc = LcdDriverDrawText(soc_str, LCD_APP_DEBUG_SOC_TWODIGIT_X, LCD_APP_SOC_Y, LCD_APP_DEBUG_SOC_FONT, LCD_APP_DEBUG_SOC_SPACING);
    }
    else if (*soc < 10)
    {
        sprintf(soc_str, "%01lu", (unsigned long)*soc);
        old_bb_soc = LcdDriverDrawText(soc_str, LCD_APP_DEBUG_SOC_ONEDIGIT_X, LCD_APP_DEBUG_SOC_Y, LCD_APP_DEBUG_SOC_FONT, LCD_APP_DEBUG_SOC_SPACING);
    }
    else if (*soc < 100)
    {
        sprintf(soc_str, "%02lu", (unsigned long)*soc);
        old_bb_soc = LcdDriverDrawText(soc_str, LCD_APP_DEBUG_SOC_TWODIGIT_X, LCD_APP_DEBUG_SOC_Y, LCD_APP_DEBUG_SOC_FONT, LCD_APP_DEBUG_SOC_SPACING);
    }
    else
    {
        sprintf(soc_str, "%03lu", (unsigned long)*soc);
        old_bb_soc = LcdDriverDrawText(soc_str, LCD_APP_DEBUG_SOC_THREEDIGIT_X, LCD_APP_DEBUG_SOC_Y, LCD_APP_DEBUG_SOC_FONT, LCD_APP_DEBUG_SOC_SPACING);
    }

    LcdDriverDrawChar(LCD_APP_SOC_UNITS, LCD_APP_DEBUG_SOC_UNITS_X, LCD_APP_DEBUG_SOC_Y, LCD_APP_DEBUG_SOC_UNITS_FONT);

    LcdDriverRefresh();
}

void LcdAppDisplayDriveStateDebugPage(volatile DriveStateStates* state)
{
    char state_str[2] = {LCD_APP_ERROR_SYMBOL, '\0'}; // Default to error symbol.

    if (state == NULL)
    { // Stale data for drive state
        sprintf(state_str, "-");
    }
    else
    {
        switch (*state)
        {
        case FORWARD:
            state_str[0] = LCD_APP_FORWARD_SYMBOL;
            break;
        case PARK:
            state_str[0] = LCD_APP_PARK_SYMBOL;
            break;
        case REVERSE:
            state_str[0] = LCD_APP_REVERSE_SYMBOL;
            break;
        default:
            state_str[0] = LCD_APP_ERROR_SYMBOL;
            break;
        }
    }

    LcdDriverClearBoundingBox(
        LCD_APP_DEBUG_STATE_X, LCD_APP_DEBUG_STATE_Y, 20, LCD_DRIVER_BOTTOM_RIGHT_Y);
    old_bb_drive_state = LcdDriverDrawText(state_str, LCD_APP_DEBUG_STATE_X, LCD_APP_DEBUG_STATE_Y, LCD_APP_DEBUG_STATE_FONT, LCD_APP_DEBUG_STATE_SPACING);

    LcdDriverRefresh();
}
