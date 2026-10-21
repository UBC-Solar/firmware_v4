#include "lcd_app.h"
#include "lcd_bitmap.h"
#include "lcd_sim_display.h"

#include <SDL.h>
#include <stdint.h>
#include <stdio.h>

unsigned char TestBitmap[] =
{
    // Font Header
    0x01,   // FONT_TYPE_PROPORTIONAL
    0x02,   // FONT_ORIENTATION_VERTICAL_CEILING
    0x58,   // Start char = 88 ('X')
    0x01,   // One character
    0x0B,   // Height = 11

    // Bitmap Index
    0x00, 0x07,

    // Bitmap
    0x08,
    0, 0,
    0, 0,
    0, 0,
    0, 0,
    0, 0,
    0, 0,
    0, 0,
    0, 0
};

int main(void)
{
    if (LcdSimDisplayInit() != 0)
    {
        return 1;
    }

    const uint8_t width = 8;
    const uint8_t height = 11;

    uint8_t pixels[11][8] =
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

    uint8_t bitmap[16];

    LcdBitmapEncode(&pixels[0][0], width, height, bitmap);

    for (uint8_t i = 0; i < 16; i++)
    {
        TestBitmap[8 + i] = bitmap[i];
    }

    printf("Generated bitmap: ");

    for (uint8_t i = 0; i < 16; i++)
    {
        printf("0x%02X ", bitmap[i]);
    }

    printf("\n");

    LcdDriverChangeScreen();

    LcdDriverDrawChar('X', 10, 10, TestBitmap);

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
        }

        SDL_Delay(10);
    }

    LcdSimDisplayShutdown();

    return 0;
}