#ifndef LCD_DRIVER_SIM_H
#define LCD_DRIVER_SIM_H

#include <stdint.h>

/** LCD Screen Constants */

#define LCD_DRIVER_SCREEN_HEIGHT 64
#define LCD_DRIVER_SCREEN_WIDTH 128

#define LCD_DRIVER_BOTTOM_RIGHT_X 127
#define LCD_DRIVER_BOTTOM_RIGHT_Y 63

/* Font Parameters */

#define LCD_DRIVER_FONT_HEADER_TYPE 0
#define LCD_DRIVER_FONT_HEADER_ORIENTATION 1
#define LCD_DRIVER_FONT_HEADER_START 2
#define LCD_DRIVER_FONT_HEADER_LETTERS 3
#define LCD_DRIVER_FONT_HEADER_HEIGHT 4

#define LCD_DRIVER_FONT_TYPE_FIXED 0
#define LCD_DRIVER_FONT_TYPE_PROPORTIONAL 1

#define LCD_DRIVER_FONT_ORIENTATION_VERTICAL_CEILING 2

typedef struct
{
    uint8_t x1;
    uint8_t y1;
    uint8_t x2;
    uint8_t y2;
} LcdDriverBoundingBox;

/* LCD Drawing Functions */

/**
 * @brief Sets or clears a single pixel in the internal display buffer.
 *
 * @param x The x coordinate (1-based).
 * @param y The y coordinate (1-based).
 * @param color 1 to set the pixel, 0 to clear it.
 */
void LcdDriverSetPixel(uint8_t x, uint8_t y, uint8_t colour);

/**
 * @brief Clears a rectangular area in the internal display buffer.
 *
 * @param x1 Left coordinate (0-based).
 * @param y1 Top coordinate (0-based).
 * @param x2 Right coordinate (0-based).
 * @param y2 Bottom coordinate (0-based).
 */
void LcdDriverClearBoundingBox(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);

/**
 * @brief Refreshes the simulated LCD display from the internal display buffer.
 */
void LcdDriverRefresh(void);

/**
 * @brief Draws a rectangle outline using the internal pixel function.
 *
 * @param x1 Left coordinate (1-based).
 * @param y1 Top coordinate (1-based).
 * @param x2 Right coordinate (1-based).
 * @param y2 Bottom coordinate (1-based).
 * @param color 1 to draw pixel.
 */
void LcdDriverDrawRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color);

/**
 * @brief Forces a clear a rectangular area in the internal display buffer.
 *
 * @param x1 Left coordinate (0-based).
 * @param y1 Top coordinate (0-based).
 * @param x2 Right coordinate (0-based).
 * @param y2 Bottom coordinate (0-based).
 */
void LcdDriverForceClearBoundingBox(unsigned char x1,unsigned char y1,unsigned char x2,unsigned char y2);

/**
 * @brief Draws a text string using an external graphics library.
 *
 * @param str The null-terminated string to draw.
 * @param x Starting x coordinate.
 * @param y Starting y coordinate.
 * @param font Pointer to the font to use.
 * @param spacing Spacing between characters.
 * @return LcdDriverBoundingBox The bounding box of the drawn text.
 */
LcdDriverBoundingBox LcdDriverDrawText(char* string, unsigned char x, unsigned char y, const unsigned char* font, unsigned char spacing);

/**
 * @brief Draws a single character using an external graphics library.
 *
 * @param c The character to draw.
 * @param x Starting x coordinate.
 * @param y Starting y coordinate.
 * @param font Pointer to the font to use.
 * @return LcdDriverBoundingBox The bounding box of the drawn character.
 */
LcdDriverBoundingBox LcdDriverDrawChar(unsigned char c, unsigned char x, unsigned char y, const unsigned char* font);

/**
 * @brief Changes the screen
 */
void LcdDriverChangeScreen(void);

#endif /* LCD_DRIVER_SIM_H */