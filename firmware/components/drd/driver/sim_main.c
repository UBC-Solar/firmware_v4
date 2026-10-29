#include "lcd_app.h"
#include "lcd_bitmap.h"
#include "lcd_sim_display.h"

#include <SDL.h>
#include <stdint.h>
#include <stdio.h>

enum
{
    TEST_BITMAP_WIDTH = 8,
    TEST_BITMAP_HEIGHT = 11,
    TEST_BITMAP_BYTES_PER_COL = (TEST_BITMAP_HEIGHT + 7) / 8,
    TEST_BITMAP_SIZE = TEST_BITMAP_WIDTH * TEST_BITMAP_BYTES_PER_COL,
    TEST_BITMAP_DATA_START = 8
};

unsigned char TestBitmap[TEST_BITMAP_DATA_START + TEST_BITMAP_SIZE] =
{
    // Font Header
    0x01,   // FONT_TYPE_PROPORTIONAL
    0x02,   // FONT_ORIENTATION_VERTICAL_CEILING
    0x58,   // Start char
    0x01,   // One character
    0x0B,   // Height

    // Bitmap Index
    0x00, 0x07,

    // Bitmap
    0x08    // Width
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

    LcdBitmapEncode(
        &pixels[0][0],
        TEST_BITMAP_WIDTH,
        TEST_BITMAP_HEIGHT,
        bitmap
    );

    for (uint8_t i = 0; i < TEST_BITMAP_SIZE; i++)
    {
        TestBitmap[TEST_BITMAP_DATA_START + i] = bitmap[i];
    }

    printf("Generated bitmap: ");

    for (uint8_t i = 0; i < TEST_BITMAP_SIZE; i++)
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