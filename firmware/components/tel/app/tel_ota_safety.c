#include "tel_ota_safety.h"

#include "can_app.h"

#include <stddef.h>
#include <string.h>

#define MDU_DRIVE_ACTION_STOP       0U
#define DRD_DRIVE_STATE_PARK        0x03U
#define DRD_SPEED_TIMEOUT_MASK      (1U << 0)
#define DRD_DRIVE_TIMEOUT_MASK      (1U << 1)
#define DRD_REQUIRED_TIMEOUT_MASK   (DRD_SPEED_TIMEOUT_MASK | DRD_DRIVE_TIMEOUT_MASK)

static bool IsFresh(bool received,
                    uint32_t timestamp_ms,
                    uint32_t now_ms)
{
    return received &&
           ((uint32_t)(now_ms - timestamp_ms) <= TEL_OTA_SAFETY_FRESHNESS_MS);
}

static bool HasSafeVehicleState(const TelOtaSafetyState *state,
                                uint32_t now_ms)
{
    return IsFresh(state->mdu_speed_received,
                   state->mdu_speed_timestamp_ms,
                   now_ms) &&
           IsFresh(state->mdu_state_received,
                   state->mdu_state_timestamp_ms,
                   now_ms) &&
           IsFresh(state->motor_command_received,
                   state->motor_command_timestamp_ms,
                   now_ms) &&
           IsFresh(state->drd_diagnostics_received,
                   state->drd_diagnostics_timestamp_ms,
                   now_ms) &&
           (state->motor_speed_raw == 0U) &&
           (state->drive_action_status == MDU_DRIVE_ACTION_STOP) &&
           (state->accelerator_position_raw == 0U) &&
           (state->output_target_raw == 0U) &&
           (state->throttle_dac == 0U) &&
           (state->regen_dac == 0U) &&
           (state->drd_drive_state == DRD_DRIVE_STATE_PARK) &&
           state->brake_pressed &&
           !state->motor_comm_fault &&
           ((state->drd_timeout_flags & DRD_REQUIRED_TIMEOUT_MASK) == 0U);
}

static void UpdateStableState(TelOtaSafetyState *state, uint32_t now_ms)
{
    if (!HasSafeVehicleState(state, now_ms)) {
        state->stable_safe_state = false;
        state->safe_since_ms = 0U;
        return;
    }

    if (!state->stable_safe_state) {
        state->stable_safe_state = true;
        state->safe_since_ms = now_ms;
    }
}

static uint16_t ReadLittleEndian16(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8U);
}

void TelOtaSafetyInit(TelOtaSafetyState *state)
{
    if (state != NULL) {
        memset(state, 0, sizeof(*state));
    }
}

void TelOtaSafetyObserveCanFrame(TelOtaSafetyState *state,
                                uint32_t can_id,
                                bool is_extended_id,
                                uint8_t dlc,
                                const uint8_t *data,
                                uint32_t now_ms)
{
    if ((state == NULL) || (data == NULL)) {
        return;
    }

    if (is_extended_id && (can_id == MDU_FRAME_0_ID)) {
        state->mdu_speed_received = dlc == 8U;
        if (state->mdu_speed_received) {
            /* DBC: MotorRotatingSpeed, Intel start bit 35, length 12. */
            state->motor_speed_raw =
                (uint16_t)(((uint16_t)data[4] >> 3U) |
                           ((uint16_t)(data[5] & 0x7FU) << 5U));
            state->mdu_speed_timestamp_ms = now_ms;
        }
        UpdateStableState(state, now_ms);
        return;
    }

    if (is_extended_id && (can_id == MDU_FRAME_1_ID)) {
        state->mdu_state_received = dlc == 5U;
        if (state->mdu_state_received) {
            /* DBC Intel signals: AcceleratorPosition 2|10,
             * OutputTargetValue 26|10, DriveActionStatus 36|2. */
            state->accelerator_position_raw =
                (uint16_t)((((uint16_t)data[0] >> 2U) |
                            ((uint16_t)data[1] << 6U)) & 0x03FFU);
            state->output_target_raw =
                (uint16_t)((((uint16_t)data[3] >> 2U) |
                            ((uint16_t)data[4] << 6U)) & 0x03FFU);
            state->drive_action_status = (uint8_t)((data[4] >> 4U) & 0x03U);
            state->mdu_state_timestamp_ms = now_ms;
        }
        UpdateStableState(state, now_ms);
        return;
    }

    if (!is_extended_id && (can_id == DRD_MOTOR_COMMAND_ID)) {
        state->motor_command_received = dlc == 5U;
        if (state->motor_command_received) {
            state->throttle_dac = ReadLittleEndian16(&data[0]);
            state->regen_dac = ReadLittleEndian16(&data[2]);
            state->motor_command_timestamp_ms = now_ms;
        }
        UpdateStableState(state, now_ms);
        return;
    }

    if (!is_extended_id && (can_id == DRD_DIAGNOSTICS_ID)) {
        state->drd_diagnostics_received = dlc == 8U;
        if (state->drd_diagnostics_received) {
            state->brake_pressed = (data[4] & (1U << 0)) != 0U;
            state->motor_comm_fault = (data[4] & (1U << 5)) != 0U;
            state->drd_drive_state = data[5];
            state->drd_timeout_flags = data[6];
            state->drd_diagnostics_timestamp_ms = now_ms;
        }
        UpdateStableState(state, now_ms);
    }
}

bool TelOtaSafetyUpdateAllowed(TelOtaSafetyState *state, uint32_t now_ms)
{
    if (state == NULL) {
        return false;
    }

    UpdateStableState(state, now_ms);
    return state->stable_safe_state &&
           ((uint32_t)(now_ms - state->safe_since_ms) >=
            TEL_OTA_SAFETY_STABLE_MS);
}
