#include "mst_main.h"

#include <stdint.h>
#include <stdio.h>

#include "debug_io.h"
#include "main.h"

#include "mst_defs.h"
#include "mst_types.h"
#include "balancing.h"
#include "can_messages.h"
#include "emergency.h"
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



void Initialize() {
    UART_Init(&huart1);
    CAN_Init(&hcan);

    // Includes Slave_Init (SPI perhiperal initialization)
    Module_Init(&hspi2, slaves);
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
    RetrieveVoltageMeasurement(slaves, pack_modules);
    if (pack_state.balancing_active) {
        ResumeAllBalancing();
    }

    uint32_t temp_start_ms = HAL_GetTick();
    
    for (int mux_idx = 0; mux_idx < SLAVE_NUM_MODULES_PER_TEMP_VAL; mux_idx++) {
        SetTempMuxState(slaves, mux_idx);
        HAL_Delay(10); // Give the physical multiplexer chips time to settle
        RequestTemperatureMeasurement();
        RetrieveTemperatureMeasurement(slaves, pack_modules);
    }
    
    uint32_t temp_end_ms = HAL_GetTick();
    
    LOG_DEBUG("Voltage measurement: %lu ms, Temperature measurement: %lu ms, Total: %lu ms", 
              temp_start_ms - voltage_start_ms, 
              temp_end_ms - temp_start_ms, 
              temp_end_ms - voltage_start_ms);
}


void AnalyzeModuleData() {
    CheckForEmergency(pack_modules, &pack_faults, &pack_warnings);

    if (pack_faults.raw != 0) {
        LOG_ERROR("Pack fault bits were not zero");
        Fault();
    }
    
    if (pack_warnings.raw != 0) {
        LOG_INFO("Pack warnings present: 0x%X", pack_warnings.raw);
    }

    ComputePackStatistics(pack_modules, &pack_state);
    LOG_INFO("Pack stats - Total V: %lu mV, Avg T: %ld mC", pack_state.total_voltage_mV, pack_state.avg_temp_mC);
}

void DriveOutputs() {
    GPIO_Write(HLIM_EN_OUT_GPIO_Port, HLIM_EN_OUT_Pin, pack_state.hlim_enable);
    GPIO_Write(LLIM_EN_OUT_GPIO_Port, LLIM_EN_OUT_Pin, pack_state.llim_enable);
    
    LOG_INFO("Outputs driven - HLIM: %d, LLIM: %d", pack_state.hlim_enable, pack_state.llim_enable);

    uint32_t balancing_start_ms = HAL_GetTick();
    DoBalancing(&pack_state, pack_modules, slaves);
    uint32_t balancing_end_ms = HAL_GetTick();

    LOG_DEBUG("Balancing commands: %lu ms", balancing_end_ms - balancing_start_ms);
}


void SendCanMessages() {
    // CAN_SendMessage0x622();
    // CAN_SendMessage0x623();
    // CAN_SendMessage0x625();
    // CAN_SendMessage0x626();
    // CAN_SendMessage0x627();
    // CAN_SendMessage0x628();
    // CAN_SendMessage0x629();
    LOG_DEBUG("All CAN messages queued for transmission.");
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
    GPIO_Write(HLIM_EN_OUT_GPIO_Port, HLIM_EN_OUT_Pin, GPIO_PIN_SET);
    HAL_Delay(1000);
    GPIO_Write(HLIM_EN_OUT_GPIO_Port, HLIM_EN_OUT_Pin, GPIO_PIN_RESET);

    DEBUG_IO_PRINT("LLIM_EN signal HIGH. Other signals should be LOW.\r\n");
    UART_Transmit("LLIM_EN signal HIGH. Other signals should be LOW.\r\n");
    GPIO_Write(LLIM_EN_OUT_GPIO_Port, LLIM_EN_OUT_Pin, GPIO_PIN_SET);
    HAL_Delay(1000);
    GPIO_Write(LLIM_EN_OUT_GPIO_Port, LLIM_EN_OUT_Pin, GPIO_PIN_RESET);

    DEBUG_IO_PRINT("CONTACTOR_EN signal HIGH. Other signals should be LOW.\r\n");
    UART_Transmit("CONTACTOR_EN signal HIGH. Other signals should be LOW.\r\n");
    GPIO_Write(CONTACTOR_EN_OUT_GPIO_Port, CONTACTOR_EN_OUT_Pin, GPIO_PIN_SET);
    HAL_Delay(1000);
    GPIO_Write(CONTACTOR_EN_OUT_GPIO_Port, CONTACTOR_EN_OUT_Pin, GPIO_PIN_RESET);

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

#if (UNIT_TEST_SLAVE == RUN)
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
    static bool scrutineering_enabled = true;
    static int loop_count = 0;
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

    (void) reg_group_match;

    // Refer to the ADBMS1818 datasheet pages 65, 68, 69 for 
    // format and content of configuration register groups A and B
    uint8_t config_val_a[SLAVE_NUM_DEVICES][SLAVE_REG_SIZE_BYTES];
    uint8_t config_val_b[SLAVE_NUM_DEVICES][SLAVE_REG_SIZE_BYTES];

    for (int i = 0; i < SLAVE_NUM_DEVICES; i++) {
        config_val_a[i][0] = 0xF8 | (REFON << 2) | ADCOPT; // GPIO 1-5 pull-downs off, REFON, ADCOPT
        config_val_a[i][1] = (VUV & 0xFF); // VUV[7:0]
        config_val_a[i][2] = ((uint8_t) (VOV << 4)) | (((uint8_t) (VUV >> 8)) & 0x0F); // VOV[4:0] | VUV[11:8]
        config_val_a[i][3] = (VOV >> 4); // VOV[11:4]
        config_val_a[i][4] = 0x00; // Discharge off for cells 1 through 8
        config_val_a[i][5] = 0x00; // Discharge off for cells 9 through 12, Discharge timer disabled

        config_val_b[i][0] = 0x0F ^ (!scrutineering_enabled << 1); // Discharge off for cells 13 through 16, GPIO 6-9 = 1
        config_val_b[i][1] = 0x00; // FDRF = 0, PS = 0, Discharge off for cells 17 and 18
        config_val_b[i][2] = 0x00;
        config_val_b[i][3] = 0x00;
        config_val_b[i][4] = 0x00;
        config_val_b[i][5] = 0x00;
    }
    
    if (++loop_count % 4 == 0) {
        scrutineering_enabled = !scrutineering_enabled;
        GPIO_Write(FAULT_OUT_GPIO_Port, FAULT_OUT_Pin, scrutineering_enabled);
    }

    Slave_WriteRegisterGroup(CMD_WRCFGA, config_val_a); // Write to Config. Reg. Group A
    Slave_WriteRegisterGroup(CMD_WRCFGB, config_val_b); // Write to Config. Reg. Group B

    comm_status = Slave_ReadRegisterGroup(CMD_RDCOMM, test_data_rx);
    reg_group_match = DoesRegGroupMatch_(test_data, test_data_rx);
    
    GPIO_Write(LED_OUT_GPIO_Port, LED_OUT_Pin, reg_group_match);

    LOG_INFO("Reg group match: %d. Comm error: %d. Config B first byte: %02X", reg_group_match, comm_status.error, config_val_b[0][0]);
    HAL_Delay(500);
}

void Debug_SlaveTestBalanceCycle(void) {
    static bool balance_enabled = true;
    Slave_WakeUp();

    ResumeAllBalancing();
    Debug_DoBalancing(slaves, balance_enabled);

    if (balance_enabled) {
        // Refer to the ADBMS1818 datasheet pages 65, 68, 69 for 
        // format and content of configuration register groups A and B
        uint8_t config_val_a[SLAVE_NUM_DEVICES][SLAVE_REG_SIZE_BYTES];
        uint8_t config_val_b[SLAVE_NUM_DEVICES][SLAVE_REG_SIZE_BYTES];

        for (int i = 0; i < SLAVE_NUM_DEVICES; i++) {
            config_val_a[i][0] = 0xF8 | (REFON << 2) | ADCOPT; // GPIO 1-5 pull-downs off, REFON, ADCOPT
            config_val_a[i][1] = (VUV & 0xFF); // VUV[7:0]
            config_val_a[i][2] = ((uint8_t) (VOV << 4)) | (((uint8_t) (VUV >> 8)) & 0x0F); // VOV[4:0] | VUV[11:8]
            config_val_a[i][3] = (VOV >> 4); // VOV[11:4]
            config_val_a[i][4] = 0xFF; // Discharge off for cells 1 through 8
            config_val_a[i][5] = 0x0F; // Discharge off for cells 9 through 12, Discharge timer disabled

            config_val_b[i][0] = 0xFF; // Discharge off for cells 13 through 16, GPIO 6-9 = 1
            config_val_b[i][1] = 0x00; // FDRF = 0, PS = 0, Discharge off for cells 17 and 18
            config_val_b[i][2] = 0x00;
            config_val_b[i][3] = 0x00;
            config_val_b[i][4] = 0x00;
            config_val_b[i][5] = 0x00;
        }

        Slave_WriteRegisterGroup(CMD_WRCFGA, config_val_a); // Write to Config. Reg. Group A
        Slave_WriteRegisterGroup(CMD_WRCFGB, config_val_b); // Write to Config. Reg. Group B
    }
    else {
        // Refer to the ADBMS1818 datasheet pages 65, 68, 69 for 
        // format and content of configuration register groups A and B
        uint8_t config_val_a[SLAVE_NUM_DEVICES][SLAVE_REG_SIZE_BYTES];
        uint8_t config_val_b[SLAVE_NUM_DEVICES][SLAVE_REG_SIZE_BYTES];

        for (int i = 0; i < SLAVE_NUM_DEVICES; i++) {
            config_val_a[i][0] = 0xF8 | (REFON << 2) | ADCOPT; // GPIO 1-5 pull-downs off, REFON, ADCOPT
            config_val_a[i][1] = (VUV & 0xFF); // VUV[7:0]
            config_val_a[i][2] = ((uint8_t) (VOV << 4)) | (((uint8_t) (VUV >> 8)) & 0x0F); // VOV[4:0] | VUV[11:8]
            config_val_a[i][3] = (VOV >> 4); // VOV[11:4]
            config_val_a[i][4] = 0x00; // Discharge off for cells 1 through 8
            config_val_a[i][5] = 0x00; // Discharge off for cells 9 through 12, Discharge timer disabled

            config_val_b[i][0] = 0x0F; // Discharge off for cells 13 through 16, GPIO 6-9 = 1
            config_val_b[i][1] = 0x00; // FDRF = 0, PS = 0, Discharge off for cells 17 and 18
            config_val_b[i][2] = 0x00;
            config_val_b[i][3] = 0x00;
            config_val_b[i][4] = 0x00;
            config_val_b[i][5] = 0x00;
        }
        
        Slave_WriteRegisterGroup(CMD_WRCFGA, config_val_a); // Write to Config. Reg. Group A
        Slave_WriteRegisterGroup(CMD_WRCFGB, config_val_b); // Write to Config. Reg. Group B
    }
    LOG_INFO("Turned all balancing pins %s", balance_enabled ? "ON" : "OFF");
    balance_enabled = !balance_enabled;


    HAL_Delay(500);
}

void Debug_SlaveTestMuxCycle(void) {
    static unsigned current_mux_state = 0;

    LOG_INFO("Set temp mux state to %u (SEL2 = %d, SEL1 = %d)", current_mux_state, (current_mux_state & 0x02) ? 1 : 0, (current_mux_state & 0x01) ? 1 : 0);


    int wait_time_ms = 10000;
    for (int i = 0; i < wait_time_ms / 100; i++) {
        SetTempMuxState(slaves, current_mux_state);
        HAL_Delay(100);
    }

    current_mux_state = (current_mux_state + 1) % 4;
}
#endif