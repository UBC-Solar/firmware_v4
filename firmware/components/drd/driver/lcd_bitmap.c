#include "lcd_bitmap.h"

void LcdBitmapEncode(const uint8_t *pixels, uint8_t width, uint8_t height, uint8_t *bitmap)
{
    uint8_t bytes_per_column = (height + 7) / 8;
    uint16_t bitmap_size = width * bytes_per_column;

    for (uint16_t i = 0; i < bitmap_size; i++)
    {
        bitmap[i] = 0;
    }

    for (uint8_t x = 0; x < width; x++)
    {
        for (uint8_t y = 0; y < height; y++)
        {
            if (pixels[y * width + x])
            {
                uint8_t byte_in_column = y / 8;
                uint8_t bit_in_byte = y % 8;

                uint16_t bitmap_index = x * bytes_per_column + byte_in_column;

                bitmap[bitmap_index] |= 1 << bit_in_byte;
            }
        }
    }
}