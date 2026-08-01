/**
 * @file    gpio_driver.c
 * @brief   GPIO driver implementation for UBC Solar STR board
 * @author  Tony Chen
 * @date    Jun 7, 2026
 */

/* INCLUDES */
#include "gpio_driver.h"

#include "gpio_app.h"
#include "main.h"

/* GLOBAL VARIABLES */
volatile StrGpioCtx gpio_pin_state = {0};

/* GPIO POLLING */
void LightState(void)
{
    if (!HAL_GPIO_ReadPin(LTS_IN_GPIO_Port, LTS_IN_Pin))
    {
        gpio_pin_state.lights_state.rts_en = true;
    } else {
        gpio_pin_state.lights_state.rts_en = false;
    }

    if (!HAL_GPIO_ReadPin(RTS_IN_GPIO_Port, RTS_IN_Pin))
    {
        gpio_pin_state.lights_state.lts_en = true;
    } else {
        gpio_pin_state.lights_state.lts_en = false;
    }
}

/**
 * @brief Updates cruise set speed on switch edges while cruise is enabled.
 * @param velocity Current vehicle velocity in km/h.
 */
void CruiseState(uint32_t velocity)
{
    static bool last_cruise_inc = false;
    static bool last_cruise_dec = false;

    uint32_t cruise_set_velocity_kmh = ReadCruiseSetVelocity();

    bool cruise_inc_now = (HAL_GPIO_ReadPin(CRUISE_INC_GPIO_Port, CRUISE_INC_Pin) == GPIO_PIN_RESET);
    bool cruise_dec_now = (HAL_GPIO_ReadPin(CRUISE_DEC_GPIO_Port, CRUISE_DEC_Pin) == GPIO_PIN_RESET);

    if (!gpio_pin_state.cruise_state.cruise_en || (velocity == 0U))
    {
        gpio_pin_state.cruise_state.cruise_inc = false;
        gpio_pin_state.cruise_state.cruise_dec = false;
        last_cruise_inc = cruise_inc_now;
        last_cruise_dec = cruise_dec_now;
        return;
    }

    if (cruise_inc_now && !last_cruise_inc)
    {
        cruise_set_velocity_kmh++;
        gpio_pin_state.cruise_state.cruise_inc = true;
    }
    else {
        gpio_pin_state.cruise_state.cruise_inc = false;
    }

    if (cruise_dec_now && !last_cruise_dec)
    {
        if (cruise_set_velocity_kmh > 0U)
        {
            cruise_set_velocity_kmh--;
        }
        gpio_pin_state.cruise_state.cruise_dec = true;
    }
    else {
        gpio_pin_state.cruise_state.cruise_dec = false;
    }

    last_cruise_inc = cruise_inc_now;
    last_cruise_dec = cruise_dec_now;

    GetCruiseSetVelocity(cruise_set_velocity_kmh);
}

void GpioPollState(void)
{
    // if (!HAL_GPIO_ReadPin(HORN_MCU_GPIO_Port, HORN_MCU_Pin))
    // {
    //     gpio_pin_state.horn_en = true;
    // } else {
    //     gpio_pin_state.horn_en = false;
    // }

    // if (!HAL_GPIO_ReadPin(PTT_MCU_GPIO_Port, PTT_MCU_Pin))
    // {
    //     gpio_pin_state.ptt_en = true;
    // } else {
    //     gpio_pin_state.ptt_en = false;
    // }

    // if (!HAL_GPIO_ReadPin(NEXT_PAGE_GPIO_Port, NEXT_PAGE_Pin))
    // {
    //     gpio_pin_state.next_page = true;
    // } else {
    //     gpio_pin_state.next_page = false;
    // }

    // if (HAL_GPIO_ReadPin(REGEN_GPIO_Port, REGEN_Pin))
    // {
    //     gpio_pin_state.regen_en = true;
    // } else {
    //     gpio_pin_state.regen_en = false;
    // }
}


// static bool page = false;
/* GPIO INTERRUPTS */
/**
 * @brief Handles STR GPIO interrupt events.
 * @param GPIO_Pin GPIO pin that triggered the interrupt.
 */
void StrInterruptHandler(uint16_t GPIO_Pin)
{
    switch (GPIO_Pin)
    {
        case HORN_MCU_Pin:
            gpio_pin_state.horn_en = !gpio_pin_state.horn_en;
            break;

        case NEXT_PAGE_Pin:

            gpio_pin_state.next_page = !gpio_pin_state.next_page;
            // page = gpio_pin_state.next_page;
            break; 

        case PTT_MCU_Pin:
            gpio_pin_state.ptt_en = !gpio_pin_state.ptt_en;
            break;

        case REGEN_Pin:
            gpio_pin_state.regen_en = !gpio_pin_state.regen_en;
            break;

        case CRUISE_CONTROL_Pin:
            gpio_pin_state.cruise_state.cruise_en = !gpio_pin_state.cruise_state.cruise_en;

            if (gpio_pin_state.cruise_state.cruise_en)
            {
                GetCruiseSetVelocity(ReadCurrentVelocity());
            }
            break;

        default:
            break;
    }

//     if (GPIO_PIN != CRUISE_CONTROL_Pin)
//     {
//         return;
//     }

//     HAL_GPIO_TogglePin(DEBUG_GPIO_Port, DEBUG_Pin);

//     gpio_pin_state.cruise_state.cruise_en = !gpio_pin_state.cruise_state.cruise_en;
//     if (gpio_pin_state.cruise_state.cruise_en)
//     {
//         GetCruiseSetVelocity(ReadCurrentVelocity());
//     }
}
