/**
 * @file    mdi_driver.h
 * @brief   MDI hardware control definitions.
 */
#ifndef __MDI_DRIVER_H__
#define __MDI_DRIVER_H__

#include <stdbool.h>
#include <stdint.h>

#define MDI_MAX_DAC_VALUE ((uint16_t)(0.90f * 0x3FFU))
#define MDI_MAX_TIMEOUT_VALUE 100U
#define MDI_DIAGNOSTICS_DELAY 1000U

typedef enum {
    MDI_DAC_REGEN = (0b0001101u << 1),
    MDI_DAC_ACCEL = (0b0001110u << 1)
} MdiDacAddr;

typedef enum {
    MDI_DIRECTION_FORWARD = 0,
    MDI_DIRECTION_REVERSE = 1
} MdiDirection;

typedef enum {
    MDI_POWER_MODE = 0,
    MDI_ECO_MODE = 1
} MdiPowerMode;

typedef struct {
    uint16_t accel_DAC_value;
    uint16_t regen_DAC_value;
    bool direction_value;
    bool eco_mode_value;
} MdiMotorCommand;

void MdiDriverSetDacVoltage(MdiDacAddr dac_addr, uint16_t voltage_value);
void MdiDriverSetMotorCommand(const MdiMotorCommand *command);
void MdiDriverParseMotorCommand(const uint8_t *buffer, MdiMotorCommand *command);
void MdiDriverStopMotor(void);

#endif /* __MDI_DRIVER_H__ */
