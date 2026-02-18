#include "cyclic_data_handler.h"
#include "cyclic_data.h"
#include <stdint.h>
#include <stdbool.h>
#include "lcd_app.h"

// #include "drive_state.h"

       
// CYCLIC_DATA(type, name, max_cycle_time)
CYCLIC_DATA(uint32_t, cyclic_speed, MAX_CYCLE_TIME);            // Vehicle speed (km/h)
CYCLIC_DATA(int16_t, cyclic_pack_current, MAX_CYCLE_TIME);      // Battery pack current
CYCLIC_DATA(uint16_t, cyclic_pack_voltage, MAX_CYCLE_TIME);     // Battery pack voltage
CYCLIC_DATA(uint8_t, cyclic_drive_state, MAX_CYCLE_TIME);       // Drive state (ie. PARK, FORWARD)
CYCLIC_DATA(uint8_t, cyclic_soc, MAX_CYCLE_TIME);               // State of Charge (SOC %)
CYCLIC_DATA(uint8_t, cyclic_mppta_temperature, MAX_CYCLE_TIME); // MPPTA Temperature (°C)
CYCLIC_DATA(uint8_t, cyclic_mpptb_temperature, MAX_CYCLE_TIME); // MPPTB Temperature (°C)
CYCLIC_DATA(uint8_t, cyclic_mpptc_temperature, MAX_CYCLE_TIME); // MPPTC Temperature (°C)
CYCLIC_DATA(uint8_t, cyclic_mpptd_temperature, MAX_CYCLE_TIME); // MPPTD Temperature (°C)
CYCLIC_DATA(uint8_t, cyclic_batt_min_temperature, MAX_CYCLE_TIME);  // Battery Minimum Temp (°C)
CYCLIC_DATA(uint8_t, cyclic_batt_max_temperature, MAX_CYCLE_TIME);  // Battery Maximum Temp(°C)
CYCLIC_DATA(uint8_t, cyclic_mtr_cont_temperature, MAX_CYCLE_TIME);  // Motor Controller Temp (°C)
CYCLIC_DATA(uint8_t, cyclic_mtr_therm_temperature, MAX_CYCLE_TIME); // Motor Thermistor Temp (°C)


// Create functions that update the cyclic data.
void CyclicDataSetSpeed(uint32_t speed) { CYCLIC_DATA_SET(cyclic_speed, speed); }
void CyclicDataSetPackCurrent(int16_t current) { CYCLIC_DATA_SET(cyclic_pack_current, current); }
void CyclicDataSetPackVoltage(uint16_t voltage) { CYCLIC_DATA_SET(cyclic_pack_voltage, voltage); }
void CyclicDataSetDriveState(uint8_t state) { CYCLIC_DATA_SET(cyclic_drive_state, state); }
void CyclicDataSetSoc(uint8_t soc) { CYCLIC_DATA_SET(cyclic_soc, soc); }
void CyclicDataSetMpptATemperature(uint8_t temperature) { CYCLIC_DATA_SET(cyclic_mppta_temperature, temperature); }
void CyclicDataSetMpptBTemperature(uint8_t temperature) { CYCLIC_DATA_SET(cyclic_mpptb_temperature, temperature); }
void CyclicDataSetMpptCTemperature(uint8_t temperature) { CYCLIC_DATA_SET(cyclic_mpptc_temperature, temperature); }
void CyclicDataSetMpptDTemperature(uint8_t temperature) { CYCLIC_DATA_SET(cyclic_mpptd_temperature, temperature); }
void CyclicDataSetBatteryMinTemperature(uint8_t temperature) { CYCLIC_DATA_SET(cyclic_batt_min_temperature, temperature); }
void CyclicDataSetBatteryMaxTemperature(uint8_t temperature) { CYCLIC_DATA_SET(cyclic_batt_max_temperature, temperature); }
void CyclicDataSetMtrContTemperature(uint8_t temperature) { CYCLIC_DATA_SET(cyclic_mtr_cont_temperature, temperature); }
void CyclicDataSetMtrThermTemperature(uint8_t temperature) { CYCLIC_DATA_SET(cyclic_mtr_therm_temperature, temperature); }

// TODO: Adjust the structs for faults and warnings to be based on ECU


// Create functions that get the cyclic_data
uint32_t* CyclicDataGetSpeed(void) { return CYCLIC_DATA_GET(cyclic_speed); }
int16_t* CyclicDataGetPackCurrent(void) { return CYCLIC_DATA_GET(cyclic_pack_current); }
uint16_t* CyclicDataGetPackVoltage(void) { return CYCLIC_DATA_GET(cyclic_pack_voltage); }
uint8_t* CyclicDataGetMpptATemperature(void) { return CYCLIC_DATA_GET(cyclic_mppta_temperature); }
uint8_t* CyclicDataGetMpptBTemperature(void) { return CYCLIC_DATA_GET(cyclic_mpptb_temperature); }
uint8_t* CyclicDataGetMpptCTemperature(void) { return CYCLIC_DATA_GET(cyclic_mpptc_temperature); }
uint8_t* CyclicDataGetMpptDTemperature(void) { return CYCLIC_DATA_GET(cyclic_mpptd_temperature); }
uint8_t* CyclicDataGetBatteryMinTemperature(void) { return CYCLIC_DATA_GET(cyclic_batt_min_temperature); }
uint8_t* CyclicDataGetBatteryMaxTemperature(void) { return CYCLIC_DATA_GET(cyclic_batt_max_temperature); }
uint8_t* CyclicDataGetMtrContTemperature(void) { return CYCLIC_DATA_GET(cyclic_mtr_cont_temperature); }
uint8_t* CyclicDataGetMtrThermTemperature(void) { return CYCLIC_DATA_GET(cyclic_mtr_therm_temperature); }
uint8_t* CyclicDataGetDriveState(void)
{
    if (CyclicDataGetSpeed() == NULL)
    {
        return NULL; // Stale data for drive state
    }
    else
    {
        return CYCLIC_DATA_GET(cyclic_drive_state);
    }
}
uint8_t* CyclicDataGetSoc(void)
{
    if ((CyclicDataGetPackVoltage() == NULL) || (CyclicDataGetPackCurrent() == NULL))
    {
        return NULL;
    }
    else
    {
        return CYCLIC_DATA_GET(cyclic_soc);
    }
}
