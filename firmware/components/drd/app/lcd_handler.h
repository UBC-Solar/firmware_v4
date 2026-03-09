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

typedef enum
{
    DRIVE_PAGE = 0x01,
    FAULTS_PAGE = 0x02,
    WARNINGS_PAGE = 0x03,
    TEMPERATURE_PAGE = 0x04,
    DEBUG_PAGE = 0x05
} LcdAppScreens;

/*	User Variables	*/
extern LcdAppData g_lcd_data;
extern LcdAppBattFaults g_lcd_batt_faults;
extern LcdAppMotorFaults g_lcd_motor_faults;
extern LcdAppWarnings g_lcd_warnings;
extern LcdAppTemperature g_lcd_temperatures[8];
extern uint8_t g_lcd_page;
extern uint8_t g_lcd_page_change;

/**
 * @brief Changes the screen
 */
void LcdHandlerChangeScreen();

/**
 * @brief Initializes the LCD App and SPI interface.
 *
 * @param hspi Pointer to the SPI handle.
 */
void LcdHandlerInit(SPI_HandleTypeDef* hspi);

/**
 * @brief Handles the screen logic for the LCD App, including page changes and updating displayed data.
 */
 void LcdHandlerPageController(void);


#endif /* __LCD_HANDLER_H */