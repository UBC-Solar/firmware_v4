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

/* PRIVATE FUNCTION PROTOTYPES */
static void HandleLightsInterrupt(uint16_t GPIO_Pin);
static void HandleButtonInterrupt(uint16_t GPIO_Pin);
static void HandleRegenInterrupt(void);
static void HandleCruiseInterrupt(uint16_t GPIO_Pin);

/* GPIO STATE */
void GPIOInitState(void)
{
    gpio_pin_state.regen_en = (HAL_GPIO_ReadPin(REGEN_GPIO_Port, REGEN_Pin) == GPIO_PIN_SET);
}

/* PRIVATE FUNCTIONS */
static void HandleLightsInterrupt(uint16_t GPIO_Pin)
{
    switch (GPIO_Pin)
    {
        case RTS_IN_Pin:
            gpio_pin_state.lights_state.rts_en = !gpio_pin_state.lights_state.rts_en;
            break;

        case LTS_IN_Pin:
            gpio_pin_state.lights_state.lts_en = !gpio_pin_state.lights_state.lts_en;
            break;

        default:
            break;
    }
}

static void HandleButtonInterrupt(uint16_t GPIO_Pin)
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

        default:
            break;
    }
}

static void HandleRegenInterrupt(void)
{
    gpio_pin_state.regen_en = (HAL_GPIO_ReadPin(REGEN_GPIO_Port, REGEN_Pin) == GPIO_PIN_SET);
}

static void HandleCruiseInterrupt(uint16_t GPIO_Pin)
{
    switch (GPIO_Pin)
    {
        case CRUISE_INC_Pin:
        {
            if (!gpio_pin_state.cruise_state.cruise_en || (GPIOAppGetVehicleVelocity() == 0U))
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
        case LTS_IN_Pin:
            HandleLightsInterrupt(GPIO_Pin);
            break;

        case HORN_MCU_Pin:
        case NEXT_PAGE_Pin:
        case PTT_MCU_Pin:
            HandleButtonInterrupt(GPIO_Pin);
            break;

        case REGEN_Pin:
            HandleRegenInterrupt();
            break;

        case CRUISE_INC_Pin:
        case CRUISE_DEC_Pin:
        case CRUISE_CONTROL_Pin:
            HandleCruiseInterrupt(GPIO_Pin);
            break;

        default:
            break;
    }
}
