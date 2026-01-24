#include "lcd_driver.h"
#include "main.h"

/* Internal buffer for pixel operations (assumes a 128x64 display) */
static uint8_t lcd_buffer[(128 * 64) / 8];

/* Internal SPI handle for LCD communication */
static SPI_HandleTypeDef* sg_spi_handle = NULL;

static uint8_t lcd_flipped = 0;

// DIRTY PAGES optimization variable
#ifdef LCD_DRIVER_ST7565_DIRTY_PAGES
static uint8_t lcd_dirty_pages;
#endif

/*--------------------------------------------------------------------------
  Internal Helper Functions
--------------------------------------------------------------------------*/

/**
 * @brief Sets or clears a single pixel in the internal display buffer.
 *
 * @param x The x coordinate (0-based).
 * @param y The y coordinate (0-based).
 * @param color 1 to set the pixel, 0 to clear it.
 */
void LcdDriverSetPixel(uint8_t x, uint8_t y, uint8_t colour)
{
    if (x >= LCD_DRIVER_SCREEN_WIDTH || y >= LCD_DRIVER_SCREEN_HEIGHT)
        return;

    unsigned short array_pos = x + ((y / 8) * 128);

#ifdef LCD_DRIVER_ST7565_DIRTY_PAGES
    lcd_dirty_pages |= 1 << (array_pos / 128);
#endif

    if (colour)
    {
        lcd_buffer[array_pos] |= 1 << (y % 8);
    }
    else
    {
        lcd_buffer[array_pos] &= 0xFF ^ 1 << (y % 8);
    }
}

/**
 * @brief Clears a rectangular area in the internal display buffer.
 *
 * @param x1 Left coordinate
 * @param y1 Top coordinate
 * @param x2 Right coordinate
 * @param y2 Bottom coordinate
 */
void LcdDriverClearBoundingBox(unsigned char x1,
                               unsigned char y1,
                               unsigned char x2,
                               unsigned char y2)
{
    if (x1 >= LCD_DRIVER_SCREEN_WIDTH || x2 >= LCD_DRIVER_SCREEN_WIDTH ||
        y1 >= LCD_DRIVER_SCREEN_HEIGHT || y2 >= LCD_DRIVER_SCREEN_HEIGHT || x1 > x2 || y1 > y2)
        return;

    for (unsigned char y = y1; y <= y2; y++)
    {
        for (unsigned char x = x1; x <= x2; x++)
        {
            unsigned short array_pos = x + ((y / 8) * 128);
            lcd_buffer[array_pos] = 0;
        }
    }
}

/**
 * @brief Refreshes the LCD display by calling the ST7565 display update.
 */
void LcdDriverRefresh()
{
    for (int y = 0; y < 8; y++)
    {

#ifdef LCD_DRIVER_ST7565_DIRTY_PAGES
        // Only copy this page if it is marked as "dirty"
        if (!(lcd_dirty_pages & (1 << y)))
            continue;
#endif

        LcdDriverWriteCommand(LCD_DRIVER_CMD_SET_PAGE | y);

        // Reset column to the left side.  The internal memory of the
        // screen is 132*64, we need to account for this if the display
        // is flipped.
        //
        // Some screens seem to map the internal memory to the screen
        // pixels differently, the ST7565_REVERSE define allows this to
        // be controlled if necessary.
#ifdef ST7565_REVERSE
        if (!lcd_flipped)
        {
#else
        if (lcd_flipped)
        {
#endif
            LcdDriverWriteCommand(LCD_DRIVER_CMD_COLUMN_LOWER | 4);
        }
        else
        {
            LcdDriverWriteCommand(LCD_DRIVER_CMD_COLUMN_LOWER);
        }
        LcdDriverWriteCommand(LCD_DRIVER_CMD_COLUMN_UPPER);

        for (int x = 0; x < 128; x++)
        {
            LcdDriverWriteData(lcd_buffer[y * 128 + x]);
        }
    }

#ifdef LCD_DRIVER_ST7565_DIRTY_PAGES
    // All pages have now been updated, reset the indicator.
    lcd_dirty_pages = 0;
#endif
}

/**
 * @brief Draws a rectangle outline using the internal pixel function.
 *
 * @param x1 Left coordinate (1-based).
 * @param y1 Top coordinate (1-based).
 * @param x2 Right coordinate (1-based).
 * @param y2 Bottom coordinate (1-based).
 * @param color 1 to draw pixel.
 */
void LcdDriverDrawRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color)
{
    for (uint8_t x = x1; x <= x2; x++)
    {
        LcdDriverSetPixel(x, y1, color);
        LcdDriverSetPixel(x, y2, color);
    }
    for (uint8_t y = y1; y <= y2; y++)
    {
        LcdDriverSetPixel(x1, y, color);
        LcdDriverSetPixel(x2, y, color);
    }
}

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
                              unsigned char spacing)
{
    BoundingBox ret;
    BoundingBox tmp = {0};

    ret.x1 = x;
    ret.y1 = y;

    spacing += 1;

    // BUG: As we move right between chars we don't actually wipe the space
    while (*string != 0)
    {
        tmp = LcdDriverDrawChar(*string++, x, y, font);

        // Leave a single space between characters
        x = tmp.x2 + spacing;
    }

    ret.x2 = tmp.x2;
    ret.y2 = tmp.y2;

    return ret;
}

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
LcdDriverDrawChar(unsigned char c, unsigned char x, unsigned char y, const unsigned char* font)
{
    unsigned short pos;
    uint8_t width;
    BoundingBox ret;

    ret.x1 = x;
    ret.y1 = y;
    ret.x2 = x;
    ret.y2 = y;

    // Read first byte, should be 0x01 for proportional
    if (font[LCD_DRIVER_FONT_HEADER_TYPE] != LCD_DRIVER_FONT_TYPE_PROPORTIONAL)
        return ret;

    // Check second byte, should be 0x02 for "vertical ceiling"
    if (font[LCD_DRIVER_FONT_HEADER_ORIENTATION] != LCD_DRIVER_FONT_ORIENTATION_VERTICAL_CEILING)
        return ret;

    // Check that font start + number of bitmaps contains c
    if (!(c >= font[LCD_DRIVER_FONT_HEADER_START] &&
          c <= font[LCD_DRIVER_FONT_HEADER_START] + font[LCD_DRIVER_FONT_HEADER_LETTERS]))
        return ret;

    // Adjust for start position of font vs. the char passed
    c -= font[LCD_DRIVER_FONT_HEADER_START];

    // Work out where in the array the character is
    pos = font[c * LCD_DRIVER_FONT_HEADER_START + 5];
    pos <<= 8;
    pos |= font[c * LCD_DRIVER_FONT_HEADER_START + 6];

    // Read first byte from this position, this gives letter width
    width = font[pos];

    // Draw left to right
    uint8_t i;
    for (i = 0; i < width; i++)
    {

        // Draw top to bottom
        for (uint8_t j = 0; j < font[LCD_DRIVER_FONT_HEADER_HEIGHT]; j++)
        {
            // Increment one data byte every 8 bits, or
            // at the start of a new column  HiTech optimizes
            // the modulo, so no need to try and avoid it.
            if (j % 8 == 0)
                pos++;

            if (font[pos] & 1 << (j % 8))
            {
                LcdDriverSetPixel(x + i, y + j, 1);
            }
            else
            {
                LcdDriverSetPixel(x + i, y + j, 0);
            }
        }
    }

    ret.x2 = ret.x1 + width - 1;
    ret.y2 = ret.y1 + font[LCD_DRIVER_FONT_HEADER_HEIGHT];

    return ret;
}

/**
 * @brief Changes the screen
 */
void LcdDriverChangeScreen()
{
    lcd_dirty_pages = LCD_DRIVER_DIRTY_PAGE_CHANGE;
    LcdDriverClearBoundingBox(0, 0, LCD_DRIVER_BOTTOM_RIGHT_X, LCD_DRIVER_BOTTOM_RIGHT_Y);
    LcdDriverRefresh();
}

/**
 * @brief Sends a command to the LCD via SPI.
 *
 * @param cmd The command byte to send.
 */
void LcdDriverWriteCommand(uint8_t cmd)
{
    /* Set A0 low for command */
    HAL_GPIO_WritePin(DISPLAY_A0_GPIO_Port, DISPLAY_A0_Pin, GPIO_PIN_RESET);

    uint8_t cmd_arr[1] = {cmd};
    HAL_SPI_Transmit(sg_spi_handle, cmd_arr, 1, 10);
}

/**
 * @brief Sends data to the LCD via SPI so that the specified pixels turn black (or white).
 *
 * @param data The data byte to send.
 */
void LcdDriverWriteData(uint8_t data)
{
    /* Set A0 high for data */
    HAL_GPIO_WritePin(DISPLAY_A0_GPIO_Port, DISPLAY_A0_Pin, GPIO_PIN_SET);

    uint8_t data_arr[1] = {data};
    HAL_SPI_Transmit(sg_spi_handle, data_arr, 1, 10);
}

/**
 * @brief Initializes the LCD based on the ST7565 library and prints the field names.
 *
 * @param hspi Pointer to the SPI handle.
 */
void LcdDriverInit(SPI_HandleTypeDef* hspi)
{
    HAL_GPIO_WritePin(DISPLAY_RESET_GPIO_Port, DISPLAY_RESET_Pin, GPIO_PIN_RESET);
    HAL_Delay(30);
    HAL_GPIO_WritePin(DISPLAY_RESET_GPIO_Port, DISPLAY_RESET_Pin, GPIO_PIN_SET);
    HAL_Delay(30);

    sg_spi_handle = hspi;

    LcdDriverWriteCommand(LCD_DRIVER_CMD_SET_ADC_NORMAL);
    LcdDriverWriteCommand(LCD_DRIVER_CMD_DISPLAY_OFF);
    LcdDriverWriteCommand(LCD_DRIVER_CMD_SET_COM_NORMAL + 8); // This makes the drawing flipped
    LcdDriverWriteCommand(LCD_DRIVER_CMD_SET_BIAS_9);
    LcdDriverWriteCommand(LCD_DRIVER_CMD_SET_POWER_CONTROL | 0x7);
    LcdDriverWriteCommand(
        LCD_DRIVER_CMD_SET_RESISTOR_RATIO |
        0x6); // set lcd operating voltage (regulator resistor, ref voltage resistor)
    LcdDriverWriteCommand(LCD_DRIVER_CMD_SET_VOLUME_FIRST);
    LcdDriverWriteCommand(LCD_DRIVER_CMD_SET_CONTRAST - 5);
    LcdDriverWriteCommand(LCD_DRIVER_CMD_DISPLAY_START);
    LcdDriverWriteCommand(LCD_DRIVER_CMD_DISPLAY_ON);
    LcdDriverWriteCommand(LCD_DRIVER_CMD_SET_ALLPTS_NORMAL);
}
