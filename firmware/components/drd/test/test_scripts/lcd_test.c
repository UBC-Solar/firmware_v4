/**
 * @file    LcdTest.c
 * @brief   LCD test implementation for UBC Solar DRD board
 *
 * This file contains test functions that set up mock data for each LCD page.
 * These functions are used for testing and debugging LCD functionality.
 *
 * @author  Gregory Bian
 * @date    Mar 12 2026
 */

#include "lcd_test.h"
#include "lcd_handler.h"
#include "lcd_app.h"
#include "cyclic_data_handler.h"
#include <stdbool.h>
#include <stdint.h>
#include <drive_state.h>

static uint32_t g_lcdtest_speed = 0;
static uint8_t g_lcdtest_soc = 0;
static uint8_t g_lcdtest_faultcount = 0;
static uint8_t g_lcdtest_warningcount = 0;
static uint8_t g_lcdtest_temperature = 0;
static int16_t g_lcd_pack_current = -3000;


void LcdTestInit(){
    LcdTestDebugPage();
    // LcdTestFaultsPage();
    // LcdTestWarningsPage();
    LcdTestTemperaturePage();
}

void LcdTestDebugPage(void)
{
    /**
     * Sets up test data for DRIVE_PAGE
     * - Speed
     * - SOC
     * - Pack Current: 50A
     * - Pack Voltage: 48V
     */
    CyclicDataSetSpeed(g_lcdtest_speed);
    CyclicDataSetSoc(g_lcdtest_soc);
    CyclicDataSetPackCurrent(g_lcd_pack_current);
    CyclicDataSetPackVoltage(1);

    g_lcdtest_speed = (g_lcdtest_speed % 101) + 1;
    g_lcdtest_soc = (g_lcdtest_soc % 101) + 1;
    g_lcd_pack_current += 100;
    if(g_lcd_pack_current >= 5400){
        g_lcd_pack_current = -3000;
    }
}

void LcdTestFaultsPage(void)
{
    /**
     * Sets up test data for FAULTS_PAGE
     * - Battery faults>
     * - Motor faults 
     * - All faults set to true for visible display
     */

    /* Battery Faults - Set all to true for visible test display */

    switch(g_lcdtest_faultcount){
        // Input 8 faults with delay
        case 1:
            LcdHandlerSetBatteryFault(true);
            break;
        case 2:
            LcdHandlerSetBatterySupplyLow(true);
            break;
        case 3:
            LcdHandlerSetBMSSelfTestFault(true);
            break;
        case 4:
            LcdHandlerSetBatteryVoltageHigh(true);
            break;
        case 5:
            LcdHandlerSetBatteryVoltageLow(true);
            break;
        case 6:
            LcdHandlerSetBatteryOvertemp(true);
            break;
        case 7:
            LcdHandlerSetBatterySlaveBoardCommFault(true);
            break;
        case 8:
            LcdHandlerSetBatteryOvervoltFault(true);
            break;
        case 9: // Clear 8 faults
            LcdHandlerSetBatteryFault(false);
            LcdHandlerSetBatterySupplyLow(false);
            LcdHandlerSetBMSSelfTestFault(false);
            LcdHandlerSetBatteryVoltageHigh(false);
            LcdHandlerSetBatteryVoltageLow(false);
            LcdHandlerSetBatteryOvertemp(false);
            LcdHandlerSetBatterySlaveBoardCommFault(false);
            LcdHandlerSetBatteryOvervoltFault(false);
            break;
        case 10:
            LcdHandlerSetBatteryUndervoltFault(true);
            break;
        case 11:
            LcdHandlerSetBatteryChargeOvercurrentFault(true);
            break;
        case 12:
            LcdHandlerSetBatteryDischargeOvercurrentFault(true);
            break;
        case 13:
            LcdHandlerSetBatteryResetFromWatchdogFault(true);
            break;
        case 14:
            LcdHandlerSetMotorSystemFault(true);
            break;
        case 15:
            LcdHandlerSetMotorOvercurrentFault(true);
            break;
        case 16:
            LcdHandlerSetMotorOvervoltageFault(true);
            break;
        case 17:
            LcdHandlerSetMotorFetThermistorError(true);
            break;
        case 18: // clear 3 faults
            LcdHandlerSetBatteryUndervoltFault(false);
            LcdHandlerSetBatteryChargeOvercurrentFault(false);
            LcdHandlerSetBatteryDischargeOvercurrentFault(false);
            break;
        case 19:
            LcdHandlerSetMotorCommFault(true);
            break;
        case 20:
            LcdHandlerSetMotorThrottleAdcOutOfRange(true);
            break;
        case 21:
            LcdHandlerSetMotorThrottleAdcMismatch(true);
            break;
        case 22: // clear all faults
            LcdHandlerSetBatteryFault(false);
            LcdHandlerSetBatterySupplyLow(false);
            LcdHandlerSetBMSSelfTestFault(false);
            LcdHandlerSetBatteryVoltageHigh(false);
            LcdHandlerSetBatteryVoltageLow(false);
            LcdHandlerSetBatteryOvertemp(false);
            LcdHandlerSetBatterySlaveBoardCommFault(false);
            LcdHandlerSetBatteryOvervoltFault(false);
            LcdHandlerSetBatteryUndervoltFault(false);
            LcdHandlerSetBatteryChargeOvercurrentFault(false);
            LcdHandlerSetBatteryDischargeOvercurrentFault(false);
            LcdHandlerSetBatteryResetFromWatchdogFault(false);
            LcdHandlerSetMotorSystemFault(false);
            LcdHandlerSetMotorOvercurrentFault(false);
            LcdHandlerSetMotorOvervoltageFault(false);
            LcdHandlerSetMotorFetThermistorError(false);
            LcdHandlerSetMotorCommFault(false);
            LcdHandlerSetMotorThrottleAdcOutOfRange(false);
            LcdHandlerSetMotorThrottleAdcMismatch(false);
            break;
    }
    g_lcdtest_faultcount = (g_lcdtest_faultcount % 23) + 1;
    
}

void LcdTestWarningsPage(void)
{
    /**
     * Sets up test data for WARNINGS_PAGE
     * - All warnings set with alternating true/false pattern
     */
    
    switch (g_lcdtest_warningcount){
        case 1:
            LcdHandlerSetLowVoltWarning(true);
            break;
        case 2:
            LcdHandlerSetHighVoltWarning(true);
            break;
        case 3:
            LcdHandlerSetLowTempWarning(true);
            break;
        case 4:
            LcdHandlerSetLowVoltWarning(false);
            LcdHandlerSetHighVoltWarning(false);
            LcdHandlerSetLowTempWarning(false);
            break;
        case 5:
            LcdHandlerSetHighTempWarning(true);
            break;
        case 6:
            LcdHandlerSetNoEcuMessageWarning(true);
            break;
        case 7:
            LcdHandlerSetPackOverdischargeWarning(false);
            break;
        case 8:
            LcdHandlerSetHighTempWarning(false);
            LcdHandlerSetNoEcuMessageWarning(false);
            LcdHandlerSetPackOverdischargeWarning(false);
            break;            
        case 9:
            LcdHandlerSetPackOverchargeWarning(true);
            break;
        case 10:
            LcdHandlerSetLowVoltWarning(true);
            LcdHandlerSetHighVoltWarning(true);
            LcdHandlerSetLowTempWarning(true);
            LcdHandlerSetHighTempWarning(true);
            LcdHandlerSetNoEcuMessageWarning(true);
            LcdHandlerSetPackOverdischargeWarning(true);
            LcdHandlerSetPackOverchargeWarning(true);
            break;
        case 11:
            LcdHandlerSetLowVoltWarning(false);
            LcdHandlerSetHighVoltWarning(false);
            LcdHandlerSetLowTempWarning(false);
            LcdHandlerSetHighTempWarning(false);
            LcdHandlerSetNoEcuMessageWarning(false);
            LcdHandlerSetPackOverdischargeWarning(false);
            LcdHandlerSetPackOverchargeWarning(false);
            break;
    }
}

void LcdTestTemperaturePage(void)
{
    /**
     * Sets up test data for TEMPERATURE_PAGE
     * - 8 temperature sensors with values from 1-100
     * - MPPTA
     * - MPPTB
     * - MPPTC
     * - MPPTD
     * - BATT_MIN
     * - BATT_MAX
     * - MOTOR_CONT
     * - MOTOR_THERM
     */
    CyclicDataSetMpptATemperature(g_lcdtest_temperature);
    CyclicDataSetMpptBTemperature(g_lcdtest_temperature);
    CyclicDataSetMpptCTemperature(g_lcdtest_temperature);
    CyclicDataSetMpptDTemperature(g_lcdtest_temperature);
    CyclicDataSetBatteryMinTemperature(g_lcdtest_temperature);
    CyclicDataSetBatteryMaxTemperature(g_lcdtest_temperature);
    CyclicDataSetMtrContTemperature(g_lcdtest_temperature);
    CyclicDataSetMtrThermTemperature(g_lcdtest_temperature);

    g_lcdtest_temperature = (g_lcdtest_temperature % 101) + 1;
}