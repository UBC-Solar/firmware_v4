#include "lcd_app.h"

int main(void)
{
    volatile uint32_t speed = 42;
    volatile uint8_t units = LCD_APP_KPH;

    LcdAppDisplaySpeedDrivePage(&speed, units);

    return 0;
}