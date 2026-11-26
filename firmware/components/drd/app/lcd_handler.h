/**
 * @file    lcd_handler.h
 * @brief   LCD handler header file for UBC Solar DRD board
 *
 * This header declares the data structures, constants, and function prototypes for the LCD handler. 
 * The module implements a controller to handle what is displayed on each page and handles the page transitions.
 *
 * @author  Gregory Bian
 * @date    Feb 4 2026
 */

#include <stdint.h>
#include <stdbool.h>
#include "spi.h"

#ifndef __LCD_HANDLER_H
#define __LCD_HANDLER_H

/** LCD Screen Constants */
#define LCD_HANDLER_MAXPAGES 5

#define LCD_HANDLER_UPDATE_DELAY 200

// #define LCD_TEST

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

typedef enum
{
    MPPTA = (uint8_t)0x00,
    MPPTB = (uint8_t)0x01,
    MPPTC = (uint8_t)0x02,
    MPPTD = (uint8_t)0x03,
    BATT_MIN = (uint8_t)0x04,
    BATT_MAX = (uint8_t)0x05,
    MOTOR_CONT = (uint8_t)0x06,
    MOTOR_THERM = (uint8_t)0x07
} LcdAppTemperatureLabel;

typedef struct
{
    volatile bool battery_fault;
    volatile bool selftest_fault;
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
    volatile bool analog_sensor_error;   
    volatile bool motor_curr_sensor_u_error; 
    volatile bool motor_curr_sensor_w_error;   
    volatile bool batt_voltage_sensor_error;   
    volatile bool batt_current_sensor_error;   
    volatile bool batt_current_sensor_adjust_error;  
    volatile bool motor_current_sensor_adjust_error;   
    volatile bool accelerator_position_error;   
    volatile bool controller_voltage_sensor_error;  
    volatile bool power_system_fault;   
    volatile bool motor_overcurrent_limit; 
    volatile bool motor_lock;   
    volatile bool hall_sensor_short;  
    volatile bool hall_sensor_open;   
    volatile uint8_t motor_controller_overheat;   
} LcdAppMotorFaults;

typedef struct
{   
    volatile bool drd_watchdog_reset;
    volatile bool drd_mcu_overtemp;
    volatile bool drd_mcu_undervolt;
} LcdAppDrdFaults;

typedef struct
{
    volatile bool estop;
    volatile bool imd_fault;
    volatile bool mst_fault;
    volatile bool mst_unhealthy;
    volatile bool overcurrent;
    volatile bool reverse_current;
    volatile bool dist_fault;
    volatile bool dcdc_fault;
    volatile bool supp_fault;
    volatile bool watchdog_reset;
    volatile bool dcdc_overtemp;
    volatile bool tel_startup_timeout;
    volatile bool mst_handshake_timeout;
    volatile bool mst_status_check_timeout;
    volatile bool hv_connect_timeout;
    volatile bool open_discharge_relay_timeout;
    volatile bool motor_precharge_timeout;
    volatile bool mppt_precharge_timeout;
    volatile bool lv_powerup_timeout;
    volatile bool tel_heartbeat_timeout;
    volatile bool mst_heartbeat_timeout;
    volatile bool dist_heartbeat_timeout;
} LcdAppHvcFaults;

typedef struct
{
    volatile bool watchdog_reset;
    volatile bool overvoltage;
    volatile bool motor_overtemp;
    volatile bool rtd_sensor_fault;
    volatile bool rtd_comm_error;
} LcdAppMdiFaults;

// MPPT indexes
#define LCD_APP_MPPT_A 0
#define LCD_APP_MPPT_B 1
#define LCD_APP_MPPT_C 2
#define LCD_APP_MPPT_D 3
#define LCD_APP_NUM_MPPTS 4

typedef struct
{
    volatile bool hw_overvoltage;
    volatile bool hw_overcurrent;
    volatile bool undervoltage_12v;
    volatile bool mosfet_overheat;
} LcdAppMpptFaults;

typedef struct
{
    volatile bool comm_fail;
    volatile bool self_check_fail;
    volatile bool cell_volt_high;
    volatile bool cell_volt_low;
    volatile bool cell_temp_high;
    volatile bool cell_temp_low;
} LcdAppMstFaults;

typedef struct
{
    volatile bool mcu_undervolt;
    volatile bool mcu_overtemp;
} LcdAppStrFaults;

typedef struct
{
    volatile bool watchdog_reset;
    volatile bool imu_read_fail;
    volatile bool imu_write_fail;
    volatile bool gps_read_fail;
    volatile bool gps_write_fail;
    volatile bool mcu_overtemp;
    volatile bool mcu_undervolt;
} LcdAppTelFaults;

typedef struct
{
    volatile bool hw_protection;
    volatile bool temp_protection;
    volatile bool input_volt_fault;
    volatile bool output_undervolt;
    volatile bool output_overvolt;
    volatile bool output_overcurr;
    volatile bool output_short;
    volatile bool comm_timeout;
    volatile bool socket_overheat;
    volatile bool elec_lock_fault;
} LcdAppObcFaults;

typedef struct 
{
    volatile bool low_volt_warning;
    volatile bool high_volt_warning;
    volatile bool low_temp_warning;
    volatile bool high_temp_warning;
    volatile bool no_ecu_message;
    volatile bool pack_overdischarge;
    volatile bool pack_overcharge;
    volatile bool throttle_adc_outofrange;
    volatile bool throttle_adc_mismatch;
    volatile bool mst_cell_volt_high;
    volatile bool mst_cell_volt_low;
    volatile bool mst_cell_temp_high;
    volatile bool drd_speed_timeout;
    volatile bool drd_drive_state_timeout;
    volatile bool drd_soc_timeout;
    volatile bool drd_pack_voltage_timeout;
    volatile bool drd_current_timeout;
    volatile bool drd_accel_timeout;
} LcdAppWarnings;

typedef enum
{
    DRIVE_PAGE = 0x01,
    FAULTS_PAGE = 0x02,
    WARNINGS_PAGE = 0x03,
    TEMPERATURE_PAGE = 0x04,
    DEBUG_PAGE = 0x05
} LcdAppScreens;

/**
 * @brief Initializes the LCD App and SPI interface.
 *
 * @param hspi Pointer to the SPI handle.
 * @param speed_units Initial speed-unit selection for the LCD display.
 */
void LcdHandlerInit(SPI_HandleTypeDef* hspi, volatile uint8_t speed_units);

/**
 * @brief Returns the currently selected speed-unit mode for the LCD display.
 */
uint8_t LcdHandlerGetSpeedUnits(void);

/**
 * @brief Handles the screen logic for the LCD App, including page changes and updating displayed data.
 */
void LcdHandlerPageController(void);

/**
 * @brief Handles the page change logic from a CAN Message
 */
void LcdHandlerChangePage(bool change_page);


/* LCD HANDLER BATTERY FAULT DATA SETTERS */
void LcdHandlerSetBatteryFault(bool fault);
void LcdHandlerSetBatterySupplyLow(bool fault);
void LcdHandlerSetBMSSelfTestFault(bool fault);
void LcdHandlerSetBatteryVoltageHigh(bool fault);
void LcdHandlerSetBatteryVoltageLow(bool fault);
void LcdHandlerSetBatteryOvertemp(bool fault);
void LcdHandlerSetBatterySlaveBoardCommFault(bool fault);
void LcdHandlerSetBatteryOvervoltFault(bool fault);
void LcdHandlerSetBatteryUndervoltFault(bool fault);
void LcdHandlerSetBatteryChargeOvercurrentFault(bool fault);
void LcdHandlerSetBatteryDischargeOvercurrentFault(bool fault);
void LcdHandlerSetBatteryResetFromWatchdogFault(bool fault);

/* LCD HANDLER MOTOR FAULT DATA SETTERS */
void LcdHandlerSetMotorSystemFault(bool fault);
void LcdHandlerSetMotorOvercurrentFault(bool fault);
void LcdHandlerSetMotorOvervoltageFault(bool fault);
void LcdHandlerSetMotorFetThermistorError(bool fault);
void LcdHandlerSetMotorCommFault(bool fault);
void LcdHandlerSetAnalogSensorError(bool fault);
void LcdHandlerSetMotorCurrSensorUError(bool fault);
void LcdHandlerSetMotorCurrSensorWError(bool fault);
void LcdHandlerSetBattVoltageSensorError(bool fault);
void LcdHandlerSetBattCurrentSensorError(bool fault);
void LcdHandlerSetBattCurrentSensorAdjustError(bool fault);
void LcdHandlerSetMotorCurrentSensorAdjustError(bool fault);
void LcdHandlerSetAcceleratorPositionError(bool fault);
void LcdHandlerSetControllerVoltageSensorError(bool fault);
void LcdHandlerSetPowerSystemFault(bool fault);
void LcdHandlerSetMotorOvercurrentLimit(bool fault);
void LcdHandlerSetMotorLock(bool fault);
void LcdHandlerSetHallSensorShort(bool fault);
void LcdHandlerSetHallSensorOpen(bool fault);
void LcdHandlerSetMotorControllerOverHeat(uint8_t fault);

/* LCD HANDLER DRD FAULT DATA SETTERS */
void LcdHandlerSetDrdWatchdogReset(bool fault);
void LcdHandlerSetDrdMcuOvertemp(bool fault);
void LcdHandlerSetDrdMcuUndervolt(bool fault);

/* LCD HANDLER HVC FAULT DATA SETTERS */
void LcdHandlerSetHvcEstop(bool fault);
void LcdHandlerSetHvcImdFault(bool fault);
void LcdHandlerSetHvcMstFault(bool fault);
void LcdHandlerSetHvcMstUnhealthy(bool fault);
void LcdHandlerSetHvcOvercurrent(bool fault);
void LcdHandlerSetHvcReverseCurrent(bool fault);
void LcdHandlerSetHvcDistFault(bool fault);
void LcdHandlerSetHvcDcdcFault(bool fault);
void LcdHandlerSetHvcSuppFault(bool fault);
void LcdHandlerSetHvcWatchdogReset(bool fault);
void LcdHandlerSetHvcDcdcOvertemp(bool fault);
void LcdHandlerSetHvcTelStartupTimeout(bool fault);
void LcdHandlerSetHvcMstHandshakeTimeout(bool fault);
void LcdHandlerSetHvcMstStatusCheckTimeout(bool fault);
void LcdHandlerSetHvcHvConnectTimeout(bool fault);
void LcdHandlerSetHvcOpenDischargeRelayTimeout(bool fault);
void LcdHandlerSetHvcMotorPrechargeTimeout(bool fault);
void LcdHandlerSetHvcMpptPrechargeTimeout(bool fault);
void LcdHandlerSetHvcLvPowerupTimeout(bool fault);
void LcdHandlerSetHvcTelHeartbeatTimeout(bool fault);
void LcdHandlerSetHvcMstHeartbeatTimeout(bool fault);
void LcdHandlerSetHvcDistHeartbeatTimeout(bool fault);

/* LCD HANDLER MDI FAULT DATA SETTERS */
void LcdHandlerSetMdiWatchdogReset(bool fault);
void LcdHandlerSetMdiOvervoltage(bool fault);
void LcdHandlerSetMdiMotorOvertemp(bool fault);
void LcdHandlerSetMdiRtdSensorFault(bool fault);
void LcdHandlerSetMdiRtdCommError(bool fault);

/* LCD HANDLER MPPT FAULT DATA SETTERS */
void LcdHandlerSetMpptHwOvervoltage(uint8_t mppt, bool fault);
void LcdHandlerSetMpptHwOvercurrent(uint8_t mppt, bool fault);
void LcdHandlerSetMppt12vUndervoltage(uint8_t mppt, bool fault);
void LcdHandlerSetMpptMosfetOverheat(uint8_t mppt, bool fault);

/* LCD HANDLER MST FAULT DATA SETTERS */
void LcdHandlerSetMstCommFail(bool fault);
void LcdHandlerSetMstSelfCheckFail(bool fault);
void LcdHandlerSetMstCellVoltHigh(bool fault);
void LcdHandlerSetMstCellVoltLow(bool fault);
void LcdHandlerSetMstCellTempHigh(bool fault);
void LcdHandlerSetMstCellTempLow(bool fault);

/* LCD HANDLER STR FAULT DATA SETTERS */
void LcdHandlerSetStrMcuUndervolt(bool fault);
void LcdHandlerSetStrMcuOvertemp(bool fault);

/* LCD HANDLER TEL FAULT DATA SETTERS */
void LcdHandlerSetTelWatchdogReset(bool fault);
void LcdHandlerSetTelImuReadFail(bool fault);
void LcdHandlerSetTelImuWriteFail(bool fault);
void LcdHandlerSetTelGpsReadFail(bool fault);
void LcdHandlerSetTelGpsWriteFail(bool fault);
void LcdHandlerSetTelMcuOvertemp(bool fault);
void LcdHandlerSetTelMcuUndervolt(bool fault);

/* LCD HANDLER OBC FAULT DATA SETTERS */
void LcdHandlerSetObcHwProtection(bool fault);
void LcdHandlerSetObcTempProtection(bool fault);
void LcdHandlerSetObcInputVoltFault(bool fault);
void LcdHandlerSetObcOutputUndervolt(bool fault);
void LcdHandlerSetObcOutputOvervolt(bool fault);
void LcdHandlerSetObcOutputOvercurr(bool fault);
void LcdHandlerSetObcOutputShort(bool fault);
void LcdHandlerSetObcCommTimeout(bool fault);
void LcdHandlerSetObcSocketOverheat(bool fault);
void LcdHandlerSetObcElecLockFault(bool fault);

/* LCD HANDLER WARNING DATA SETTERS */
void LcdHandlerSetLowVoltWarning(bool warning);
void LcdHandlerSetHighVoltWarning(bool warning);
void LcdHandlerSetLowTempWarning(bool warning);
void LcdHandlerSetHighTempWarning(bool warning);
void LcdHandlerSetNoEcuMessageWarning(bool warning);
void LcdHandlerSetPackOverdischargeWarning(bool warning);
void LcdHandlerSetPackOverchargeWarning(bool warning);  
void LcdHandlerSetMotorThrottleAdcOutOfRange(bool warning);
void LcdHandlerSetMotorThrottleAdcMismatch(bool warning);
void LcdHandlerSetMstCellVoltHighWarning(bool warning);
void LcdHandlerSetMstCellVoltLowWarning(bool warning);
void LcdHandlerSetMstCellTempHighWarning(bool warning);
void LcdHandlerSetDrdSpeedTimeoutWarning(bool warning);
void LcdHandlerSetDrdDriveStateTimeoutWarning(bool warning);
void LcdHandlerSetDrdSocTimeoutWarning(bool warning);
void LcdHandlerSetDrdPackVoltageTimeoutWarning(bool warning);
void LcdHandlerSetDrdCurrentTimeoutWarning(bool warning);
void LcdHandlerSetDrdAccelTimeoutWarning(bool warning);


#endif /* __LCD_HANDLER_H */