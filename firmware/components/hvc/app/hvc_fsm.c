/**
 * @file hvc_fsm.c
 * @brief HVC FSM core — init, dispatch, helpers, and interrupt callbacks
 */

#include "hvc_fsm_private.h"

/*============================================================================*/
/* STATE TABLE */

static void (*const HVC_state_table[])(void) = {
    HVC_State_Reset,
    HVC_State_HVConnect,
    HVC_State_MotorPrecharge,
    HVC_State_CloseMotorBus,
    HVC_State_MpptPrecharge,
    HVC_State_CloseMpptBus,
    HVC_State_Monitoring,
    HVC_State_Fault,
};

/*============================================================================*/
/* SHARED VARIABLES */

volatile HVC_State_t hvc_state;
HVC_Ticks_t ticks;
bool startup_complete = false;

/*============================================================================*/
/* PUBLIC API */

/**
 * @brief Initializes the FSM. Transitions to FAULT on watchdog-triggered reset.
 */
void HVC_FSM_Init(void)
{
    if (RCC->CSR & RCC_CSR_IWDGRSTF) {
        __HAL_RCC_CLEAR_RESET_FLAGS();
        DEBUG_IO_print("HVC: watchdog reset\r\n");
        hvc_state = HVC_STATE_FAULT;
    } else {
        hvc_state = HVC_STATE_RESET;
    }
}

/**
 * @brief Dispatches to the current state handler. Call from HVC_Main().
 */
void HVC_FSM_Run(void)
{
    HVC_state_table[hvc_state]();
}

/*============================================================================*/
/* INTERRUPT CALLBACKS */

void HVC_ESTOPCallback(void)
{
    GPIO_Write(ESTOP_LED_GPIO_Port, ESTOP_LED_Pin, GPIO_PIN_SET);
    // TODO: set ESTOP flag in CAN data struct
    hvc_state = HVC_STATE_FAULT;
    HVC_FSM_Run();
}

void HVC_IMDFaultCallback(void)
{
    // TODO: set IMD fault flag in CAN data struct
    hvc_state = HVC_STATE_FAULT;
    HVC_FSM_Run();
}

void HVC_MasterboardFaultCallback(void)
{
    // TODO: set masterboard fault flag in CAN data struct
    hvc_state = HVC_STATE_FAULT;
    HVC_FSM_Run();
}

void HVC_HVCurrentAlertCallback(void)
{
    // TODO: read INA228 DIAG_ALRT register to distinguish over/under current
    hvc_state = HVC_STATE_FAULT;
    HVC_FSM_Run();
}

/*============================================================================*/
/* HELPERS */

bool timer_elapsed(uint32_t interval, uint32_t *last_tick)
{
    if (HAL_GetTick() - *last_tick >= interval) {
        *last_tick = HAL_GetTick();
        return true;
    }
    return false;
}

void open_all_contactors(void)
{
    GPIO_Write(NEG_CTRL_GPIO_Port,      NEG_CTRL_Pin,      HVC_CONTACTOR_OPEN);
    GPIO_Write(POS_CTRL_GPIO_Port,      POS_CTRL_Pin,      HVC_CONTACTOR_OPEN);
    GPIO_Write(LLIM_CTRL_GPIO_Port,     LLIM_CTRL_Pin,     HVC_CONTACTOR_OPEN);
    GPIO_Write(HLIM_CTRL_GPIO_Port,     HLIM_CTRL_Pin,     HVC_CONTACTOR_OPEN);
    GPIO_Write(MOTOR_PC_CTRL_GPIO_Port, MOTOR_PC_CTRL_Pin, HVC_CONTACTOR_OPEN);
    GPIO_Write(MPPT_PC_CTRL_GPIO_Port,  MPPT_PC_CTRL_Pin,  HVC_CONTACTOR_OPEN);
}

void check_supp_voltage(void)
{
    // TODO: convert ADC supp_sense to mV and drive SUPP_LOW_LED
    // ADC_Voltages adc = ADC_GetVoltages();
    // GPIO_PinState state = (adc.supp_sense < HVC_SUPP_LOW_THRESHOLD_MV)
    //                       ? GPIO_PIN_SET : GPIO_PIN_RESET;
    // GPIO_Write(SUPP_LOW_LED_GPIO_Port, SUPP_LOW_LED_Pin, state);
}
