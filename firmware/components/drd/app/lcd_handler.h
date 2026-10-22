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
#include "lcd_types.h"

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


#endif /* __LCD_HANDLER_H */