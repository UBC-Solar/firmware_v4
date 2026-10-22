#ifndef LCD_SIM_DISPLAY_H
#define LCD_SIM_DISPLAY_H

#include <stdint.h>

int LcdSimDisplayInit();
void LcdSimDisplayRender(const uint8_t* buffer);
void LcdSimDisplayShutdown();

#endif