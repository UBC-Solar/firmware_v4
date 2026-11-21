/**
 * @file adc_runtime.c
 * @brief eFuse and TEL currents in mA, converted from the ADC driver's raw readings.
 *
 * The equations and constants are explained in dist_defs.h (current sensing section).
 */

#include "adc_runtime.h"

// Drivers
#include "adc_driver.h"

// DIST app
#include "dist_defs.h"

static float CountsToVolts(uint16_t counts);
static float IlmToAmps(uint16_t counts, float rilm_ohms);
static float Correct910(float amps);
static float TelSenseToAmps(uint16_t counts);
static int32_t AmpsToMilliamps(float amps);

static ADC_Values adc_values = {0};

/**
 * @brief Reads every current sense channel from the ADC driver and converts it to mA.
 *
 * Call once per pass, then read the result with ADC_Runtime_GetValues().
 * The 910 ohm eFuse channels also get the bench-measured correction (Correct910()).
 *
 * Side effects: runs one blocking conversion per channel (ADC_GetReadings()).
 */
void ADC_Runtime_ProcessReadings(void) {
    ADC_Readings readings = ADC_GetReadings();

    adc_values.drd_ma =
        AmpsToMilliamps(Correct910(IlmToAmps(readings.drd_ilm, EFUSE_RILM_DRD_OHMS)));
    adc_values.mdi_ma =
        AmpsToMilliamps(IlmToAmps(readings.mdi_ilm, EFUSE_RILM_MDI_OHMS));
    adc_values.spare_ctrl_ma =
        AmpsToMilliamps(Correct910(IlmToAmps(readings.spare_ctrl_ilm, EFUSE_RILM_SPARE_CTRL_OHMS)));
    adc_values.spare_mux_ma =
        AmpsToMilliamps(Correct910(IlmToAmps(readings.spare_mux_ilm, EFUSE_RILM_SPARE_MUX_OHMS)));
    adc_values.spare_ma =
        AmpsToMilliamps(Correct910(IlmToAmps(readings.spare_ilm, EFUSE_RILM_SPARE_OHMS)));
    adc_values.tel_ma = AmpsToMilliamps(TelSenseToAmps(readings.tel_sense));
}

/**
 * @brief Gets the latest currents from ADC_Runtime_ProcessReadings().
 * @return The latest currents, in mA.
 */
ADC_Values ADC_Runtime_GetValues(void) {
    return adc_values;
}

/**
 * @brief Converts raw ADC counts to the voltage at the ADC pin.
 * @param counts Raw 12-bit reading.
 * @return Voltage at the pin, in V.
 */
static float CountsToVolts(uint16_t counts) {
    return ((float)counts / ADC_FULL_SCALE_COUNTS) * ADC_VREF_V;
}

/**
 * @brief Converts an eFuse ILM reading to its load current: IOUT = VILM / (GIMON x RILM).
 * @param counts    Raw ILM reading.
 * @param rilm_ohms That eFuse's RILM resistor.
 * @return Load current, in A.
 */
static float IlmToAmps(uint16_t counts, float rilm_ohms) {
    return CountsToVolts(counts) / (EFUSE_GIMON * rilm_ohms);
}

/**
 * @brief Applies the bench-measured correction for the 910 ohm eFuse channels.
 * @param amps Current from IlmToAmps().
 * @return Corrected current, in A.
 */
static float Correct910(float amps) {
    return (amps + EFUSE_910_CORRECTION_OFFSET_A) / EFUSE_910_CORRECTION_SLOPE;
}

/**
 * @brief Converts the TEL current sensor reading to current: IOUT = (VIOUT - VZERO) / SENS.
 * @param counts Raw TEL sense reading.
 * @return TEL rail current, in A.
 */
static float TelSenseToAmps(uint16_t counts) {
    return (CountsToVolts(counts) - TEL_SENSE_ZERO_V) / TEL_SENSE_V_PER_A;
}

/**
 * @brief Converts A to mA, rounded to the nearest mA.
 * @param amps Current, in A.
 * @return Current, in mA.
 */
static int32_t AmpsToMilliamps(float amps) {
    float milliamps = amps * 1000.0f;

    return (int32_t)((milliamps >= 0.0f) ? (milliamps + 0.5f) : (milliamps - 0.5f));
}
