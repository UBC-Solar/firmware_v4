/**
 * @file dist_main.c
 * @brief DIST entry points for cube main.c: init and one pass of the main loop.
 *
 * The top of the DIST code: cube main.c only calls these two.
 * DIST_Init() starts every driver and the FSM.
 * DIST_Main() reads this pass's inputs and runs the FSM.
 * The DIST's own logic lives in fsm.c.
 */

#include "dist_main.h"

// Standard library
#include <stdbool.h>

// Drivers
#include "adc_driver.h"
#include "can_driver.h"
#include "gpio_driver.h"
#include "i2c_driver.h"

// Runtime
#include "adc_runtime.h"
#include "gpio_runtime.h"
#include "led_runtime.h"

// DIST app
#include "can_messages.h"
#include "debug_output.h"
#include "fsm.h"

// Called from cube main.c: DIST_Init() once, DIST_Main() every pass.

/**
 * @brief Initializes every DIST peripheral and the FSM. Call once from main.c before DIST_Main().
 *
 * In order: starts the I2C bus and looks for the IS31FL3236 LED panel on it, calibrates the ADC,
 * turns on the eFuse FAULT pin interrupts (ignored until the FSM arms them), starts CAN with RX
 * filters for every ID in CAN_RX_IDS (can_messages.c), sets the starting FSM state, then prints the
 * startup lines (DEBUG_Output_Init(), debug_output.c).
 *
 * The LED panel is optional: if it is not found, the DIST runs without it.
 *
 * Each parameter is named after its peripheral, so it matches the CubeMX handle main.c passes.
 *
 * @param adc1     ADC1 (hadc1), for the eFuse and TEL current sense inputs.
 * @param led_i2c1 I2C1 (hi2c1), the bus to the IS31FL3236 LED panel.
 * @param can      CAN (hcan), the only CAN peripheral.
 */
void DIST_Init(ADC_HandleTypeDef *adc1, I2C_HandleTypeDef *led_i2c1, CAN_HandleTypeDef *can) {
    bool led_panel_found;

    // Drivers: start the hardware
    I2C_Init(led_i2c1);
    ADC_Init(adc1);
    GPIO_InitEfuseFaultInterrupts();
    CAN_InitFilterList(can, CAN_RX_IDS, CAN_RX_ID_COUNT);
    CAN_Init(can);

    // Devices on the drivers: LED panel
    led_panel_found = LED_Init();

    // FSM: starting state
    FSM_Init();

    // Debug output: startup lines
    DEBUG_Output_Init(led_panel_found);
}

/**
 * @brief Runs one pass of the main loop.
 *
 * Terms used across the DIST code:
 *   main loop:      the while (1) in cube main.c.
 *   pass:           one time around the main loop. DIST_Main() is called once per pass.
 *   main-loop code: code run from the main loop, as opposed to from an interrupt.
 *
 * Kept as its own function so per-pass work is added here, not in cube main.c: CubeMX deletes
 * anything outside the USER CODE sections when it regenerates main.c.
 *
 * In order: converts the ADC once into g_shared.adc and the input pins once into g_shared.pins, so
 * the FSM, CAN, and debug output use the same values all pass; then the FSM runs.
 */
void DIST_Main(void) {
    ADC_Runtime_ProcessReadings();
    g_shared.adc = ADC_Runtime_GetValues(); // Read once, so the FSM and CAN use the same values
    g_shared.pins = GPIO_Runtime_Read();    // Read once, so the FSM uses the same levels all pass
    FSM_Run();
}
