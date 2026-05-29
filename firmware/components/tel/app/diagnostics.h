#ifndef DIAGNOSTIC_H_
#define DIAGNOSTIC_H_

#include "stdint.h"
#include "stdbool.h"

typedef union {
    struct {
        volatile bool tel_crash_iwdg : 1;         
        volatile bool imu_read_fail : 1;
        volatile bool imu_write_fail : 1;
        volatile bool gps_read_fail : 1;
        volatile bool gps_write_fail : 1;
    } bits;
    volatile uint8_t raw;
} DiagnosticTEL;

//TODO: remove extern and change to getter functions when telemetry app is fully implemented
extern volatile uint32_t g_time_since_bootup;
extern DiagnosticTEL g_tel_diagnostic_flags;

/**
 * @brief  Initializes the diagnostics module
 * @retval None
 */
void DiagnosticsInit();
/**
 * @brief  Sends the time since bootup via CAN
 * @retval None
 */
void DiagnosticsTransmit();


#endif /* DIAGNOSTIC_H_ */
