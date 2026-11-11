/**
 * @file fsm.c
 * @brief HVC FSM core: init and run, states, interrupt callbacks, and helpers
 *
 * The FSM takes the HVC from power-up to running. In state order, it: powers DIST and waits
 * for TEL, checks MST, starts the fans, connects the pack (NEG, POS), opens the discharge
 * relay, precharges the motor and MPPT sides, closes LLIM and HLIM, asks DIST to power up
 * the LV boards, then monitors. Any fault goes to STATE_FAULT, which opens every contactor
 * and stays there until the HVC is reset.
 *
 * Sections, same order as fsm.h:
 *   GLOBAL VARIABLES:    the HVC-wide variables (types in hvc_types.h).
 *   PUBLIC API:          FsmInit() and FsmRun(), called from hvc_main.c.
 *   STATES:              one function per FsmState, in state order.
 *   INTERRUPT CALLBACKS: called from the fault pin interrupts, in fault flag order.
 *   HELPERS:             small functions used by the states and FsmRun().
 */

#include "fsm.h"

// HAL and CubeMX
#include "stm32f1xx_hal.h"
#include "main.h" // Pin names
#include "tim.h"  // htim4 (fan PWM timer)

// Drivers
#include "gpio_driver.h"

// Runtime
#include "shunt_runtime.h"

// HVC app
#include "can_messages.h"
#include "debug_output.h"
#include "hvc_defs.h"

/*==============================================================================
 * === GLOBAL VARIABLES ========================================================
 * The one variable of each HVC-wide type (types in hvc_types.h). Any file that
 * includes fsm.h can use them. g_fault_flags is volatile because interrupts
 * write it. g_state stays volatile to be safe, but only the main loop writes it.
 * g_shared marks its interrupt-written fields volatile.
 *============================================================================*/

volatile FsmState   g_state;
volatile FaultFlags g_fault_flags;
SharedData          g_shared;
Ticks               g_ticks;

/*==============================================================================
 * === PUBLIC API ==============================================================
 * Called from hvc_main.c: FsmInit() once at startup, FsmRun() every pass.
 *============================================================================*/

/**
 * @brief Sets the starting FSM state. Call once from HVC_Init().
 *
 * If the last reset was caused by the watchdog, starts in STATE_FAULT with watchdog_fired set;
 * otherwise starts in STATE_RESET.
 *
 * Side effects: writes g_state; may set g_fault_flags.watchdog_fired; clears the reset-cause flags.
 */
void FsmInit(void)
{
    if (RCC->CSR & RCC_CSR_IWDGRSTF)
    {
        __HAL_RCC_CLEAR_RESET_FLAGS();
        g_fault_flags.watchdog_fired = true;
        g_state = STATE_FAULT;
    }
    else
    {
        g_state = STATE_RESET;
    }
}

/**
 * @brief Runs one pass of the FSM. Call from HVC_Main() every pass.
 *
 * In order: prints the debug output (DebugOutputRun(), debug_output.c), checks heartbeat
 * timeouts, switches to STATE_FAULT if any fault flag is set, sends periodic CAN messages,
 * updates the supp-low LED, then runs the current state's function.
 *
 * A fault reaches STATE_FAULT one of two ways. A state that finds a fault sets its flag and goes
 * to STATE_FAULT itself, in the same pass. An interrupt only sets a flag; the AnyFaultActive()
 * check here moves to STATE_FAULT on the next pass.
 *
 * Side effects: may set heartbeat timeout flags and change g_state.
 */
void FsmRun(void)
{
    DebugOutputRun(); // First, so it sees this pass's flags before anything clears them

    CheckHeartbeats();

    if (AnyFaultActive())
    {
        g_state = STATE_FAULT;
    }

    CanMessagesSendAll();
    CheckSuppVoltage();

    switch (g_state)
    {
        case STATE_RESET:
            Reset();
            break;
        case STATE_TEL_STARTUP:
            TelStartup();
            break;
        case STATE_MST_HANDSHAKE:
            MstHandshake();
            break;
        case STATE_MST_STATUS_CHECK:
            MstStatusCheck();
            break;
        case STATE_FANS_POWERUP:
            FansPowerup();
            break;
        case STATE_HV_CONNECT:
            HvConnect();
            break;
        case STATE_OPEN_DISCHARGE_RELAY:
            OpenDischargeRelay();
            break;
        case STATE_MOTOR_PRECHARGE:
            MotorPrecharge();
            break;
        case STATE_MPPT_PRECHARGE:
            MpptPrecharge();
            break;
        case STATE_CLOSE_LLIM:
            CloseLlim();
            break;
        case STATE_CLOSE_HLIM:
            CloseHlim();
            break;
        case STATE_LV_POWERUP:
            LvPowerup();
            break;
        case STATE_MONITORING:
            Monitoring();
            break;
        case STATE_FAULT:
        default:
            Fault();
            break;
    }
}

/*==============================================================================
 * === STATES ==================================================================
 * One function per FsmState, in state order. FsmRun() calls the current state's
 * function every pass. Each state's doc lists its exit conditions and the state
 * each one goes to. Every state change restarts g_ticks.state_timeout and
 * g_ticks.state_delay, so the next state's timers start from zero.
 *============================================================================*/

/**
 * @brief Checks the fault inputs already active at power-up, then puts every output in its safe
 *        starting position.
 *
 * Runs once, on the first pass. CheckStartupFaultPins() runs first, before any output is set,
 * and reads this pass's pins, after BenchFakeInputs(). Then opens all contactors, sets
 * DISCHARGE_TOGGLE_OFF low (the discharge relay stays closed, see OpenDischargeRelay()), turns
 * fans, MPPT, and DIST off, and sets IMD_CTRL.
 *
 * Exit Condition: no fault flag set (immediately).
 * Exit State: STATE_TEL_STARTUP
 *
 * Exit Condition: a fault flag set, by CheckStartupFaultPins() or before (immediately).
 * Exit State: STATE_FAULT
 */
void Reset(void)
{
    CheckStartupFaultPins();

    OpenAllContactors();
    GPIO_Write(DISCHARGE_TOGGLE_OFF_GPIO_Port, DISCHARGE_TOGGLE_OFF_Pin, GPIO_PIN_RESET);
    GPIO_Write(FAN_CTRL_GPIO_Port,             FAN_CTRL_Pin,             GPIO_PIN_RESET);
    GPIO_Write(MPPT_CTRL_GPIO_Port,            MPPT_CTRL_Pin,            GPIO_PIN_RESET);
    GPIO_Write(DIST_CTRL_GPIO_Port,            DIST_CTRL_Pin,            GPIO_PIN_RESET);
    GPIO_Write(IMD_CTRL_GPIO_Port,             IMD_CTRL_Pin,             GPIO_PIN_SET);

    g_ticks.state_timeout = HAL_GetTick();
    g_ticks.state_delay = HAL_GetTick();
    if (AnyFaultActive())
    {
        g_state = STATE_FAULT; // Straight to STATE_FAULT, without a pass in STATE_TEL_STARTUP
    }
    else
    {
        g_state = STATE_TEL_STARTUP;
    }
}

/**
 * @brief Powers DIST and waits for telemetry to come up.
 *
 * Waiting for TEL is enough: DIST powers TEL, so a TEL heartbeat means DIST is up too.
 *
 * Exit Condition: TEL heartbeat received. Enables the TEL and DIST heartbeat checks.
 * Exit State: STATE_MST_HANDSHAKE
 *
 * Exit Condition: TEL_STARTUP_TIMEOUT_MS elapsed.
 * Exit State: STATE_FAULT
 */
void TelStartup(void)
{
    static bool dist_powered = false;

    if (!dist_powered)
    {
        GPIO_Write(DIST_CTRL_GPIO_Port, DIST_CTRL_Pin, GPIO_PIN_SET);
        dist_powered = true;
        PRINT_STEP("DIST on, waiting for TEL heartbeat");
    }

    if (g_shared.tel_heartbeat_received)
    {
        g_shared.tel_heartbeat_received = false;
        dist_powered = false;
        g_shared.tel_dist_heartbeat_check_enabled = true;
        g_ticks.tel_heartbeat = HAL_GetTick(); // Heartbeat timeouts count from now
        g_ticks.dist_heartbeat = HAL_GetTick();
        g_ticks.state_timeout = HAL_GetTick();
        g_ticks.state_delay = HAL_GetTick();
        g_state = STATE_MST_HANDSHAKE;
        return;
    }

    if (TimerElapsed(TEL_STARTUP_TIMEOUT_MS, &g_ticks.state_timeout))
    {
        dist_powered = false;
        g_fault_flags.tel_startup_timeout = true;
        g_state = STATE_FAULT;
        return;
    }
}

/**
 * @brief MST handshake: waits for MST to pull MASTERBOARD_FAULT high, then low.
 *
 * MST holds the line high while it initializes, then pulls it low when it is done. Seeing both
 * proves MST started and finished. Once the line is low, arms the MASTERBOARD_FAULT interrupt
 * (mst_irq_armed) so a later high is treated as a real fault. MstStatusCheck() comes next.
 *
 * Exit Condition: MASTERBOARD_FAULT seen high, then low.
 * Exit State: STATE_MST_STATUS_CHECK
 *
 * Exit Condition: MST_HANDSHAKE_TIMEOUT_MS elapsed.
 * Exit State: STATE_FAULT
 */
void MstHandshake(void)
{
    static bool pin_went_high = false;

    if (!pin_went_high && (g_shared.pins.mst_fault == GPIO_PIN_SET))
    {
        pin_went_high = true;
        PRINT_STEP("MASTERBOARD_FAULT high (MST starting)");
    }

    if (pin_went_high && (g_shared.pins.mst_fault == GPIO_PIN_RESET))
    {
        g_shared.mst_irq_armed = true;
        PRINT_STEP("MASTERBOARD_FAULT low, MST fault check armed");
        pin_went_high = false;
        g_ticks.state_timeout = HAL_GetTick();
        g_ticks.state_delay = HAL_GetTick();
        g_state = STATE_MST_STATUS_CHECK;
        return;
    }

    if (TimerElapsed(MST_HANDSHAKE_TIMEOUT_MS, &g_ticks.state_timeout))
    {
        pin_went_high = false;
        g_fault_flags.mst_handshake_timeout = true;
        g_state = STATE_FAULT;
        return;
    }
}

/**
 * @brief Waits for MST's status message (its CAN heartbeat) to report a healthy pack.
 *
 * Proves MST's CAN works and its cells have no errors, faults, or warnings. Runs after
 * MstHandshake().
 *
 * Exit Condition: MST status reports healthy. Enables the MST heartbeat check.
 * Exit State: STATE_FANS_POWERUP
 *
 * Exit Condition: MST_STATUS_CHECK_TIMEOUT_MS elapsed.
 * Exit State: STATE_FAULT
 */
void MstStatusCheck(void)
{
    if (g_shared.mst_healthy)
    {
        PRINT_STEP("MST healthy, pack %ld mV", (long)g_shared.mst_pack_voltage_mv);
        g_shared.mst_heartbeat_check_enabled = true;
        g_ticks.mst_heartbeat = HAL_GetTick(); // Heartbeat timeout counts from now
        g_ticks.state_timeout = HAL_GetTick();
        g_ticks.state_delay = HAL_GetTick();
        g_state = STATE_FANS_POWERUP;
        return;
    }

    if (TimerElapsed(MST_STATUS_CHECK_TIMEOUT_MS, &g_ticks.state_timeout))
    {
        g_fault_flags.mst_status_check_timeout = true;
        g_state = STATE_FAULT;
        return;
    }
}

/**
 * @brief Turns on the pack fans before the HV sequence.
 *
 * Runs the fans at full speed for FANS_FULL_SPEED_DURATION_MS, then drops them to half speed.
 *
 * Exit Condition: FANS_FULL_SPEED_DURATION_MS elapsed.
 * Exit State: STATE_HV_CONNECT
 */
void FansPowerup(void)
{
    static bool full_speed_started = false;

    if (!full_speed_started)
    {
        GPIO_Write(FAN_CTRL_GPIO_Port, FAN_CTRL_Pin, GPIO_PIN_SET);
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, FANS_FULL_SPEED);
        full_speed_started = true;
        PRINT_STEP("fans at full speed");
    }

    if (TimerElapsed(FANS_FULL_SPEED_DURATION_MS, &g_ticks.state_delay))
    {
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, FANS_HALF_SPEED);
        PRINT_STEP("fans at half speed");
        full_speed_started = false;
        g_ticks.state_timeout = HAL_GetTick();
        g_ticks.state_delay = HAL_GetTick();
        g_state = STATE_HV_CONNECT;
        return;
    }
}

/**
 * @brief Closes the NEG then POS contactors, waiting CONTACTOR_SETTLE_MS before and after each.
 *
 * Once the contactors are closed, the power-path prioritizer switches the LV supply from the
 * supp battery to the DCDC. DCDC_ACTIVE and SUPP_ACTIVE are active-low: low = in use. On exit,
 * arms the DCDC_ACTIVE interrupt (dcdc_irq_armed) so a later high is treated as a real fault.
 *
 * Exit Condition: POS closed at least CONTACTOR_SETTLE_MS ago, DCDC_ACTIVE low and SUPP_ACTIVE
 *                 high.
 * Exit State: STATE_OPEN_DISCHARGE_RELAY
 *
 * Exit Condition: HV_CONNECT_TIMEOUT_MS elapsed.
 * Exit State: STATE_FAULT
 */
void HvConnect(void)
{
    static bool neg_closed = false;
    static bool pos_closed = false;

    // Each wait restarts g_ticks.state_delay when it ends, so the next wait starts from there
    if (!neg_closed && TimerElapsed(CONTACTOR_SETTLE_MS, &g_ticks.state_delay))
    {
        GPIO_Write(NEG_CTRL_GPIO_Port, NEG_CTRL_Pin, CONTACTOR_CLOSE);
        neg_closed = true;
        PRINT_STEP("NEG closed");
    }

    if (neg_closed && !pos_closed && TimerElapsed(CONTACTOR_SETTLE_MS, &g_ticks.state_delay))
    {
        GPIO_Write(POS_CTRL_GPIO_Port, POS_CTRL_Pin, CONTACTOR_CLOSE);
        pos_closed = true;
        PRINT_STEP("POS closed");
    }

    // TimerElapsed() last, so it only runs once the pins are right (see TimerElapsed())
    if (pos_closed &&
        (g_shared.pins.dcdc_active == GPIO_PIN_RESET) &&
        (g_shared.pins.supp_active == GPIO_PIN_SET) &&
        TimerElapsed(CONTACTOR_SETTLE_MS, &g_ticks.state_delay))
    {
        g_shared.dcdc_irq_armed = true;
        PRINT_STEP("DCDC took over from supp, DCDC fault check armed");
        neg_closed = false;
        pos_closed = false;
        g_ticks.state_timeout = HAL_GetTick();
        g_ticks.state_delay = HAL_GetTick();
        g_state = STATE_OPEN_DISCHARGE_RELAY;
        return;
    }

    if (TimerElapsed(HV_CONNECT_TIMEOUT_MS, &g_ticks.state_timeout))
    {
        neg_closed = false;
        pos_closed = false;
        g_fault_flags.hv_connect_timeout = true;
        g_state = STATE_FAULT;
        return;
    }
}

/**
 * @brief Opens the discharge relay once the motor side reads discharged.
 *
 * The discharge relay is latching: it stays where it was last put, and it is closed
 * mechanically when the car turns on or off, which discharges the motor side. A
 * DISCHARGE_RELAY_PULSE_MS pulse on DISCHARGE_TOGGLE_OFF opens it, so the pack can charge the
 * motor side in STATE_MOTOR_PRECHARGE. The pin then goes back low, so the relay coil is not left
 * powered.
 *
 * Waits for the motor-side voltage (g_shared.adc.motor_precharge_mv) to be below
 * DISCHARGE_COMPLETE_MV before the pulse. Once started, the pulse always finishes.
 *
 * Exit Condition: pulse finished.
 * Exit State: STATE_MOTOR_PRECHARGE
 *
 * Exit Condition: OPEN_DISCHARGE_RELAY_TIMEOUT_MS elapsed before the motor side read discharged.
 * Exit State: STATE_FAULT
 */
void OpenDischargeRelay(void)
{
    static bool pulse_started = false;

    if (!pulse_started &&
        ((int64_t)g_shared.adc.motor_precharge_mv < DISCHARGE_COMPLETE_MV))
    {
        GPIO_Write(DISCHARGE_TOGGLE_OFF_GPIO_Port, DISCHARGE_TOGGLE_OFF_Pin, GPIO_PIN_SET);
        pulse_started = true;
        g_ticks.state_delay = HAL_GetTick();
        PRINT_STEP("motor side %ld mV, pulsing discharge relay open",
                   (long)g_shared.adc.motor_precharge_mv);
    }

    if (pulse_started && TimerElapsed(DISCHARGE_RELAY_PULSE_MS, &g_ticks.state_delay))
    {
        GPIO_Write(DISCHARGE_TOGGLE_OFF_GPIO_Port, DISCHARGE_TOGGLE_OFF_Pin, GPIO_PIN_RESET);
        PRINT_STEP("discharge relay open");
        pulse_started = false;
        g_ticks.state_timeout = HAL_GetTick();
        g_ticks.state_delay = HAL_GetTick();
        g_state = STATE_MOTOR_PRECHARGE;
        return;
    }

    if (!pulse_started && TimerElapsed(OPEN_DISCHARGE_RELAY_TIMEOUT_MS, &g_ticks.state_timeout))
    {
        g_fault_flags.open_discharge_relay_timeout = true;
        g_state = STATE_FAULT;
        return;
    }
}

/**
 * @brief Closes the motor precharge contactor and waits for the motor side to charge.
 *
 * Exit Condition: motor precharge voltage >= PRECHARGE_PCT_OF_PACK % of the MST pack voltage,
 *                 and at least MOTOR_PRECHARGE_MIN_MS elapsed.
 * Exit State: STATE_MPPT_PRECHARGE
 *
 * Exit Condition: MOTOR_PRECHARGE_TIMEOUT_MS elapsed.
 * Exit State: STATE_FAULT
 */
void MotorPrecharge(void)
{
    static bool precharge_started = false;

    if (!precharge_started)
    {
        GPIO_Write(MOTOR_PC_CTRL_GPIO_Port, MOTOR_PC_CTRL_Pin, CONTACTOR_CLOSE);
        precharge_started = true;
        PRINT_STEP("motor precharge started (target %ld mV)",
                   (long)((int64_t)g_shared.mst_pack_voltage_mv * PRECHARGE_PCT_OF_PACK / 100));
    }

    // TimerElapsed() last, so it only runs once the voltage is reached (see TimerElapsed())
    if (((int64_t)g_shared.adc.motor_precharge_mv >=
         (int64_t)g_shared.mst_pack_voltage_mv * PRECHARGE_PCT_OF_PACK / 100) &&
        TimerElapsed(MOTOR_PRECHARGE_MIN_MS, &g_ticks.state_delay))
    {
        PRINT_STEP("motor precharge done (%ld mV)", (long)g_shared.adc.motor_precharge_mv);
        precharge_started = false;
        g_ticks.state_timeout = HAL_GetTick();
        g_ticks.state_delay = HAL_GetTick();
        g_state = STATE_MPPT_PRECHARGE;
        return;
    }

    if (TimerElapsed(MOTOR_PRECHARGE_TIMEOUT_MS, &g_ticks.state_timeout))
    {
        precharge_started = false;
        g_fault_flags.motor_precharge_timeout = true;
        g_state = STATE_FAULT;
        return;
    }
}

/**
 * @brief Closes the MPPT precharge contactor and waits for the MPPT side to charge.
 *
 * Exit Condition: MPPT precharge voltage >= PRECHARGE_PCT_OF_PACK % of the MST pack voltage,
 *                 and at least MPPT_PRECHARGE_MIN_MS elapsed.
 * Exit State: STATE_CLOSE_LLIM
 *
 * Exit Condition: MPPT_PRECHARGE_TIMEOUT_MS elapsed.
 * Exit State: STATE_FAULT
 */
void MpptPrecharge(void)
{
    static bool precharge_started = false;

    if (!precharge_started)
    {
        GPIO_Write(MPPT_PC_CTRL_GPIO_Port, MPPT_PC_CTRL_Pin, CONTACTOR_CLOSE);
        precharge_started = true;
        PRINT_STEP("MPPT precharge started (target %ld mV)",
                   (long)((int64_t)g_shared.mst_pack_voltage_mv * PRECHARGE_PCT_OF_PACK / 100));
    }

    // TimerElapsed() last, so it only runs once the voltage is reached (see TimerElapsed())
    if (((int64_t)g_shared.adc.mppt_precharge_mv >=
         (int64_t)g_shared.mst_pack_voltage_mv * PRECHARGE_PCT_OF_PACK / 100) &&
        TimerElapsed(MPPT_PRECHARGE_MIN_MS, &g_ticks.state_delay))
    {
        PRINT_STEP("MPPT precharge done (%ld mV)", (long)g_shared.adc.mppt_precharge_mv);
        precharge_started = false;
        g_ticks.state_timeout = HAL_GetTick();
        g_ticks.state_delay = HAL_GetTick();
        g_state = STATE_CLOSE_LLIM;
        return;
    }

    if (TimerElapsed(MPPT_PRECHARGE_TIMEOUT_MS, &g_ticks.state_timeout))
    {
        precharge_started = false;
        g_fault_flags.mppt_precharge_timeout = true;
        g_state = STATE_FAULT;
        return;
    }
}

/**
 * @brief Closes the LLIM contactor, then opens the motor precharge contactor.
 *
 * Exit Condition: CONTACTOR_SETTLE_MS elapsed after closing LLIM.
 * Exit State: STATE_CLOSE_HLIM
 */
void CloseLlim(void)
{
    GPIO_Write(LLIM_CTRL_GPIO_Port, LLIM_CTRL_Pin, CONTACTOR_CLOSE);

    if (TimerElapsed(CONTACTOR_SETTLE_MS, &g_ticks.state_delay))
    {
        GPIO_Write(MOTOR_PC_CTRL_GPIO_Port, MOTOR_PC_CTRL_Pin, CONTACTOR_OPEN);
        PRINT_STEP("LLIM closed, motor precharge open");
        g_ticks.state_timeout = HAL_GetTick();
        g_ticks.state_delay = HAL_GetTick();
        g_state = STATE_CLOSE_HLIM;
    }
}

/**
 * @brief Closes the HLIM contactor, then opens the MPPT precharge contactor and turns MPPT on.
 *
 * Exit Condition: CONTACTOR_SETTLE_MS elapsed after closing HLIM.
 * Exit State: STATE_LV_POWERUP
 */
void CloseHlim(void)
{
    GPIO_Write(HLIM_CTRL_GPIO_Port, HLIM_CTRL_Pin, CONTACTOR_CLOSE);

    if (TimerElapsed(CONTACTOR_SETTLE_MS, &g_ticks.state_delay))
    {
        GPIO_Write(MPPT_PC_CTRL_GPIO_Port, MPPT_PC_CTRL_Pin, CONTACTOR_OPEN);
        GPIO_Write(MPPT_CTRL_GPIO_Port,    MPPT_CTRL_Pin,    GPIO_PIN_SET);
        PRINT_STEP("HLIM closed, MPPT precharge open, MPPT on");
        g_ticks.state_timeout = HAL_GetTick();
        g_ticks.state_delay = HAL_GetTick();
        g_state = STATE_LV_POWERUP;
    }
}

/**
 * @brief Asks the rest of the LV systems to power up, now that the HV path is closed.
 *
 * Sends the LV power-up request every LV_POWERUP_RETRY_MS until it is acknowledged. DIST
 * answers with LV_POWERUP_ACK_ID once the boards it powers are on.
 *
 * Exit Condition: LV_POWERUP_ACK_ID received.
 * Exit State: STATE_MONITORING
 *
 * Exit Condition: LV_POWERUP_TIMEOUT_MS elapsed.
 * Exit State: STATE_FAULT
 */
void LvPowerup(void)
{
    if (TimerElapsed(LV_POWERUP_RETRY_MS, &g_ticks.lv_powerup_retry))
    {
        CanMessagesSendLvPowerupRequest();
    }

    if (g_shared.lv_powerup_received)
    {
        g_shared.lv_powerup_received = false;
        PRINT_STEP("LV power-up ack received");
        g_ticks.state_timeout = HAL_GetTick();
        g_ticks.state_delay = HAL_GetTick();
        g_state = STATE_MONITORING;
        return;
    }

    if (TimerElapsed(LV_POWERUP_TIMEOUT_MS, &g_ticks.state_timeout))
    {
        g_fault_flags.lv_powerup_timeout = true;
        g_state = STATE_FAULT;
        return;
    }
}

/**
 * @brief Normal driving state. Checks the DCDC temperature.
 *
 * Other faults (ESTOP, IMD, MST, DIST, DCDC, current, heartbeats) are caught by FsmRun().
 *
 * Exit Condition: DCDC temperature above DCDC_TEMP_MAX_MC.
 * Exit State: STATE_FAULT
 */
void Monitoring(void)
{
    if (g_shared.adc.dcdc_temp_mc > DCDC_TEMP_MAX_MC)
    {
        g_fault_flags.dcdc_overtemp = true;
        g_state = STATE_FAULT;
        return;
    }
}

/**
 * @brief Safe state. Opens all contactors, turns MPPT off, keeps DIST powered, blinks FAULT_LED.
 *
 * DIST stays powered so TEL stays on and can report the fault. The fans are left on to keep
 * cooling. Has no exit; the HVC stays here until it is reset.
 *
 * Side effects: disarms the DCDC_ACTIVE interrupt (dcdc_irq_armed).
 */
void Fault(void)
{
    g_shared.dcdc_irq_armed = false; // Opening the contactors drops the DCDC: not a DCDC fault
    OpenAllContactors();
    // Discharge relay coil off, in case STATE_FAULT came mid-pulse (see OpenDischargeRelay())
    GPIO_Write(DISCHARGE_TOGGLE_OFF_GPIO_Port, DISCHARGE_TOGGLE_OFF_Pin, GPIO_PIN_RESET);
    GPIO_Write(MPPT_CTRL_GPIO_Port, MPPT_CTRL_Pin, GPIO_PIN_RESET);
    GPIO_Write(DIST_CTRL_GPIO_Port, DIST_CTRL_Pin, GPIO_PIN_SET);

    if (TimerElapsed(FAULT_LED_BLINK_MS, &g_ticks.fault_led))
    {
        GPIO_Toggle(FAULT_LED_GPIO_Port, FAULT_LED_Pin);
    }
}

/*==============================================================================
 * === INTERRUPT CALLBACKS =====================================================
 * Called from the fault pin interrupts (HAL_GPIO_EXTI_Callback() in
 * gpio_driver.c) when a fault input goes high, in fault flag order. Each one only
 * sets its fault flag (EstopCallback() also turns on ESTOP_LED): FsmRun() sees
 * the flag and goes to STATE_FAULT on the next pass. Nothing prints here: the
 * fault is printed from the main loop (debug_output.c). MstFaultCallback() and
 * DcdcFaultCallback() do nothing until the FSM arms them.
 *============================================================================*/

/**
 * @brief Handles ESTOP going high: turns on ESTOP_LED and sets the estop fault.
 *
 * Called from the GPIO interrupt, and from CheckStartupFaultPins() if already high at startup.
 *
 * Side effects: sets g_fault_flags.estop.
 */
void EstopCallback(void)
{
    GPIO_Write(ESTOP_LED_GPIO_Port, ESTOP_LED_Pin, GPIO_PIN_SET);
    g_fault_flags.estop = true;
}

/**
 * @brief Handles the IMD fault input going high: sets the IMD fault.
 *
 * Called from the GPIO interrupt, and from CheckStartupFaultPins() if already high at startup.
 *
 * Side effects: sets g_fault_flags.imd_fault.
 */
void ImdFaultCallback(void)
{
    g_fault_flags.imd_fault = true;
}

/**
 * @brief Handles MASTERBOARD_FAULT going high: sets the MST fault if it is armed.
 *
 * MST also pulls this line high during the startup handshake, so the GPIO interrupt passes
 * g_shared.mst_irq_armed, which only becomes true in MstHandshake().
 *
 * Side effects: sets g_fault_flags.mst_fault.
 *
 * @param armed true to record the fault, false to ignore this edge.
 */
void MstFaultCallback(bool armed)
{
    if (armed)
    {
        g_fault_flags.mst_fault = true;
    }
}

/**
 * @brief Handles the shunt monitor alert going high: sets the overcurrent or reverse current fault.
 *
 * Reads the shunt monitor's alert register to tell which limit was crossed.
 * Called from the GPIO interrupt, and from CheckStartupFaultPins() if already high at startup.
 *
 * Side effects: sets g_fault_flags.overcurrent or g_fault_flags.reverse_current.
 */
void HvCurrentAlertCallback(void)
{
    if (Shunt_IsOvercurrent())
    {
        g_fault_flags.overcurrent = true;
    }
    else
    {
        g_fault_flags.reverse_current = true;
    }
}

/**
 * @brief Handles DCDC_ACTIVE going high: sets the DCDC fault if it is armed.
 *
 * DCDC_ACTIVE is active-low and only goes low once the contactors close in HvConnect(), so
 * the GPIO interrupt passes g_shared.dcdc_irq_armed, which only becomes true in HvConnect().
 *
 * Side effects: sets g_fault_flags.dcdc_fault.
 *
 * @param armed true to record the fault, false to ignore this edge.
 */
void DcdcFaultCallback(bool armed)
{
    if (armed)
    {
        g_fault_flags.dcdc_fault = true;
    }
}

/*==============================================================================
 * === HELPERS =================================================================
 * Small functions used by the states and FsmRun(). Grouped by topic: general
 * ones first (Timing, Faults), then in hvc_defs.h order (Heartbeats,
 * Supplemental battery, Contactors). Debug output helpers are in debug_output.c.
 *============================================================================*/

// --- Timing ---

/**
 * @brief Checks whether interval_ms have passed since *last_tick, and if so restarts the timer.
 *
 * In a condition with other checks, put it last: it restarts the timer when it returns true, so
 * it must only run once the other checks are already true.
 *
 * @param interval_ms Time to wait, in ms.
 * @param last_tick   Timestamp (HAL_GetTick()) the interval is measured from.
 *                    Set to now when it returns true.
 * @return true if the interval has elapsed.
 */
bool TimerElapsed(uint32_t interval_ms, uint32_t* last_tick)
{
    if (HAL_GetTick() - *last_tick >= interval_ms)
    {
        *last_tick = HAL_GetTick();
        return true;
    }
    return false;
}

// --- Faults ---

/**
 * @brief Checks whether any fault flag is set.
 *
 * Reads g_fault_flags as an array of bools, so new flags are checked automatically.
 *
 * @return true if at least one fault flag is set.
 */
bool AnyFaultActive(void)
{
    const volatile bool* flag = (const volatile bool*)&g_fault_flags;

    for (uint32_t i = 0U; i < sizeof(g_fault_flags); i++)
    {
        if (flag[i])
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief Sets the fault for each fault input that is already in its fault level at startup.
 *
 * Fault inputs only interrupt on a change, so one that is already high at power-up never
 * fires. Reads this pass's g_shared.pins. MASTERBOARD_FAULT is skipped: MST holds it high
 * during the startup handshake. DCDC_ACTIVE is skipped: it is high until HvConnect().
 * SUPP_ACTIVE (active-low) must be low: the supp battery powers the HVC until HvConnect().
 * Called once from Reset().
 *
 * Side effects: may set g_fault_flags.estop, imd_fault, overcurrent, reverse_current, or
 * supp_fault.
 */
void CheckStartupFaultPins(void)
{
    // TODO: IMD will get a startup handshake like MST. Then skip it here and arm its interrupt
    // like mst_irq_armed.
    if (g_shared.pins.estop == GPIO_PIN_SET)
    {
        EstopCallback();
    }
    if (g_shared.pins.imd_fault == GPIO_PIN_SET)
    {
        ImdFaultCallback();
    }
    if (g_shared.pins.hv_current_alert == GPIO_PIN_SET)
    {
        HvCurrentAlertCallback();
    }
    if (g_shared.pins.supp_active == GPIO_PIN_SET)
    {
        g_fault_flags.supp_fault = true;
    }
}

// --- Heartbeats ---

/**
 * @brief Sets a heartbeat timeout fault for each checked board whose heartbeat is late, and
 *        mst_unhealthy if the MST heartbeat reports a problem.
 *
 * Each board's heartbeat timer (g_ticks) starts when its check is enabled: TEL and DIST in
 * STATE_TEL_STARTUP, MST in STATE_MST_STATUS_CHECK. Each heartbeat received (its _received
 * flag, set by the CAN receive interrupt) restarts the timer. A heartbeat is late when
 * HEARTBEAT_TIMEOUT_MS passes with none received. STATE_MST_STATUS_CHECK only exits on a
 * healthy MST, so after it, a heartbeat reporting a problem is a fault.
 *
 * Side effects: clears the checked boards' _heartbeat_received flags, restarts their g_ticks
 * heartbeat timers, and may set g_fault_flags.tel_heartbeat_timeout, dist_heartbeat_timeout,
 * mst_heartbeat_timeout, or mst_unhealthy.
 */
void CheckHeartbeats(void)
{
    if (g_shared.tel_dist_heartbeat_check_enabled)
    {
        // A heartbeat received since the last pass restarts its timer
        if (g_shared.tel_heartbeat_received)
        {
            g_shared.tel_heartbeat_received = false;
            g_ticks.tel_heartbeat = HAL_GetTick();
        }
        if (g_shared.dist_heartbeat_received)
        {
            g_shared.dist_heartbeat_received = false;
            g_ticks.dist_heartbeat = HAL_GetTick();
        }

        if (TimerElapsed(HEARTBEAT_TIMEOUT_MS, &g_ticks.tel_heartbeat))
        {
            g_fault_flags.tel_heartbeat_timeout = true;
        }
        if (TimerElapsed(HEARTBEAT_TIMEOUT_MS, &g_ticks.dist_heartbeat))
        {
            g_fault_flags.dist_heartbeat_timeout = true;
        }
    }

    if (g_shared.mst_heartbeat_check_enabled)
    {
        // A heartbeat received since the last pass restarts its timer
        if (g_shared.mst_heartbeat_received)
        {
            g_shared.mst_heartbeat_received = false;
            g_ticks.mst_heartbeat = HAL_GetTick();
        }

        if (TimerElapsed(HEARTBEAT_TIMEOUT_MS, &g_ticks.mst_heartbeat))
        {
            g_fault_flags.mst_heartbeat_timeout = true;
        }
    }

    if (g_shared.mst_heartbeat_check_enabled && !g_shared.mst_healthy)
    {
        g_fault_flags.mst_unhealthy = true;
    }
}

// --- Supplemental battery ---

/**
 * @brief Turns SUPP_LOW_LED on when the supplemental battery is below SUPP_LOW_THRESHOLD_MV.
 */
void CheckSuppVoltage(void)
{
    uint32_t supp_mv = (uint32_t)g_shared.adc.supp_mv;

    GPIO_PinState state = (supp_mv < SUPP_LOW_THRESHOLD_MV) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    GPIO_Write(SUPP_LOW_LED_GPIO_Port, SUPP_LOW_LED_Pin, state);
}

// --- Contactors ---

/**
 * @brief Opens every contactor: NEG, POS, LLIM, HLIM, motor precharge, and MPPT precharge.
 *
 * Used by Reset(), Fault(), and Error_Handler() in main.c.
 */
void OpenAllContactors(void)
{
    GPIO_Write(NEG_CTRL_GPIO_Port,      NEG_CTRL_Pin,      CONTACTOR_OPEN);
    GPIO_Write(POS_CTRL_GPIO_Port,      POS_CTRL_Pin,      CONTACTOR_OPEN);
    GPIO_Write(LLIM_CTRL_GPIO_Port,     LLIM_CTRL_Pin,     CONTACTOR_OPEN);
    GPIO_Write(HLIM_CTRL_GPIO_Port,     HLIM_CTRL_Pin,     CONTACTOR_OPEN);
    GPIO_Write(MOTOR_PC_CTRL_GPIO_Port, MOTOR_PC_CTRL_Pin, CONTACTOR_OPEN);
    GPIO_Write(MPPT_PC_CTRL_GPIO_Port,  MPPT_PC_CTRL_Pin,  CONTACTOR_OPEN);
}
