#ifndef TEL_OTA_SAFETY_H
#define TEL_OTA_SAFETY_H

#include <stdbool.h>
#include <stdint.h>

#define TEL_OTA_SAFETY_FRESHNESS_MS 1000U
#define TEL_OTA_SAFETY_STABLE_MS     500U

typedef struct {
    bool mdu_speed_received;
    bool mdu_state_received;
    bool motor_command_received;
    bool drd_diagnostics_received;
    bool stable_safe_state;
    uint32_t mdu_speed_timestamp_ms;
    uint32_t mdu_state_timestamp_ms;
    uint32_t motor_command_timestamp_ms;
    uint32_t drd_diagnostics_timestamp_ms;
    uint32_t safe_since_ms;
    uint16_t motor_speed_raw;
    uint16_t accelerator_position_raw;
    uint16_t output_target_raw;
    uint16_t throttle_dac;
    uint16_t regen_dac;
    uint8_t drive_action_status;
    uint8_t drd_drive_state;
    uint8_t drd_timeout_flags;
    bool brake_pressed;
    bool motor_comm_fault;
} TelOtaSafetyState;

void TelOtaSafetyInit(TelOtaSafetyState *state);
void TelOtaSafetyObserveCanFrame(TelOtaSafetyState *state,
                                uint32_t can_id,
                                bool is_extended_id,
                                uint8_t dlc,
                                const uint8_t *data,
                                uint32_t now_ms);
bool TelOtaSafetyUpdateAllowed(TelOtaSafetyState *state, uint32_t now_ms);

#endif /* TEL_OTA_SAFETY_H */
