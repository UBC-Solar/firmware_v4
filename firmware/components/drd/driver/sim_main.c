#include "lcd_bitmap.h"
#include "lcd_driver_sim.h"
#include "lcd_sim_display.h"

#include <SDL.h>
#include <stdint.h>
#include <stdio.h>

enum
{
    TEST_BITMAP_X = 10,
    TEST_BITMAP_Y = 10,
    TEST_BITMAP_WIDTH = 8,
    TEST_BITMAP_HEIGHT = 11,
    TEST_BITMAP_BYTES_PER_COL = (TEST_BITMAP_HEIGHT + 7) / 8,
    TEST_BITMAP_SIZE = TEST_BITMAP_WIDTH * TEST_BITMAP_BYTES_PER_COL
};

int main(void)
{
    if (LcdSimDisplayInit() != 0)
    {
        return 1;
    }

    uint8_t pixels[TEST_BITMAP_HEIGHT][TEST_BITMAP_WIDTH] =
    {
        {1,1,1,1,1,1,1,1},
        {1,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,1},
        {1,0,0,0,0,0,0,1},
        {1,1,1,1,1,1,1,1}
    };

    uint8_t bitmap[TEST_BITMAP_SIZE];

    LcdBitmapEncode(&pixels[0][0], TEST_BITMAP_WIDTH, TEST_BITMAP_HEIGHT, bitmap);

    printf("Generated bitmap: ");

    for (uint8_t i = 0; i < TEST_BITMAP_SIZE; i++)
    {
        printf("0x%02X ", bitmap[i]);
    }

    printf("\n");

    LcdDriverChangeScreen();
    LcdDriverDrawBitmap(TEST_BITMAP_X, TEST_BITMAP_Y, TEST_BITMAP_WIDTH, TEST_BITMAP_HEIGHT, bitmap);
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
                    if (lcd_x >= TEST_BITMAP_X &&
                        lcd_x < TEST_BITMAP_X + TEST_BITMAP_WIDTH &&
                        lcd_y >= TEST_BITMAP_Y &&
                        lcd_y < TEST_BITMAP_Y + TEST_BITMAP_HEIGHT)
                    {
                        uint8_t pixel_x = lcd_x - TEST_BITMAP_X;
                        uint8_t pixel_y = lcd_y - TEST_BITMAP_Y;

                        pixels[pixel_y][pixel_x] ^= 1; // toggle pixel

                        LcdBitmapEncode(&pixels[0][0], TEST_BITMAP_WIDTH, TEST_BITMAP_HEIGHT, bitmap);
                        LcdDriverDrawBitmap(TEST_BITMAP_X, TEST_BITMAP_Y, TEST_BITMAP_WIDTH, TEST_BITMAP_HEIGHT, bitmap);
                        LcdDriverRefresh();

                        printf("Pixel (%u, %u): %s\n", lcd_x, lcd_y, pixels[pixel_y][pixel_x] ? "ON" : "OFF");

                        printf("Generated bitmap: ");

                        for (uint8_t i = 0; i < TEST_BITMAP_SIZE; i++)
                        {
                            printf("0x%02X ", bitmap[i]);
                        }

                        printf("\n");
                    }
                }
            }
        }

        SDL_Delay(10);
    }

    LcdSimDisplayShutdown();

    return 0;
}