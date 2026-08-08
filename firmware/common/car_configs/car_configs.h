/**
 * @file car_configs.h
 *
 * @brief Car-wide configuration
 * This library contains a header that a user can include to read the settings that vary
 * between builds of the car (telemetry link, telemetry CAN message set, display units).
 *
 * @attention This is the single place these settings are changed. Edit the macros in the
 * "CONFIGURATION" section below; everything else reads the derived variables underneath it.
 */

#ifndef __CAR_CONFIGS_H__
#define __CAR_CONFIGS_H__


/* Includes */
#include <stdbool.h>
#include <stdint.h>


/* ==========================================================================
   CONFIGURATION - edit these
   ========================================================================== */

/**
 * @brief Selects which telemetry transport the TEL board transmits on.
 *
 * 1 = cellular module on huart2, 0 = radio module on huart4.
 *
 * @attention ONLY SET THIS TO 0 IF FLASHING WITH ST-LINK OR POWERING WITH A POWER SUPPLY.
 * J-LINK DOES NOT SUPPLY ENOUGH CURRENT FOR THE RADIO MODULE. ONLY USE ST-LINK FOR THIS.
 */
#define CAR_CONFIG_CELLULAR 1

/**
 * @brief Selects which CAN messages the TEL board forwards to telemetry.
 *
 * 1 = every received message, 0 = only the competition subset in filter_whitelist.
 *
 * Independent of CAR_CONFIG_CELLULAR, so the radio can be bench-tested with full traffic.
 */
#define CAR_CONFIG_CAN_MSG_ALL 1

/**
 * @brief Selects the speed units shown on the DRD dashboard LCD.
 *
 * 1 = kph, 0 = mph. Values match LCD_APP_KPH / LCD_APP_MPH in drd/app/lcd_app.h.
 */
#define CAR_CONFIG_SPEED_KPH 1


/* ==========================================================================
   DERIVED - use these in code
   ========================================================================== */

/** @brief True when telemetry transmits over cellular, false for radio. */
static const bool car_config_cellular __attribute__((unused)) = (bool)CAR_CONFIG_CELLULAR;

/** @brief True when telemetry forwards every CAN message, false to apply filter_whitelist. */
static const bool car_config_can_msg_all __attribute__((unused)) = (bool)CAR_CONFIG_CAN_MSG_ALL;

/** @brief Speed units passed to LcdHandlerInit(); matches the LCD_APP_KPH / LCD_APP_MPH encoding. */
static const uint8_t car_config_speed_units __attribute__((unused)) = CAR_CONFIG_SPEED_KPH;


#endif /* __CAR_CONFIGS_H__ */
