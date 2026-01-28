#include "lcd_app.h"
// #include "cyclic_data_handler.h"
// #include "diagnostic.h"
// #include "drd_freertos.h"
// #include "fault_lights.h"
#include "lcd_driver.h"
// #include "soc.h"
#include <stdio.h>

/*--------------------------------------------------------------------------
  Internal Types & Variables
--------------------------------------------------------------------------*/

/* Static variables to store old bounding boxes for updating text fields */
static BoundingBox old_bb_speed = {0, 0, 0, 0};
static BoundingBox old_bb_drive_state = {0, 0, 0, 0};
static BoundingBox old_bb_drive_mode = {0, 0, 0, 0};
static BoundingBox old_bb_soc = {0, 0, 0, 0};
static BoundingBox old_bb_fault_indicator = {0, 0, 0, 0};
static BoundingBox old_bb_warning_indicator = {0, 0, 0, 0};

static char faults[8][10] = {0};
static char warning_char[8][10] = {0};

/* External variables*/
LcdAppData g_lcd_data = {0};
LcdAppBattFaults g_lcd_batt_faults = {0};
LcdAppMotorFaults g_lcd_motor_faults = {0};
LcdAppWarnings g_lcd_warnings = {0};
LcdAppTemperature g_lcd_temperatures[8] = {0};

uint8_t g_LCD_page = 1;
uint8_t g_LCD_page_change = 0;

/*--------------------------------------------------------------------------
  HELPER FUNCTIONS
--------------------------------------------------------------------------*/

/**
 * @brief Checks and updates the faults.
 *
 * @param batt_faults A struct containing the battery faults.
 * @param motor_faults A struct containing the motor faults.
 * @return The count of faults.
 */
uint8_t LcdAppCheckFaults(LcdAppBattFaults* batt_faults, LcdAppMotorFaults* motor_faults)
{
    uint8_t fault_count = 0;

    if (batt_faults->battery_fault)
    {
        sprintf(faults[fault_count], "%s", LCD_APP_BATT_FLT_CHARS);
        fault_count++;
    }
    if (batt_faults->supp_lo)
    {
        sprintf(faults[fault_count], "%s", LCD_APP_BATT_SUPPLO_FLT_CHARS);
        fault_count++;
    }
    if (batt_faults->voltage_high)
    {
        sprintf(faults[fault_count], "%s", LCD_APP_BATT_VOLTHIGH_FLT_CHARS);
        fault_count++;
    }
    if (batt_faults->voltage_low)
    {
        sprintf(faults[fault_count], "%s", LCD_APP_BATT_VOLTLOW_FLT_CHARS);
        fault_count++;
    }
    if (batt_faults->slave_board_comm_fault)
    {
        sprintf(faults[fault_count], "%s", LCD_APP_BATT_SLAVE_COMM_FLT_CHARS);
        fault_count++;
    }
    if (batt_faults->overvolt_fault)
    {
        sprintf(faults[fault_count], "%s", LCD_APP_BATT_OVERVOLT_FLT_CHARS);
        fault_count++;
    }
    if (batt_faults->undervolt_fault)
    {
        sprintf(faults[fault_count], "%s", LCD_APP_BATT_UNDERVOLT_FLT_CHARS);
        fault_count++;
    }
    if (batt_faults->overtemp_fault)
    {
        sprintf(faults[fault_count], "%s", LCD_APP_BATT_OVERTEMP_FLT_CHARS);
        fault_count++;
    }
    if (batt_faults->charge_overcurrent_fault)
    {
        sprintf(faults[fault_count], "%s", LCD_APP_BATT_CHARGE_OC_FLT_CHARS);
        fault_count++;
    }
    if (batt_faults->discharge_overcurrent_fault)
    {
        sprintf(faults[fault_count], "%s", LCD_APP_BATT_DISCHARGE_OC_FLT_CHARS);
        fault_count++;
    }
    if (batt_faults->reset_from_watchdog)
    {
        sprintf(faults[fault_count], "%s", LCD_APP_BATT_RST_FROM_WATCH_FLT_CHARS);
        fault_count++;
    }
    if (motor_faults->motor_system_error)
    {
        sprintf(faults[fault_count], "%s", LCD_APP_MTR_SYSTEM_FLT_CHARS);
        fault_count++;
    }
    if (motor_faults->overcurrent_fault)
    {
        sprintf(faults[fault_count], "%s", LCD_APP_MTR_OVERCURR_FLT_CHARS);
        fault_count++;
    }
    if (motor_faults->overvoltage_fault)
    {
        sprintf(faults[fault_count], "%s", LCD_APP_MTR_OVERVOLT_FLT_CHARS);
        fault_count++;
    }
    if (motor_faults->fet_thermistor_error)
    {
        sprintf(faults[fault_count], "%s", LCD_APP_MTR_OVERTEMP_FLT_CHARS);
        fault_count++;
    }
    if (motor_faults->motor_comm_fault)
    {
        sprintf(faults[fault_count], "%s", LCD_APP_MTR_COMM_FLT_CHARS);
        fault_count++;
    }
    if (motor_faults->throttle_adc_outofrange)
    {
        sprintf(faults[fault_count], "%s", LCD_APP_MTR_THROT_ADC_OOR_FLT_CHARS);
        fault_count++;
    }
    if (motor_faults->throttle_adc_mismatch)
    {
        sprintf(faults[fault_count], "%s", LCD_APP_MTR_THROT_ADC_MISMATCH_FLT_CHARS);
        fault_count++;
    }

    return fault_count;
}

/**
 * @brief Checks and updates the warnings.
 *
 * @param warnings A struct containing the warnings
 *
 * @return The count of warnings.
 */
uint8_t LcdAppCheckWarnings(LcdAppWarnings* warnings)
{
    uint8_t warning_count = 0;

    if (warnings->high_temp_warning)
    {
        sprintf(warning_char[warning_count], "%s", LCD_APP_HIGHTEMP_WARN_CHARS);
        warning_count++;
    }
    if (warnings->high_volt_warning)
    {
        sprintf(warning_char[warning_count], "%s", LCD_APP_HIGHVOLT_WARN_CHARS);
        warning_count++;
    }
    if (warnings->low_temp_warning)
    {
        sprintf(warning_char[warning_count], "%s", LCD_APP_LOWTEMP_WARN_CHARS);
        warning_count++;
    }
    if (warnings->low_volt_warning)
    {
        sprintf(warning_char[warning_count], "%s", LCD_APP_LOWVOLT_WARN_CHARS);
        warning_count++;
    }
    if (warnings->no_ecu_message)
    {
        sprintf(warning_char[warning_count], "%s", LCD_APP_NOMSG_WARN_CHARS);
        warning_count++;
    }
    if (warnings->pack_overcharge)
    {
        sprintf(warning_char[warning_count], "%s", LCD_APP_PACK_OC_WARN_CHARS);
        warning_count++;
    }
    if (warnings->pack_overdischarge)
    {
        sprintf(warning_char[warning_count], "%s", LCD_APP_PACK_OD_WARN_CHARS);
        warning_count++;
    }

    return warning_count;
}

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
    LcdDriverClearBoundingBox(LCD_APP_SPEED_THREEDIGIT_X,
                              old_bb_speed.y1 + 10,
                              LCD_DRIVER_BOTTOM_RIGHT_X,
                              old_bb_speed.y2);

    if (speed == NULL)
    { // Stale speed data
        sprintf(speed_str, "XX");
        old_bb_speed = LcdDriverDrawText(speed_str,
                                         LCD_APP_SPEED_TWODIGIT_X + 10,
                                         LCD_APP_SPEED_Y + 10,
                                         LCD_APP_SPEED_NULL_FONT,
                                         LCD_APP_SPEED_SPACING + 10);
        // g_diagnostics.cyclic_flags.speed_timeout = true;
    }
    else if (*speed < 10)
    { // Single digit speed
        sprintf(speed_str, "%01lu", (unsigned long)*speed);
        old_bb_speed = LcdDriverDrawText(speed_str,
                                         LCD_APP_SPEED_ONEDIGIT_X,
                                         LCD_APP_SPEED_Y,
                                         LCD_APP_SPEED_FONT,
                                         LCD_APP_SPEED_SPACING);
        // g_diagnostics.cyclic_flags.speed_timeout = false;
    }
    else if (*speed < 100)
    { // Double digit second
        sprintf(speed_str, "%02lu", (unsigned long)*speed);
        old_bb_speed = LcdDriverDrawText(speed_str,
                                         LCD_APP_SPEED_TWODIGIT_X,
                                         LCD_APP_SPEED_Y,
                                         LCD_APP_SPEED_FONT,
                                         LCD_APP_SPEED_SPACING);
        // g_diagnostics.cyclic_flags.speed_timeout = false;
    }
    else
    {
        sprintf(speed_str, "%03lu", (unsigned long)*speed);
        old_bb_speed = LcdDriverDrawText(speed_str,
                                         LCD_APP_SPEED_THREEDIGIT_X,
                                         LCD_APP_SPEED_Y,
                                         LCD_APP_SPEED_FONT,
                                         LCD_APP_SPEED_SPACING);
        // g_diagnostics.cyclic_flags.speed_timeout = false;
    }

    /* Draw the speed units */
    switch (units)
    {
    case LCD_APP_KPH:
        LcdDriverDrawText("kph",
                          LCD_APP_SPEED_X + LCD_APP_SPEED_UNIT_KPH_X,
                          LCD_APP_SPEED_UNIT_Y,
                          LCD_APP_SPEED_UNITS_FONT,
                          LCD_APP_SPEED_UNITS_SPACING);
        break;
    case LCD_APP_MPH:
        LcdDriverDrawText("mph",
                          LCD_APP_SPEED_X + LCD_APP_SPEED_UNIT_MPH_X,
                          LCD_APP_SPEED_UNIT_Y,
                          LCD_APP_SPEED_UNITS_FONT,
                          LCD_APP_SPEED_UNITS_SPACING);
        break;
    default:
        LcdDriverDrawText("xxx",
                          LCD_APP_SPEED_X + LCD_APP_SPEED_UNIT_MPH_X,
                          LCD_APP_SPEED_UNIT_Y,
                          LCD_APP_SPEED_UNITS_FONT,
                          LCD_APP_SPEED_UNITS_SPACING);
        break;
    }

    LcdDriverRefresh();
}

/**
 * @brief Displays the state of charge (SOC) on the LCD drive page.
 *
 * @param soc The state of charge (in percent).
 */
void LcdAppDisplaySOCDrivePage(volatile uint32_t* soc)
{
    char soc_str[12];

    LcdDriverClearBoundingBox(
        old_bb_soc.x1, old_bb_soc.y1, old_bb_soc.x2, old_bb_soc.y2 - LCD_APP_SOC_SPACING);

    // Check for stale data and display "--" if so.
    if (soc == NULL)
    {
        sprintf(soc_str, "--");
        old_bb_soc = LcdDriverDrawText(
            soc_str, LCD_APP_SOC_TWODIGIT_X, LCD_APP_SOC_Y, LCD_APP_SOC_FONT, LCD_APP_SOC_SPACING);
        // g_diagnostics.cyclic_flags.soc_timeout = true;
    }
    else if (*soc < 10)
    {
        sprintf(soc_str, "%01lu", (unsigned long)*soc);
        old_bb_soc = LcdDriverDrawText(
            soc_str, LCD_APP_SOC_ONEDIGIT_X, LCD_APP_SOC_Y, LCD_APP_SOC_FONT, LCD_APP_SOC_SPACING);
        // g_diagnostics.cyclic_flags.soc_timeout = false;
    }
    else if (*soc < 100)
    {
        sprintf(soc_str, "%02lu", (unsigned long)*soc);
        old_bb_soc = LcdDriverDrawText(
            soc_str, LCD_APP_SOC_TWODIGIT_X, LCD_APP_SOC_Y, LCD_APP_SOC_FONT, LCD_APP_SOC_SPACING);
        // g_diagnostics.cyclic_flags.soc_timeout = false;
    }
    else
    {
        sprintf(soc_str, "%03lu", (unsigned long)*soc);
        old_bb_soc = LcdDriverDrawText(soc_str,
                                       LCD_APP_SOC_THREEDIGIT_X,
                                       LCD_APP_SOC_Y,
                                       LCD_APP_SOC_FONT,
                                       LCD_APP_SOC_SPACING);
        // g_diagnostics.cyclic_flags.soc_timeout = false;
    }

    LcdDriverDrawChar(
        LCD_APP_SOC_UNITS, LCD_APP_SOC_UNITS_X, LCD_APP_SOC_Y, LCD_APP_SOC_UNITS_FONT);

    LcdDriverRefresh();
}

/**
 * @brief Displays an E for ECO mode and P for POWER mode
 *
 * @param drive_mode The drive mode
 */
void LcdAppDisplayDriveModeDrivePage(volatile uint8_t drive_mode)
{
    LcdDriverClearBoundingBox(old_bb_drive_mode.x1,
                              old_bb_drive_mode.y1 + 4,
                              old_bb_drive_mode.x2,
                              old_bb_drive_mode.y2 - 2);

    // Drive mode is valid, display the corresponding symbol.
    switch (drive_mode)
    {
    case LCD_APP_DRIVE_MODE_ECO:
        old_bb_drive_mode = LcdDriverDrawChar(
            LCD_APP_ECO_SYMBOL, LCD_APP_ECO_MODE_X, LCD_APP_ECO_MODE_Y, LCD_APP_ECO_MODE_FONT);
        break;
    case LCD_APP_DRIVE_MODE_POWER:
        old_bb_drive_mode = LcdDriverDrawChar(LCD_APP_POWER_SYMBOL,
                                              LCD_APP_POWER_MODE_X,
                                              LCD_APP_POWER_MODE_Y,
                                              LCD_APP_POWER_MODE_FONT);
        break;
    default:
        old_bb_drive_mode = LcdDriverDrawChar(
            LCD_APP_ERROR_SYMBOL, LCD_APP_ECO_MODE_X, LCD_APP_ECO_MODE_Y, LCD_APP_ECO_MODE_FONT);
        break;
    }

    LcdDriverRefresh();
}

/**
 * @brief Displays the drive state on the LCD drive page.
 *
 * @param state The drive state (e.g., FORWARD_STATE, PARK_STATE, REVERSE_STATE).
 */
void LcdAppDisplayDriveStateDrivePage(volatile drive_state_t* state)
{
    char state_str[2] = {LCD_APP_ERROR_SYMBOL, '\0'}; // Default to error symbol.

    if (state == NULL)
    { // Stale data for drive state
        sprintf(state_str, "-");
        // g_diagnostics.cyclic_flags.drive_state_timeout = true;
    }
    else
    {
        switch (*state)
        {
        case LCD_APP_FORWARD_STATE:
            state_str[0] = LCD_APP_FORWARD_SYMBOL;
            break;
        case LCD_APP_PARK_STATE:
            state_str[0] = LCD_APP_PARK_SYMBOL;
            break;
        case LCD_APP_REVERSE_STATE:
            state_str[0] = LCD_APP_REVERSE_SYMBOL;
            break;
        default:
            state_str[0] = LCD_APP_ERROR_SYMBOL;
            break;
        }
        // g_diagnostics.cyclic_flags.drive_state_timeout = false;
    }

    LcdDriverClearBoundingBox(
        LCD_APP_STATE_X, LCD_APP_STATE_Y, old_bb_drive_state.x2, LCD_DRIVER_BOTTOM_RIGHT_Y);
    old_bb_drive_state = LcdDriverDrawText(
        state_str, LCD_APP_STATE_X, LCD_APP_STATE_Y, LCD_APP_STATE_FONT, LCD_APP_STATE_SPACING);

    LcdDriverRefresh();
}

/**
 * @brief Displays a fault indicator on the LCD Drive Page
 *
 * @param fault_indicator A general indicator to signal a fault to prompt the driver to change pages
 */
void LcdAppDisplayFaultIndicator(LcdAppBattFaults* batt_faults, LcdAppMotorFaults* motor_faults)
{
    LcdDriverClearBoundingBox(old_bb_fault_indicator.x1,
                              old_bb_fault_indicator.y1,
                              old_bb_fault_indicator.x2,
                              old_bb_fault_indicator.y2);

    uint8_t fault_count = LcdAppCheckFaults(batt_faults, motor_faults);

    // Check if there is an existing fault
    if (fault_count > 0)
    {
        old_bb_fault_indicator = LcdDriverDrawChar(
            LCD_APP_FAULT_SYMBOL, LCD_APP_FAULT_X, LCD_APP_FAULT_Y, LCD_APP_FAULT_SYMBOL_FONT);
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
    LcdDriverClearBoundingBox(old_bb_warning_indicator.x1,
                              old_bb_warning_indicator.y1,
                              old_bb_warning_indicator.x2,
                              old_bb_warning_indicator.y2);

    uint8_t warning_count = LcdAppCheckWarnings(warnings);

    // Check if there is an existing warning
    if (warning_count > 0)
    {
        old_bb_warning_indicator = LcdDriverDrawChar(LCD_APP_WARNING_SYMBOL,
                                                     LCD_APP_WARNING_X,
                                                     LCD_APP_WARNING_Y,
                                                     LCD_APP_WARNING_SYMBOL_FONT);
    }
    LcdDriverRefresh();
}

/*--------------------------------------------------------------------------
  PAGE 2 (FAULT PAGE) FUNCTIONS
--------------------------------------------------------------------------*/

/**
 * @brief Dynamically displays battery and motor faults on the LCD
 *
 * @param batt_faults The battery faults to be displayed on the LCD
 * @param motor_faults The motor faults to be displayed on the LCD
 */
void LcdAppDisplayFaults(LcdAppBattFaults* batt_faults, LcdAppMotorFaults* motor_faults)
{

    LcdDriverClearBoundingBox(
        0, LCD_APP_FAULT_FOUR_Y1, LCD_DRIVER_BOTTOM_RIGHT_X, LCD_DRIVER_BOTTOM_RIGHT_Y);

    uint8_t fault_count = LcdAppCheckFaults(batt_faults, motor_faults);

    LcdDriverDrawText(LCD_APP_FAULT_LABEL_CHARS,
                      LCD_APP_FAULT_LABEL_X,
                      LCD_APP_FAULT_LABEL_Y,
                      LCD_APP_FAULT_LABEL_FONT,
                      LCD_APP_FAULT_SPACING);
    for (uint8_t i = 0; i < LCD_APP_FAULT_LABEL_UNDERLINE_X; i++)
    {
        LcdDriverSetPixel(i, LCD_APP_FAULT_LABEL_UNDERLINE_Y, 1);
    }
    if (fault_count <= 3)
    {
        LcdDriverDrawText(faults[0],
                          LCD_APP_FAULT_FOUR_X1,
                          LCD_APP_FAULT_FOUR_Y1,
                          LCD_APP_FAULT_FOUR_FONT,
                          LCD_APP_FAULT_SPACING);
        LcdDriverDrawText(faults[1],
                          LCD_APP_FAULT_FOUR_X2,
                          LCD_APP_FAULT_FOUR_Y2,
                          LCD_APP_FAULT_FOUR_FONT,
                          LCD_APP_FAULT_SPACING);
        LcdDriverDrawText(faults[2],
                          LCD_APP_FAULT_FOUR_X3,
                          LCD_APP_FAULT_FOUR_Y3,
                          LCD_APP_FAULT_FOUR_FONT,
                          LCD_APP_FAULT_SPACING);
    }
    else if (fault_count <= 8)
    {
        LcdDriverDrawText(faults[0],
                          LCD_APP_FAULT_EIGHT_X1,
                          LCD_APP_FAULT_EIGHT_Y1,
                          LCD_APP_FAULT_EIGHT_FONT,
                          LCD_APP_FAULT_SPACING);
        LcdDriverDrawText(faults[1],
                          LCD_APP_FAULT_EIGHT_X2,
                          LCD_APP_FAULT_EIGHT_Y2,
                          LCD_APP_FAULT_EIGHT_FONT,
                          LCD_APP_FAULT_SPACING);
        LcdDriverDrawText(faults[2],
                          LCD_APP_FAULT_EIGHT_X3,
                          LCD_APP_FAULT_EIGHT_Y3,
                          LCD_APP_FAULT_EIGHT_FONT,
                          LCD_APP_FAULT_SPACING);
        LcdDriverDrawText(faults[3],
                          LCD_APP_FAULT_EIGHT_X4,
                          LCD_APP_FAULT_EIGHT_Y4,
                          LCD_APP_FAULT_EIGHT_FONT,
                          LCD_APP_FAULT_SPACING);
        LcdDriverDrawText(faults[4],
                          LCD_APP_FAULT_EIGHT_X5,
                          LCD_APP_FAULT_EIGHT_Y5,
                          LCD_APP_FAULT_EIGHT_FONT,
                          LCD_APP_FAULT_SPACING);
        LcdDriverDrawText(faults[5],
                          LCD_APP_FAULT_EIGHT_X6,
                          LCD_APP_FAULT_EIGHT_Y6,
                          LCD_APP_FAULT_EIGHT_FONT,
                          LCD_APP_FAULT_SPACING);
        LcdDriverDrawText(faults[6],
                          LCD_APP_FAULT_EIGHT_X7,
                          LCD_APP_FAULT_EIGHT_Y7,
                          LCD_APP_FAULT_EIGHT_FONT,
                          LCD_APP_FAULT_SPACING);
        LcdDriverDrawText(faults[7],
                          LCD_APP_FAULT_EIGHT_X8,
                          LCD_APP_FAULT_EIGHT_Y8,
                          LCD_APP_FAULT_EIGHT_FONT,
                          LCD_APP_FAULT_SPACING);
    }

    LcdDriverRefresh();
}

/*--------------------------------------------------------------------------
  PAGE 3 (WARNING PAGE) FUNCTIONS
--------------------------------------------------------------------------*/

/**
 * @brief Displays a motor faults on the LCD
 *
 * @param fault_indicator An indicator to see who
 */
void LcdAppDisplayWarnings(LcdAppWarnings* warnings)
{
    LcdDriverClearBoundingBox(
        0, LCD_APP_WARNING_FOUR_Y1, LCD_DRIVER_BOTTOM_RIGHT_X, LCD_DRIVER_BOTTOM_RIGHT_Y);

    uint8_t warning_count = LcdAppCheckWarnings(warnings);

    LcdDriverDrawText(LCD_APP_WARNING_LABEL_CHARS,
                      LCD_APP_WARNING_LABEL_X,
                      LCD_APP_WARNING_LABEL_Y,
                      LCD_APP_WARNING_LABEL_FONT,
                      LCD_APP_WARNING_SPACING);
    for (uint8_t i = 0; i < LCD_APP_WARNING_LABEL_UNDERLINE_X; i++)
    {
        LcdDriverSetPixel(i, LCD_APP_WARNING_LABEL_UNDERLINE_Y, 1);
    }
    if (warning_count <= 3)
    {
        LcdDriverDrawText(warning_char[0],
                          LCD_APP_WARNING_FOUR_X1,
                          LCD_APP_WARNING_FOUR_Y1,
                          LCD_APP_WARNING_FOUR_FONT,
                          LCD_APP_WARNING_SPACING);
        LcdDriverDrawText(warning_char[1],
                          LCD_APP_WARNING_FOUR_X2,
                          LCD_APP_WARNING_FOUR_Y2,
                          LCD_APP_WARNING_FOUR_FONT,
                          LCD_APP_WARNING_SPACING);
        LcdDriverDrawText(warning_char[2],
                          LCD_APP_WARNING_FOUR_X3,
                          LCD_APP_WARNING_FOUR_Y3,
                          LCD_APP_WARNING_FOUR_FONT,
                          LCD_APP_WARNING_SPACING);
    }
    else if (warning_count <= 8)
    {
        LcdDriverDrawText(warning_char[0],
                          LCD_APP_WARNING_EIGHT_X1,
                          LCD_APP_WARNING_EIGHT_Y1,
                          LCD_APP_WARNING_EIGHT_FONT,
                          LCD_APP_WARNING_SPACING);
        LcdDriverDrawText(warning_char[1],
                          LCD_APP_WARNING_EIGHT_X2,
                          LCD_APP_WARNING_EIGHT_Y2,
                          LCD_APP_WARNING_EIGHT_FONT,
                          LCD_APP_WARNING_SPACING);
        LcdDriverDrawText(warning_char[2],
                          LCD_APP_WARNING_EIGHT_X3,
                          LCD_APP_WARNING_EIGHT_Y3,
                          LCD_APP_WARNING_EIGHT_FONT,
                          LCD_APP_WARNING_SPACING);
        LcdDriverDrawText(warning_char[3],
                          LCD_APP_WARNING_EIGHT_X4,
                          LCD_APP_WARNING_EIGHT_Y4,
                          LCD_APP_WARNING_EIGHT_FONT,
                          LCD_APP_WARNING_SPACING);
        LcdDriverDrawText(warning_char[4],
                          LCD_APP_WARNING_EIGHT_X5,
                          LCD_APP_WARNING_EIGHT_Y5,
                          LCD_APP_WARNING_EIGHT_FONT,
                          LCD_APP_WARNING_SPACING);
        LcdDriverDrawText(warning_char[5],
                          LCD_APP_WARNING_EIGHT_X6,
                          LCD_APP_WARNING_EIGHT_Y6,
                          LCD_APP_WARNING_EIGHT_FONT,
                          LCD_APP_WARNING_SPACING);
        LcdDriverDrawText(warning_char[6],
                          LCD_APP_WARNING_EIGHT_X7,
                          LCD_APP_WARNING_EIGHT_Y7,
                          LCD_APP_WARNING_EIGHT_FONT,
                          LCD_APP_WARNING_SPACING);
        LcdDriverDrawText(warning_char[7],
                          LCD_APP_WARNING_EIGHT_X8,
                          LCD_APP_WARNING_EIGHT_Y8,
                          LCD_APP_WARNING_EIGHT_FONT,
                          LCD_APP_WARNING_SPACING);
    }

    LcdDriverRefresh();
}

/*--------------------------------------------------------------------------
  PAGE 4 (TEMPERATURE PAGE) FUNCTIONS
--------------------------------------------------------------------------*/

/**
 * @brief Displays a Temperature on the LCD (0-255)
 *
 * @param temperature A struct containing the temperature and id of the temperature.
 */
void LcdAppDisplayTemperature(LcdAppTemperature temperature_data)
{

    // TODO: When assigning with CAN ensure that the name is set too

    // Stores a Bounding Box used for changing temp symbol position
    BoundingBox bb = {0};

    // Variables to hold values based on what temperature is being displayed
    char temp_label[8];
    char temp_str[4];
    uint8_t temp_x;
    uint8_t temp_y;
    uint8_t temp_shift;

    switch (temperature_data.temp_label)
    {
    case LCD_APP_MPPT_A_LABEL:
        sprintf(temp_label, "%s", LCD_APP_MPPT_A_CHARS);
        temp_x = LCD_APP_MPPT_A_X;
        temp_y = LCD_APP_MPPT_A_Y;
        temp_shift = LCD_APP_TEMP_MPPT_OFFSET;
        LcdDriverClearBoundingBox(
            LCD_APP_TEMP_MPPT_OFFSET, LCD_APP_MPPT_A_Y, LCD_APP_BATT_MAX_X - 2, LCD_APP_MPPT_B_Y);
        break;
    case LCD_APP_MPPT_B_LABEL:
        sprintf(temp_label, "%s", LCD_APP_MPPT_B_CHARS);
        temp_x = LCD_APP_MPPT_B_X;
        temp_y = LCD_APP_MPPT_B_Y;
        temp_shift = LCD_APP_TEMP_MPPT_OFFSET;
        LcdDriverClearBoundingBox(
            LCD_APP_TEMP_MPPT_OFFSET, LCD_APP_MPPT_B_Y, LCD_APP_BATT_MAX_X - 2, LCD_APP_MPPT_C_Y);
        break;
    case LCD_APP_MPPT_C_LABEL:
        sprintf(temp_label, "%s", LCD_APP_MPPT_C_CHARS);
        temp_x = LCD_APP_MPPT_C_X;
        temp_y = LCD_APP_MPPT_C_Y;
        temp_shift = LCD_APP_TEMP_MPPT_OFFSET;
        LcdDriverClearBoundingBox(
            LCD_APP_TEMP_MPPT_OFFSET, LCD_APP_MPPT_C_Y, LCD_APP_BATT_MAX_X - 2, LCD_APP_MPPT_D_Y);
        break;
    case LCD_APP_MPPT_D_LABEL:
        sprintf(temp_label, "%s", LCD_APP_MPPT_D_CHARS);
        temp_x = LCD_APP_MPPT_D_X;
        temp_y = LCD_APP_MPPT_D_Y;
        temp_shift = LCD_APP_TEMP_MPPT_OFFSET;
        LcdDriverClearBoundingBox(LCD_APP_TEMP_MPPT_OFFSET,
                                  LCD_APP_MPPT_D_Y,
                                  LCD_APP_BATT_MAX_X - 2,
                                  LCD_DRIVER_BOTTOM_RIGHT_Y);
        break;
    case LCD_APP_BATT_MAX_LABEL:
        sprintf(temp_label, "%s", LCD_APP_BATT_MAX_CHARS);
        temp_x = LCD_APP_BATT_MAX_X;
        temp_y = LCD_APP_BATT_MAX_Y;
        temp_shift = LCD_APP_TEMP_BATT_OFFSET;
        LcdDriverClearBoundingBox(LCD_APP_BATT_MAX_X + LCD_APP_TEMP_BATT_OFFSET,
                                  0,
                                  LCD_DRIVER_BOTTOM_RIGHT_X,
                                  LCD_APP_BATT_MIN_Y);
        break;
    case LCD_APP_BATT_MIN_LABEL:
        sprintf(temp_label, "%s", LCD_APP_BATT_MIN_CHARS);
        temp_x = LCD_APP_BATT_MIN_X;
        temp_y = LCD_APP_BATT_MIN_Y;
        temp_shift = LCD_APP_TEMP_BATT_OFFSET;
        LcdDriverClearBoundingBox(LCD_APP_BATT_MAX_X + LCD_APP_TEMP_BATT_OFFSET,
                                  LCD_APP_BATT_MIN_Y,
                                  LCD_DRIVER_BOTTOM_RIGHT_X,
                                  LCD_APP_MTR_CONT_Y);
        break;
    case LCD_APP_MTR_CONT_LABEL:
        sprintf(temp_label, "%s", LCD_APP_MTR_CONT_CHARS);
        temp_x = LCD_APP_MTR_CONT_X;
        temp_y = LCD_APP_MTR_CONT_Y;
        temp_shift = LCD_APP_TEMP_MTR_OFFSET;
        LcdDriverClearBoundingBox(LCD_APP_MTR_CONT_X + LCD_APP_TEMP_MTR_OFFSET,
                                  LCD_APP_MTR_CONT_Y,
                                  LCD_DRIVER_BOTTOM_RIGHT_X,
                                  LCD_APP_MTR_THERM_Y);
        break;
    case LCD_APP_MTR_THERM_LABEL:
        sprintf(temp_label, "%s", LCD_APP_MTR_THERM_CHARS);
        temp_x = LCD_APP_MTR_THERM_X;
        temp_y = LCD_APP_MTR_THERM_Y;
        temp_shift = LCD_APP_TEMP_MTR_OFFSET;
        LcdDriverClearBoundingBox(LCD_APP_MTR_CONT_X + LCD_APP_TEMP_MTR_OFFSET,
                                  LCD_APP_MTR_THERM_Y,
                                  LCD_DRIVER_BOTTOM_RIGHT_X,
                                  LCD_DRIVER_BOTTOM_RIGHT_Y);
        break;
    default:
        break;
    }

    LcdDriverDrawText(temp_label, temp_x, temp_y, LCD_APP_TEMP_LABEL_FONT, LCD_APP_TEMP_SPACING);

    // Check digits in temperature data received
    if (temperature_data.temperature == NULL)
    { // temperature not read
        sprintf(temp_str, "--");
        bb = LcdDriverDrawText(
            temp_str, temp_x + temp_shift, temp_y, LCD_APP_TEMP_FONT, LCD_APP_TEMP_SPACING);
    }
    else if (*temperature_data.temperature < 10)
    { // Single digit temperature
        sprintf(temp_str, "%01lu", (unsigned long)*temperature_data.temperature);
        bb = LcdDriverDrawText(
            temp_str, temp_x + temp_shift, temp_y, LCD_APP_TEMP_FONT, LCD_APP_TEMP_SPACING);
    }
    else if (*temperature_data.temperature < 100)
    { // Double digit temperature
        sprintf(temp_str, "%02lu", (unsigned long)*temperature_data.temperature);
        bb = LcdDriverDrawText(
            temp_str, temp_x + temp_shift, temp_y, LCD_APP_TEMP_FONT, LCD_APP_TEMP_SPACING);
    }
    else
    { // Triple digit
        sprintf(temp_str, "%03lu", (unsigned long)*temperature_data.temperature);
        bb = LcdDriverDrawText(
            temp_str, temp_x + temp_shift, temp_y, LCD_APP_TEMP_FONT, LCD_APP_TEMP_SPACING);
    }

    // Draws the Degrees Celsius symbol according to the position of the bounding box
    LcdDriverDrawChar(LCD_APP_TEMP_DEGREES_SYMBOL,
                      bb.x2 + LCD_APP_TEMP_DEGREES_OFFSET_X,
                      temp_y - LCD_APP_TEMP_DEGREES_OFFSET_Y,
                      LCD_APP_TEMP_DEGREES_FONT);
    LcdDriverDrawChar(
        LCD_APP_TEMP_UNITS, bb.x2 + LCD_APP_TEMP_UNITS_OFFSET, temp_y, LCD_APP_TEMP_UNITS_FONT);

    LcdDriverRefresh();
}

/*--------------------------------------------------------------------------
  PAGE 5 (DEBUG PAGE) FUNCTIONS
--------------------------------------------------------------------------*/

/**
 * @brief Displays a battery power bar based on pack current and voltage.
 *
 * @param pack_current The battery pack current.
 * @param pack_voltage The battery pack voltage.
 */
void LcdAppDisplayPowerBar(volatile int16_t* pack_current, volatile uint16_t* pack_voltage)
{
    /* Clear the drawing area (including extra space for the center line) */
    LcdDriverClearBoundingBox(
        LCD_APP_BAR_LEFT, LCD_APP_BAR_TOP, LCD_APP_BAR_RIGHT, LCD_APP_BAR_BOTTOM + 3);

    /* Draw the outline of the power bar */
    LcdDriverDrawRectangle(
        LCD_APP_BAR_LEFT, LCD_APP_BAR_TOP, LCD_APP_BAR_RIGHT, LCD_APP_BAR_BOTTOM, 1);

    // g_diagnostics.cyclic_flags.current_timeout = (pack_current == NULL) ? true : false;
    // g_diagnostics.cyclic_flags.voltage_timeout = (pack_voltage == NULL) ? true : false;

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

/**
 * @brief Displays the speed on the LCD debug page.
 *
 * @param speed The speed value to display.
 * @param units The speed units (LCD_SPEED_UNITS_MPH or LCD_SPEED_UNITS_KPH).
 */
void LcdAppDisplaySpeedDebugPage(volatile uint32_t* speed, volatile uint8_t units)
{
    char speed_str[12];

    /* Clear the previous speed and unit areas */
    LcdDriverClearBoundingBox(LCD_APP_DEBUG_SPEED_THREEDIGIT_X,
                              old_bb_speed.y1,
                              LCD_DRIVER_BOTTOM_RIGHT_X,
                              old_bb_speed.y2 - 3);

    if (speed == NULL)
    { // Stale speed data
        sprintf(speed_str, "XX");
        old_bb_speed = LcdDriverDrawText(speed_str,
                                         LCD_APP_DEBUG_SPEED_TWODIGIT_X + 10,
                                         LCD_APP_DEBUG_SPEED_Y + 10,
                                         LCD_APP_DEBUG_SPEED_FONT,
                                         LCD_APP_DEBUG_SPEED_SPACING + 10);
        // g_diagnostics.cyclic_flags.speed_timeout = true;
    }
    else if (*speed < 10)
    { // Single digit speed
        sprintf(speed_str, "%01lu", (unsigned long)*speed);
        old_bb_speed = LcdDriverDrawText(speed_str,
                                         LCD_APP_DEBUG_SPEED_ONEDIGIT_X,
                                         LCD_APP_DEBUG_SPEED_Y,
                                         LCD_APP_DEBUG_SPEED_FONT,
                                         LCD_APP_DEBUG_SPEED_SPACING);
        // g_diagnostics.cyclic_flags.speed_timeout = false;
    }
    else if (*speed < 100)
    { // Double digit second
        sprintf(speed_str, "%02lu", (unsigned long)*speed);
        old_bb_speed = LcdDriverDrawText(speed_str,
                                         LCD_APP_DEBUG_SPEED_TWODIGIT_X,
                                         LCD_APP_DEBUG_SPEED_Y,
                                         LCD_APP_DEBUG_SPEED_FONT,
                                         LCD_APP_DEBUG_SPEED_SPACING);
        // g_diagnostics.cyclic_flags.speed_timeout = false;
    }
    else
    {
        sprintf(speed_str, "%03lu", (unsigned long)*speed);
        old_bb_speed = LcdDriverDrawText(speed_str,
                                         LCD_APP_DEBUG_SPEED_THREEDIGIT_X,
                                         LCD_APP_DEBUG_SPEED_Y,
                                         LCD_APP_DEBUG_SPEED_FONT,
                                         LCD_APP_DEBUG_SPEED_SPACING);
        // g_diagnostics.cyclic_flags.speed_timeout = false;
    }

    /* Draw the speed units */
    switch (units)
    {
    case LCD_APP_KPH:
        LcdDriverDrawText("kph",
                          LCD_APP_DEBUG_SPEED_X + LCD_APP_DEBUG_SPEED_UNIT_KPH_X,
                          LCD_APP_DEBUG_SPEED_UNIT_Y,
                          LCD_APP_DEBUG_SPEED_UNITS_FONT,
                          LCD_APP_DEBUG_SPEED_UNITS_SPACING);
        break;
    case LCD_APP_MPH:
        LcdDriverDrawText("mph",
                          LCD_APP_DEBUG_SPEED_X + LCD_APP_DEBUG_SPEED_UNIT_MPH_X,
                          LCD_APP_DEBUG_SPEED_UNIT_Y,
                          LCD_APP_DEBUG_SPEED_UNITS_FONT,
                          LCD_APP_DEBUG_SPEED_UNITS_SPACING);
        break;
    default:
        LcdDriverDrawText("xxx",
                          LCD_APP_DEBUG_SPEED_X + LCD_APP_DEBUG_SPEED_UNIT_MPH_X,
                          LCD_APP_DEBUG_SPEED_UNIT_Y,
                          LCD_APP_DEBUG_SPEED_UNITS_FONT,
                          LCD_APP_DEBUG_SPEED_UNITS_SPACING);
        break;
    }

    LcdDriverRefresh();
}

/**
 * @brief Displays the state of charge (SOC) on the LCD debug page.
 *
 * @param soc The state of charge (in percent).
 */
void LcdAppDisplaySocDebugPage(volatile uint32_t* soc)
{
    char soc_str[12];

    LcdDriverClearBoundingBox(old_bb_soc.x1, old_bb_soc.y1, old_bb_soc.x2, old_bb_soc.y2 - 5);

    // Check for stale data and display "--" if so.
    if (soc == NULL)
    {
        sprintf(soc_str, "--");
        old_bb_soc = LcdDriverDrawText(soc_str,
                                       LCD_APP_DEBUG_SOC_TWODIGIT_X,
                                       LCD_APP_SOC_Y,
                                       LCD_APP_DEBUG_SOC_FONT,
                                       LCD_APP_DEBUG_SOC_SPACING);
        // g_diagnostics.cyclic_flags.soc_timeout = true;
    }
    else if (*soc < 10)
    {
        sprintf(soc_str, "%01lu", (unsigned long)*soc);
        old_bb_soc = LcdDriverDrawText(soc_str,
                                       LCD_APP_DEBUG_SOC_ONEDIGIT_X,
                                       LCD_APP_DEBUG_SOC_Y,
                                       LCD_APP_DEBUG_SOC_FONT,
                                       LCD_APP_DEBUG_SOC_SPACING);
        // g_diagnostics.cyclic_flags.soc_timeout = false;
    }
    else if (*soc < 100)
    {
        sprintf(soc_str, "%02lu", (unsigned long)*soc);
        old_bb_soc = LcdDriverDrawText(soc_str,
                                       LCD_APP_DEBUG_SOC_TWODIGIT_X,
                                       LCD_APP_DEBUG_SOC_Y,
                                       LCD_APP_DEBUG_SOC_FONT,
                                       LCD_APP_DEBUG_SOC_SPACING);
        // g_diagnostics.cyclic_flags.soc_timeout = false;
    }
    else
    {
        sprintf(soc_str, "%03lu", (unsigned long)*soc);
        old_bb_soc = LcdDriverDrawText(soc_str,
                                       LCD_APP_DEBUG_SOC_THREEDIGIT_X,
                                       LCD_APP_DEBUG_SOC_Y,
                                       LCD_APP_DEBUG_SOC_FONT,
                                       LCD_APP_DEBUG_SOC_SPACING);
        // g_diagnostics.cyclic_flags.soc_timeout = false;
    }

    LcdDriverDrawChar(LCD_APP_SOC_UNITS,
                      LCD_APP_DEBUG_SOC_UNITS_X,
                      LCD_APP_DEBUG_SOC_Y,
                      LCD_APP_DEBUG_SOC_UNITS_FONT);

    LcdDriverRefresh();
}

/**
 * @brief Displays the drive state on the LCD debug page.
 *
 * @param state The drive state (e.g., FORWARD_STATE, PARK_STATE, REVERSE_STATE).
 */
void LcdAppDisplayDriveStateDebugPage(volatile drive_state_t* state)
{
    char state_str[2] = {LCD_APP_ERROR_SYMBOL, '\0'}; // Default to error symbol.

    if (state == NULL)
    { // Stale data for drive state
        sprintf(state_str, "-");
        // g_diagnostics.cyclic_flags.drive_state_timeout = true;
    }
    else
    {
        switch (*state)
        {
        case LCD_APP_FORWARD_STATE:
            state_str[0] = LCD_APP_FORWARD_SYMBOL;
            break;
        case LCD_APP_PARK_STATE:
            state_str[0] = LCD_APP_PARK_SYMBOL;
            break;
        case LCD_APP_REVERSE_STATE:
            state_str[0] = LCD_APP_REVERSE_SYMBOL;
            break;
        default:
            state_str[0] = LCD_APP_ERROR_SYMBOL;
            break;
        }
        // g_diagnostics.cyclic_flags.drive_state_timeout = false;
    }

    LcdDriverClearBoundingBox(
        LCD_APP_DEBUG_STATE_X, LCD_APP_DEBUG_STATE_Y, 20, LCD_DRIVER_BOTTOM_RIGHT_Y);
    old_bb_drive_state = LcdDriverDrawText(state_str,
                                           LCD_APP_DEBUG_STATE_X,
                                           LCD_APP_DEBUG_STATE_Y,
                                           LCD_APP_DEBUG_STATE_FONT,
                                           LCD_APP_DEBUG_STATE_SPACING);

    LcdDriverRefresh();
}

/*
 * @brief CAN rx function which parses message data needed by the LCD
 *
 * @param msg_id 	The id of the CAN message
 * @param data  	The data of the CAN message
 */
void LcdAppCanRxHandle(uint32_t msg_id, uint8_t* data)
{
    // if (msg_id == CAN_ID_PACK_CURRENT)
    // {
    //     int16_t tmp_pack_current = (data[1] << 8) | (data[0]);
    //     tmp_pack_current /= 65.535;
    //     set_cyclic_pack_current(tmp_pack_current);

    //     g_pack_current_soc = tmp_pack_current;
    // }

    // if (msg_id == CAN_ID_PACK_VOLTAGE)
    // {
    //     uint16_t tmp_pack_voltage = (data[1] << 8) | (data[0]);
    //     tmp_pack_voltage /= PACK_VOLTAGE_DIVISOR;
    //     set_cyclic_pack_voltage(tmp_pack_voltage);

    //     g_total_pack_voltage_soc = tmp_pack_voltage;

    //     osEventFlagsSet(calculate_soc_flagHandle, SOC_CALCULATE_ON);
    // }

    // if (msg_id == STR_CAN_MSG_ID)
    // {
    //     uint8_t next_page = (data[0] & 1);

    //     if (next_page)
    //     {
    //         if (g_LCD_page < LCD_APP_MAXPAGES)
    //         {
    //             g_LCD_page_change = 1;
    //             g_LCD_page++;
    //         }
    //         else
    //         {
    //             g_LCD_page_change = 1;
    //             g_LCD_page = 1;
    //         }
    //     }
    //     //    	else if(previous_page){
    //     //    		if(g_LCD_page > 1){
    //     //    			g_LCD_page_change = 1;
    //     //    			g_LCD_page--;
    //     //			}
    //     //    	}
    // }

    // if (msg_id == CAN_ID_MDI_TEMP)
    // {
    //     uint8_t temperature = data[0];
    //     set_cyclic_temperature(temperature);
    // }
}
