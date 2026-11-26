/**
 * @file    cruise_control.c
 * @brief   This file handles the cruise states for our system.
 *
 * This file handles the entry and exit conditions for cruise control.
 *
 * @author Shlok Lande
 * @date Nov 25 2026
 */

/* INCLUDES */
#include "cruise_control.h"
#include <stddef.h>
#include "debug_io.h"

/* DEFINES */
#define MS_TO_KMH 3.6f

static CruiseState s_state = CRUISE_OFF;


static bool SetpointValid(float setpoint_kmh)
{
    return (setpoint_kmh >= CRUISE_SETPOINT_MIN_KMH) && (setpoint_kmh <= CRUISE_SETPOINT_MAX_KMH);
}

static bool CanEngage(const CruiseInputs* in)
{
    float speed_kmh = in->speed_ms * MS_TO_KMH;

    return (speed_kmh >= CRUISE_ENGAGE_MIN_KMH) &&
           !in->brake_on &&
           !in->speed_stale &&
           !in->fault_or_estop &&
           SetpointValid(in->setpoint_kmh);
}

static bool ShouldExit(const CruiseInputs* in)
{
    float speed_kmh = in->speed_ms * MS_TO_KMH;

    return in->brake_on ||
           !in->cruise_flag ||
           in->speed_stale ||
           in->fault_or_estop ||
           (speed_kmh < CRUISE_EXIT_MIN_KMH) ||
           !SetpointValid(in->setpoint_kmh);
    // TODO: pedal override?
}

void CruiseControlReset(void)
{
    // clear the integral here, gotta do ts after looking at the PI loop
}

CruiseOutputs CruiseControlStep(const CruiseInputs* in)
{
    CruiseOutputs out = {
        .state     = s_state,
        .accel_dac = CRUISE_DAC_OFF,
        .regen_dac = CRUISE_DAC_OFF,
    };

    if (in == NULL)
    {
        return out;
    }

    switch (s_state)
    {
    case CRUISE_OFF:
        if (in->cruise_flag && CanEngage(in)){
            s_state = CRUISE_ACTIVE;
        }
        break;

    case CRUISE_ACTIVE:
        if (ShouldExit(in)){
            CruiseControlReset();
            s_state = CRUISE_OFF;
        }
        break;

    default:
        s_state = CRUISE_OFF;
        break;
    }

    out.state = s_state; //No DAC values being updated yet, will add that when PI loop is brought into this branch
    return out;
}