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
static LcdAppMotorFaults g_lcd_motor_faults = {0};
static LcdAppDrdFaults g_lcd_drd_faults = {0};
static LcdAppHvcFaults g_lcd_hvc_faults = {0};
static LcdAppMdiFaults g_lcd_mdi_faults = {0};
static LcdAppMstFaults g_lcd_mst_faults = {0};
static LcdAppStrFaults g_lcd_str_faults = {0};
static LcdAppTelFaults g_lcd_tel_faults = {0};
static LcdAppObcFaults g_lcd_obc_faults = {0};
static LcdAppMpptFaults g_lcd_mppt_faults[LCD_APP_NUM_MPPTS] = {0};
static LcdAppWarnings g_lcd_warnings = {0};
static LcdAppTemperature g_lcd_temperatures[8] = {0};
static uint8_t g_lcd_page = 1;
static bool g_str_change_page_flag = false;
static volatile bool g_prev_change_page = false;
static volatile bool g_prev_fault = false;

/* STATIC FUNCTION DECLARATION*/
/**
 * @brief Gets relevant data for LCD controller to parse
 */
static void LcdHandlerGetData(void);

void LcdHandlerInit(SPI_HandleTypeDef* hspi, volatile uint8_t speed_units)
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

    g_lcd_data.speed_units = speed_units;

    LcdDriverInit(hspi); // Initialize the LCD driver
}

void LcdHandlerPageController(void)
{
    // Get the latest data for the LCD
    LcdHandlerGetData();

    // Fault Logic - Trigger Page Change on rising edge of fault detection
    bool check_fault = LcdAppCheckFaults(&g_lcd_motor_faults, &g_lcd_drd_faults, &g_lcd_hvc_faults, &g_lcd_mdi_faults, g_lcd_mppt_faults, &g_lcd_mst_faults, &g_lcd_str_faults, &g_lcd_tel_faults, &g_lcd_obc_faults);
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
    // Triggers Change of page to fault page 1 time
    if (check_fault && g_prev_fault == false){
        LcdDriverChangeScreen();
        g_lcd_page = FAULTS_PAGE;
        g_prev_fault = true;
    }

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
        LcdAppDisplayFaultIndicator(&g_lcd_motor_faults, &g_lcd_drd_faults, &g_lcd_hvc_faults, &g_lcd_mdi_faults, g_lcd_mppt_faults, &g_lcd_mst_faults, &g_lcd_str_faults, &g_lcd_tel_faults, &g_lcd_obc_faults);
        LcdAppDisplayWarningIndicator(&g_lcd_warnings);
        break;
    case FAULTS_PAGE:
        LcdAppDisplayFaults(&g_lcd_motor_faults, &g_lcd_drd_faults, &g_lcd_hvc_faults, &g_lcd_mdi_faults, g_lcd_mppt_faults, &g_lcd_mst_faults, &g_lcd_str_faults, &g_lcd_tel_faults, &g_lcd_obc_faults);
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

uint8_t LcdHandlerGetSpeedUnits(void)
{
    return g_lcd_data.speed_units;
}

/* LCD HANDLER MOTOR FAULT DATA SETTERS */
void LcdHandlerSetMotorSystemFault(bool fault) { g_lcd_motor_faults.motor_system_error = fault; }
void LcdHandlerSetMotorOvercurrentFault(bool fault) { g_lcd_motor_faults.overcurrent_fault = fault; }
void LcdHandlerSetMotorOvervoltageFault(bool fault) { g_lcd_motor_faults.overvoltage_fault = fault; }
void LcdHandlerSetMotorFetThermistorError(bool fault) { g_lcd_motor_faults.fet_thermistor_error = fault; }
void LcdHandlerSetMotorCommFault(bool fault) { g_lcd_motor_faults.motor_comm_fault = fault; }
void LcdHandlerSetAnalogSensorError(bool fault) { g_lcd_motor_faults.analog_sensor_error = fault; }    
void LcdHandlerSetMotorCurrSensorUError(bool fault) { g_lcd_motor_faults.motor_curr_sensor_u_error = fault; }   
void LcdHandlerSetMotorCurrSensorWError(bool fault) { g_lcd_motor_faults.motor_curr_sensor_w_error = fault; }   
void LcdHandlerSetBattVoltageSensorError(bool fault) { g_lcd_motor_faults.batt_voltage_sensor_error = fault; }  
void LcdHandlerSetBattCurrentSensorError(bool fault) { g_lcd_motor_faults.batt_current_sensor_error = fault; }  
void LcdHandlerSetBattCurrentSensorAdjustError(bool fault) { g_lcd_motor_faults.batt_current_sensor_adjust_error = fault; } 
void LcdHandlerSetMotorCurrentSensorAdjustError(bool fault) { g_lcd_motor_faults.motor_current_sensor_adjust_error = fault; } 
void LcdHandlerSetAcceleratorPositionError(bool fault) { g_lcd_motor_faults.accelerator_position_error = fault; }  
void LcdHandlerSetControllerVoltageSensorError(bool fault) { g_lcd_motor_faults.controller_voltage_sensor_error = fault; } 
void LcdHandlerSetPowerSystemFault(bool fault) { g_lcd_motor_faults.power_system_fault = fault; }   
void LcdHandlerSetMotorOvercurrentLimit(bool fault) { g_lcd_motor_faults.motor_overcurrent_limit = fault; } 
void LcdHandlerSetMotorLock(bool fault) { g_lcd_motor_faults.motor_lock = fault; }   
void LcdHandlerSetHallSensorShort(bool fault) { g_lcd_motor_faults.hall_sensor_short = fault; } 
void LcdHandlerSetHallSensorOpen(bool fault) { g_lcd_motor_faults.hall_sensor_open = fault; }   
void LcdHandlerSetMotorControllerOverHeat(uint8_t fault) { g_lcd_motor_faults.motor_controller_overheat = fault; } 

/* LCD HANDLER DRD FAULT DATA SETTERS*/
void LcdHandlerSetDrdWatchdogReset(bool fault) { g_lcd_drd_faults.drd_watchdog_reset = fault; }
void LcdHandlerSetDrdMcuOvertemp(bool fault) { g_lcd_drd_faults.drd_mcu_overtemp = fault; }
void LcdHandlerSetDrdMcuUndervolt(bool fault) { g_lcd_drd_faults.drd_mcu_undervolt = fault; }

/* LCD HANDLER HVC FAULT DATA SETTERS */
void LcdHandlerSetHvcEstop(bool fault) { g_lcd_hvc_faults.estop = fault; }
void LcdHandlerSetHvcImdFault(bool fault) { g_lcd_hvc_faults.imd_fault = fault; }
void LcdHandlerSetHvcMstFault(bool fault) { g_lcd_hvc_faults.mst_fault = fault; }
void LcdHandlerSetHvcMstUnhealthy(bool fault) { g_lcd_hvc_faults.mst_unhealthy = fault; }
void LcdHandlerSetHvcOvercurrent(bool fault) { g_lcd_hvc_faults.overcurrent = fault; }
void LcdHandlerSetHvcReverseCurrent(bool fault) { g_lcd_hvc_faults.reverse_current = fault; }
void LcdHandlerSetHvcDistFault(bool fault) { g_lcd_hvc_faults.dist_fault = fault; }
void LcdHandlerSetHvcDcdcFault(bool fault) { g_lcd_hvc_faults.dcdc_fault = fault; }
void LcdHandlerSetHvcSuppFault(bool fault) { g_lcd_hvc_faults.supp_fault = fault; }
void LcdHandlerSetHvcWatchdogReset(bool fault) { g_lcd_hvc_faults.watchdog_reset = fault; }
void LcdHandlerSetHvcDcdcOvertemp(bool fault) { g_lcd_hvc_faults.dcdc_overtemp = fault; }
void LcdHandlerSetHvcTelStartupTimeout(bool fault) { g_lcd_hvc_faults.tel_startup_timeout = fault; }
void LcdHandlerSetHvcMstHandshakeTimeout(bool fault) { g_lcd_hvc_faults.mst_handshake_timeout = fault; }
void LcdHandlerSetHvcMstStatusCheckTimeout(bool fault) { g_lcd_hvc_faults.mst_status_check_timeout = fault; }
void LcdHandlerSetHvcHvConnectTimeout(bool fault) { g_lcd_hvc_faults.hv_connect_timeout = fault; }
void LcdHandlerSetHvcOpenDischargeRelayTimeout(bool fault) { g_lcd_hvc_faults.open_discharge_relay_timeout = fault; }
void LcdHandlerSetHvcMotorPrechargeTimeout(bool fault) { g_lcd_hvc_faults.motor_precharge_timeout = fault; }
void LcdHandlerSetHvcMpptPrechargeTimeout(bool fault) { g_lcd_hvc_faults.mppt_precharge_timeout = fault; }
void LcdHandlerSetHvcLvPowerupTimeout(bool fault) { g_lcd_hvc_faults.lv_powerup_timeout = fault; }
void LcdHandlerSetHvcTelHeartbeatTimeout(bool fault) { g_lcd_hvc_faults.tel_heartbeat_timeout = fault; }
void LcdHandlerSetHvcMstHeartbeatTimeout(bool fault) { g_lcd_hvc_faults.mst_heartbeat_timeout = fault; }
void LcdHandlerSetHvcDistHeartbeatTimeout(bool fault) { g_lcd_hvc_faults.dist_heartbeat_timeout = fault; }

/* LCD HANDLER MDI FAULT DATA SETTERS */
void LcdHandlerSetMdiWatchdogReset(bool fault) { g_lcd_mdi_faults.watchdog_reset = fault; }
void LcdHandlerSetMdiOvervoltage(bool fault) { g_lcd_mdi_faults.overvoltage = fault; }
void LcdHandlerSetMdiMotorOvertemp(bool fault) { g_lcd_mdi_faults.motor_overtemp = fault; }
void LcdHandlerSetMdiRtdSensorFault(bool fault) { g_lcd_mdi_faults.rtd_sensor_fault = fault; }
void LcdHandlerSetMdiRtdCommError(bool fault) { g_lcd_mdi_faults.rtd_comm_error = fault; }

/* LCD HANDLER MPPT FAULT DATA SETTERS (out-of-range mppt indexes are ignored) */
void LcdHandlerSetMpptHwOvervoltage(uint8_t mppt, bool fault) { if (mppt < LCD_APP_NUM_MPPTS) { g_lcd_mppt_faults[mppt].hw_overvoltage = fault; } }
void LcdHandlerSetMpptHwOvercurrent(uint8_t mppt, bool fault) { if (mppt < LCD_APP_NUM_MPPTS) { g_lcd_mppt_faults[mppt].hw_overcurrent = fault; } }
void LcdHandlerSetMppt12vUndervoltage(uint8_t mppt, bool fault) { if (mppt < LCD_APP_NUM_MPPTS) { g_lcd_mppt_faults[mppt].undervoltage_12v = fault; } }
void LcdHandlerSetMpptMosfetOverheat(uint8_t mppt, bool fault) { if (mppt < LCD_APP_NUM_MPPTS) { g_lcd_mppt_faults[mppt].mosfet_overheat = fault; } }

/* LCD HANDLER MST FAULT DATA SETTERS */
void LcdHandlerSetMstCommFail(bool fault) { g_lcd_mst_faults.comm_fail = fault; }
void LcdHandlerSetMstSelfCheckFail(bool fault) { g_lcd_mst_faults.self_check_fail = fault; }
void LcdHandlerSetMstCellVoltHigh(bool fault) { g_lcd_mst_faults.cell_volt_high = fault; }
void LcdHandlerSetMstCellVoltLow(bool fault) { g_lcd_mst_faults.cell_volt_low = fault; }
void LcdHandlerSetMstCellTempHigh(bool fault) { g_lcd_mst_faults.cell_temp_high = fault; }
void LcdHandlerSetMstCellTempLow(bool fault) { g_lcd_mst_faults.cell_temp_low = fault; }

/* LCD HANDLER STR FAULT DATA SETTERS */
void LcdHandlerSetStrMcuUndervolt(bool fault) { g_lcd_str_faults.mcu_undervolt = fault; }
void LcdHandlerSetStrMcuOvertemp(bool fault) { g_lcd_str_faults.mcu_overtemp = fault; }

/* LCD HANDLER TEL FAULT DATA SETTERS */
void LcdHandlerSetTelWatchdogReset(bool fault) { g_lcd_tel_faults.watchdog_reset = fault; }
void LcdHandlerSetTelImuReadFail(bool fault) { g_lcd_tel_faults.imu_read_fail = fault; }
void LcdHandlerSetTelImuWriteFail(bool fault) { g_lcd_tel_faults.imu_write_fail = fault; }
void LcdHandlerSetTelGpsReadFail(bool fault) { g_lcd_tel_faults.gps_read_fail = fault; }
void LcdHandlerSetTelGpsWriteFail(bool fault) { g_lcd_tel_faults.gps_write_fail = fault; }
void LcdHandlerSetTelMcuOvertemp(bool fault) { g_lcd_tel_faults.mcu_overtemp = fault; }
void LcdHandlerSetTelMcuUndervolt(bool fault) { g_lcd_tel_faults.mcu_undervolt = fault; }

/* LCD HANDLER OBC FAULT DATA SETTERS */
void LcdHandlerSetObcHwProtection(bool fault) { g_lcd_obc_faults.hw_protection = fault; }
void LcdHandlerSetObcTempProtection(bool fault) { g_lcd_obc_faults.temp_protection = fault; }
void LcdHandlerSetObcInputVoltFault(bool fault) { g_lcd_obc_faults.input_volt_fault = fault; }
void LcdHandlerSetObcOutputUndervolt(bool fault) { g_lcd_obc_faults.output_undervolt = fault; }
void LcdHandlerSetObcOutputOvervolt(bool fault) { g_lcd_obc_faults.output_overvolt = fault; }
void LcdHandlerSetObcOutputOvercurr(bool fault) { g_lcd_obc_faults.output_overcurr = fault; }
void LcdHandlerSetObcOutputShort(bool fault) { g_lcd_obc_faults.output_short = fault; }
void LcdHandlerSetObcCommTimeout(bool fault) { g_lcd_obc_faults.comm_timeout = fault; }
void LcdHandlerSetObcSocketOverheat(bool fault) { g_lcd_obc_faults.socket_overheat = fault; }
void LcdHandlerSetObcElecLockFault(bool fault) { g_lcd_obc_faults.elec_lock_fault = fault; }

/* LCD HANDLER WARNING DATA SETTERS */
void LcdHandlerSetLowVoltWarning(bool warning) { g_lcd_warnings.low_volt_warning = warning; }
void LcdHandlerSetHighVoltWarning(bool warning) { g_lcd_warnings.high_volt_warning = warning; }
void LcdHandlerSetLowTempWarning(bool warning) { g_lcd_warnings.low_temp_warning = warning; }
void LcdHandlerSetHighTempWarning(bool warning) { g_lcd_warnings.high_temp_warning = warning; }
void LcdHandlerSetMotorThrottleAdcOutOfRange(bool warning) { g_lcd_warnings.throttle_adc_outofrange = warning; }
void LcdHandlerSetMotorThrottleAdcMismatch(bool warning) { g_lcd_warnings.throttle_adc_mismatch = warning; }
void LcdHandlerSetMstCellVoltHighWarning(bool warning) { g_lcd_warnings.mst_cell_volt_high = warning; }
void LcdHandlerSetMstCellVoltLowWarning(bool warning) { g_lcd_warnings.mst_cell_volt_low = warning; }
void LcdHandlerSetMstCellTempHighWarning(bool warning) { g_lcd_warnings.mst_cell_temp_high = warning; }
void LcdHandlerSetDrdSpeedTimeoutWarning(bool warning) { g_lcd_warnings.drd_speed_timeout = warning; }
void LcdHandlerSetDrdDriveStateTimeoutWarning(bool warning) { g_lcd_warnings.drd_drive_state_timeout = warning; }
void LcdHandlerSetDrdSocTimeoutWarning(bool warning) { g_lcd_warnings.drd_soc_timeout = warning; }
void LcdHandlerSetDrdPackVoltageTimeoutWarning(bool warning) { g_lcd_warnings.drd_pack_voltage_timeout = warning; }
void LcdHandlerSetDrdCurrentTimeoutWarning(bool warning) { g_lcd_warnings.drd_current_timeout = warning; }
void LcdHandlerSetDrdAccelTimeoutWarning(bool warning) { g_lcd_warnings.drd_accel_timeout = warning; }
