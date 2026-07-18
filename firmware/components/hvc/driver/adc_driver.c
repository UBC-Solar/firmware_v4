#include "adc_driver.h"

static void ADC1_ProcessReadings(int half);

static ADC_HandleTypeDef *hadc1;

static volatile uint16_t adc_buffer[ADC1_NUM_CHANNELS * ADC1_SAMPLE_COUNT * 2];

static ADC_Readings adc1_readings = {0};
static ADC_Voltages adc1_voltages = {0};

/**
 * @brief Initialize ADC driver and start DMA in circular mode.
 * @param _hadc1 Pointer to the ADC handle configured in CubeMX.
 */
void ADC_Init(ADC_HandleTypeDef *_hadc1, TIM_HandleTypeDef *_htim3) {
    hadc1 = _hadc1;

    HAL_TIM_Base_Start(_htim3); // TIM3 triggered ADC conversion

    HAL_ADC_Start_DMA(hadc1, (uint32_t*)adc_buffer, ADC1_NUM_CHANNELS * ADC1_SAMPLE_COUNT * 2);

    //UART_Printf("ADC Initialized with DMA buffer size: %d\n\r", ADC1_NUM_CHANNELS * ADC1_SAMPLE_COUNT * 2);
}

/**
 * @brief Read averaged ADC values for a given half of the buffer, then convert to voltages and store in global structs.
 * @param half 0 for the first half, 1 for the second half.
 * @return None
 */
static void ADC1_ProcessReadings(int half) {
    uint32_t sum[ADC1_NUM_CHANNELS] = {0};
    uint16_t results[ADC1_NUM_CHANNELS] = {0};

    // Sum readings
    for (uint8_t sample = 0; sample < ADC1_SAMPLE_COUNT; sample++) {
        for (uint8_t channel = 0; channel < ADC1_NUM_CHANNELS; channel++) {
            sum[channel] += adc_buffer[half * ADC1_NUM_CHANNELS * ADC1_SAMPLE_COUNT + sample * ADC1_NUM_CHANNELS + channel];
        }
    }

    // Average readings (divide sum by SAMPLE_COUNT)
    for (int channel = 0; channel < ADC1_NUM_CHANNELS; channel++) {
        results[channel] = sum[channel] / ADC1_SAMPLE_COUNT;
    }

    // Store readings and voltages
    for (int channel = 0; channel < ADC1_NUM_CHANNELS; channel++) {
        uint16_t voltage = (uint64_t)results[channel] * ADC_VOLTAGE_REFERENCE / ADC_RESOLUTION; // Convert to millivolts with uint32_t for no truncation
        switch (channel) {
            case 0:
                adc1_readings.dcdc_thermistor = results[channel];
                adc1_voltages.dcdc_thermistor = voltage;
            break;

            case 1:
                adc1_readings.motor_precharge = results[channel];
                adc1_voltages.motor_precharge = voltage;
            break;
            
            case 2:
                adc1_readings.mppt_precharge = results[channel];
                adc1_voltages.mppt_precharge = voltage;
            break;
            
            case 3:
                adc1_readings.supp_sense = results[channel];
                adc1_voltages.supp_sense = voltage;
            break;

            case 4:
                adc1_readings.lv_curr_sense = results[channel];
                adc1_voltages.lv_curr_sense = voltage;
            break;
        }
    }
}

ADC_Readings ADC_GetReadings(void) {
    return adc1_readings;
}

ADC_Voltages ADC_GetVoltages(void) {
    return adc1_voltages;
}

/**
 * @brief HAL invokes this callback when the first half of the DMA buffer is filled.
 * @param hadc Pointer to the ADC handle that triggered the callback.
 */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc) {
    if (hadc == hadc1) {
        ADC1_ProcessReadings(0);
    }
}

/**
 * @brief HAL invokes this callback when the second half of the DMA buffer is filled.
 * @param hadc Pointer to the ADC handle that triggered the callback.
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
    if (hadc == hadc1) {
        ADC1_ProcessReadings(1);
    }
}

/**
 * Thermistor voltage-to-temperature look-up table.
 * See https://www.vishay.com/docs/29050/ntclg100.pdf.
 * First value is temperature in milli-Celsius,
 * second value is resistance (computed at initialization time),
 * third value is thermistor resistance.
 */
thermistor_mapping_t thermistor_temp_lut[THERMISTOR_LUT_TABLE_SIZE] = {
    { -30000, 0, 175200 },
    {      0, 0, 32554  },
    {  10000, 0, 19872  },
    {  20000, 0, 12488  },
    // values above are out of expected range, and thus more sparse
    {  25000, 0, 10000  },
    {  30000, 0, 8059   },
    {  35000, 0, 6535   },
    {  40000, 0, 5330   },
    {  45000, 0, 4372   },
    {  50000, 0, 3605   },
    {  55000, 0, 2989   },
    {  60000, 0, 2490   },
    {  65000, 0, 2084   },
    // values below are out of expected range, and thus more sparse
    {  80000, 0, 1256   },
    { 100000, 0, 677.3  },
    { 150000, 0, 182.6  },
    { 200000, 0, 63.67  }
};

//COPIED FROM MST
void InitTemperatureReading() {
    for (int i = 0; i < THERMISTOR_LUT_TABLE_SIZE; i++) {
        //V_T: thermistor voltage. R_T: thermistor resistance.
        // V_T = V_ref2 * (R_T)/(10,000 + R_T)
        //Vref2 = 3300000 uV
        uint64_t resistance_Ohm = (uint64_t) 3300000 * thermistor_temp_lut[i].resistance_Ohm / (10000 + thermistor_temp_lut[i].resistance_Ohm);
        thermistor_temp_lut[i].voltage_uV = resistance_Ohm;
    }
}

//COPIED FROM MST
/**
 * @brief Converts a voltage measurement to Temperature in milli-Celsius using pure integer math
 * @param thermistor_uV The raw ADC reading in microvolts
 * @return Temperature in milli-Celsius (e.g., 25500 = 25.5 C)
 */
int32_t ThermistorVoltToTemp_(uint32_t thermistor_uV) {
    // 1. Clamp to maximum or minimum values
    if (thermistor_uV >= thermistor_temp_lut[0].voltage_uV) {
        return thermistor_temp_lut[0].temperature_mC;
    }
    if (thermistor_uV <= thermistor_temp_lut[THERMISTOR_LUT_TABLE_SIZE - 1].voltage_uV) {
        return thermistor_temp_lut[THERMISTOR_LUT_TABLE_SIZE - 1].temperature_mC;
    }

    // 2. Iterate temperature look up table to find a match for the measured thermistor voltage
    for (uint16_t i = 0; i < THERMISTOR_LUT_TABLE_SIZE - 1; i++) {
        // Find the bounding ADC values (remember: voltage_uV is descending)
        if (thermistor_uV <= thermistor_temp_lut[i].voltage_uV && thermistor_uV >= thermistor_temp_lut[i + 1].voltage_uV) {

            int32_t adc1 = thermistor_temp_lut[i].voltage_uV;     // Higher ADC (Colder)
            int32_t t1   = thermistor_temp_lut[i].temperature_mC;       // Lower Temp

            int32_t adc2 = thermistor_temp_lut[i + 1].voltage_uV; // Lower ADC (Hotter)
            int32_t t2   = thermistor_temp_lut[i + 1].temperature_mC;   // Higher Temp

            // 3. Linear interpolation between the two values
            // Formula: T = T1 + ( (T2 - T1) * (ADC1 - thermistor_uV) ) / (ADC1 - ADC2)
            // Note: We arrange the subtraction to keep values positive before division
            int32_t temp_diff = t2 - t1;
            int32_t adc_diff = adc1 - adc2;
            int32_t meas_diff = adc1 - thermistor_uV;

            // Multiply before dividing to preserve precision.
            // Max temp_diff (e.g., 10000 mC) * Max meas_diff (e.g., 10000 ADC) = 100,000,000.
            // This easily fits inside a signed 32-bit integer without overflowing.
            int64_t temperature_mC = (int64_t) t1 + (((int64_t)temp_diff * meas_diff) / adc_diff);

            return temperature_mC;
        }
    }

    // Fallback error code (e.g., absolute zero in mC)
    return -273150;
}