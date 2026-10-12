#include "lcd_driver_sim.h"

static uint8_t lcd_buffer[(LCD_DRIVER_SCREEN_WIDTH * LCD_DRIVER_SCREEN_HEIGHT) / 8] = {0};

void LcdDriverSetPixel(uint8_t x, uint8_t y, uint8_t color)
{
    if (x >= LCD_DRIVER_SCREEN_WIDTH || y >= LCD_DRIVER_SCREEN_HEIGHT)
        return;

    uint16_t array_pos = x + ((y / 8) * LCD_DRIVER_SCREEN_WIDTH);

    if (color)
    {
        lcd_buffer[array_pos] |= 1 << (y % 8);
    }
    else
    {
        lcd_buffer[array_pos] &= ~(1 << (y % 8));
    }
}

void LcdDriverClearBoundingBox(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
    if (x1 >= LCD_DRIVER_SCREEN_WIDTH ||
        x2 >= LCD_DRIVER_SCREEN_WIDTH ||
        y1 >= LCD_DRIVER_SCREEN_HEIGHT ||
        y2 >= LCD_DRIVER_SCREEN_HEIGHT ||
        x1 > x2 ||
        y1 > y2)
        {
            return;
        }

    for (uint8_t i = x1; i <= x2; i++)
    {
        for (uint8_t j = y1; j <= y2; j++)
        {
            uint16_t array_pos = i + ((j / 8) * LCD_DRIVER_SCREEN_WIDTH);

            lcd_buffer[array_pos] = 0;
        }
    }
}

void LcdDriverDrawRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color)
{
    for (uint8_t x = x1; x <= x2; x++)
    {
        LcdDriverSetPixel(x, y1, color);
        LcdDriverSetPixel(x, y2, color);
    }
    for (uint8_t y = y1; y <= y2; y++)
    {
        LcdDriverSetPixel(x1, y, color);
        LcdDriverSetPixel(x2, y, color);
    }
}

LcdDriverBoundingBox LcdDriverDrawChar(unsigned char c, unsigned char x, unsigned char y, const unsigned char* font)
{
    unsigned short pos;
    uint8_t width;
    LcdDriverBoundingBox ret;

    ret.x1 = x;
    ret.y1 = y;
    ret.x2 = x;
    ret.y2 = y;

    // Read first byte, should be 0x01 for proportional
    if (font[LCD_DRIVER_FONT_HEADER_TYPE] != LCD_DRIVER_FONT_TYPE_PROPORTIONAL)
        return ret;

    // Check second byte, should be 0x02 for "vertical ceiling"
    if (font[LCD_DRIVER_FONT_HEADER_ORIENTATION] != LCD_DRIVER_FONT_ORIENTATION_VERTICAL_CEILING)
        return ret;

    // Check that font start + number of bitmaps contains c
    if (!(c >= font[LCD_DRIVER_FONT_HEADER_START] &&
          c <= font[LCD_DRIVER_FONT_HEADER_START] + font[LCD_DRIVER_FONT_HEADER_LETTERS]))
        return ret;

    // Adjust for start position of font vs. the char passed
    c -= font[LCD_DRIVER_FONT_HEADER_START];

    // Work out where in the array the character is
    pos = font[c * LCD_DRIVER_FONT_HEADER_START + 5];
    pos <<= 8;
    pos |= font[c * LCD_DRIVER_FONT_HEADER_START + 6];

    // Read first byte from this position, this gives letter width
    width = font[pos];

    // Draw left to right
    uint8_t i;
    for (i = 0; i < width; i++)
    {

        // Draw top to bottom
        for (uint8_t j = 0; j < font[LCD_DRIVER_FONT_HEADER_HEIGHT]; j++)
        {
            // Increment one data byte every 8 bits, or
            // at the start of a new column  HiTech optimizes
            // the modulo, so no need to try and avoid it.
            if (j % 8 == 0)
                pos++;

            if (font[pos] & 1 << (j % 8))
            {
                LcdDriverSetPixel(x + i, y + j, 1);
            }
            else
            {
                LcdDriverSetPixel(x + i, y + j, 0);
            }
        }
    }

    ret.x2 = ret.x1 + width - 1;
    ret.y2 = ret.y1 + font[LCD_DRIVER_FONT_HEADER_HEIGHT];

    return ret;
}

LcdDriverBoundingBox LcdDriverDrawText(char* string, unsigned char x, unsigned char y, const unsigned char* font, unsigned char spacing)
{
    LcdDriverBoundingBox ret;
    LcdDriverBoundingBox tmp = {0};

    ret.x1 = x;
    ret.y1 = y;

    spacing += 1;

    // BUG: As we move right between chars we don't actually wipe the space
    while (*string != 0)
    {
        tmp = LcdDriverDrawChar(*string++, x, y, font);

        // Leave a single space between characters
        x = tmp.x2 + spacing;
    }

    ret.x2 = tmp.x2;
    ret.y2 = tmp.y2;

    return ret;
}