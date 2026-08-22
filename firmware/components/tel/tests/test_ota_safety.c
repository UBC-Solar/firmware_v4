#include "can_app.h"
#include "tel_ota_safety.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void ObserveMduSpeed(TelOtaSafetyState *state,
                            uint16_t speed_raw,
                            uint8_t dlc,
                            uint32_t now_ms)
{
    uint8_t data[8] = {0};
    data[4] = (uint8_t)((speed_raw & 0x1FU) << 3U);
    data[5] = (uint8_t)((speed_raw >> 5U) & 0x7FU);
    TelOtaSafetyObserveCanFrame(state,
                                MDU_FRAME_0_ID,
                                true,
                                dlc,
                                data,
                                now_ms);
}

static void ObserveMduState(TelOtaSafetyState *state,
                            uint8_t drive_action,
                            uint16_t accelerator_raw,
                            uint16_t output_target_raw,
                            uint32_t now_ms)
{
    uint8_t data[5] = {0};
    data[0] = (uint8_t)((accelerator_raw & 0x3FU) << 2U);
    data[1] = (uint8_t)((accelerator_raw >> 6U) & 0x0FU);
    data[3] = (uint8_t)((output_target_raw & 0x3FU) << 2U);
    data[4] = (uint8_t)(((output_target_raw >> 6U) & 0x0FU) |
                        ((drive_action & 0x03U) << 4U));
    TelOtaSafetyObserveCanFrame(state,
                                MDU_FRAME_1_ID,
                                true,
                                sizeof(data),
                                data,
                                now_ms);
}

static void ObserveMotorCommand(TelOtaSafetyState *state,
                                uint16_t throttle,
                                uint16_t regen,
                                uint32_t now_ms)
{
    uint8_t data[5] = {
        (uint8_t)throttle,
        (uint8_t)(throttle >> 8U),
        (uint8_t)regen,
        (uint8_t)(regen >> 8U),
        0U,
    };
    TelOtaSafetyObserveCanFrame(state,
                                DRD_MOTOR_COMMAND_ID,
                                false,
                                sizeof(data),
                                data,
                                now_ms);
}

static void ObserveDiagnostics(TelOtaSafetyState *state,
                               uint8_t drive_state,
                               bool brake_pressed,
                               bool motor_comm_fault,
                               uint8_t timeout_flags,
                               uint32_t now_ms)
{
    uint8_t data[8] = {0};
    if (brake_pressed) {
        data[4] |= 1U << 0;
    }
    if (motor_comm_fault) {
        data[4] |= 1U << 5;
    }
    data[5] = drive_state;
    data[6] = timeout_flags;
    TelOtaSafetyObserveCanFrame(state,
                                DRD_DIAGNOSTICS_ID,
                                false,
                                sizeof(data),
                                data,
                                now_ms);
}

static void ObserveSafeState(TelOtaSafetyState *state, uint32_t now_ms)
{
    ObserveMduSpeed(state, 0U, 8U, now_ms);
    ObserveMduState(state, 0U, 0U, 0U, now_ms);
    ObserveMotorCommand(state, 0U, 0U, now_ms);
    ObserveDiagnostics(state, 0x03U, true, false, 0U, now_ms);
}

int main(void)
{
    TelOtaSafetyState state;
    TelOtaSafetyInit(&state);

    assert(!TelOtaSafetyUpdateAllowed(&state, 0U));

    ObserveSafeState(&state, 100U);
    assert(!TelOtaSafetyUpdateAllowed(&state, 599U));
    assert(TelOtaSafetyUpdateAllowed(&state, 600U));

    ObserveMduSpeed(&state, 1U, 8U, 601U);
    assert(!TelOtaSafetyUpdateAllowed(&state, 601U));

    ObserveSafeState(&state, 700U);
    assert(!TelOtaSafetyUpdateAllowed(&state, 1199U));
    assert(TelOtaSafetyUpdateAllowed(&state, 1200U));

    ObserveMotorCommand(&state, 1U, 0U, 1201U);
    assert(!TelOtaSafetyUpdateAllowed(&state, 1201U));

    ObserveSafeState(&state, 1300U);
    ObserveDiagnostics(&state, 0x01U, true, false, 0U, 1301U);
    assert(!TelOtaSafetyUpdateAllowed(&state, 2000U));

    ObserveSafeState(&state, 2100U);
    ObserveDiagnostics(&state, 0x03U, false, false, 0U, 2101U);
    assert(!TelOtaSafetyUpdateAllowed(&state, 2700U));

    ObserveSafeState(&state, 2800U);
    ObserveDiagnostics(&state, 0x03U, true, true, 0U, 2801U);
    assert(!TelOtaSafetyUpdateAllowed(&state, 3400U));

    ObserveSafeState(&state, 3500U);
    ObserveDiagnostics(&state, 0x03U, true, false, 1U, 3501U);
    assert(!TelOtaSafetyUpdateAllowed(&state, 4100U));

    ObserveSafeState(&state, 4200U);
    assert(TelOtaSafetyUpdateAllowed(&state, 4700U));
    assert(!TelOtaSafetyUpdateAllowed(
        &state,
        4200U + TEL_OTA_SAFETY_FRESHNESS_MS + 1U));

    ObserveSafeState(&state, 6000U);
    assert(TelOtaSafetyUpdateAllowed(&state, 6500U));
    ObserveMduSpeed(&state, 0U, 7U, 6501U);
    assert(!TelOtaSafetyUpdateAllowed(&state, 6501U));

    puts("TEL OTA safety tests passed");
    return 0;
}
