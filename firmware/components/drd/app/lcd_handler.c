/**
 * @file    lcd_handler.c
 * @brief   LCD handler implementation for UBC Solar DRD board
 *
 * This file contains the implementation of the LCD handler functions for the DRD board.
 *
 * @author  Gregory Bian
 * @date    Feb 4 2026
 */

#include "lcd_handler.h"
#include "lcd_test.h"
#include "lcd_app.h"
#include "lcd_driver.h"
#include "cyclic_data_handler.h"
#include <stdbool.h>

/* STATIC VARIABLES */
static LcdAppData g_lcd_data = {0};
static LcdAppBattFaults g_lcd_batt_faults = {0};
static LcdAppMotorFaults g_lcd_motor_faults = {0};
static LcdAppWarnings g_lcd_warnings = {0};
static LcdAppTemperature g_lcd_temperatures[8] = {0};
static uint8_t g_lcd_page = 1;
static bool g_str_change_page_flag = false;
static volatile bool g_prev_change_page = true;
static volatile bool g_prev_fault = false;

/* STATIC FUNCTION DECLARATION*/
/**
 * @brief Gets relevant data for LCD controller to parse
 */
static void LcdHandlerGetData(void);

void LcdHandlerInit(SPI_HandleTypeDef* hspi)
{
    // Initialize the temperature labels for each temperature struct in the array
    g_lcd_temperatures[0].temp_label = MPPTA;
    g_lcd_temperatures[1].temp_label = MPPTB;
    g_lcd_temperatures[2].temp_label = MPPTC;
    g_lcd_temperatures[3].temp_label = MPPTD;
    g_lcd_temperatures[4].temp_label = BATT_MIN;
    g_lcd_temperatures[5].temp_label = BATT_MAX;
    g_lcd_temperatures[6].temp_label = MOTOR_CONT;
    g_lcd_temperatures[7].temp_label = MOTOR_THERM;

    g_lcd_data.speed_units = LCD_APP_MPH;

    LcdDriverInit(hspi); // Initialize the LCD driver
}

void LcdHandlerPageController(void)
{
    // Get the latest data for the LCD
    LcdHandlerGetData();

    // Fault Logic - Trigger Page Change on rising edge of fault detection
    bool check_fault = LcdAppCheckFaults(&g_lcd_batt_faults, &g_lcd_motor_faults);
    // Page Logic - Trigger Page Change on flag set by CAN message
    if(g_str_change_page_flag){
        // Check if page is above 5 pages
        if (g_lcd_page < LCD_HANDLER_MAXPAGES) {
            LcdDriverChangeScreen();
            g_lcd_page++;
        }
        else { // Go to first page if exceed max pages
            LcdDriverChangeScreen();
            g_lcd_page = 1;
        }
        g_str_change_page_flag = false;
    }
    if(check_fault && !g_prev_fault) {
        LcdDriverChangeScreen();
        g_lcd_page = FAULTS_PAGE;
    }
    g_prev_fault = check_fault;

#ifdef LCD_TEST
    LcdTestInit();
#endif

    // Handles what is displayed
    switch (g_lcd_page)
    {
    case DRIVE_PAGE:
        LcdAppDisplaySpeedDrivePage(g_lcd_data.speed, g_lcd_data.speed_units);
        LcdAppDisplaySocDrivePage((volatile uint32_t*)g_lcd_data.soc);
        LcdAppDisplayDriveModeDrivePage(g_lcd_data.drive_mode);
        LcdAppDisplayDriveStateDrivePage((volatile DriveStateStates*) g_lcd_data.drive_state);
        LcdAppDisplayFaultIndicator(&g_lcd_batt_faults, &g_lcd_motor_faults);
        LcdAppDisplayWarningIndicator(&g_lcd_warnings);
        break;
    case FAULTS_PAGE:
        LcdAppDisplayFaults(&g_lcd_batt_faults, &g_lcd_motor_faults);
        break;
    case WARNINGS_PAGE:
        LcdAppDisplayWarnings(&g_lcd_warnings);
        break;
    case TEMPERATURE_PAGE:
        g_lcd_temperatures[MPPTA].temperature = CyclicDataGetMpptATemperature();
        g_lcd_temperatures[MPPTB].temperature = CyclicDataGetMpptBTemperature();
        g_lcd_temperatures[MPPTC].temperature = CyclicDataGetMpptCTemperature();
        g_lcd_temperatures[MPPTD].temperature = CyclicDataGetMpptDTemperature();
        g_lcd_temperatures[BATT_MIN].temperature = CyclicDataGetBatteryMinTemperature();
        g_lcd_temperatures[BATT_MAX].temperature = CyclicDataGetBatteryMaxTemperature();
        g_lcd_temperatures[MOTOR_CONT].temperature = CyclicDataGetMtrContTemperature();
        g_lcd_temperatures[MOTOR_THERM].temperature = CyclicDataGetMtrThermTemperature();

        LcdAppDisplayTemperature(g_lcd_temperatures[MPPTA]);
        LcdAppDisplayTemperature(g_lcd_temperatures[MPPTB]);
        LcdAppDisplayTemperature(g_lcd_temperatures[MPPTC]);
        LcdAppDisplayTemperature(g_lcd_temperatures[MPPTD]);
        LcdAppDisplayTemperature(g_lcd_temperatures[BATT_MIN]);
        LcdAppDisplayTemperature(g_lcd_temperatures[BATT_MAX]);
        LcdAppDisplayTemperature(g_lcd_temperatures[MOTOR_CONT]);
        LcdAppDisplayTemperature(g_lcd_temperatures[MOTOR_THERM]);
        break;
    case DEBUG_PAGE:
        LcdAppDisplaySpeedDebugPage(g_lcd_data.speed, g_lcd_data.speed_units);
        LcdAppDisplayDriveStateDebugPage((volatile DriveStateStates*) g_lcd_data.drive_state);
        LcdAppDisplaySocDebugPage((volatile uint32_t*)g_lcd_data.soc);
        LcdAppDisplayPowerBar(g_lcd_data.pack_current, g_lcd_data.pack_voltage);
        break;
    default:
        break;
    }
}

void LcdHandlerChangePage(bool change_page){
    // Trigger on falling edge of next_page from CAN Message
    if (g_prev_change_page == true && change_page == false) {
        g_str_change_page_flag = true;
    }
    // Set prev_change_page to be change_page to only trigger on falling edge. 
    g_prev_change_page = change_page;
}

static void LcdHandlerGetData(void) {
    /* DRIVE DATA*/
    g_lcd_data.speed = CyclicDataGetSpeed();
    g_lcd_data.drive_state = CyclicDataGetDriveState();
    g_lcd_data.drive_mode = DriveStateGetDriveMode();
    g_lcd_data.soc = CyclicDataGetSoc();
    g_lcd_data.pack_current = CyclicDataGetPackCurrent();
    g_lcd_data.pack_voltage = CyclicDataGetPackVoltage();
}

/* LCD HANDLER BATTERY FAULT DATA SETTERS */
void LcdHandlerSetBatteryFault(bool fault) { g_lcd_batt_faults.battery_fault = fault; }
void LcdHandlerSetBatterySupplyLow(bool fault) { g_lcd_batt_faults.supp_lo = fault; }
void LcdHandlerSetBMSSelfTestFault(bool fault) { g_lcd_batt_faults.selftest_fault = fault; }
void LcdHandlerSetBatteryVoltageHigh(bool fault) { g_lcd_batt_faults.voltage_high = fault; }
void LcdHandlerSetBatteryVoltageLow(bool fault) { g_lcd_batt_faults.voltage_low = fault; }
void LcdHandlerSetBatteryOvertemp(bool fault) { g_lcd_batt_faults.overtemp_fault = fault; }
void LcdHandlerSetBatterySlaveBoardCommFault(bool fault) { g_lcd_batt_faults.slave_board_comm_fault = fault; }
void LcdHandlerSetBatteryOvervoltFault(bool fault) { g_lcd_batt_faults.overvolt_fault = fault; }
void LcdHandlerSetBatteryUndervoltFault(bool fault) { g_lcd_batt_faults.undervolt_fault = fault; }
void LcdHandlerSetBatteryChargeOvercurrentFault(bool fault) { g_lcd_batt_faults.charge_overcurrent_fault = fault; }
void LcdHandlerSetBatteryDischargeOvercurrentFault(bool fault) { g_lcd_batt_faults.discharge_overcurrent_fault = fault; }
void LcdHandlerSetBatteryResetFromWatchdogFault(bool fault) { g_lcd_batt_faults.reset_from_watchdog = fault; }

/* LCD HANDLER MOTOR FAULT DATA SETTERS */
void LcdHandlerSetMotorSystemFault(bool fault) { g_lcd_motor_faults.motor_system_error = fault; }
void LcdHandlerSetMotorOvercurrentFault(bool fault) { g_lcd_motor_faults.overcurrent_fault = fault; }
void LcdHandlerSetMotorOvervoltageFault(bool fault) { g_lcd_motor_faults.overvoltage_fault = fault; }
void LcdHandlerSetMotorFetThermistorError(bool fault) { g_lcd_motor_faults.fet_thermistor_error = fault; }
void LcdHandlerSetMotorCommFault(bool fault) { g_lcd_motor_faults.motor_comm_fault = fault; }
void LcdHandlerSetMotorThrottleAdcOutOfRange(bool fault) { g_lcd_motor_faults.throttle_adc_outofrange = fault; }
void LcdHandlerSetMotorThrottleAdcMismatch(bool fault) { g_lcd_motor_faults.throttle_adc_mismatch = fault; }

/* LCD HANDLER WARNING DATA SETTERS */
void LcdHandlerSetLowVoltWarning(bool warning) { g_lcd_warnings.low_volt_warning = warning; }
void LcdHandlerSetHighVoltWarning(bool warning) { g_lcd_warnings.high_volt_warning = warning; }
void LcdHandlerSetLowTempWarning(bool warning) { g_lcd_warnings.low_temp_warning = warning; }
void LcdHandlerSetHighTempWarning(bool warning) { g_lcd_warnings.high_temp_warning = warning; }
void LcdHandlerSetNoEcuMessageWarning(bool warning) { g_lcd_warnings.no_ecu_message = warning; }
void LcdHandlerSetPackOverdischargeWarning(bool warning) { g_lcd_warnings.pack_overdischarge = warning; }
void LcdHandlerSetPackOverchargeWarning(bool warning) { g_lcd_warnings.pack_overcharge = warning; }  