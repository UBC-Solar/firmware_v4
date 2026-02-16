#include "tasks.h"
#include "cmsis_os2.h"
#include "cyclic_data_handler.h"
#include "lcd_app.h"
#include "spi.h"

/* DRIVE STATE TASK */
void TasksDriveState(void)
{
    for (;;)
    {
        // function calls begin here
        osDelay(1);
    }
}

/* LCD UPDATE TASK */
void LcdUpdateTask(void)
{

    LcdAppInit(&hspi1);

    // KPH or MPH
    g_lcd_data.speed_units = LCD_APP_MPH;

    for (;;)
    {
        // Handles clearing the screen
        if (g_lcd_page_change == 1)
        {
            LcdAppChangeScreen();
            g_lcd_page_change = 0;
        }

        // Constantly gets faults
        // TODO: HANDLE WITH CYCLIC DATA
        g_lcd_batt_faults.battery_fault = false;
        g_lcd_batt_faults.charge_overcurrent_fault = false;
        g_lcd_batt_faults.discharge_overcurrent_fault = false;
        g_lcd_batt_faults.overtemp_fault = false;
        g_lcd_batt_faults.overvolt_fault = false;
        g_lcd_batt_faults.reset_from_watchdog = false;
        g_lcd_batt_faults.slave_board_comm_fault = false;
        g_lcd_batt_faults.supp_lo = false;
        g_lcd_batt_faults.undervolt_fault = false;
        g_lcd_batt_faults.voltage_high = false;
        g_lcd_batt_faults.voltage_low = false;

        g_lcd_motor_faults.fet_thermistor_error = false;
        g_lcd_motor_faults.motor_comm_fault = false;
        g_lcd_motor_faults.motor_system_error = false;
        g_lcd_motor_faults.overcurrent_fault = false;
        g_lcd_motor_faults.overvoltage_fault = false;
        g_lcd_motor_faults.throttle_adc_mismatch = false;
        g_lcd_motor_faults.throttle_adc_outofrange = false;

        g_lcd_warnings.high_temp_warning = false;
        g_lcd_warnings.high_volt_warning = false;
        g_lcd_warnings.low_temp_warning = false;
        g_lcd_warnings.low_volt_warning = false;
        g_lcd_warnings.no_ecu_message = false;
        g_lcd_warnings.pack_overcharge = false;
        g_lcd_warnings.pack_overdischarge = false;

        // TODO: PUT THIS IN LCD APP, LcdAppPageController function

        // Handles what is displayed
        switch (g_lcd_page)
        {
        case DRIVE_PAGE:
            g_lcd_data.speed = CyclicDataGetSpeed();
            g_lcd_data.drive_state = CyclicDataGetDriveState();
            g_lcd_data.soc = CyclicDataGetSoc();

            LcdAppDisplaySpeedDrivePage(g_lcd_data.speed, g_lcd_data.speed_units);
            LcdAppDisplayDriveStateDrivePage(g_lcd_data.drive_state);
            LcdAppDisplaySocDrivePage((volatile uint32_t*)g_lcd_data.soc);
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
            g_lcd_temperatures[MOTOR_CONT].temperature = CyclicDataGetMtrControllerTemperature();
            g_lcd_temperatures[MOTOR_THERM].temperature = CyclicDataGetMtrThermistorTemperature();

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
            g_lcd_data.speed = CyclicDataGetSpeed();
            g_lcd_data.drive_state = CyclicDataGetDriveState();
            g_lcd_data.soc = CyclicDataGetSoc();
            g_lcd_data.pack_current = CyclicDataGetPackCurrent();
            g_lcd_data.pack_voltage = CyclicDataGetPackVoltage();

            LcdAppDisplaySpeedDebugPage(g_lcd_data.speed, g_lcd_data.speed_units);
            LcdAppDisplayDriveStateDebugPage(g_lcd_data.drive_state);
            LcdAppDisplaySocDebugPage((volatile uint32_t*)g_lcd_data.soc);
            LcdAppDisplayPowerBar(g_lcd_data.pack_current, g_lcd_data.pack_voltage);
            break;
        default:
            break;
        }
    }
    osDelay(LCD_APP_UPDATE_DELAY);
}
