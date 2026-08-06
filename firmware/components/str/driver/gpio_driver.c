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

/* TEMPORARY CRUISE DEBUG VARIABLES */
volatile bool debug_cruise_en = false;
volatile uint32_t debug_cruise_inc = 0U;
volatile uint32_t debug_cruise_dec = 0U;
volatile uint32_t debug_cruise_speed = 0U;

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
            break;

        case PTT_MCU_Pin:
            gpio_pin_state.ptt_en = !gpio_pin_state.ptt_en;
            break;

        case REGEN_Pin:
            gpio_pin_state.regen_en = !gpio_pin_state.regen_en;
            break;

        case CRUISE_INC_Pin:
        {
            if (!gpio_pin_state.cruise_state.cruise_en ||
                (VehicleGetVelocity() == 0U))
            {
                break;
            }

            uint32_t cruise_set_velocity_kmh = CruiseGetVelocity();

            cruise_set_velocity_kmh++;
            CruiseSetVelocity(cruise_set_velocity_kmh);

            gpio_pin_state.cruise_state.cruise_inc = true;
            debug_cruise_inc++;
            debug_cruise_speed = cruise_set_velocity_kmh;
            break;
        }

        case CRUISE_DEC_Pin:
        {
            if (!gpio_pin_state.cruise_state.cruise_en ||
                (VehicleGetVelocity() == 0U))
            {
                break;
            }

            uint32_t cruise_set_velocity_kmh = CruiseGetVelocity();

            if (cruise_set_velocity_kmh > 0U)
            {
                cruise_set_velocity_kmh--;
                CruiseSetVelocity(cruise_set_velocity_kmh);
                gpio_pin_state.cruise_state.cruise_dec = true;
                debug_cruise_dec++;
                debug_cruise_speed = cruise_set_velocity_kmh;
            }

            break;
        }

        case CRUISE_CONTROL_Pin:
            gpio_pin_state.cruise_state.cruise_en = !gpio_pin_state.cruise_state.cruise_en;
            debug_cruise_en = gpio_pin_state.cruise_state.cruise_en;

            gpio_pin_state.cruise_state.cruise_inc = false;
            gpio_pin_state.cruise_state.cruise_dec = false;

            if (gpio_pin_state.cruise_state.cruise_en)
            {
                uint32_t current_velocity_kmh = VehicleGetVelocity();

                CruiseSetVelocity(current_velocity_kmh);
                debug_cruise_speed = current_velocity_kmh;
            }
            break;

        default:
            break;
    }
}
