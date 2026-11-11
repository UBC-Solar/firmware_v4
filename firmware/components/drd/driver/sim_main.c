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

static void PrintBitmap(const uint8_t* bitmap)
{
    printf("\nconst uint8_t bitmap[%d] =\n{\n", EDITOR_BITMAP_SIZE);

    for (uint16_t i = 0; i < EDITOR_BITMAP_SIZE; i++)
    {
        printf("0x%02X", bitmap[i]);

        if (i < EDITOR_BITMAP_SIZE - 1)
        {
            printf(", ");
        }

        if ((i + 1) % 16 == 0)
        {
            printf("\n");
        }
    }

    printf("};\n\n");
}

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

            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_e)
            {
                LcdBitmapEncode(&pixels[0][0], EDITOR_WIDTH, EDITOR_HEIGHT, bitmap);
                PrintBitmap(bitmap);
            }
        }

        SDL_Delay(10);
    }

    LcdSimDisplayShutdown();

    return 0;
}