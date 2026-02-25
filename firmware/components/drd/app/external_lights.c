/*
 * external_lights.c
 *
 *	@brief   Contains functions to handle front/rear turn signals and brake state.
 *
 *  Created on: Feb 12, 2025
 *      Author: Martin Wu
 *
 *
 */

/*	Includes	*/
#include "external_lights.h"
#include "external_lights_driver.h"
#include <stdint.h>

/*	Symbolic Constants		*/
#define EXTERNAL_LIGHTS_IDLE_STATE 0
#define EXTERNAL_LIGHTS_LTS_STATE 1
#define EXTERNAL_LIGHTS_RTS_STATE 2
#define EXTERNAL_LIGHTS_BRAKE_STATE 3

/*	Global Variables	*/
volatile uint8_t g_EXTERNAL_LIGHTS_left_turn_signal = 0;
volatile uint8_t g_EXTERNAL_LIGHTS_right_turn_signal = 0;
volatile uint8_t g_EXTERNAL_LIGHTS_braking = 0;

/*
 * @brief State machine to handle vehicle external lights.
 *
 */
void ExternalLightsStateMachine()
{
    static uint8_t prev_state =
        EXTERNAL_LIGHTS_IDLE_STATE; // keep track of previous state to reset flash counts
    static uint8_t flash_count = 0; // every xth count, the pin flips state, causing flash
    static uint8_t flts = 0;        // front left turn signal
    static uint8_t frts = 0;        // front right turn signal
    static uint8_t blts = 0;        // back left turn signal
    static uint8_t brts = 0;        // back right turn signal

    if (g_EXTERNAL_LIGHTS_left_turn_signal)
    {
        if (prev_state != EXTERNAL_LIGHTS_LTS_STATE)
        {
            flash_count = 0;
            flts = 0;
            blts = 0;
        }

        flash_count++;

        if (flash_count >= EXTERNAL_LIGHTS_FLIP_COUNT)
        {
            flts = !flts;
            blts = !blts;
            flash_count = 0;
        }

        frts = 0;
        brts = 0;
        prev_state = EXTERNAL_LIGHTS_LTS_STATE;
    }
    else if (g_EXTERNAL_LIGHTS_right_turn_signal)
    {
        if (prev_state != EXTERNAL_LIGHTS_RTS_STATE)
        {
            flash_count = 0;
            frts = 0;
            brts = 0;
        }

        flash_count++;

        if (flash_count >= EXTERNAL_LIGHTS_FLIP_COUNT)
        {
            frts = !frts;
            brts = !brts;
            flash_count = 0;
        }

        flts = 0;
        blts = 0;
        prev_state = EXTERNAL_LIGHTS_RTS_STATE;
    }
    else if (g_EXTERNAL_LIGHTS_braking)
    {
        // brake with no turn signal: rear LEDs solid on, front off
        // when braking + turn signal, LTS/RTS branches handle rear LEDs as blinkers
        flts = 0;
        frts = 0;
        blts = 1;
        brts = 1;
        prev_state = EXTERNAL_LIGHTS_BRAKE_STATE;
    }
    else
    {
        // idle: all signals off
        flts = 0;
        frts = 0;
        blts = 0;
        brts = 0;
        prev_state = EXTERNAL_LIGHTS_IDLE_STATE;
    }

    // BRK_OUT stays on whenever braking regardless of turn signal state
    ExternalLightsDriverSet(flts, frts, blts, brts, g_EXTERNAL_LIGHTS_braking);
}

/**
 * @brief Handles incoming CAN messages to update turn signal and brake state.
 *
 * Updates g_EXTERNAL_LIGHTS_left_turn_signal, g_EXTERNAL_LIGHTS_right_turn_signal,
 * and g_EXTERNAL_LIGHTS_braking based on the STR CAN message. Should be called
 * from the CAN Rx callback.
 *
 * @param can_id CAN message ID
 * @param data   Pointer to CAN message data bytes
 */
void ExternalLightsCANRxHandle(uint32_t can_id, uint8_t* data)
{
    uint8_t rts;
    uint8_t lts;
    uint8_t brake;

    if (can_id == STR_CAN_MSG_ID)
    {
        rts = (data[0] & (1 << 0));
        lts = (data[0] & (1 << 1));
        brake = (data[0] & (1 << 2)); // NOTE: UNSURE IF THIS IS CORRECT. NEED TO KNOW WHERE
                                      // THE BRAKE BIT IS IN THE CAN MSG

        if (lts)
        {
            g_EXTERNAL_LIGHTS_left_turn_signal = 1;
            g_EXTERNAL_LIGHTS_right_turn_signal = 0;
        }
        else if (rts)
        {
            g_EXTERNAL_LIGHTS_right_turn_signal = 1;
            g_EXTERNAL_LIGHTS_left_turn_signal = 0;
        }
        else // neither the left or right turn signals are on.
        {
            g_EXTERNAL_LIGHTS_left_turn_signal = 0;
            g_EXTERNAL_LIGHTS_right_turn_signal = 0;
        }

        g_EXTERNAL_LIGHTS_braking = brake ? 1 : 0;
    }
}
