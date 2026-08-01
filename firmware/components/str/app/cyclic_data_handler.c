/**
 * @file    cyclic_data_handler.c
 * @brief   Cyclic Data Handler for the STR Module
 *
 * Defines cyclic data for steering-wheel display values and provides getter /
 * setter helpers. Speed is the only cyclic value used for now.
 *
 * @author  Martin W
 * @date    Jul 15 2026
 */

#include "cyclic_data_handler.h"
#include "cyclic_data.h"

#include <stdint.h>

/*--------------------------------------------------------------------------
  CYCLIC DATA DEFINITIONS
--------------------------------------------------------------------------*/

CYCLIC_DATA(uint32_t, cyclic_speed, MAX_CYCLE_TIME);              // Vehicle speed (km/h)
CYCLIC_DATA(uint32_t, cyclic_can_rx_timestamp, MAX_CYCLE_TIME);   // Last CAN RX timestamp (ms)

/*--------------------------------------------------------------------------
  CYCLIC DATA SETTERS
--------------------------------------------------------------------------*/

void CyclicDataSetSpeed(uint32_t speed)
{
    CYCLIC_DATA_SET(cyclic_speed, speed);
}

void CyclicDataSetCanRxTimestamp(uint32_t timestamp_ms)
{
    CYCLIC_DATA_SET(cyclic_can_rx_timestamp, timestamp_ms);
}

/*--------------------------------------------------------------------------
  CYCLIC DATA GETTERS
--------------------------------------------------------------------------*/

uint32_t* CyclicDataGetSpeed(void)
{
    return CYCLIC_DATA_GET(cyclic_speed);
}

uint32_t* CyclicDataGetCanRxTimestamp(void)
{
    return CYCLIC_DATA_GET(cyclic_can_rx_timestamp);
}
