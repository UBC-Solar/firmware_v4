#include "hex_app.h"

#include <math.h>

#include "hex_driver.h"

enum
{
    STR_DISPLAY_MAX = 99U,
};

#define STR_WHEEL_RADIUS_M 0.283f

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

void SteeringVelocityCanMsgHandler(uint8_t* data)
{
    if (data == NULL)
    {
        return;
    }

    uint32_t rpm = (data[4] >> 3) | ((data[5] & 0x7f) << 5);
    float velocity_mps = (STR_WHEEL_RADIUS_M * 2.0f * (float)M_PI * (float)rpm) / 60.0f;
    uint32_t velocity_kmh = (uint32_t)(velocity_mps * 3.6f);

    if (velocity_kmh > STR_DISPLAY_MAX)
    {
        velocity_kmh = STR_DISPLAY_MAX;
    }

    HexDisplayWriteDecimal((uint8_t)velocity_kmh);
}