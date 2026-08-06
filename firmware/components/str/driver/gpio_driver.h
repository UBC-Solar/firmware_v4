/**
 * @file    gpio_driver.h
 * @brief   GPIO driver interface for UBC Solar STR board
 * @author  Tony Chen
 * @date    Jun 7, 2026
 */

#ifndef __GPIO_DRIVER_H__
#define __GPIO_DRIVER_H__

/* INCLUDES */
#include <stdbool.h>
#include <stdint.h>

/* TYPE DEFINITIONS */
/**
 * @brief Turn-signal light switch state.
 */
typedef struct {
    /** Right turn signal enable state. */
    volatile bool rts_en;
    /** Left turn signal enable state. */
    volatile bool lts_en;
} LightsCtx;

/**
 * @brief Cruise control switch state.
 */
typedef struct {
    /** Cruise control enable state. */
    volatile bool cruise_en;
    /** Cruise increment switch edge state. */
    volatile bool cruise_inc;
    /** Cruise decrement switch edge state. */
    volatile bool cruise_dec;
} CruiseCtx;

/**
 * @brief Complete STR GPIO input state.
 */
typedef struct {
    /** Horn switch state. */
    volatile bool horn_en;
    /** Push-to-talk switch state. */
    volatile bool ptt_en;
    /** Dashboard LCD next-page switch state. */
    volatile bool next_page;
    /** Regenerative braking switch state. */
    volatile bool regen_en;
    /** Turn-signal switch state. */
    LightsCtx lights_state;
    /** Cruise control switch state. */
    CruiseCtx cruise_state;
} StrGpioCtx;

/* EXTERNAL VARIABLES */
extern volatile StrGpioCtx gpio_pin_state;

/* FUNCTION PROTOTYPES */
/**
 * @brief Handles STR GPIO interrupt events.
 * @param GPIO_Pin GPIO pin that triggered the interrupt.
 */
void StrInterruptHandler(uint16_t GPIO_Pin);

#endif /* __GPIO_DRIVER_H__ */
