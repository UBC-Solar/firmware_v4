/*
 * external_lights.c
 *
 *	@brief   Contains functions to handle front/rear turn signals, hazard lights, and brake state.
 *
 *  Created on: Feb 12, 2025
 *      Author: Martin Wu
 *
 *
 */

/*	Includes	*/
#include "external_lights.h"
#include "external_lights_driver.h"
#include "gpio_driver.h"
#include "main.h"
#include <stdbool.h>
#include <stdint.h>

/*	Global Variables	*/
volatile bool g_external_lights_left_turn_signal = false;
volatile bool g_external_lights_right_turn_signal = false;

/*
 * @brief State machine to handle vehicle external lights.
 *
 */
void ExternalLightsStateMachine()
{
    // Read brake state via gpio_driver
    bool braking = ReadBrakePin(BRAKE_INPUT_PORT, BRAKE_INPUT_PIN);

    // Read hazard switch via gpio_driver
    bool hazard_on = ReadHazardPin(HAZARD_PORT, HAZARD_PIN);

    static ExternalLightsState prev_state =
        EXTERNAL_LIGHTS_IDLE_STATE; // keep track of previous state to reset flash counts
    static uint8_t flash_count = 0; // every xth count, the pin flips state, causing flash
    static bool flts = false;       // front left turn signal
    static bool frts = false;       // front right turn signal
    static bool blts = false;       // back left turn signal
    static bool brts = false;       // back right turn signal

    if (hazard_on)
    {
        if (prev_state != EXTERNAL_LIGHTS_HAZARD_STATE)
        {
            flash_count = 0;
            flts = false;
            frts = false;
            blts = false;
            brts = false;
        }

        flash_count++;

        if (flash_count >= EXTERNAL_LIGHTS_FLIP_COUNT)
        {
            flts = !flts;
            frts = !frts;
            blts = !blts;
            brts = !brts;
            flash_count = 0;
        }

        prev_state = EXTERNAL_LIGHTS_HAZARD_STATE;
    }
    else if (g_external_lights_left_turn_signal)
    {
        if (prev_state != EXTERNAL_LIGHTS_LTS_STATE)
        {
            flash_count = 0;
            flts = false;
            blts = false;
        }

        flash_count++;

        if (flash_count >= EXTERNAL_LIGHTS_FLIP_COUNT)
        {
            flts = !flts;
            blts = !blts;
            flash_count = 0;
        }

        frts = false;
        brts = false;
        prev_state = EXTERNAL_LIGHTS_LTS_STATE;
    }
    else if (g_external_lights_right_turn_signal)
    {
        if (prev_state != EXTERNAL_LIGHTS_RTS_STATE)
        {
            flash_count = 0;
            frts = false;
            brts = false;
        }

        flash_count++;

        if (flash_count >= EXTERNAL_LIGHTS_FLIP_COUNT)
        {
            frts = !frts;
            brts = !brts;
            flash_count = 0;
        }

        flts = false;
        blts = false;
        prev_state = EXTERNAL_LIGHTS_RTS_STATE;
    }
    else if (braking)
    {
        // brake with no turn signal: rear LEDs solid on, front off
        // when braking + turn signal, LTS/RTS branches handle rear LEDs as blinkers
        flts = false;
        frts = false;
        blts = true;
        brts = true;
        prev_state = EXTERNAL_LIGHTS_BRAKE_STATE;
    }
    else
    {
        // idle: all signals off
        flts = false;
        frts = false;
        blts = false;
        brts = false;
        prev_state = EXTERNAL_LIGHTS_IDLE_STATE;
    }

    // BRK_OUT stays on whenever braking regardless of turn signal state
    ExternalLightsDriverSet(flts, frts, blts, brts, braking);
}

/**
 * @brief Handles incoming CAN messages to update turn signal state.
 *
 * Updates g_external_lights_left_turn_signal and g_external_lights_right_turn_signal
 * based on the STR CAN message. Should be called from the CAN Rx callback.
 *
 * @param can_id CAN message ID
 * @param data   Pointer to CAN message data bytes
 */
void ExternalLightsCanRxHandle(uint32_t can_id, uint8_t* data)
{
    uint8_t rts;
    uint8_t lts;

    if (can_id == STR_CAN_MSG_ID)
    {
        rts = (data[0] & (1 << 0));
        lts = (data[0] & (1 << 1));

        if (lts)
        {
            g_external_lights_left_turn_signal = true;
            g_external_lights_right_turn_signal = false;
        }
        else if (rts)
        {
            g_external_lights_right_turn_signal = true;
            g_external_lights_left_turn_signal = false;
        }
        else // neither the left or right turn signals are on.
        {
            g_external_lights_left_turn_signal = false;
            g_external_lights_right_turn_signal = false;
        }
    }
}
