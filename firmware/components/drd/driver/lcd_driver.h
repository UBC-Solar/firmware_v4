#include <stdint.h>

/** LCD Screen Constants */
#define LCD_DRIVER_DIRTY_PAGE_CHANGE 0xFF

#define LCD_DRIVER_SCREEN_HEIGHT 64
#define LCD_DRIVER_SCREEN_WIDTH 128

#define LCD_DRIVER_BOTTOM_RIGHT_X 127
#define LCD_DRIVER_BOTTOM_RIGHT_Y 63

#define LCD_DRIVER_ST7565_DIRTY_PAGES

/* Font Parameters */
#define LCD_DRIVER_FONT_HEADER_TYPE 0
#define LCD_DRIVER_FONT_HEADER_ORIENTATION 1
#define LCD_DRIVER_FONT_HEADER_START 2
#define LCD_DRIVER_FONT_HEADER_LETTERS 3
#define LCD_DRIVER_FONT_HEADER_HEIGHT 4

#define LCD_DRIVER_FONT_TYPE_FIXED 0
#define LCD_DRIVER_FONT_TYPE_PROPORTIONAL 1

#define LCD_DRIVER_FONT_ORIENTATION_VERTICAL_CEILING 2

/* LCD COMMAND PARAMS */
#define LCD_DRIVER_CMD_SET_ADC_NORMAL 0xA0
#define LCD_DRIVER_CMD_DISPLAY_OFF 0xAE
#define LCD_DRIVER_CMD_SET_COM_NORMAL 0xC0
#define LCD_DRIVER_CMD_SET_BIAS_9 0xA2
#define LCD_DRIVER_CMD_SET_POWER_CONTROL 0x28
#define LCD_DRIVER_CMD_SET_RESISTOR_RATIO 0x20
#define LCD_DRIVER_CMD_SET_VOLUME_FIRST 0x81
#define LCD_DRIVER_CMD_SET_CONTRAST 0x11
#define LCD_DRIVER_CMD_DISPLAY_ON 0xAF
#define LCD_DRIVER_CMD_SET_ALLPTS_NORMAL 0xA4

/** Command: Set the current page (0..7). */
#define LCD_DRIVER_CMD_SET_PAGE 0b10110000
/** Command: set the least significant 4 bits of the column address. */
#define LCD_DRIVER_CMD_COLUMN_LOWER 0b00000000
/** Command: set the most significant 4 bits of the column address. */
#define LCD_DRIVER_CMD_COLUMN_UPPER 0b00010000
#define LCD_DRIVER_CMD_DISPLAY_START 0b01000000

typedef struct
{
    uint8_t x1;
    uint8_t y1;
    uint8_t x2;
    uint8_t y2;
} BoundingBox;

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
 * @param x1 Left coordinate (1-based).
 * @param y1 Top coordinate (1-based).
 * @param x2 Right coordinate (1-based).
 * @param y2 Bottom coordinate (1-based).
 */
void LcdDriverClearBoundingBox(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);

/**
 * @brief Refreshes the LCD display by calling the ST7565 display update.
 */
void LcdDriverRefresh();

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
 * @brief Draws a text string using an external graphics library.
 *
 * @param str The null-terminated string to draw.
 * @param x Starting x coordinate.
 * @param y Starting y coordinate.
 * @param font Pointer to the font to use.
 * @param spacing Spacing between characters.
 * @return BoundingBox The bounding box of the drawn text.
 */
BoundingBox LcdDriverDrawText(char* string,
                              unsigned char x,
                              unsigned char y,
                              const unsigned char* font,
                              unsigned char spacing);

/**
 * @brief Draws a single character using an external graphics library.
 *
 * @param c The character to draw.
 * @param x Starting x coordinate.
 * @param y Starting y coordinate.
 * @param font Pointer to the font to use.
 * @return BoundingBox The bounding box of the drawn character.
 */
BoundingBox
LcdDriverDrawChar(unsigned char c, unsigned char x, unsigned char y, const unsigned char* font);

/* LCD Initializing Functions */

/**
 * @brief Sends a command to the LCD via SPI.
 *
 * @param cmd The command byte to send.
 */
void LcdDriverWriteCommand(uint8_t cmd);

/**
 * @brief Sends data to the LCD via SPI.
 *
 * @param data The data byte to send.
 */
void LcdDriverWriteData(uint8_t data);
