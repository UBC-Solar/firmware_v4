#include "can_app.h"

#include "can_driver.h"
#include "main.h"
#include "stm32f1xx_hal_gpio.h"

enum
{
    STR_NEXT_PAGE_BIT = 2U,
};

static const CAN_TxHeaderTypeDef str_lcd_page_header = {
    .StdId = STR_LCD_PAGE_CAN_ID,
    .ExtId = 0x0000,
    .IDE = CAN_ID_STD,
    .RTR = CAN_RTR_DATA,
    .DLC = 1,
    .TransmitGlobalTime = DISABLE,
};

void CanAppInit(void)
{
    CanDriverInit();
}

void CanAppTransmitNextPage(void)
{
    uint8_t data[8] = {0};
    GPIO_PinState next_page_state = !HAL_GPIO_ReadPin(NEXT_PAGE_GPIO_Port, NEXT_PAGE_Pin);

    data[0] |= (uint8_t)((next_page_state == GPIO_PIN_SET ? 1U : 0U) << STR_NEXT_PAGE_BIT);

    CanDriverSend(&str_lcd_page_header, data);
}