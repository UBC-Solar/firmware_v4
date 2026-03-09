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
#include "lcd_app.h"
#include "lcd_driver.h"
#include "cyclic_data_handler.h"

/* EXTERNAL VARIABLES */
LcdAppData g_lcd_data = {0};
LcdAppBattFaults g_lcd_batt_faults = {0};
LcdAppMotorFaults g_lcd_motor_faults = {0};
LcdAppWarnings g_lcd_warnings = {0};
LcdAppTemperature g_lcd_temperatures[8] = {0};

uint8_t g_lcd_page = 1;
uint8_t g_lcd_page_change = 0;

/* STATIC FUNCTION DECLARATION*/
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

    LcdDriverInit(hspi); // Initialize the LCD driver
}

void LcdHandlerPageController(void)
{
    // Get the latest data for the LCD
    LcdHandlerGetData();

    // Changes pages if fault flag is set
    if(LcdAppCheckFaults(&g_lcd_batt_faults, &g_lcd_motor_faults)) {
        g_lcd_page = FAULTS_PAGE;
    }

    // Handles what is displayed
    switch (g_lcd_page)
    {
    case DRIVE_PAGE:
        LcdAppDisplaySpeedDrivePage(g_lcd_data.speed, g_lcd_data.speed_units);
        LcdAppDisplaySocDrivePage((volatile uint32_t*)g_lcd_data.soc);
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

void LcdHandlerChangeScreen() { LcdDriverChangeScreen(); }

static void LcdHandlerGetData(void) {
    /* FAULTS AND WARNINGS */
    g_lcd_batt_faults.battery_fault = CyclicDataGetBatteryFault();
    g_lcd_batt_faults.charge_overcurrent_fault = CyclicDataGetBatteryChargeOvercurrentFault();
    g_lcd_batt_faults.discharge_overcurrent_fault = CyclicDataGetBatteryDischargeOvercurrentFault();
    g_lcd_batt_faults.overtemp_fault = CyclicDataGetBatteryOvertemp();
    g_lcd_batt_faults.overvolt_fault = CyclicDataGetBatteryVoltageHigh();
    g_lcd_batt_faults.reset_from_watchdog = CyclicDataGetBatteryResetFromWatchdogFault();
    g_lcd_batt_faults.slave_board_comm_fault = CyclicDataGetBatterySlaveBoardCommFault();
    g_lcd_batt_faults.supp_lo = false;
    g_lcd_batt_faults.undervolt_fault = CyclicDataGetBatteryUndervoltFault();
    g_lcd_batt_faults.voltage_high = CyclicDataGetBatteryVoltageHigh();
    g_lcd_batt_faults.voltage_low = CyclicDataGetBatteryVoltageLow();

    g_lcd_motor_faults.fet_thermistor_error = CyclicDataGetMotorFetThermistorError();
    g_lcd_motor_faults.motor_comm_fault = CyclicDataGetMotorCommFault();
    g_lcd_motor_faults.motor_system_error = CyclicDataGetMotorSystemFault();
    g_lcd_motor_faults.overcurrent_fault = CyclicDataGetMotorOvercurrentFault();
    g_lcd_motor_faults.overvoltage_fault = CyclicDataGetMotorOvervoltageFault();
    g_lcd_motor_faults.throttle_adc_mismatch = CyclicDataGetMotorThrottleAdcMismatch();
    g_lcd_motor_faults.throttle_adc_outofrange = CyclicDataGetMotorThrottleAdcOutOfRange();

    g_lcd_warnings.high_temp_warning = CyclicDataGetHighTempWarning();
    g_lcd_warnings.high_volt_warning = CyclicDataGetHighVoltWarning();
    g_lcd_warnings.low_temp_warning = CyclicDataGetLowTempWarning();
    g_lcd_warnings.low_volt_warning = CyclicDataGetLowVoltWarning();
    g_lcd_warnings.no_ecu_message = CyclicDataGetNoEcuMessageWarning();
    g_lcd_warnings.pack_overcharge = CyclicDataGetPackOverchargeWarning();
    g_lcd_warnings.pack_overdischarge = CyclicDataGetPackOverdischargeWarning();

    /* DRIVE DATA*/
    g_lcd_data.speed = CyclicDataGetSpeed();
    g_lcd_data.drive_state = CyclicDataGetDriveState();
    g_lcd_data.soc = CyclicDataGetSoc();
    g_lcd_data.pack_current = CyclicDataGetPackCurrent();
    g_lcd_data.pack_voltage = CyclicDataGetPackVoltage();
}