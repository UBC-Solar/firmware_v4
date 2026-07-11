#include "mst_main.h"

#include <stdint.h>
#include <stdio.h>

#include "debug_io.h"
#include "main.h"

#include "mst_defs.h"
#include "mst_types.h"
#include "balancing.h"
#include "can_messages.h"
#include "analysis.h"
#include "module_data.h"
#include "spi.h"
#include "usart.h"

#include "can_driver.h"
#include "gpio_driver.h"
#include "spi_driver.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include "uart_driver.h"

module_t pack_modules[NUM_MODULES] = {0};
faults_t pack_faults = {0};
warnings_t pack_warnings = {0};
pack_state_t pack_state = {0};

slave_t slaves[SLAVE_NUM_DEVICES] = {0};


void Fault_() {
    GPIO_Write(FAULT_OUT_GPIO_Port, FAULT_OUT_Pin, GPIO_PIN_SET);
}


void IncrementCommError() {
    uint32_t current_time = HAL_GetTick();
    LOG_ERROR("SPI communication error!");
    if (current_time - pack_state.last_comm_fail_time >= CONSECUTIVE_TIMEFRAME_MS) {
        pack_state.num_consecutive_comm_fails = 0;
        pack_state.error_comm_fail = true;
        pack_state.last_comm_fail_time = HAL_GetTick();
    }

    pack_state.num_consecutive_comm_fails++;
    if (pack_state.num_consecutive_comm_fails >= NUM_CONSECUTIVE_COMM_ERR) {
        #if !ISOSPI_CONNECTED
        return;
        #endif
        ERROR_HANDLER_LOGGED();
    }
}


void Initialize() {
    // HVC expects MST to pull Fault pin HIGH during initialization
    GPIO_Write(FAULT_OUT_GPIO_Port, FAULT_OUT_Pin, GPIO_PIN_SET);

    // Contactors will conditionally be enabled again as mainloop executes
    GPIO_Write(HLIM_DIS_OUT_GPIO_Port, HLIM_DIS_OUT_Pin, GPIO_PIN_SET);
    GPIO_Write(LLIM_DIS_OUT_GPIO_Port, LLIM_DIS_OUT_Pin, GPIO_PIN_SET);
    GPIO_Write(CONTACTOR_DIS_OUT_GPIO_Port, CONTACTOR_DIS_OUT_Pin, GPIO_PIN_SET);

    UART_Init(&huart1);
    CAN_Init(&hcan);

    // Includes Slave_Init (SPI perhiperal initialization)
    Module_Init(&hspi2, slaves);

#if (SLAVEBOARD_REV == 1)
    SetScrutineeringMode(slaves, false);
#endif // (SLAVEBOARD_REV == 1)

    HAL_Delay(2000);
    GPIO_Write(FAULT_OUT_GPIO_Port, FAULT_OUT_Pin, GPIO_PIN_RESET);
    LOG_INFO("MST initialization complete.");
}


void CollectBoardData() {
    GPIO_PinState balancePinState = 
        GPIO_Read(BALANCE_EN_IN_GPIO_Port, BALANCE_EN_IN_Pin);
    pack_state.balancing_enable = balancePinState == GPIO_PIN_SET ? true : false;

    GPIO_PinState scrutineeringPinState = 
        GPIO_Read(SCRUTINEERING_EN_IN_GPIO_Port, SCRUTINEERING_EN_IN_Pin);
    pack_state.scrutineering_enable = scrutineeringPinState == GPIO_PIN_SET ? true : false;

    LOG_DEBUG("Balance enable: %d, Scrutineering mode: %d.", balancePinState, scrutineeringPinState);
}


void CollectModuleData() {
    uint32_t voltage_start_ms = HAL_GetTick();
    
    if (pack_state.balancing_active) {
        PauseAllBalancing();
    }
    RequestVoltageMeasurement();
    uint32_t voltage_measure_end_ms = HAL_GetTick();
    if (RetrieveVoltageMeasurement(slaves, pack_modules) != Slave_OK) {
        IncrementCommError();
    }
    if (pack_state.balancing_active) {
        ResumeAllBalancing();
    }
    

    uint32_t voltage_calc_end_ms = HAL_GetTick();
    
    #if TEMP_STRATEGY_ALL_AT_ONCE
    for (int mux_idx = 0; mux_idx < SLAVE_NUM_MODULES_PER_TEMP_VAL; mux_idx++) {
        SetTempMuxState(slaves, mux_idx);

        HAL_Delay(5);
        RequestTemperatureMeasurement();
        if (RetrieveTemperatureMeasurement(slaves, pack_modules) != Slave_OK) {
            IncrementCommError();
        }
    }
    #else // TEMP_STRATEGY_ALL_AT_ONCE is false
    RequestTemperatureMeasurement();
    if (RetrieveTemperatureMeasurement(slaves, pack_modules) != Slave_OK) {
        IncrementCommError();
    }
    SetTempMuxState(slaves, (slaves[0].temp_mux_state+1) % SLAVE_NUM_MODULES_PER_TEMP_VAL);
    #endif // TEMP_STRATEGY_ALL_AT_ONCE
    
    uint32_t temp_end_ms = HAL_GetTick();
    
    uint32_t voltage_measure_duration = voltage_measure_end_ms - voltage_start_ms;
    uint32_t voltage_calc_duration = voltage_calc_end_ms - voltage_measure_end_ms;

    uint32_t temp_duration = temp_end_ms - voltage_calc_end_ms;
    uint32_t total_duration = temp_end_ms - voltage_start_ms;
    
    LOG_DEBUG("Voltage measurement: %lu ms, Voltage calculation: %lu ms, Temperature measurement: %lu ms, Total: %lu ms", 
              voltage_measure_duration, voltage_calc_duration, temp_duration, total_duration);
}


void AnalyzeModuleData() {
    CheckForEmergency(pack_modules, &pack_faults, &pack_warnings);

    if (pack_faults.raw != 0) {
        LOG_ERROR("Pack fault bits were not zero");
        Fault_();
    }
    
    if (pack_warnings.raw != 0) {
        LOG_INFO("Pack warnings present: 0x%X", pack_warnings.raw);
    }

    ComputePackStatistics(pack_modules, &pack_state);
}


void DriveOutputs() {
    pack_state.hlim_enable = !pack_warnings.bits.warn_high_voltage;
    pack_state.llim_enable = !pack_warnings.bits.warn_low_voltage;
    pack_state.contactor_enable = !pack_faults.raw;
    GPIO_Write(HLIM_DIS_OUT_GPIO_Port, HLIM_DIS_OUT_Pin, !pack_state.hlim_enable);
    GPIO_Write(LLIM_DIS_OUT_GPIO_Port, LLIM_DIS_OUT_Pin, !pack_state.llim_enable);
    GPIO_Write(CONTACTOR_DIS_OUT_GPIO_Port, CONTACTOR_DIS_OUT_Pin, !pack_state.contactor_enable);
    bool debug_led_on = !pack_state.hlim_enable || !pack_state.llim_enable || !pack_state.contactor_enable;
    GPIO_Write(LED_OUT_GPIO_Port, LED_OUT_Pin, debug_led_on);
    
    LOG_INFO("Outputs driven - HLIM: %d, LLIM: %d", pack_state.hlim_enable, pack_state.llim_enable);

    uint32_t balancing_start_ms = HAL_GetTick();
    DoBalancing(&pack_state, pack_modules, slaves);
    uint32_t balancing_end_ms = HAL_GetTick();

    SetScrutineeringMode(slaves, pack_state.scrutineering_enable);

    uint32_t balancing_duration = balancing_end_ms - balancing_start_ms;
    LOG_DEBUG("Balancing commands: %lu ms", balancing_duration);
}


void SendCanMessages() {
    #if CAN_CONNECTED
    CAN_SendHeartbeatMessage();
    CAN_SendVoltageSummaryMessage();
    CAN_SendTempSummaryMessage();
    CAN_SendModuleVoltMessage();
    CAN_SendModuleTempMessage();
    CAN_SendModuleStatusMessage();
    CAN_SendBalanceStatusMessage();
    LOG_DEBUG("All CAN messages queued for transmission.");
    #endif
}


#if (UNIT_TEST_MCU == RUN)
void Debug_McuTestCycle(void) {
    DEBUG_IO_PRINT("Debug_McuTestCycle start (debug IO)\r\n");
    UART_Transmit("Debug_McuTestCycle start (UART_Transmit)\r\n");

    GPIO_Write(LED_OUT_GPIO_Port, LED_OUT_Pin, GPIO_PIN_SET);
    HAL_Delay(1000);

    DEBUG_IO_PRINT("Debug_McuTestCycle end (debug IO)\r\n");
    UART_Transmit("Debug_McuTestCycle end (UART_Transmit)\r\n");

    GPIO_Write(LED_OUT_GPIO_Port, LED_OUT_Pin, GPIO_PIN_RESET);
    HAL_Delay(1000);
}
#endif // UNIT_TEST_MCU

#if (UNIT_TEST_IO == RUN)
void Debug_DigitalIoTestCycle(void) {
    DEBUG_IO_PRINT("FAULT signal HIGH. Other signals should be LOW.\r\n");
    UART_Transmit("FAULT signal HIGH. Other signals should be LOW.\r\n");
    GPIO_Write(FAULT_OUT_GPIO_Port, FAULT_OUT_Pin, GPIO_PIN_SET);
    HAL_Delay(1000);
    GPIO_Write(FAULT_OUT_GPIO_Port, FAULT_OUT_Pin, GPIO_PIN_RESET);

    DEBUG_IO_PRINT("HLIM_EN signal HIGH. Other signals should be LOW.\r\n");
    UART_Transmit("HLIM_EN signal HIGH. Other signals should be LOW.\r\n");
    GPIO_Write(HLIM_DIS_OUT_GPIO_Port, HLIM_DIS_OUT_Pin, GPIO_PIN_SET);
    HAL_Delay(1000);
    GPIO_Write(HLIM_DIS_OUT_GPIO_Port, HLIM_DIS_OUT_Pin, GPIO_PIN_RESET);

    DEBUG_IO_PRINT("LLIM_EN signal HIGH. Other signals should be LOW.\r\n");
    UART_Transmit("LLIM_EN signal HIGH. Other signals should be LOW.\r\n");
    GPIO_Write(LLIM_DIS_OUT_GPIO_Port, LLIM_DIS_OUT_Pin, GPIO_PIN_SET);
    HAL_Delay(1000);
    GPIO_Write(LLIM_DIS_OUT_GPIO_Port, LLIM_DIS_OUT_Pin, GPIO_PIN_RESET);

    DEBUG_IO_PRINT("CONTACTOR_EN signal HIGH. Other signals should be LOW.\r\n");
    UART_Transmit("CONTACTOR_EN signal HIGH. Other signals should be LOW.\r\n");
    GPIO_Write(CONTACTOR_DIS_OUT_GPIO_Port, CONTACTOR_DIS_OUT_Pin, GPIO_PIN_SET);
    HAL_Delay(1000);
    GPIO_Write(CONTACTOR_DIS_OUT_GPIO_Port, CONTACTOR_DIS_OUT_Pin, GPIO_PIN_RESET);

    GPIO_Write(LED_OUT_GPIO_Port, LED_OUT_Pin, GPIO_PIN_SET);

}
#endif // UNIT_TEST_IO

#if (UNIT_TEST_CAN == RUN)
void Debug_CanTestCycle(void) {
    static int canCyclecCount = 1;
    DEBUG_IO_PRINT("Debug_CanTestCycle round %d (debug IO)\r\n", canCyclecCount);

    CAN_SendMessgeDebug();
    
    HAL_Delay(2000);

    canCyclecCount++;
}
#endif // UNIT_TEST_CAN

#if (UNIT_TEST_ISOSPI == RUN)
void Debug_IsoSpiTestCycle(void) {
    static int isoSpiCycleCount = 1;
    DEBUG_IO_PRINT("Debug_IsoSpiTestCycle round %d (debug IO)\r\n", isoSpiCycleCount);

    Slave_SendCmd(CMD_ADCV);

    HAL_Delay(2000);

    isoSpiCycleCount++;
}
#endif // UNIT_TEST_ISOSPI

#if (INT_TEST_SLAVE == RUN)
static bool DoesRegGroupMatch_(uint8_t reg_group1[SLAVE_NUM_DEVICES][SLAVE_REG_SIZE_BYTES],
                              uint8_t reg_group2[SLAVE_NUM_DEVICES][SLAVE_REG_SIZE_BYTES])
{
    for (int ic_num = 0; ic_num < SLAVE_NUM_DEVICES; ic_num++)
    {
        for (int i = 0; i < SLAVE_REG_SIZE_BYTES; i++)
        {
            if (reg_group1[ic_num][i] != reg_group2[ic_num][i])
                return false;
        }
    }
    return true;
}

void Debug_SlaveTestCommsCycle(void) {
    uint8_t test_data[SLAVE_NUM_DEVICES][SLAVE_REG_SIZE_BYTES] = {
        {0x55, 0x6E, 0x69, 0x42, 0x43, 0x20}
#if SLAVE_NUM_DEVICES > 1U
        , {0x53, 0x6F, 0x6C, 0x61, 0x72, 0x21}
#endif // SLAVE_NUM_DEVICES > 1
    };


    Slave_Status_t comm_status = {Slave_OK, 0};
    uint8_t test_data_rx[SLAVE_NUM_DEVICES][SLAVE_REG_SIZE_BYTES] = {0};

    bool reg_group_match;

    Slave_WakeUp();
    Slave_WriteRegisterGroup(CMD_WRCOMM, test_data);
    HAL_Delay(500);

    (void) reg_group_match;

    comm_status = Slave_ReadRegisterGroup(CMD_RDCOMM, test_data_rx);
    reg_group_match = DoesRegGroupMatch_(test_data, test_data_rx);
    
    GPIO_Write(LED_OUT_GPIO_Port, LED_OUT_Pin, reg_group_match);
    LOG_INFO("Reg group match: %d. Comm error: %d. Config B first byte: %02X", reg_group_match, comm_status.error, slaves[0].config_regs[0][0]);
    HAL_Delay(500);
}

#endif // (INT_TEST_SLAVE == RUN)
#if (INT_TEST_SLAVE_BAL_VOLT == RUN)
void Debug_SlaveTestBalancingVoltageDrop(void) {
    uint32_t previous_voltages[NUM_MODULES] = {0};
    uint32_t current_voltages[NUM_MODULES] = {0};
    pack_state.balancing_enable = true;

    Slave_WakeUp();

    RequestVoltageMeasurement();
    if (RetrieveVoltageMeasurement(slaves, pack_modules) != Slave_OK) {
        IncrementCommError();
    }

    for (int module_idx = 0; module_idx < NUM_MODULES; module_idx++) {
        previous_voltages[module_idx] = pack_modules[module_idx].voltage_mv;
    }

    ComputePackStatistics(pack_modules, &pack_state);

    /** Either: only enable one module's balancing */
    // pack_modules[12].voltage_mv = pack_state.min_voltage_mV + 300;
    // DoBalancing(&pack_state, pack_modules, slaves);

    /** Or: enable balancing for every module */
    Debug_DoBalancing(slaves, true);

    HAL_Delay(500);
    RequestVoltageMeasurement();
    if (RetrieveVoltageMeasurement(slaves, pack_modules) != Slave_OK) {
        IncrementCommError();
    }

    // 3 newlines to separate test data for each round of tests
    LOG_INFO("");
    LOG_INFO("");
    LOG_INFO("");

    for (int module_idx = 0; module_idx < NUM_MODULES; module_idx++) {
        current_voltages[module_idx] = pack_modules[module_idx].voltage_mv;
        int32_t voltage_delta_mv = (int32_t)current_voltages[module_idx] - (int32_t)previous_voltages[module_idx];
        LOG_INFO("Module %d voltage: %lu mV (bal off) -> %lu mV (bal on). Note: %ld mV diff",
                 module_idx,
                 previous_voltages[module_idx],
                 current_voltages[module_idx],
                 voltage_delta_mv);
    }

    Debug_DoBalancing(slaves, false);
    HAL_Delay(2000);
}
#endif // (INT_TEST_SLAVE_BAL_VOLT == RUN)


#if (INT_TEST_SLAVE_BAL_SCRUT == RUN)
void Debug_SlaveTestBalanceScrutCycle(void) {
    bool balance_enabled = pack_state.balancing_enable;
    bool scrutineering_enabled = pack_state.scrutineering_enable;
    Slave_WakeUp();

    ResumeAllBalancing();

    SetScrutineeringMode(slaves, scrutineering_enabled);
    Debug_DoBalancing(slaves, balance_enabled);

    LOG_INFO("Balancing pins %s, Scrutineering mode %s", balance_enabled ? "ON" : "OFF", scrutineering_enabled ? "ON" : "OFF");

    for (int i = 0; i < 5; i++) {
        HAL_Delay(500);
        PauseAllBalancing();
        HAL_Delay(500);
        ResumeAllBalancing();
    }
    HAL_Delay(500);
}
#endif // (INT_TEST_SLAVE_BAL_SCRUT == RUN)

#if (INT_TEST_SLAVE_MUX == RUN)
void Debug_SlaveTestMuxCycle(void) {
    static unsigned current_mux_state = 0;

    LOG_INFO("Set temp mux state to %u (SEL2 = %d, SEL1 = %d)", current_mux_state, (current_mux_state & 0x02) ? 1 : 0, (current_mux_state & 0x01) ? 1 : 0);


    int wait_time_ms = 10000;
    bool debug_led = true;
    for (int i = 0; i < wait_time_ms / 100; i++) {
        SetTempMuxState(slaves, current_mux_state);
        GPIO_Write(LED_OUT_GPIO_Port, LED_OUT_Pin, debug_led ? GPIO_PIN_SET : GPIO_PIN_RESET);
        debug_led = !debug_led;
        HAL_Delay(100);
    }

    current_mux_state = (current_mux_state + 1) % 4;
}
#endif // (INT_TEST_SLAVE_MUX == RUN)
