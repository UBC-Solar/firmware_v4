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

/* GPIO INTERRUPTS */
/**
 * @brief Handles STR GPIO interrupt events.
 * @param GPIO_Pin GPIO pin that triggered the interrupt.
 */
void StrInterruptHandler(uint16_t GPIO_Pin)
{
    switch (GPIO_Pin)
    {
        case RTS_IN_Pin:
            gpio_pin_state.lights_state.rts_en = !gpio_pin_state.lights_state.rts_en;
            break;

        case LTS_IN_Pin:
            gpio_pin_state.lights_state.lts_en = !gpio_pin_state.lights_state.lts_en;
            break;

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
                (GPIOAppGetVehicleVelocity() == 0U))
            {
                break;
            }

            uint32_t cruise_set_velocity_kmh = GPIOAppGetCruiseVelocity();

            cruise_set_velocity_kmh++;
            GPIOAppSetCruiseVelocity(cruise_set_velocity_kmh);

            gpio_pin_state.cruise_state.cruise_inc = true;
            break;
        }

        case CRUISE_DEC_Pin:
        {
            if (!gpio_pin_state.cruise_state.cruise_en ||
                (GPIOAppGetVehicleVelocity() == 0U))
            {
                break;
            }

            uint32_t cruise_set_velocity_kmh = GPIOAppGetCruiseVelocity();

            if (cruise_set_velocity_kmh > 0U)
            {
                cruise_set_velocity_kmh--;
                GPIOAppSetCruiseVelocity(cruise_set_velocity_kmh);
                gpio_pin_state.cruise_state.cruise_dec = true;
            }

            break;
        }

        case CRUISE_CONTROL_Pin:
            gpio_pin_state.cruise_state.cruise_en = !gpio_pin_state.cruise_state.cruise_en;

            gpio_pin_state.cruise_state.cruise_inc = false;
            gpio_pin_state.cruise_state.cruise_dec = false;

            if (gpio_pin_state.cruise_state.cruise_en)
            {
                uint32_t current_velocity_kmh = GPIOAppGetVehicleVelocity();

                GPIOAppSetCruiseVelocity(current_velocity_kmh);
            }
            break;

        default:
            break;
    }
}
