#ifndef LCD_GRAPHICS_H
#define LCD_GRAPHICS_H

/**
 * References the library: https://github.com/mberntsen/STM32-Libraries
 */

#include "drive_state.h"
#include "font_verdana.h"
#include "spi.h"
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
#define LCD_APP_PARK_STATE 0x03
#define LCD_APP_PARK_SYMBOL 'P'
#define LCD_APP_REVERSE_STATE 0x04
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

#define LCD_APP_MPPT_A_LABEL 0x1
#define LCD_APP_MPPT_A_CHARS "PTA:"
#define LCD_APP_MPPT_A_X 0
#define LCD_APP_MPPT_A_Y 0

#define LCD_APP_MPPT_B_LABEL 0x2
#define LCD_APP_MPPT_B_CHARS "PTB:"
#define LCD_APP_MPPT_B_X 0
#define LCD_APP_MPPT_B_Y 16

#define LCD_APP_MPPT_C_LABEL 0x3
#define LCD_APP_MPPT_C_CHARS "PTC:"
#define LCD_APP_MPPT_C_X 0
#define LCD_APP_MPPT_C_Y 32

#define LCD_APP_MPPT_D_LABEL 0x4
#define LCD_APP_MPPT_D_CHARS "PTD:"
#define LCD_APP_MPPT_D_X 0
#define LCD_APP_MPPT_D_Y 48

#define LCD_APP_BATT_MAX_LABEL 0x5
#define LCD_APP_BATT_MAX_CHARS "BMAX:"
#define LCD_APP_BATT_MAX_X 62
#define LCD_APP_BATT_MAX_Y 0

#define LCD_APP_BATT_MIN_LABEL 0x6
#define LCD_APP_BATT_MIN_CHARS "BMIN:"
#define LCD_APP_BATT_MIN_X 62
#define LCD_APP_BATT_MIN_Y 16

#define LCD_APP_MTR_CONT_LABEL 0x7
#define LCD_APP_MTR_CONT_CHARS "MC:"
#define LCD_APP_MTR_CONT_X 62
#define LCD_APP_MTR_CONT_Y 32

#define LCD_APP_MTR_THERM_LABEL 0x8
#define LCD_APP_MTR_THERM_CHARS "MT:"
#define LCD_APP_MTR_THERM_X 62
#define LCD_APP_MTR_THERM_Y 48

/** Debug Page */
#define LCD_APP_MAX_POSITIVE_POWER 5400.0f
#define LCD_APP_MAX_NEGATIVE_POWER 3000.0f // use the absolute value for negative power
#define LCD_APP_BAR_LEFT 0
#define LCD_APP_BAR_TOP 0
#define LCD_APP_BAR_BOTTOM 15
#define LCD_APP_BAR_RIGHT BOTTOM_RIGHT_X
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

/** LCD Screen Constants */
#define LCD_APP_MAXPAGES 5

#define LCD_APP_LCD_UPDATE_DELAY 200

/*	Datatypes */
typedef struct
{
    volatile uint32_t* speed;
    volatile uint8_t speed_units;
    volatile int16_t* pack_current;
    volatile uint16_t* pack_voltage;
    volatile uint8_t* drive_state;
    volatile uint8_t* soc;
    volatile uint8_t drive_mode;
} LcdAppData;

typedef struct
{
    volatile uint8_t* temperature;
    uint8_t temp_label;
} LcdAppTemperature;

// typedef struct {
//	volatile uint8_t* mppt_a_temperature;
//	volatile uint8_t* mppt_b_temperature;
//	volatile uint8_t* mppt_c_temperature;
//	volatile uint8_t* mppt_d_temperature;
//	volatile uint8_t* batt_min_temperature;
//	volatile uint8_t* batt_max_temperature;
//	volatile uint8_t* motor_cont_temperature;
//	volatile uint8_t* motor_therm_temperature;
// }; LcdAppTemperature;

typedef struct
{
    volatile bool battery_fault;
    volatile bool supp_lo;
    volatile bool voltage_high;
    volatile bool voltage_low;
    volatile bool slave_board_comm_fault;
    volatile bool overvolt_fault;
    volatile bool undervolt_fault;
    volatile bool overtemp_fault;
    volatile bool charge_overcurrent_fault;
    volatile bool discharge_overcurrent_fault;
    volatile bool reset_from_watchdog;
} LcdAppBattFaults;

typedef struct
{
    volatile bool motor_system_error;
    volatile bool overcurrent_fault;
    volatile bool overvoltage_fault;
    volatile bool fet_thermistor_error;
    volatile bool motor_comm_fault;
    volatile bool throttle_adc_outofrange;
    volatile bool throttle_adc_mismatch;
} LcdAppMotorFaults;

typedef struct
{
    volatile bool low_volt_warning;
    volatile bool high_volt_warning;
    volatile bool low_temp_warning;
    volatile bool high_temp_warning;
    volatile bool no_ecu_message;
    volatile bool pack_overdischarge;
    volatile bool pack_overcharge;
} LcdAppWarnings;

/*	User Variables	*/
extern LcdAppData g_lcd_data;
extern LcdAppBattFaults g_lcd_batt_faults;
extern LcdAppMotorFaults g_lcd_motor_faults;
extern LcdAppWarnings g_lcd_warnings;
extern LcdAppTemperature g_lcd_temperatures[8];
extern uint8_t g_LCD_page;
extern uint8_t g_LCD_page_change;

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
void LcdAppDisplayDriveStateDrivePage(volatile drive_state_t* state);

/**
 * @brief Displays the state of charge (SOC) on the LCD.
 *
 * @param soc The state of charge (in percent).
 */
void LcdAppDisplaySOCDrivePage(volatile uint32_t* soc);

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
 * @brief Displays the drive state on the LCD.
 *
 * @param state The drive state (e.g., FORWARD_STATE, PARK_STATE, REVERSE_STATE).
 */
void LcdAppDisplayDriveStateDebugPage(volatile drive_state_t* state);

/**
 * @brief Displays the state of charge (SOC) on the LCD.
 *
 * @param soc The state of charge (in percent).
 */
void LcdAppDisplaySocDebugPage(volatile uint32_t* soc);

/**
 * @brief Changes the screen
 */
void LcdDriverChangeScreen();

/*
 * @brief CAN rx function which parses message data needed by the LCD
 *
 * @param msg_id 	The id of the CAN message
 * @param data  	The data of the CAN message
 */
void LcdAppCanRxHandle(uint32_t msg_id, uint8_t* data);

/**
 * @brief Initializes the LCD and SPI interface.
 *
 * @param hspi Pointer to the SPI handle.
 */
void LcdDriverInit(SPI_HandleTypeDef* hspi);

#endif // LCD_GRAPHICS_H
