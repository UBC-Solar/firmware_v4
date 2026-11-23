#include "lcd_app.h"
#include "lcd_bitmap.h"
#include "lcd_driver_sim.h"
#include "lcd_sim_display.h"

#include <SDL.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifdef LOAD_EXPORTED_BITMAP
#include "lcd_bitmap_export.h"
#endif

#ifndef LCD_BITMAP_EXPORT_PATH
#define LCD_BITMAP_EXPORT_PATH "app/lcd_bitmap_export.h"
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
    FILE* file = fopen(LCD_BITMAP_EXPORT_PATH, "w");

    if (!file)
    {
        printf("Failed to save bitmap\n");
        return;
    }

    fprintf(file, "#ifndef LCD_BITMAP_EXPORT_H\n");
    fprintf(file, "#define LCD_BITMAP_EXPORT_H\n\n");
    fprintf(file, "#include <stdint.h>\n\n");

    fprintf(file, "#define LCD_BITMAP_EXPORT_WIDTH %d\n", EDITOR_WIDTH);
    fprintf(file, "#define LCD_BITMAP_EXPORT_HEIGHT %d\n\n", EDITOR_HEIGHT);
    fprintf(file, "static const uint8_t lcd_bitmap_export[%d] =\n{\n", EDITOR_BITMAP_SIZE);

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

    printf("Saved bitmap to %s\n", LCD_BITMAP_EXPORT_PATH);
}

static void SyncEditorFromDisplay(uint8_t pixels[EDITOR_HEIGHT][EDITOR_WIDTH], uint8_t* bitmap)
{
    for (uint8_t y = 0; y < EDITOR_HEIGHT; y++)
    {
        for (uint8_t x = 0; x < EDITOR_WIDTH; x++)
        {
            pixels[y][x] = LcdDriverGetPixel(x, y);
        }
    }

    LcdBitmapEncode(&pixels[0][0], EDITOR_WIDTH, EDITOR_HEIGHT, bitmap);
}

static void LoadDrivePage(void)
{
    uint32_t speed = 42;
    uint32_t soc = 75;
    uint8_t drive_mode = LCD_APP_DRIVE_MODE_ECO;
    DriveStateStates drive_state = FORWARD;

    LcdAppBattFaults batt_faults = {0};
    LcdAppMotorFaults motor_faults = {0};
    LcdAppWarnings warnings = {0};

    LcdDriverChangeScreen();

    LcdAppDisplaySpeedDrivePage(&speed, LCD_APP_KPH);
    LcdAppDisplaySocDrivePage(&soc);
    LcdAppDisplayDriveModeDrivePage(drive_mode);
    LcdAppDisplayDriveStateDrivePage(&drive_state);
    LcdAppDisplayFaultIndicator(&batt_faults, &motor_faults);
    LcdAppDisplayWarningIndicator(&warnings);

    printf("Loaded Drive page\n");
}

static void LoadFaultsPage(void)
{
    LcdAppBattFaults batt_faults = {0};
    LcdAppMotorFaults motor_faults = {0};

    batt_faults.battery_fault = true;
    batt_faults.voltage_high = true;
    batt_faults.overtemp_fault = true;

    LcdDriverChangeScreen();
    LcdAppDisplayFaults(&batt_faults, &motor_faults);

    printf("Loaded Faults page\n");
}

static void LoadWarningsPage(void)
{
    LcdAppWarnings warnings = {0};

    warnings.low_volt_warning = true;
    warnings.high_temp_warning = true;
    warnings.no_ecu_message = true;

    LcdDriverChangeScreen();
    LcdAppDisplayWarnings(&warnings);

    printf("Loaded Warnings page\n");
}

static void LoadTemperaturePage(void)
{
    uint8_t temperatures[8] = {25, 30, 35, 40, 45, 50, 55, 60};

    LcdDriverChangeScreen();

    for (uint8_t i = 0; i < 8; i++)
    {
        LcdAppTemperature temperature_data =
        {
            .temperature = &temperatures[i],
            .temp_label = i
        };

        LcdAppDisplayTemperature(temperature_data);
    }

    printf("Loaded Temperature page\n");
}

static void LoadDebugPage(void)
{
    uint32_t speed = 42;
    uint32_t soc = 75;
    DriveStateStates drive_state = FORWARD;
    int16_t pack_current = 50;
    uint16_t pack_voltage = 48;

    LcdDriverChangeScreen();

    LcdAppDisplaySpeedDebugPage(&speed, LCD_APP_KPH);
    LcdAppDisplaySocDebugPage(&soc);
    LcdAppDisplayDriveStateDebugPage(&drive_state);
    LcdAppDisplayPowerBar(&pack_current, &pack_voltage);

    printf("Loaded Debug page\n");
}

int main(void)
{
    if (LcdSimDisplayInit() != 0)
    {
        return 1;
    }

    uint8_t pixels[EDITOR_HEIGHT][EDITOR_WIDTH] = {0};
    uint8_t bitmap[EDITOR_BITMAP_SIZE];

#ifdef LOAD_EXPORTED_BITMAP

    LcdDriverChangeScreen();
    LcdDriverDrawBitmap(0, 0, LCD_BITMAP_EXPORT_WIDTH, LCD_BITMAP_EXPORT_HEIGHT, lcd_bitmap_export);
    LcdDriverRefresh();

#else

    LoadDrivePage();

#endif

    SyncEditorFromDisplay(pixels, bitmap);

    printf("Controls:\n");
    printf("1 = Drive\n");
    printf("2 = Faults\n");
    printf("3 = Warnings\n");
    printf("4 = Temperature\n");
    printf("5 = Debug\n");
    printf("E = Print bitmap\n");
    printf("S = Save bitmap\n");

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

            if (event.type == SDL_KEYDOWN)
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_1:
                    LoadDrivePage();
                    SyncEditorFromDisplay(pixels, bitmap);
                    break;

                case SDLK_2:
                    LoadFaultsPage();
                    SyncEditorFromDisplay(pixels, bitmap);
                    break;

                case SDLK_3:
                    LoadWarningsPage();
                    SyncEditorFromDisplay(pixels, bitmap);
                    break;

                case SDLK_4:
                    LoadTemperaturePage();
                    SyncEditorFromDisplay(pixels, bitmap);
                    break;

                case SDLK_5:
                    LoadDebugPage();
                    SyncEditorFromDisplay(pixels, bitmap);
                    break;

                case SDLK_e:
                    LcdBitmapEncode(&pixels[0][0], EDITOR_WIDTH, EDITOR_HEIGHT, bitmap);
                    PrintBitmap(bitmap);
                    break;

                case SDLK_s:
                    LcdBitmapEncode(&pixels[0][0], EDITOR_WIDTH, EDITOR_HEIGHT, bitmap);
                    SaveBitmap(bitmap);
                    break;

                default:
                    break;
                }
            }
        }

        SDL_Delay(10);
    }

    LcdSimDisplayShutdown();

    return 0;
}