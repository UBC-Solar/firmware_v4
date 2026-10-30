#ifndef LCD_SIM_DISPLAY_H
#define LCD_SIM_DISPLAY_H

#include <stdint.h>

int LcdSimDisplayInit();
void LcdSimDisplayRender(const uint8_t* buffer);
int LcdSimDisplayWindowToPixel(int window_x, int window_y, uint8_t* lcd_x, uint8_t* lcd_y);
void LcdSimDisplayShutdown();

#endif