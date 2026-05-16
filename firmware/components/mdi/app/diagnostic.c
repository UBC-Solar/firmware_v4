#include "diagnostic.h"

#include "can_driver.h"

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

void DiagnosticSendFlags(void)
{
	uint8_t data[1] = {s_diagnostic_flags.raw};

	CanDriverSend(&mdi_diagnostic_flags_header, data);
}
