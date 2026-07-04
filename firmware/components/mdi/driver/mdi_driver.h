/**
 * @file    mdi_driver.h
 * @brief   MDI hardware control definitions.
 * 
 * @author  Martin Wu & Tony Chen
 * @date    May 22, 2026
 */

#ifndef __MDI_DRIVER_H__
#define __MDI_DRIVER_H__

#include <stdbool.h>
#include <stdint.h>

#define MDI_MAX_DAC_VALUE ((uint16_t)(0.90f * 0x3FFU))
#define MDI_MAX_TIMEOUT_VALUE 100U
#define MDI_DIAGNOSTICS_DELAY 1000U
#define MDI_DAC7571_WRITE_ADDR (0x4Cu << 1U)

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

/**
 * @brief Sets a DAC output value for the selected motor control channel.
 * @param dac_addr Target DAC channel address (accel or regen).
 * @param voltage_value Requested DAC code before clamping/scaling.
 */
void MdiSetDacVoltage(MdiDacAddr dac_addr, uint16_t voltage_value);

/**
 * @brief Applies a parsed motor command to DAC and GPIO outputs.
 * @param command Pointer to the motor command to apply.
 */
void MdiSetMotorCommand(const MdiMotorCommand *command);

/**
 * @brief Parses raw CAN payload bytes into an MDI motor command struct.
 * @param buffer Pointer to the incoming CAN payload buffer.
 * @param command Output motor command populated from the payload.
 */
void MdiParseMotorCommand(const uint8_t *buffer, MdiMotorCommand *command);

/**
 * @brief Drives outputs to a safe stopped motor state.
 */
void MdiStopMotor(void);

#endif /* __MDI_DRIVER_H__ */
