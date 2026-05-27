#include "hex_driver.h"

#define AS1115_ADDR_7BIT       0x00
#define AS1115_ADDR_HAL        (AS1115_ADDR_7BIT << 1)

#define AS1115_REG_DECODE      0x09
#define AS1115_REG_INTENSITY   0x0A
#define AS1115_REG_SCAN_LIMIT  0x0B
#define AS1115_REG_SHUTDOWN    0x0C
#define AS1115_REG_TEST        0x0F

extern I2C_HandleTypeDef hi2c1;

bool HexDisplayInit(void)
{
    // Disable display test mode
    if (HexDisplayWriteReg(AS1115_REG_TEST, 0x00) != HAL_OK)
    {
        return false;
    }

    // Enable Hex decode for display digit 0 and 1
    if (HexDisplayWriteReg(AS1115_REG_DECODE, 0x03) != HAL_OK)
    {
        return false;
    }

    // Scan only display digit 1 and 2
    if (HexDisplayWriteReg(AS1115_REG_SCAN_LIMIT, 0x01) != HAL_OK)
    {
        return false;
    }

    // Set brightness, 0x00 = dimmest and 0x0F = brightest
    if (HexDisplayWriteReg(AS1115_REG_INTENSITY, 0x08) != HAL_OK)
    {
        return false;
    }

    // Leave shutdown and enter normal operation
    if (HexDisplayWriteReg(AS1115_REG_SHUTDOWN, 0x01) != HAL_OK)
    {
        return false;
    }

    return true;
}

HAL_StatusTypeDef HexDisplayWriteReg(uint8_t reg, uint8_t data)
{
    uint8_t i2c_buf[2] = {reg, data};

    return HAL_I2C_Master_Transmit(&hi2c1, AS1115_ADDR_HAL, i2c_buf, 2, HAL_MAX_DELAY);
}