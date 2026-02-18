#include "tasks.h"
#include "cmsis_os2.h"
#include "cyclic_data_handler.h"
#include "lcd_app.h"
#include "spi.h"

/* DRIVE STATE TASK */
void TasksDriveState(void)
{
    for (;;)
    {
        // function calls begin here
        osDelay(1);
    }
}

/* LCD UPDATE TASK */
void TasksLcdUpdate(void *argument)
{
    LcdAppInit(&hspi1);

    // KPH or MPH
    g_lcd_data.speed_units = LCD_APP_MPH;

    for (;;)
    {
        // Handles clearing the screen
        if (g_lcd_page_change == 1)
        {
            LcdAppChangeScreen();
            g_lcd_page_change = 0;
        }
        LcdAppPageController();

    }
    osDelay(LCD_APP_UPDATE_DELAY);
}
