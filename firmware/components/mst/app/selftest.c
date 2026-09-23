#include <stdint.h>
#include <stdlib.h>

#include "module_data.h"
#include "mst_defs.h"
#include "mst_types.h"
#include "spi_driver.h"
#include "selftest.h"

#include "adc.h"

extern const module_t pack_modules[NUM_MODULES];
extern const faults_t pack_faults;
extern const warnings_t pack_warnings;
extern const pack_state_t pack_state;
extern const slave_t slaves[SLAVE_NUM_DEVICES];


uint32_t Get_ADC_Noise(ADC_HandleTypeDef *hadc)
{
    uint32_t seed = 0;

    for(int i = 0; i < 32; i++)
    {
        HAL_ADC_Start(hadc);
        if (HAL_ADC_PollForConversion(hadc, 10) == HAL_OK)
        {
            // Grab the lowest bit of the ADC reading (the most volatile/noisy bit)
            seed |= (HAL_ADC_GetValue(hadc) & 0x01);
        }
        HAL_ADC_Stop(hadc);

        seed <<= 1; // Shift over to build up a multi-bit value. Note that "<<=" is the "left bitshift and assignment" operator
    }
    LOG_DEBUG("Self Check init complete. Using random seed: %x", seed);
    return seed;
}

void SelfCheck_Init(ADC_HandleTypeDef *hadc) {
    // ADC noise should be different on every bootup.
    // We must seed the random number generator with an unpredictable seed
    // to ensure the same commuication test doesn't repeat on two separate bootup's.
    srand(Get_ADC_Noise(hadc));
}

static bool DoesRegGroupMatch_(uint8_t reg_group1[SLAVE_NUM_DEVICES][SLAVE_REG_SIZE_BYTES],
                              uint8_t reg_group2[SLAVE_NUM_DEVICES][SLAVE_REG_SIZE_BYTES])
{
    for (int ic_num = 0; ic_num < SLAVE_NUM_DEVICES; ic_num++)
    {
        for (int i = 0; i < SLAVE_REG_SIZE_BYTES; i++)
        {
            if (reg_group1[ic_num][i] != reg_group2[ic_num][i])
                return false;
        }
    }
    return true;
}

Slave_Status_t SelfCheck_Comms(void) {
    uint8_t test_data[SLAVE_NUM_DEVICES][SLAVE_REG_SIZE_BYTES] = {0};

    LOG_DEBUG("Starting communication check");
    for (int ic_num = 0; ic_num < SLAVE_NUM_DEVICES; ic_num++)
    {
        for (int i = 0; i < SLAVE_REG_SIZE_BYTES; i++)
        {
            test_data[ic_num][i] = (uint8_t) rand();
        }
    }

    Slave_Status_t comm_status = {Slave_OK, 0};
    uint8_t test_data_rx[SLAVE_NUM_DEVICES][SLAVE_REG_SIZE_BYTES] = {0};
    bool reg_group_match;

    Slave_WakeUp();
    // Note: Slave_WriteRegisterGroup returns void, so a write failure can
    // only be detected via the subsequent readback comparison.
    Slave_WriteRegisterGroup(CMD_WRCOMM, test_data);

    comm_status = Slave_ReadRegisterGroup(CMD_RDCOMM, test_data_rx);
    if (comm_status.error != Slave_OK)
    {
        LOG_DEBUG("Comm readback failed. Comm error: %d.", comm_status.error);
        return comm_status;
    }

    reg_group_match = DoesRegGroupMatch_(test_data, test_data_rx);

    LOG_DEBUG("Reg group match: %d. Comm error: %d.", reg_group_match, comm_status.error);

    if (!reg_group_match)
    {
        comm_status.error = Slave_ERROR_SELFTEST;
        comm_status.device_num = 0; // Mismatch is pack-wide; cannot isolate a single device
    }

    return comm_status;
}


/**
 * @brief Checks internal die temperature of ADBMS1818's for safe operating condition
 *
 * @return If at least one ADBMS1818 has a die temperature nearing thermal shutdown
 * 		  threshold, returns an error with the device index of the first overheating IC.
 **/
Slave_Status_t SelfCheck_DieTemp(void)
{
    Slave_Status_t status = {Slave_OK, 0};
    uint8_t registerSTATA[SLAVE_NUM_DEVICES][SLAVE_REG_SIZE_BYTES];
    uint16_t itmp_adc[SLAVE_NUM_DEVICES];
    uint32_t itmp_mC[SLAVE_NUM_DEVICES];

    LOG_DEBUG("Starting ADBMS1818 internal temperature check.");
    status = Slave_SendCmdAndPoll(CMD_ADSTAT_ITMP);
    if (status.error != Slave_OK)
        return status;

    // Retrieve register reading
    status = Slave_ReadRegisterGroup(CMD_RDSTATA, registerSTATA);
    if (status.error != Slave_OK)
        return status;

    for (int board = 0; board < SLAVE_NUM_DEVICES; board++)
    {
        // Combine 2 bytes of die temperature reading
        itmp_adc[board] = (((uint16_t)registerSTATA[board][3]) << 8) | ((uint16_t) registerSTATA[board][2]);
        itmp_mC[board] = (uint32_t) itmp_adc[board] * DIE_TEMP_CONVERT_RATIO;
        LOG_DEBUG("Slaveboard number %d internal IC temperature: %u m°C\r\n", board, itmp_mC[board]);

        if (itmp_mC[board] >= ADBMS_TEMPLIMIT_mC)
        {
            status.error = Slave_ERROR_SELFTEST;
            status.device_num = board + 1;
        }
    }

    return status;
}

/**
 * @brief Measures independent reference voltage VREF2 to verify measurement of ADC1 (ADBMS1818 p.30).
 * 		  Readings outside the range 2.990V to 3.014V indicate the system is out of its specified tolerance.
 * 		  Accuracy of ADC2 measurement is verified separately using SelfCheck_OverlapVoltage() command.
 *
 * @return OK if reading is within tolerance range. Slave_ERROR_SELFTEST if ADC1 measurement is outside
 * 		   tolerance range.
 */
Slave_Status_t SelfCheck_VREF2(void)
{
    uint8_t registerAUXB[SLAVE_NUM_DEVICES][SLAVE_REG_SIZE_BYTES];
    uint16_t vref2_volt_100uV = 0;
    uint16_t vref2_volt_mV = 0;
    Slave_Status_t status = {Slave_OK, 0};

    LOG_DEBUG("Starting V_Ref2 check.");
    status = Slave_SendCmdAndPoll(CMD_ADAX_VREF2);
    if (status.error != Slave_OK)
        return status;

    status = Slave_ReadRegisterGroup(CMD_RDAUXB, registerAUXB);
    if (status.error != Slave_OK)
        return status;

    for (int board = 0; board < SLAVE_NUM_DEVICES; board++)
    {
        // Combine the 2 bytes of each cell voltage together
        vref2_volt_100uV =
            (((uint16_t) registerAUXB[board][5]) << 8) | ((uint16_t) registerAUXB[board][4]);
        vref2_volt_mV = vref2_volt_100uV / 10;

        LOG_DEBUG("Slaveboard idx %d V_ref2 voltage: %d.%03dV", 
            board,
            vref2_volt_mV / 1000, vref2_volt_mV % 1000
        );

        if (vref2_volt_mV < VREF_LOWERBOUND_mV ||
            vref2_volt_mV > VREF_UPPERBOUND_mV)
        {
            status.error = Slave_ERROR_SELFTEST;
            status.device_num = board + 1;
            LOG_DEBUG("Slaveboard idx %d failed.", board);
        }
    }

    return status;
}


/**
 * @brief Checks for any open wires between the ADCs of the ADBMS1818 and the external cells, making use of the ADOW
 *        command (see datasheet p.32).
 *
 * @return OK if no open wires detected. Slave_ERROR_SELFTEST with device_num set to
 *         the 1-indexed board number of the first open wire detected otherwise.
 **/
Slave_Status_t SelfCheck_OpenWire(void)
{
    // Stores converted voltage values measured at each pin on the ADBMS1818 for both slave boards
    module_t modules_PUP[NUM_MODULES] = {0};
    module_t modules_PDOWN[NUM_MODULES] = {0};

    Slave_Status_t status = {Slave_OK, 0};
    Slave_Error_t err;

    LOG_DEBUG("Running open wire self-check with %d pull-up and %d pull-down repetitions.", PUP_REPS, PDOWN_REPS);

    // Send open wire check command at least twice for PUP to allow capacitors to fully charge before
    // reading voltage register data.
    for (int i = 0; i < PUP_REPS; i++)
    {
        status = Slave_SendCmdAndPoll(CMD_ADOW_PUP);
        if (status.error != Slave_OK)
            return status;
    }

    // Read cell voltages from register after pull-up current is applied.
    err = RetrieveVoltageMeasurement((slave_t *)slaves, modules_PUP);
    if (err != Slave_OK)
    {
        status.error = err;
        status.device_num = 0;
        return status;
    }

    // Send open wire check command PDOWN_REPS times for PDOWN to allow capacitors to fully charge before
    // reading voltage register data.
    for (int i = 0; i < PDOWN_REPS; i++)
    {
        status = Slave_SendCmdAndPoll(CMD_ADOW_PDOWN);
        if (status.error != Slave_OK)
            return status;
    }

    // Read cell voltages from register after pull-down current is applied.
    err = RetrieveVoltageMeasurement((slave_t *)slaves, modules_PDOWN);
    if (err != Slave_OK)
    {
        status.error = err;
        status.device_num = 0;
        return status;
    }

    // Take the signed difference (PUP - PDOWN) for cells 2 to NUM_MODULES.
    // Per ADBMS1818 datasheet p.32, a difference < -400 mV indicates an open
    // wire at module n (absolute value must NOT be used: a healthy cell reads
    // nearly the same voltage in both phases, i.e. diff ~= 0).
    for (int module = 1; module < NUM_MODULES; module++)
    {
        int32_t volt_diff_mv = (int32_t)modules_PUP[module].voltage_mv -
                               (int32_t)modules_PDOWN[module].voltage_mv;

        if (volt_diff_mv < OPEN_WIRE_VOLTAGE_mV)
        {
            unsigned board = (unsigned)module / (NUM_MODULES / SLAVE_NUM_DEVICES);
            LOG_DEBUG("Open wire self-check failed at module %d (board %d) with voltage difference %ld mV",
                module,
                board,
                (long)volt_diff_mv);
            status.error = Slave_ERROR_SELFTEST;
            status.device_num = board + 1;
            return status;
        }
    }

    LOG_DEBUG("Open wire self-check passed.");
    return status;
}

/**
 * @brief Verifies that measurements taken using ADC1, ADC2 and ADC3 all agree within a certain
 * 		  range defined by ST_VOLTAGE_ERROR. Uses ADOL command to measure Cell 7 with ADC1 and ADC2.
 * 		  Then it simultaneously measures Cell 13 with both ADC2 and ADC3. This function compares
 * 		  the results of these measurements and reports any inconsistency as an error. (ADBMS1818 p. 30)
 *
 * @return OK if overlapping measurements are in agreement. Error status if ADCs do not produce the same
 * 		   voltage reading for each cell measured.
 */
Slave_Status_t SelfCheck_OverlapVoltage(void)
{
    // 2x 6-byte sets (each from a different register group of the ADBMS1818) for each ADBMS1818
    uint8_t ADC_data[OVERLAP_TEST_REGS][SLAVE_NUM_DEVICES][SLAVE_REG_SIZE_BYTES];
    float overlapVoltage[SLAVE_NUM_DEVICES][OVERLAP_READINGS_PER_BOARD];
    Slave_Status_t status = {Slave_OK, 0};

    LOG_DEBUG("Starting overlap voltage self-check.");

    status = Slave_SendCmdAndPoll(CMD_ADOL);
    if (status.error != Slave_OK)
        return status;

    status = Slave_ReadRegisterGroup(CMD_RDCVC, ADC_data[0]);
    if (status.error != Slave_OK)
        return status;

    status = Slave_ReadRegisterGroup(CMD_RDCVE, ADC_data[1]);
    if (status.error != Slave_OK)
        return status;

    // Each cell voltage is provided as a 16-bit value where
    // voltage = 0.0001V * raw value
    // Each 6-byte Cell Voltage Register Group holds 3 cell voltages
    // First 2 bytes of Cell Voltage Register Group C is C7V
    // First 2 bytes of Cell Voltage Register Group E is C13V
    for (int board = 0; board < SLAVE_NUM_DEVICES; board++)
    {
        for (int reg_group = 0; reg_group < OVERLAP_TEST_REGS; reg_group++)
        {
            for (int reading_num = 0; reading_num < OVERLAP_READINGS_PER_REG; reading_num++)
            {
                // Combine the 2 bytes of each cell voltage together
                uint16_t cell_voltage_raw = 
                    ((uint16_t)(ADC_data[reg_group][board][2 * reading_num + 1]) << 8) | (uint16_t)(ADC_data[reg_group][board][2 * reading_num]);

                
                // Convert to volts
                uint32_t converted_voltage_mv = cell_voltage_raw / 10;

                overlapVoltage[board][reading_num + 2 * reg_group] = converted_voltage_mv;
            }
        }
    }

    // Check if overlap readings agree within delta
    for (int board = 0; board < SLAVE_NUM_DEVICES; board++)
    {
        for (int cell = 0; cell < NUM_TEST_CELLS; cell++)
        {
            float ADC1_voltage = overlapVoltage[board][2 * cell];
            float ADC2_voltage = overlapVoltage[board][2 * cell + 1];
            float delta = (ADC1_voltage > ADC2_voltage) ? 
                (ADC1_voltage - ADC2_voltage) : 
                (ADC2_voltage - ADC1_voltage);

            if (delta > ADBMS_VOLTAGE_ERROR_mV)
            {
                LOG_DEBUG("Overlap voltage mismatch on board %d cell %d: ADC1=%lu mV ADC2=%lu mV delta=%lu mV",
                    board,
                    cell,
                    (uint32_t)ADC1_voltage,
                    (uint32_t)ADC2_voltage,
                    (uint32_t)delta);
                status.error = Slave_ERROR_SELFTEST;
                status.device_num = board + 1;
            }
        }
    }

    if (status.error == Slave_OK)
    {
        LOG_DEBUG("Overlap voltage self-check passed.");
    }

    return status;
}
