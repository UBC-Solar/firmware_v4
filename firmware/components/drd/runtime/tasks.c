#include "tasks.h"
#include "cyclic_data_handler.h"
#include "lcd_app.h"

/* DRIVE STATE TASK */
void TasksDriveState(void)
{
    for (;;)
    {
        // function calls begin here
        osDelay(1);
    }
}

/* DRIVE STATE TASK */
void LcdUpdateTask(void)
{
    // Handles clearing the screen
    if (g_lcd_page_change == 1)
    {
        LcdAppChangeScreen();
        g_lcd_page_change = 0;
    }

    // Constantly gets faults
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

    // Handles what is displayed
    switch (1)
    {
    case 1:
        g_lcd_data.speed = get_cyclic_speed();
        g_lcd_data.drive_state = get_cyclic_drive_state();
        g_lcd_data.soc = get_cyclic_soc();

        LcdAppDisplaySpeedDrivePage(g_lcd_data.speed, g_lcd_data.speed_units);
        LcdAppDisplayDriveStateDrivePage(g_lcd_data.drive_state);
        LcdAppDisplaySocDrivePage((volatile uint32_t*)g_lcd_data.soc);
        LcdAppDisplayFaultIndicator(&g_lcd_batt_faults, &g_lcd_motor_faults);
        LcdAppDisplayWarningIndicator(&g_lcd_warnings);
        break;

    case 2:
        LcdAppDisplayFaults(&g_lcd_batt_faults, &g_lcd_motor_faults);
        break;
    case 3:
        LcdAppDisplayWarnings(&g_lcd_warnings);
        break;
    case 4:
        g_lcd_temperatures[0].temp_label = 0x1;
        g_lcd_temperatures[1].temp_label = 0x2;
        g_lcd_temperatures[2].temp_label = 0x3;
        g_lcd_temperatures[3].temp_label = 0x4;
        g_lcd_temperatures[4].temp_label = 0x5;
        g_lcd_temperatures[5].temp_label = 0x6;
        g_lcd_temperatures[6].temp_label = 0x7;
        g_lcd_temperatures[7].temp_label = 0x8;

        g_lcd_temperatures[0].temperature = NULL;
        g_lcd_temperatures[1].temperature = NULL;
        g_lcd_temperatures[2].temperature = NULL;
        g_lcd_temperatures[3].temperature = NULL;
        g_lcd_temperatures[4].temperature = NULL;
        g_lcd_temperatures[5].temperature = NULL;
        g_lcd_temperatures[6].temperature = NULL;
        g_lcd_temperatures[7].temperature = NULL;

        LcdAppDisplayTemperature(g_lcd_temperatures[0]);
        LcdAppDisplayTemperature(g_lcd_temperatures[1]);
        LcdAppDisplayTemperature(g_lcd_temperatures[2]);
        LcdAppDisplayTemperature(g_lcd_temperatures[3]);
        LcdAppDisplayTemperature(g_lcd_temperatures[4]);
        LcdAppDisplayTemperature(g_lcd_temperatures[5]);
        LcdAppDisplayTemperature(g_lcd_temperatures[6]);
        LcdAppDisplayTemperature(g_lcd_temperatures[7]);
        break;
    case 5:
        g_lcd_data.speed = get_cyclic_speed();
        g_lcd_data.drive_state = get_cyclic_drive_state();
        g_lcd_data.soc = get_cyclic_soc();
        g_lcd_data.pack_current = get_cyclic_pack_current();
        g_lcd_data.pack_voltage = get_cyclic_pack_voltage();

        LcdAppDisplaySpeedDebugPage(g_lcd_data.speed, g_lcd_data.speed_units);
        LcdAppDisplayDriveStateDebugPage(g_lcd_data.drive_state);
        LcdAppDisplaySocDebugPage((volatile uint32_t*)g_lcd_data.soc);
        LcdAppDisplayPowerBar(g_lcd_data.pack_current, g_lcd_data.pack_voltage);
    default:
        break;
    }

    osDelay(LCD_APP_LCD_UPDATE_DELAY);
}
