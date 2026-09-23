#pragma once

#include "mst_defs.h"
#include "mst_types.h"

/**
 * @brief Seeds the random number generator used by the communication self-test.
 *
 * Samples ADC noise to build an unpredictable seed so the random COMM
 * register test pattern differs on every bootup.
 *
 * @param hadc Pointer to the HAL ADC handle used as an entropy source
 */
void SelfCheck_Init(ADC_HandleTypeDef *hadc);
/**
 * @brief Verifies isoSPI communication with the ADBMS1818 devices.
 *
 * Writes random test data to the COMM register group (CMD_WRCOMM) and reads
 * it back (CMD_RDCOMM), then compares the written and received data.
 *
 * @return Slave_OK if written and received data match, otherwise the error
 *         status returned by the underlying slave transaction
 */
Slave_Status_t SelfCheck_Comms(void);
/**
 * @brief Checks internal die temperature of ADBMS1818's for safe operating condition.
 *
 * @return If at least one ADBMS1818 has a die temperature nearing thermal shutdown
 *         threshold, returns an error with the device index of the first overheating IC.
 */
Slave_Status_t SelfCheck_DieTemp(void);
/**
 * @brief Measures independent reference voltage VREF2 to verify measurement of ADC1 (ADBMS1818 p.30).
 *        Readings outside the range 2.990V to 3.014V indicate the system is out of its specified tolerance.
 *        Accuracy of ADC2 measurement is verified separately using SelfCheck_OverlapVoltage() command.
 *
 * @return OK if reading is within tolerance range. Slave_ERROR_SELFTEST if ADC1 measurement is outside
 *         tolerance range.
 */
Slave_Status_t SelfCheck_VREF2(void);
/**
 * @brief Checks for any open wires between the ADCs of the ADBMS1818 and the external cells, making use of the ADOW
 *        command (see datasheet p.32).
 *
 * @return OK if no open wires detected. Slave_ERROR_SELFTEST with device_num set to
 *         the 1-indexed board number of the first open wire detected otherwise.
 */
Slave_Status_t SelfCheck_OpenWire(void);
/**
 * @brief Verifies that measurements taken using ADC1, ADC2 and ADC3 all agree within a certain
 *        range defined by ST_VOLTAGE_ERROR. Uses ADOL command to measure Cell 7 with ADC1 and ADC2.
 *        Then it simultaneously measures Cell 13 with both ADC2 and ADC3. This function compares
 *        the results of these measurements and reports any inconsistency as an error. (ADBMS1818 p. 30)
 *
 * @return OK if overlapping measurements are in agreement. Error status if ADCs do not produce the same
 *         voltage reading for each cell measured.
 */
Slave_Status_t SelfCheck_OverlapVoltage(void);

