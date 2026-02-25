/**
 * @file    soc.h
 * @brief   State-of-Charge (SOC) estimation module for UBC Solar DRD board
 *
 * This header declares the public API for the SOC estimation module, which uses an Extended Kalman Filter (EKF)
 * to estimate battery state-of-charge and related parameters based on voltage and current measurements.
 *
 * @author  UBC Solar
 * @date    Feb 4 2026
 */

#ifndef __SOC_H__
#define __SOC_H__

/* INCLUDES */
#include <stdint.h>

/* DEFINES */
#define TIME_STEP (100.0f)
#define SOC_TIME_STEP (TIME_STEP / 1000.0f) // ms

/* GLOBALS */
extern volatile float g_total_pack_voltage_soc;
extern volatile float g_pack_current_soc;

/* FUNCTION DECLARATIONS */
/**
 * @brief Run the SOC prediction and update step.
 *
 * @param g_total_pack_voltage_soc Measured total pack voltage (V)
 * @param g_pack_current_soc Measured pack current (A)
 * @param time_step Time step for EKF update (s)
 */
void SocPredictThenUpdate(float g_total_pack_voltage_soc,
                          float g_pack_current_soc,
                          float time_step);

/**
 * @brief Get the current estimated state-of-charge (SOC).
 * @return SOC value (0.0 to 1.1)
 */
float SocGetSoc(void);

/**
 * @brief Get the last predicted terminal voltage from the EKF.
 * @return Predicted voltage (V)
 */
float SocGetVoltage(void);

/**
 * @brief Get the current estimated capacitor voltage (Uc) from the EKF state.
 * @return Uc value (V)
 */
float SocGetUc(void);

/**
 * @brief Initialize the SOC state from a measured voltage.
 * @param voltage Measured pack voltage (V)
 */
void SocInitSoc(int voltage);

#endif //__SOC_H__
