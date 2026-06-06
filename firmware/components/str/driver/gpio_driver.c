#include "gpio_driver.h"

#include "gpio_app.h"
#include "main.h"

volatile StrGpioCtx gpio_pin_state = {0};

void LightState(void)
{
    if (!HAL_GPIO_ReadPin(RTS_IN_GPIO_Port, RTS_IN_Pin))
    {
        gpio_pin_state.lights_state.rts_en = true;
    } else {
        gpio_pin_state.lights_state.rts_en = false;
    }

    if (!HAL_GPIO_ReadPin(LTS_IN_GPIO_Port, LTS_IN_Pin))
    {
        gpio_pin_state.lights_state.lts_en = true;
    } else {
        gpio_pin_state.lights_state.lts_en = false;
    }
}

void CruiseState(uint32_t velocity)
{
    uint32_t cruise_set_velocity_kmh = ReadCruiseSetVelocity();
    bool cruise_status = (velocity > 0) && gpio_pin_state.cruise_state.cruise_en;

    if (!cruise_status)
    {
        GetCruiseSetVelocity(velocity);
        return;
    }

    if (cruise_status && gpio_pin_state.cruise_state.cruise_inc)
    {
        cruise_set_velocity_kmh++;
    }

    if (cruise_status && gpio_pin_state.cruise_state.cruise_dec)
    {
        if (cruise_set_velocity_kmh > 0U)
        {
            cruise_set_velocity_kmh--;
        }
    }

    GetCruiseSetVelocity(cruise_set_velocity_kmh);
}

void GpioPollState(void)
{
    if (!HAL_GPIO_ReadPin(HORN_MCU_GPIO_Port, HORN_MCU_Pin))
    {
        gpio_pin_state.horn_en = true;
    } else {
        gpio_pin_state.horn_en = false;
    }

    if (!HAL_GPIO_ReadPin(PTT_MCU_GPIO_Port, PTT_MCU_Pin))
    {
        gpio_pin_state.ptt_en = true;
    } else {
        gpio_pin_state.ptt_en = false;
    }

    if (!HAL_GPIO_ReadPin(NEXT_PAGE_GPIO_Port, NEXT_PAGE_Pin))
    {
        gpio_pin_state.next_page = true;
    } else {
        gpio_pin_state.next_page = false;
    }
}

void StrInterruptHandler(uint16_t toggle)
{
    HAL_GPIO_TogglePin(DEBUG_GPIO_Port, DEBUG_Pin);

    switch (toggle)
    {
    case CRUISE_CONTROL_Pin:
        gpio_pin_state.cruise_state.cruise_en = !gpio_pin_state.cruise_state.cruise_en;
        if (gpio_pin_state.cruise_state.cruise_en)
        {
            GetCruiseSetVelocity(ReadCurrentVelocity());
        }
        break;

    case REGEN_Pin:
        gpio_pin_state.regen_en = true;
        break;
    }
}