#include "lcd_app.h"
#include "lcd_sim_display.h"
#include <SDL.h>

int main(void)
{
    if (LcdSimDisplayInit() != 0)
    {
        return 1;
    }

    volatile uint32_t speed = 42;
    volatile uint8_t units = LCD_APP_KPH;
    volatile uint32_t soc = 73;
    volatile uint8_t drive_mode = LCD_APP_DRIVE_MODE_ECO;
    volatile DriveStateStates state = FORWARD;

    LcdAppDisplaySpeedDrivePage(&speed, units);
    LcdAppDisplaySocDrivePage(&soc);
    LcdAppDisplayDriveModeDrivePage(drive_mode);
    LcdAppDisplayDriveStateDrivePage(&state);

    SDL_Event event;
    int running = 1;

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = 0;
            }
        }

        SDL_Delay(10);
    }

    LcdSimDisplayShutdown();

    return 0;
}