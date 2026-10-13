#include "lcd_driver_sim.h"
#include "font_verdana.h"

int main(void)
{
    char text[] = "HELLO";
    LcdDriverDrawText(text, 10, 10, Verdana8, 1);

    LcdDriverRefresh();

    return 0;
}