#include "lcd_sim_display.h"
#include <SDL.h>

#define LCD_SIM_SCALE 8
#define LCD_SIM_WIDTH 128
#define LCD_SIM_HEIGHT 64

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;

int LcdSimDisplayInit()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        return -1;
    }

    window = SDL_CreateWindow("LCD Simulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, LCD_SIM_WIDTH * LCD_SIM_SCALE, LCD_SIM_HEIGHT * LCD_SIM_SCALE, 0);

    if (!window)
    {
        SDL_Quit();
        return -1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (!renderer)
    {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    return 0;
}

void LcdSimDisplayRender(const uint8_t* buffer)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    for (uint8_t y = 0; y < LCD_SIM_HEIGHT; y++)
    {
        for (uint8_t x = 0; x < LCD_SIM_WIDTH; x++)
        {
            uint16_t array_pos = x + ((y / 8) * LCD_SIM_WIDTH);
            uint8_t bit_pos = y % 8;

            if (buffer[array_pos] & (1 << bit_pos))
            {
                SDL_Rect pixel = {x * LCD_SIM_SCALE, y * LCD_SIM_SCALE, LCD_SIM_SCALE, LCD_SIM_SCALE};
                SDL_RenderFillRect(renderer, &pixel);
            }
        }
    }

    SDL_RenderPresent(renderer);
}

void LcdSimDisplayShutdown()
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}