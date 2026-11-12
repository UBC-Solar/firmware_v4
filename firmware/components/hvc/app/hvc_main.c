/**
 * @file hvc_main.c
 * @brief HVC entry points for cube main.c: init and one pass of the main loop.
 *
 * The top of the HVC code: cube main.c only calls these two.
 * HVC_Init() starts every driver and the FSM.
 * HVC_Main() reads this pass's inputs and runs the FSM, or the hardware test HVC_TEST picks.
 * The HVC's own logic lives in fsm.c.
 */

#include "hvc_main.h"

// Standard library
#include <stddef.h>
#include <stdint.h>

// Drivers
#include "adc_driver.h"
#include "can_driver.h"
#include "i2c_driver.h"
#include "uart_driver.h"

// Runtime
#include "adc_runtime.h"
#include "gpio_runtime.h"
#include "shunt_runtime.h"

// HVC app
#include "can_messages.h"
#include "debug_output.h"
#include "fsm.h"
#include "hvc_defs.h"
#include "tests.h"

// Called from cube main.c: HVC_Init() once, HVC_Main() every pass.

/**
 * @brief Initializes every HVC peripheral and the FSM. Call once from main.c before HVC_Main().
 *
 * In order: starts the drivers (UART2, ADC, I2C, fan PWM, CAN with RX filters), sets up the shunt
 * monitor and the thermistor lookup table, sets the starting FSM state, then prints the startup
 * lines (DEBUG_Output_Init(), debug_output.c).
 * The startup fault checks run in the FSM's first state (Reset()).
 *
 * The RX filters let in every ID in CAN_RX_IDS (can_messages.c).
 * For each board part faked in the bench overrides, also blocks that part of the real board
 * (tests.c): TESTS_BenchRemoveFakedCanIds() drops its CAN IDs from the RX filters, and
 * TESTS_BenchBlockFakedPinInterrupts() turns off its pin interrupts.
 *
 * Each parameter is named after its peripheral, so it matches the CubeMX handle main.c passes.
 *
 * @param uart2            UART2 (huart2). Set up, but nothing sends on it yet: debug prints
 *                         go over SWO.
 * @param adc1             ADC1 (hadc1), for the analog sense inputs.
 * @param adc_trigger_tim3 TIM3 (htim3), the timer that triggers the ADC conversions.
 * @param fan_pwm_tim4     TIM4 (htim4), the fan PWM timer (channel 3).
 * @param shunt_i2c1       I2C1 (hi2c1), the bus to the shunt monitor.
 * @param can              CAN (hcan), the only CAN peripheral.
 */
void HVC_Init(
    UART_HandleTypeDef *uart2,
    ADC_HandleTypeDef *adc1,
    TIM_HandleTypeDef *adc_trigger_tim3,
    TIM_HandleTypeDef *fan_pwm_tim4,
    I2C_HandleTypeDef *shunt_i2c1,
    CAN_HandleTypeDef *can) {
    // Drivers: start the hardware
    UART_Init(uart2);
    ADC_Init(adc1, adc_trigger_tim3);
    I2C_Init(shunt_i2c1);
    HAL_TIM_PWM_Start(fan_pwm_tim4, TIM_CHANNEL_3);

    // RX filters: every ID can_messages handles (CAN_RX_IDS), minus the IDs of faked boards (bench
    // overrides), so a connected real board's messages are not received
    uint16_t filter_ids[CAN_RX_ID_COUNT];
    size_t filter_count = TESTS_BenchRemoveFakedCanIds(CAN_RX_IDS, CAN_RX_ID_COUNT, filter_ids);
    CAN_InitFilterList(can, filter_ids, filter_count);
    CAN_Init(can);

    TESTS_BenchBlockFakedPinInterrupts(); // Bench overrides: turns off faked boards' pin interrupts

    // Devices on the drivers: shunt monitor chip and thermistor table
    SHUNT_Init();
    ADC_InitTemperatureReading();

    // FSM: starting state
    FSM_Init();

    // Debug output: startup lines, after FSM_Init() records a watchdog reset
    DEBUG_Output_Init();
}

/**
 * @brief Runs one pass of the main loop.
 *
 * Terms used across the HVC code:
 *   main loop:      the while (1) in cube main.c.
 *   pass:           one time around the main loop. HVC_Main() is called once per pass.
 *   main-loop code: code run from the main loop, as opposed to from an interrupt.
 *
 * Kept as its own function so per-pass work is added here, not in cube main.c: CubeMX deletes
 * anything outside the USER CODE sections when it regenerates main.c.
 *
 * Runs the FSM, or the test HVC_TEST picks in hvc_defs.h instead (TESTS_RunSelected(), tests.c).
 * With the FSM, in order: converts the ADC once into g_shared.adc and the input pins once into
 * g_shared.pins, so the FSM and CAN use the same values all pass; TESTS_BenchFakeInputs() (tests.c)
 * writes the BENCH OVERRIDES' fake inputs (hvc_defs.h) over those values; then the FSM runs.
 *
 * The watchdog is refreshed in the main loop.
 * If a pass takes longer than the watchdog timeout, the watchdog resets the HVC, and FSM_Init()
 * starts it in STATE_FAULT.
 */
void HVC_Main(void) {
#if (HVC_TEST != TEST_NONE)
    TESTS_RunSelected(); // Runs the test HVC_TEST picks, instead of the FSM
#else // (HVC_TEST == TEST_NONE)
    ADC_Runtime_ProcessReadings();
    g_shared.adc = ADC_Runtime_GetValues(); // Read once, so the FSM and CAN use the same values
    g_shared.pins = GPIO_Runtime_Read(); // Read once, so the FSM uses the same levels all pass
    TESTS_BenchFakeInputs(); // Bench overrides replace this pass's inputs before the FSM runs
    FSM_Run();
#endif // (HVC_TEST != TEST_NONE)
}
