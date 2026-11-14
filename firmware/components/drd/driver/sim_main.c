#include "lcd_app.h"
#include "lcd_bitmap.h"
#include "lcd_driver_sim.h"
#include "lcd_sim_display.h"

#include <SDL.h>
#include <stdint.h>
#include <stdio.h>

#ifdef LOAD_EXPORTED_BITMAP
#include "lcd_bitmap_export.h"
#endif

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

static void SaveBitmap(const uint8_t* bitmap)
{
    FILE* file = fopen("lcd_bitmap_export.h", "w");

    if (!file)
    {
        printf("Failed to save bitmap\n");
        return;
    }

    fprintf(file, "#ifndef LCD_BITMAP_EXPORT_H\n");
    fprintf(file, "#define LCD_BITMAP_EXPORT_H\n\n");
    fprintf(file, "#include<stdint.h>\n\n");

    fprintf(file, "#define LCD_BITMAP_EXPORT_WIDTH %d\n", EDITOR_WIDTH);
    fprintf(file, "#define LCD_BITMAP_EXPORT_HEIGHT %d\n\n", EDITOR_HEIGHT);
    fprintf(file, "static const uint8_t lcd_bitmap_export[%d] = \n{\n", EDITOR_BITMAP_SIZE);

    for (uint16_t i = 0; i < EDITOR_BITMAP_SIZE; i++)
    {
        fprintf(file, "0x%02X", bitmap[i]);

        if (i < EDITOR_BITMAP_SIZE - 1)
        {
            fprintf(file, ", ");
        }

        if ((i + 1) % 16 == 0)
        {
            fprintf(file, "\n");
        }
    }

    fprintf(file, "};\n\n");
    fprintf(file, "#endif\n");

    fclose(file);

    printf("Saved bitmap to lcd_bitmap_export.h\n");
}

int main(void)
{
    if (LcdSimDisplayInit() != 0)
    {
        return 1;
    }

    uint8_t pixels[EDITOR_HEIGHT][EDITOR_WIDTH] = {0};
    uint8_t bitmap[EDITOR_BITMAP_SIZE];

    LcdDriverChangeScreen();

#ifdef LOAD_EXPORTED_BITMAP

    LcdDriverDrawBitmap(0, 0, LCD_BITMAP_EXPORT_WIDTH, LCD_BITMAP_EXPORT_HEIGHT, lcd_bitmap_export);
    LcdDriverRefresh();

#else

    uint32_t speed = 42;
    uint32_t soc = 75;
    uint8_t drive_mode = LCD_APP_DRIVE_MODE_ECO;
    DriveStateStates drive_state = FORWARD;

    LcdAppBattFaults batt_faults = {0};
    LcdAppMotorFaults motor_faults = {0};
    LcdAppWarnings warnings = {0};

    LcdAppDisplaySpeedDrivePage(&speed, LCD_APP_KPH);
    LcdAppDisplaySocDrivePage(&soc);
    LcdAppDisplayDriveModeDrivePage(drive_mode);
    LcdAppDisplayDriveStateDrivePage(&drive_state);
    LcdAppDisplayFaultIndicator(&batt_faults, &motor_faults);
    LcdAppDisplayWarningIndicator(&warnings);

#endif

    for (uint8_t y = 0; y < EDITOR_HEIGHT; y++)
    {
        for (uint8_t x = 0; x < EDITOR_WIDTH; x++)
        {
            pixels[y][x] = LcdDriverGetPixel(x, y);
        }
    }

    LcdBitmapEncode(&pixels[0][0], EDITOR_WIDTH, EDITOR_HEIGHT, bitmap);

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

            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_s)
            {
                LcdBitmapEncode(&pixels[0][0], EDITOR_WIDTH, EDITOR_HEIGHT, bitmap);
                SaveBitmap(bitmap);
            }
        }

        SDL_Delay(10);
    }

    LcdSimDisplayShutdown();

    return 0;
}