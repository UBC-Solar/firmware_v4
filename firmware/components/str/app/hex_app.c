#include "hex_app.h"

#include <math.h>

#define STR_DISPLAY_MAX 99

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