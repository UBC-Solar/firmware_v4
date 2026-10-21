#ifndef LCD_BITMAP_H
#define LCD_BITMAP_H

#include <stdint.h>

void LcdBitmapEncode(const uint8_t *pixels, uint8_t width, uint8_t height, uint8_t *bitmap);

#endif