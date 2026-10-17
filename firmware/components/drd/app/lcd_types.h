#ifndef __LCD_TYPES_H__
#define __LCD_TYPES_H__

#include <stdbool.h>
#include <stdint.h>

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
    volatile bool throttle_adc_outofrange;
    volatile bool throttle_adc_mismatch;
} LcdAppWarnings;

#endif /* __LCD_TYPES_H__ */
