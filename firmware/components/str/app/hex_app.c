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
        HexDisplayWriteDecimal((uint8_t)(*speed));
    }
    else
    {
        //if no speed is available, display dashes
        HexDisplayWriteDashes();
    }
}
