/**
 * @file    diagnostic.c
 * @brief   MDI diagnostic telemetry and fault flag handling.
 */

#include "diagnostic.h"

#include "can_driver.h"
#include "rtd_driver.h"

static MdiDiagnosticFlags s_diagnostic_flags = {0};

static const CAN_TxHeaderTypeDef mdi_time_since_bootup_header = {
	.StdId = MDI_TIME_SINCE_BOOTUP_CAN_ID,
	.ExtId = 0x0000,
	.IDE = CAN_ID_STD,
	.RTR = CAN_RTR_DATA,
	.DLC = 4
};

static const CAN_TxHeaderTypeDef mdi_diagnostic_flags_header = {
	.StdId = MDI_DIAGNOSTIC_FLAGS_CAN_ID,
	.ExtId = 0x0000,
	.IDE = CAN_ID_STD,
	.RTR = CAN_RTR_DATA,
	.DLC = 1
};

static const CAN_TxHeaderTypeDef mdi_motor_temp_header = {
	.StdId = MDI_MOTOR_TEMP_CAN_ID,
	.ExtId = 0x0000,
	.IDE = CAN_ID_STD,
	.RTR = CAN_RTR_DATA,
	.DLC = 5
};

/**
 * @brief Sets or clears the motor over-temperature diagnostic bit.
 * @param is_motor_over_temp true to set the flag, false to clear it.
 */
static void DiagnosticSetMotorOverTemp(bool is_motor_over_temp);

void DiagnosticInit(void)
{
	s_diagnostic_flags.raw = 0U;
}

void DiagnosticSetIwdgCrash(bool has_crash)
{
	s_diagnostic_flags.bits.mdi_crash_iwdg = has_crash;
}

void DiagnosticSetVoltageOverThreshold(bool is_over_threshold)
{
	s_diagnostic_flags.bits.mdi_voltage_over_threshold = is_over_threshold;
}

static void DiagnosticSetMotorOverTemp(bool is_motor_over_temp)
{
	s_diagnostic_flags.bits.mdi_motor_over_temp = is_motor_over_temp;
}

void DiagnosticSendTimeSinceBootup(void)
{
	static uint32_t time_since_bootup_counter = 0;
	uint8_t data[4];

	data[0] = (uint8_t)(time_since_bootup_counter & 0xFFU);
	data[1] = (uint8_t)((time_since_bootup_counter >> 8) & 0xFFU);
	data[2] = (uint8_t)((time_since_bootup_counter >> 16) & 0xFFU);
	data[3] = (uint8_t)((time_since_bootup_counter >> 24) & 0xFFU);

	CanDriverSend(&mdi_time_since_bootup_header, data);
	time_since_bootup_counter++;
}

void DiagnosticSendRtdTemp(void)
{
	int32_t rtd_temp_c = 0;
	RtdStatus rtd_status = RtdDriverGetTemp(&rtd_temp_c);
	bool rtd_read_success = (rtd_status == RtdStatusOk);
	if (rtd_read_success)
	{
		if (!s_diagnostic_flags.bits.mdi_motor_over_temp && rtd_temp_c >= MOTOR_OVER_TEMP_SET_C)
		{
			DiagnosticSetMotorOverTemp(true);
		}
		else if (s_diagnostic_flags.bits.mdi_motor_over_temp && rtd_temp_c <= (MOTOR_OVER_TEMP_SET_C - 5)) // cancels out noise
		{
			DiagnosticSetMotorOverTemp(false);
		}
	}

	uint8_t data[5];
	data[0] = (uint8_t)(rtd_read_success);
	data[1] = (uint8_t)(rtd_temp_c & 0xFFU);
	data[2] = (uint8_t)((rtd_temp_c >> 8) & 0xFFU);
	data[3] = (uint8_t)((rtd_temp_c >> 16) & 0xFFU);
	data[4] = (uint8_t)((rtd_temp_c >> 24) & 0xFFU);

	CanDriverSend(&mdi_motor_temp_header, data);
}

void DiagnosticSendFlags(void)
{
	uint8_t data[1] = {s_diagnostic_flags.raw};

	CanDriverSend(&mdi_diagnostic_flags_header, data);
}
