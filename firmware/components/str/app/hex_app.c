/**
 * @file    hex_app.c
 * @brief   Hex display application implementation for UBC Solar STR board
 *
 * This file implements display formatting logic for clamping and writing vehicle speed values
 * to the steering wheel hex display.
 *
 * @author  Tony Chen
 * @date    Jun 7, 2026
 */

/* INCLUDES */
#include "hex_app.h"

#include "cyclic_data_handler.h"
#include <math.h>

/* DEFINES */
#define STR_DISPLAY_MAX 99
#define AS1115_CODE_B_DASH 0x0A // Code-B decode glyph for '-'
#define KMH_TO_MPH_MULTIPLIER 0.621371f

/* PRIVATE VARIABLES */
// Matches the DRD power-on default until the first motor command frame arrives
static volatile uint8_t s_speed_units = STR_SPEED_UNITS_KPH;

/* SPEED UNITS */
uint8_t HexAppSetSpeedUnits(uint8_t speed_units)
{
    s_speed_units = speed_units;
    return s_speed_units;
}

uint8_t HexAppGetSpeedUnits(void)
{
    return s_speed_units;
}

/* DISPLAY OUTPUT */
void HexDisplayWriteDecimal(uint8_t num)
{
    if (num > STR_DISPLAY_MAX) // display is limited to 2 decimal places
    {
        num = STR_DISPLAY_MAX;
    }

    uint8_t tens_digit = num / 10;
    uint8_t ones_digit = num % 10;

    HexDisplayWriteReg(AS1115_REG_DIGIT0, tens_digit);
    HexDisplayWriteReg(AS1115_REG_DIGIT1, ones_digit);
}

void HexDisplayWriteDashes(void)
{
    HexDisplayWriteReg(AS1115_REG_DIGIT0, AS1115_CODE_B_DASH);
    HexDisplayWriteReg(AS1115_REG_DIGIT1, AS1115_CODE_B_DASH);
}

void HexAppUpdate(void)
{
    uint32_t* speed = CyclicDataGetSpeed();

    if (speed != NULL)
    {
        uint32_t display_speed = *speed; // cyclic speed is always stored in km/h

        if (STR_SPEED_UNITS_MPH == s_speed_units)
        {
            display_speed = (uint32_t)((float)display_speed * KMH_TO_MPH_MULTIPLIER);
        }

        if (display_speed > STR_DISPLAY_MAX) // clamp before narrowing so large speeds do not wrap
        {
            display_speed = STR_DISPLAY_MAX;
        }

        HexDisplayWriteDecimal((uint8_t)display_speed);
    }
    else
    {
        //if no speed is available, display dashes
        HexDisplayWriteDashes();
    }
}
