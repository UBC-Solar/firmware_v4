/**
 * @file    lcd_app.h
 * @brief   LCD application header file for UBC Solar DRD board
 *
 * This header declares the data structures, constants, and function prototypes for the LCD application. 
 * The module implements a controller to handle what is displayed on each page and handles the page transitions.
 *
 * @author  Gregory Bian
 * @date    Feb 4 2026
 */


#ifndef __LCD_APP_H
#define __LCD_APP_H

/**
 * References the library: https://github.com/mberntsen/STM32-Libraries
 */

// #include "drive_state.h"
#include "font_verdana.h"
#include "lcd_handler.h"
#include "drive_state.h"
#include "stdbool.h"
#include "stdint.h"
#include <main.h>

/** Drive Page */
#define LCD_APP_SPEED_FONT (Verdana48_digits)
#define LCD_APP_SPEED_NULL_FONT (Verdana32)
#define LCD_APP_SPEED_X 35
#define LCD_APP_SPEED_ONEDIGIT_X 82
#define LCD_APP_SPEED_TWODIGIT_X 52
#define LCD_APP_SPEED_THREEDIGIT_X 40
#define LCD_APP_SPEED_Y 7
#define LCD_APP_SPEED_SPACING -3 // ChatGPT generated font has too much padding
#define LCD_APP_SPEED_UNIT_KPH_X 76
#define LCD_APP_SPEED_UNIT_MPH_X 72
#define LCD_APP_SPEED_UNIT_Y 1
#define LCD_APP_MPH 1
#define LCD_APP_KPH 0
#define LCD_APP_SPEED_UNITS_FONT (Verdana8)
#define LCD_APP_SPEED_UNITS_SPACING 1

#define LCD_APP_SOC_FONT (Verdana16)
#define LCD_APP_SOC_ONEDIGIT_X 13
#define LCD_APP_SOC_TWODIGIT_X 3
#define LCD_APP_SOC_THREEDIGIT_X 0
#define LCD_APP_SOC_Y 0
#define LCD_APP_SOC_SPACING 1
#define LCD_APP_SOC_UNITS_FONT (Verdana8)
#define LCD_APP_SOC_UNITS_X 27
#define LCD_APP_SOC_UNITS '%'

#define LCD_APP_ECO_MODE_X 12
#define LCD_APP_ECO_MODE_Y 24
#define LCD_APP_ECO_MODE_FONT (Verdana12)
#define LCD_APP_ECO_SYMBOL 'E'
#define LCD_APP_POWER_SYMBOL '~'
#define LCD_APP_POWER_MODE_X 9
#define LCD_APP_POWER_MODE_Y 20
#define LCD_APP_POWER_MODE_FONT (Webdings14)
#define LCD_APP_DRIVE_MODE_ECO 1 // ECO Mode is GPIO high (logic 1) for MDI to MC.
#define LCD_APP_DRIVE_MODE_POWER 0

#define LCD_APP_STATE_X 9
#define LCD_APP_STATE_Y 46
#define LCD_APP_STATE_FONT (Verdana16)
#define LCD_APP_FORWARD_STATE 0x01
#define LCD_APP_FORWARD_SYMBOL 'D'
#define LCD_APP_PARK_STATE 0x02
#define LCD_APP_PARK_SYMBOL 'P'
#define LCD_APP_REVERSE_STATE 0x03
#define LCD_APP_REVERSE_SYMBOL 'R'
#define LCD_APP_ERROR_SYMBOL 'X'
#define LCD_APP_STATE_SPACING 1

#define LCD_APP_FAULT_X 55
#define LCD_APP_FAULT_Y 0
#define LCD_APP_FAULT_SYMBOL_FONT (Wingdings)
#define LCD_APP_FAULT_SYMBOL 'N'

#define LCD_APP_WARNING_X 79
#define LCD_APP_WARNING_Y -4
#define LCD_APP_WARNING_SYMBOL_FONT (Webdings14)
#define LCD_APP_WARNING_SYMBOL 'x'

/** Fault Page */
#define LCD_APP_FAULT_SPACING 1

#define LCD_APP_FAULT_LABEL_FONT (Verdana12)
#define LCD_APP_FAULT_LABEL_X 0
#define LCD_APP_FAULT_LABEL_Y 0
#define LCD_APP_FAULT_LABEL_CHARS "FAULTS"

#define LCD_APP_FAULT_LABEL_UNDERLINE_X 57
#define LCD_APP_FAULT_LABEL_UNDERLINE_Y 14

#define LCD_APP_FAULT_FOUR_FONT (Verdana12)
#define LCD_APP_FAULT_FOUR_X1 0
#define LCD_APP_FAULT_FOUR_Y1 18
#define LCD_APP_FAULT_FOUR_X2 0
#define LCD_APP_FAULT_FOUR_Y2 34
#define LCD_APP_FAULT_FOUR_X3 0
#define LCD_APP_FAULT_FOUR_Y3 49

#define LCD_APP_FAULT_EIGHT_FONT (Verdana8)
#define LCD_APP_FAULT_EIGHT_X1 0
#define LCD_APP_FAULT_EIGHT_Y1 16
#define LCD_APP_FAULT_EIGHT_X2 64
#define LCD_APP_FAULT_EIGHT_Y2 16
#define LCD_APP_FAULT_EIGHT_X3 0
#define LCD_APP_FAULT_EIGHT_Y3 28
#define LCD_APP_FAULT_EIGHT_X4 64
#define LCD_APP_FAULT_EIGHT_Y4 28
#define LCD_APP_FAULT_EIGHT_X5 0
#define LCD_APP_FAULT_EIGHT_Y5 40
#define LCD_APP_FAULT_EIGHT_X6 64
#define LCD_APP_FAULT_EIGHT_Y6 40
#define LCD_APP_FAULT_EIGHT_X7 0
#define LCD_APP_FAULT_EIGHT_Y7 52
#define LCD_APP_FAULT_EIGHT_X8 64
#define LCD_APP_FAULT_EIGHT_Y8 52

#define LCD_APP_BATT_FLT_CHARS "BAT_FLT"
#define LCD_APP_BATT_SELFTEST_FLT_CHARS "SELF_TST"
#define LCD_APP_BATT_SUPPLO_FLT_CHARS "SUPP_LOW"
#define LCD_APP_BATT_VOLTHIGH_FLT_CHARS "VOLT_HIGH"
#define LCD_APP_BATT_VOLTLOW_FLT_CHARS "VOLT_LOW"
#define LCD_APP_BATT_SLAVE_COMM_FLT_CHARS "SLV_COMM"
#define LCD_APP_BATT_OVERVOLT_FLT_CHARS "BAT_OVLT"
#define LCD_APP_BATT_UNDERVOLT_FLT_CHARS "BAT_UVLT"
#define LCD_APP_BATT_OVERTEMP_FLT_CHARS "BAT_OTMP"
#define LCD_APP_BATT_CHARGE_OC_FLT_CHARS "BAT_COC"
#define LCD_APP_BATT_DISCHARGE_OC_FLT_CHARS "BAT_DCOC"
#define LCD_APP_BATT_RST_FROM_WATCH_FLT_CHARS "BAT_RFW"

#define LCD_APP_MTR_SYSTEM_FLT_CHARS "MTR_SYS"
#define LCD_APP_MTR_OVERCURR_FLT_CHARS "MTR_OCUR"
#define LCD_APP_MTR_OVERVOLT_FLT_CHARS "MTR_OVLT"
#define LCD_APP_MTR_OVERTEMP_FLT_CHARS "MTR_OTMP"
#define LCD_APP_MTR_COMM_FLT_CHARS "MTR_COMM"
#define LCD_APP_MTR_THROT_ADC_OOR_FLT_CHARS "THRT_OOR"
#define LCD_APP_MTR_THROT_ADC_MISMATCH_FLT_CHARS "THRT_MSM"

/** Warning Page */
#define LCD_APP_WARNING_SPACING 1

#define LCD_APP_WARNING_LABEL_FONT (Verdana12)
#define LCD_APP_WARNING_LABEL_X 0
#define LCD_APP_WARNING_LABEL_Y 0
#define LCD_APP_WARNING_LABEL_CHARS "WARNINGS"

#define LCD_APP_WARNING_LABEL_UNDERLINE_X 85
#define LCD_APP_WARNING_LABEL_UNDERLINE_Y 14

#define LCD_APP_WARNING_FOUR_FONT (Verdana12)
#define LCD_APP_WARNING_FOUR_X1 0
#define LCD_APP_WARNING_FOUR_Y1 18
#define LCD_APP_WARNING_FOUR_X2 0
#define LCD_APP_WARNING_FOUR_Y2 34
#define LCD_APP_WARNING_FOUR_X3 0
#define LCD_APP_WARNING_FOUR_Y3 49

#define LCD_APP_WARNING_EIGHT_FONT (Verdana8)
#define LCD_APP_WARNING_EIGHT_X1 0
#define LCD_APP_WARNING_EIGHT_Y1 16
#define LCD_APP_WARNING_EIGHT_X2 64
#define LCD_APP_WARNING_EIGHT_Y2 16
#define LCD_APP_WARNING_EIGHT_X3 0
#define LCD_APP_WARNING_EIGHT_Y3 28
#define LCD_APP_WARNING_EIGHT_X4 64
#define LCD_APP_WARNING_EIGHT_Y4 28
#define LCD_APP_WARNING_EIGHT_X5 0
#define LCD_APP_WARNING_EIGHT_Y5 40
#define LCD_APP_WARNING_EIGHT_X6 64
#define LCD_APP_WARNING_EIGHT_Y6 40
#define LCD_APP_WARNING_EIGHT_X7 0
#define LCD_APP_WARNING_EIGHT_Y7 52
#define LCD_APP_WARNING_EIGHT_X8 64
#define LCD_APP_WARNING_EIGHT_Y8 52

#define LCD_APP_LOWVOLT_WARN_CHARS "LOW_VOLT"
#define LCD_APP_HIGHVOLT_WARN_CHARS "HIGH_VOLT"
#define LCD_APP_LOWTEMP_WARN_CHARS "LOW_TEMP"
#define LCD_APP_HIGHTEMP_WARN_CHARS "HIGH_TEMP"
#define LCD_APP_NOMSG_WARN_CHARS "NO_MSG"
#define LCD_APP_PACK_OC_WARN_CHARS "PACK_OC"
#define LCD_APP_PACK_OD_WARN_CHARS "PACK_ODC"

/** Temperature Page */
#define LCD_APP_TEMP_FONT (Verdana12)
#define LCD_APP_TEMP_LABEL_FONT (Verdana8)
#define LCD_APP_TEMP_SPACING 1
#define LCD_APP_TEMP_MPPT_OFFSET 26
#define LCD_APP_TEMP_BATT_OFFSET 34
#define LCD_APP_TEMP_MTR_OFFSET 23
#define LCD_APP_TEMP_UNITS_FONT (Verdana8)
#define LCD_APP_TEMP_UNITS 'C'
#define LCD_APP_TEMP_UNITS_OFFSET 7
#define LCD_APP_TEMP_DEGREES_FONT (Custom)
#define LCD_APP_TEMP_DEGREES_SYMBOL 0xB0 // Hex ASCII value for °
#define LCD_APP_TEMP_DEGREES_OFFSET_X 2
#define LCD_APP_TEMP_DEGREES_OFFSET_Y 2

#define LCD_APP_MPPT_A_CHARS "PTA:"
#define LCD_APP_MPPT_A_X 0
#define LCD_APP_MPPT_A_Y 0

#define LCD_APP_MPPT_B_CHARS "PTB:"
#define LCD_APP_MPPT_B_X 0
#define LCD_APP_MPPT_B_Y 16

#define LCD_APP_MPPT_C_CHARS "PTC:"
#define LCD_APP_MPPT_C_X 0
#define LCD_APP_MPPT_C_Y 32

#define LCD_APP_MPPT_D_CHARS "PTD:"
#define LCD_APP_MPPT_D_X 0
#define LCD_APP_MPPT_D_Y 48

#define LCD_APP_BATT_MAX_CHARS "BMAX:"
#define LCD_APP_BATT_MAX_X 62
#define LCD_APP_BATT_MAX_Y 0

#define LCD_APP_BATT_MIN_CHARS "BMIN:"
#define LCD_APP_BATT_MIN_X 62
#define LCD_APP_BATT_MIN_Y 16

#define LCD_APP_MTR_CONT_CHARS "MC:"
#define LCD_APP_MTR_CONT_X 62
#define LCD_APP_MTR_CONT_Y 32

#define LCD_APP_MTR_THERM_CHARS "MT:"
#define LCD_APP_MTR_THERM_X 62
#define LCD_APP_MTR_THERM_Y 48

/** Debug Page */
#define LCD_APP_MAX_POSITIVE_POWER 5400.0f
#define LCD_APP_MAX_NEGATIVE_POWER 3000.0f // use the absolute value for negative power
#define LCD_APP_BAR_LEFT 0
#define LCD_APP_BAR_TOP 0
#define LCD_APP_BAR_BOTTOM 15
#define LCD_APP_BAR_RIGHT LCD_DRIVER_BOTTOM_RIGHT_X
#define LCD_APP_CENTER_X 43

#define LCD_APP_DEBUG_SPEED_FONT (Verdana32)
#define LCD_APP_DEBUG_SPEED_X 35
#define LCD_APP_DEBUG_SPEED_ONEDIGIT_X 84
#define LCD_APP_DEBUG_SPEED_TWODIGIT_X 57
#define LCD_APP_DEBUG_SPEED_THREEDIGIT_X 42
#define LCD_APP_DEBUG_SPEED_Y 22
#define LCD_APP_DEBUG_SPEED_SPACING 1
#define LCD_APP_DEBUG_SPEED_UNIT_KPH_X 76
#define LCD_APP_DEBUG_SPEED_UNIT_MPH_X 72
#define LCD_APP_DEBUG_SPEED_UNIT_Y 22
#define LCD_APP_DEBUG_SPEED_UNITS_FONT (Verdana8)
#define LCD_APP_DEBUG_SPEED_UNITS_SPACING 1

#define LCD_APP_DEBUG_SOC_FONT (Verdana16)
#define LCD_APP_DEBUG_SOC_ONEDIGIT_X 13
#define LCD_APP_DEBUG_SOC_TWODIGIT_X 3
#define LCD_APP_DEBUG_SOC_THREEDIGIT_X 0
#define LCD_APP_DEBUG_SOC_Y 23
#define LCD_APP_DEBUG_SOC_SPACING 1
#define LCD_APP_DEBUG_SOC_UNITS_X 27
#define LCD_APP_DEBUG_SOC_UNITS_FONT (Verdana8)

#define LCD_APP_DEBUG_ECO_MODE_X 12
#define LCD_APP_DEBUG_ECO_MODE_Y 24
#define LCD_APP_DEBUG_ECO_MODE_FONT (Verdana12)
#define LCD_APP_DEBUG_POWER_MODE_X 9
#define LCD_APP_DEBUG_POWER_MODE_Y 20
#define LCD_APP_DEBUG_POWER_MODE_FONT (Webdings14)

#define LCD_APP_DEBUG_STATE_X 9
#define LCD_APP_DEBUG_STATE_Y 45
#define LCD_APP_DEBUG_STATE_FONT (Verdana16)
#define LCD_APP_DEBUG_STATE_SPACING 1

/**
 * @brief Checks and updates the faults.
 *
 * @param batt_faults A struct containing the battery faults.
 * @param motor_faults A struct containing the motor faults.
 * @return The count of faults.
 */
uint8_t LcdAppCheckFaults(LcdAppBattFaults* batt_faults, LcdAppMotorFaults* motor_faults);
/**
 * @brief Checks and updates the warnings.
 *
 * @param warnings A struct containing the warnings
 *
 * @return The count of warnings.
 */
uint8_t LcdAppCheckWarnings(LcdAppWarnings* warnings);
/**
 * @brief Displays the speed on the LCD.
 *
 * @param speed The speed value to display.
 * @param units The speed units (LCD_SPEED_UNITS_MPH or LCD_SPEED_UNITS_KPH).
 */
void LcdAppDisplaySpeedDrivePage(volatile uint32_t* speed, volatile uint8_t units);

/**
 * @brief Displays the drive state on the LCD.
 *
 * @param state The drive state (e.g., FORWARD_STATE, PARK_STATE, REVERSE_STATE).
 */
void LcdAppDisplayDriveStateDrivePage(volatile DriveStateStates* state);

/**
 * @brief Displays the state of charge (SOC) on the LCD.
 *
 * @param soc The state of charge (in percent).
 */
void LcdAppDisplaySocDrivePage(volatile uint32_t* soc);

/**
 * @brief Displays a battery power bar based on pack current and voltage.
 *
 * @param pack_current The battery pack current.
 * @param pack_voltage The battery pack voltage.
 */
void LcdAppDisplayPowerBar(volatile int16_t* pack_current, volatile uint16_t* pack_voltage);

/**
 * @brief Displays an E for ECO mode and P for POWER mode
 *
 * @param drive_mode The drive mode
 */
void LcdAppDisplayDriveModeDrivePage(volatile uint8_t drive_mode);

/**
 * @brief Displays a Temperature on the LCD (0-255)
 *
 * @param temperature A struct containing the temperature and id of the temperature.
 */
void LcdAppDisplayTemperature(LcdAppTemperature temperature_data);

/**
 * @brief Displays a fault indicator on the LCD Drive Page
 *
 * @param fault_indicator A general indicator to signal a fault to prompt the driver to change pages
 */
void LcdAppDisplayFaultIndicator(LcdAppBattFaults* batt_faults, LcdAppMotorFaults* motor_faults);

/**
 * @brief Displays a warning indicator on the LCD Drive Page
 *
 * @param warning_indicator A general indicator to signal a warning to prompt the driver to change
 * pages
 */
void LcdAppDisplayWarningIndicator(LcdAppWarnings* warnings);

/**
 * @brief Dynamically displays battery and motor faults on the LCD
 *
 * @param batt_faults The battery faults to be displayed on the LCD
 * @param motor_faults The motor faults to be displayed on the LCD
 */
void LcdAppDisplayFaults(LcdAppBattFaults* batt_faults, LcdAppMotorFaults* motor_faults);

/**
 * @brief Displays a motor faults on the LCD
 *
 * @param fault_indicator An indicator to see who
 */
void LcdAppDisplayWarnings(LcdAppWarnings* warnings);

/**
 * @brief Displays the speed on the LCD.
 *
 * @param speed The speed value to display.
 * @param units The speed units (LCD_SPEED_UNITS_MPH or LCD_SPEED_UNITS_KPH).
 */
void LcdAppDisplaySpeedDebugPage(volatile uint32_t* speed, volatile uint8_t units);


/**
 * @brief Displays the state of charge (SOC) on the LCD.
 *
 * @param soc The state of charge (in percent).
 */
void LcdAppDisplaySocDebugPage(volatile uint32_t* soc);

/**
 * @brief Displays the drive state on the LCD debug page.
 *
 * @param state The drive state (e.g., FORWARD_STATE, PARK_STATE, REVERSE_STATE).
 */
void LcdAppDisplayDriveStateDebugPage(volatile DriveStateStates* state);



#endif // LCD_GRAPHICS_H