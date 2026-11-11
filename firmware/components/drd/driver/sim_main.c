#include "lcd_bitmap.h"
#include "lcd_driver_sim.h"
#include "lcd_sim_display.h"

#include <SDL.h>
#include <stdint.h>
#include <stdio.h>

enum
{
    EDITOR_WIDTH = LCD_DRIVER_SCREEN_WIDTH,
    EDITOR_HEIGHT = LCD_DRIVER_SCREEN_HEIGHT,
    EDITOR_BYTES_PER_COL = (EDITOR_HEIGHT + 7) / 8,
    EDITOR_BITMAP_SIZE = EDITOR_WIDTH * EDITOR_BYTES_PER_COL
};

int main(void)
{
    if (LcdSimDisplayInit() != 0)
    {
        return 1;
    }

    uint8_t pixels[EDITOR_HEIGHT][EDITOR_WIDTH] = {0};
    uint8_t bitmap[EDITOR_BITMAP_SIZE];

    LcdBitmapEncode(&pixels[0][0], EDITOR_WIDTH, EDITOR_HEIGHT, bitmap);

    LcdDriverChangeScreen();
    LcdDriverDrawBitmap(0, 0, EDITOR_WIDTH, EDITOR_HEIGHT, bitmap);
    LcdDriverRefresh();

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

            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
            {
                uint8_t lcd_x;
                uint8_t lcd_y;

                if (LcdSimDisplayWindowToPixel(event.button.x, event.button.y, &lcd_x, &lcd_y))
                {
                    pixels[lcd_y][lcd_x] ^= 1;

                    LcdBitmapEncode(&pixels[0][0], EDITOR_WIDTH, EDITOR_HEIGHT, bitmap);
                    LcdDriverDrawBitmap(0, 0, EDITOR_WIDTH, EDITOR_HEIGHT, bitmap);
                    LcdDriverRefresh();

                    printf("Pixel (%u, %u): %s\n", lcd_x, lcd_y, pixels[lcd_y][lcd_x] ? "ON" : "OFF");
                }
            }
        }

        SDL_Delay(10);
    }

    LcdSimDisplayShutdown();

    return 0;
}