#include "lcd_driver_sim.h"

static uint8_t lcd_buffer[(LCD_DRIVER_SCREEN_WIDTH * LCD_DRIVER_SCREEN_HEIGHT) / 8] = {0};

void LcdDriverSetPixel(uint8_t x, uint8_t y, uint8_t colour)
{
    if (x >= LCD_DRIVER_SCREEN_WIDTH || y >= LCD_DRIVER_SCREEN_HEIGHT)
        return;

    uint16_t array_pos = x + ((y / 8) * LCD_DRIVER_SCREEN_WIDTH);

    if (colour)
    {
        lcd_buffer[array_pos] |= 1 << (y % 8);
    }
    else
    {
        lcd_buffer[array_pos] &= ~(1 << (y % 8));
    }
}