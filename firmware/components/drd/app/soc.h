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
void SocPredictThenUpdate(float g_total_pack_voltage_soc,
                          float g_pack_current_soc,
                          float time_step);
float SocGetSoc(); // 0 - 1
float SocGetVoltage();
float SocGetUc();
void SocInitSoc(int voltage);

#endif //__SOC_H__
