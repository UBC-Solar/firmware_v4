/**
 * @file tests.c
 * @brief HVC hardware tests (run instead of the FSM) and bench override code (run with the FSM).
 *
 * Two separate features, each switched on in hvc_defs.h and explained in its banner below:
 *   HARDWARE TESTS:  small programs that replace the FSM to check one part (HVC_TEST).
 *   BENCH OVERRIDES: let the real FSM run on the bench by faking missing boards (FAKE_*,
 *                    IGNORE_*).
 * With everything off (as when flashing for real), nothing here changes what the HVC does.
 */

#include "tests.h"

// HAL and CubeMX
#include "main.h" // Pin names

// Drivers
#include "can_driver.h"
#include "gpio_driver.h"

// Runtime
#include "shunt_runtime.h"

// HVC app
#include "can_messages.h"
#include "debug_output.h"
#include "fsm.h"
#include "hvc_defs.h"

/*==============================================================================
 * === HARDWARE TESTS ==========================================================
 * Code for the HARDWARE TESTS (hvc_defs.h): small programs that run INSTEAD of
 * the FSM. HVC_TEST picks one, and TestRunSelected() runs it. Grouped like
 * hvc_defs.h: unit tests, then integration tests. Each test is named after its
 * TEST_* value, and only the picked test is compiled. Every function here is
 * listed in tests.h; only TestRunSelected() is called from outside
 * (HVC_Main()).
 *============================================================================*/

// --- Entry point ---

/**
 * @brief Runs the test HVC_TEST picks. Call from HVC_Main() every pass, in place of the FSM.
 *
 * Does nothing with TEST_NONE: HVC_Main() runs the FSM instead.
 */
void TestRunSelected(void)
{
    // To add a test: add its #elif branch, with its PrintTestStart() and its call
#if (HVC_TEST == TEST_UNIT_SHUNT)
    PrintTestStart("TEST_UNIT_SHUNT");
    TestUnitShunt();
#elif (HVC_TEST == TEST_INT_CAN)
    PrintTestStart("TEST_INT_CAN");
    TestIntCan();
#endif // HVC_TEST
}

// --- Helpers ---

/**
 * @brief Prints which hardware test is running, on the first call only.
 *
 * Called by TestRunSelected() every pass, so the line prints once, on the first pass.
 *
 * @param test_name The test's TEST_* name.
 */
void PrintTestStart(const char* test_name)
{
    static bool printed = false;

    if (!printed)
    {
        printed = true;
        PRINT_TEST("%s runs instead of the FSM", test_name);
    }
}

// --- Unit tests ---

#if (HVC_TEST == TEST_UNIT_SHUNT)
/**
 * @brief Shunt unit test (TEST_UNIT_SHUNT): reads the shunt voltage on a timer and prints it.
 *
 * The interval is TEST_UNIT_SHUNT_INTERVAL_MS. Prints the shunt voltage and current after each
 * read. Called by TestRunSelected().
 */
void TestUnitShunt(void)
{
    static uint32_t last_read_ms = 0;

    if (TimerElapsed(TEST_UNIT_SHUNT_INTERVAL_MS, &last_read_ms))
    {
        Shunt_Read_Shunt_Voltage();
        PRINT_TEST("shunt %ld nV, %ld mA", (long)Shunt_Get_Shunt_Voltage_nV(),
                   (long)Shunt_Get_Shunt_Current_mA());
    }
}
#endif // (HVC_TEST == TEST_UNIT_SHUNT)

// --- Integration tests ---

#if (HVC_TEST == TEST_INT_CAN)
/**
 * @brief Sends a fixed debug message for the CAN integration test (TEST_INT_CAN).
 *
 * Uses ID 0x42, the same debug ID as MST and DIST. Called by TestIntCan().
 */
void SendDebugCanMessage(void)
{
    CAN_TxMessage_t tx_message = {0};

    tx_message.tx_header.StdId = 0x42U;
    tx_message.tx_header.DLC = 8;

    tx_message.data[0] = 1;

    CAN_QueueTxMessage(&tx_message);
    PRINT_TEST("debug CAN message sent");
}

/**
 * @brief CAN integration test (TEST_INT_CAN): powers DIST and sends a debug CAN message on a
 *        timer.
 *
 * The interval is TEST_INT_CAN_INTERVAL_MS. Does not block, so the main loop keeps refreshing
 * the watchdog. Called by TestRunSelected().
 */
void TestIntCan(void)
{
    static uint32_t last_send_ms = 0;

    GPIO_Write(DIST_CTRL_GPIO_Port, DIST_CTRL_Pin, GPIO_PIN_SET);

    if (TimerElapsed(TEST_INT_CAN_INTERVAL_MS, &last_send_ms))
    {
        SendDebugCanMessage();
    }
}
#endif // (HVC_TEST == TEST_INT_CAN)

/*==============================================================================
 * === BENCH OVERRIDES =========================================================
 * Code for the BENCH OVERRIDES (hvc_defs.h). Runs with the FSM, and does
 * nothing during a hardware test. Only fakes inputs the FSM reads. Faking a
 * board part takes two steps:
 *   1. Block the real part, once at startup: its CAN IDs and pin interrupts
 *      arrive on their own, at any time.
 *   2. Write the fake into g_shared every pass, after the real reads.
 * Each board has up to 4 hooks, only the ones it needs, called by the entry
 * points:
 *   IsFaked<Board>CanId()           CAN IDs to block
 *   Block<Board>PinInterrupts()     pin interrupts to turn off
 *   Fake<Board>Inputs()             fake values to write this pass
 *   Print<Board>Overrides()         [BENCH] line for each override on, at startup
 * Grouped by board like hvc_defs.h: MST, TEL, DIST, HVC. Every function here
 * is listed in tests.h; only the entry points are called from outside
 * (HVC_Init(), HVC_Main(), and DebugOutputInit()).
 *============================================================================*/

// --- Entry points ---

/**
 * @brief Copies the CAN IDs that are not faked into kept_ids. Call once from HVC_Init(), on
 *        CAN_RX_IDS, before CAN_InitFilterList().
 *
 * The RX filters are then set from kept_ids, so a faked part's real messages are never received
 * and a connected real board does not mix with the fake. Keeps the order of the IDs. Removes
 * nothing during a hardware test.
 *
 * @param ids      CAN IDs to filter (not changed).
 * @param count    Number of IDs in ids.
 * @param kept_ids Gets the IDs that are not faked. Must have room for count IDs.
 * @return Number of IDs written to kept_ids.
 */
size_t BenchRemoveFakedCanIds(const uint16_t* ids, size_t count, uint16_t* kept_ids)
{
    size_t kept = 0U;

    for (size_t i = 0U; i < count; i++)
    {
        // Bench overrides do nothing during a hardware test
        bool faked = (HVC_TEST == TEST_NONE) &&
                     IsFakedMstCanId(ids[i]); // TODO: TEL, DIST

        if (!faked)
        {
            kept_ids[kept] = ids[i];
            kept++;
        }
    }
    return kept;
}

/**
 * @brief Turns off the real pin interrupts of faked or ignored board parts. Call once from
 *        HVC_Init().
 *
 * A pin interrupt sets its fault flag on its own, so a fake in g_shared.pins cannot stop it.
 * Turns off nothing during a hardware test.
 */
void BenchBlockFakedPinInterrupts(void)
{
    if (HVC_TEST == TEST_NONE) // Bench overrides do nothing during a hardware test
    {
        BlockMstPinInterrupts();
        // TODO: HVC (IGNORE_ESTOP, IGNORE_IMD, ...)
    }
}

/**
 * @brief Writes this pass's fake inputs for every board part faked. Call from HVC_Main() every
 *        pass, after the ADC and pin reads and before the FSM.
 *
 * The fakes replace the real values in g_shared, so the FSM reads the fakes.
 */
void BenchFakeInputs(void)
{
    FakeMstInputs();
    // TODO: FakeTelInputs(), FakeDistInputs()
    FakeHvcInputs();
}

/**
 * @brief Prints a [BENCH] line for every bench override that is on, or that none are. Call
 *        once from DebugOutputInit().
 *
 * Lets you check before flashing for real that every override is off. During a hardware test
 * only says that the overrides do nothing.
 */
void BenchPrintOverrides(void)
{
    if (HVC_TEST != TEST_NONE)
    {
        PRINT_BENCH("bench overrides do nothing during a hardware test");
        return;
    }

    bool any_on = false;
    any_on = PrintMstOverrides() || any_on; // Hook first, so every hook runs
    // TODO: PrintTelOverrides(), PrintDistOverrides()
    any_on = PrintHvcOverrides() || any_on;

    if (!any_on)
    {
        PRINT_BENCH("no bench overrides on");
    }
}

// --- Helpers ---

/**
 * @brief Time since start_tick, in ms. Unlike TimerElapsed(), does not restart anything.
 *
 * For checks that measure from one fixed start time, such as every fake MST part measuring
 * from mst_fake_start_ms: restarting that time would restart the whole fake MST.
 *
 * @param start_tick Timestamp (HAL_GetTick()) to measure from.
 * @return ms since start_tick.
 */
uint32_t TimeSince(uint32_t start_tick)
{
    return HAL_GetTick() - start_tick;
}

// --- MST ---
// Fakes:   FAKE_MST_FAULT_PIN, FAKE_MST_HEARTBEAT, FAKE_MST_PACK_VOLTAGE
// Ignores: IGNORE_MST_FAULT

// True if any part of MST is faked
#define FAKE_MST_ANY (FAKE_MST_FAULT_PIN || FAKE_MST_HEARTBEAT || FAKE_MST_PACK_VOLTAGE)

#if FAKE_MST_ANY
static bool     mst_fake_started  = false; // Set when the FSM first reaches STATE_MST_HANDSHAKE
static uint32_t mst_fake_start_ms = 0;     // When the fake MST started (its "power-up")
#endif // FAKE_MST_ANY

#if FAKE_MST_FAULT_PIN
/**
 * @brief Fake MASTERBOARD_FAULT pin level (FAKE_MST_FAULT_PIN).
 *
 * Reads high for FAKE_MST_INIT_HOLD_MS after the fake MST starts, then low. If
 * FAKE_MST_FAULT_AFTER_MS is not 0, reads high again that long after the hold ends.
 * Reads low before the fake MST starts, like an MST that is not powered yet.
 * Called by FakeMstInputs(), which writes it into g_shared.pins.mst_fault.
 *
 * @return GPIO_PIN_SET (high) or GPIO_PIN_RESET (low).
 */
GPIO_PinState FakeMstFaultPin(void)
{
    if (!mst_fake_started)
    {
        return GPIO_PIN_RESET;
    }

    uint32_t elapsed_ms = TimeSince(mst_fake_start_ms);

    if (elapsed_ms < FAKE_MST_INIT_HOLD_MS)
    {
        return GPIO_PIN_SET;
    }
#if (FAKE_MST_FAULT_AFTER_MS > 0)
    if (elapsed_ms >= FAKE_MST_INIT_HOLD_MS + FAKE_MST_FAULT_AFTER_MS)
    {
        return GPIO_PIN_SET;
    }
#endif // (FAKE_MST_FAULT_AFTER_MS > 0)
    return GPIO_PIN_RESET;
}

#if (FAKE_MST_FAULT_AFTER_MS > 0)
/**
 * @brief Makes the fault call once the fake pin goes high again (FAKE_MST_FAULT_AFTER_MS).
 *
 * Calls MstFaultCallback() once, the same call the GPIO interrupt makes for the real pin
 * (that interrupt is turned off by BlockMstPinInterrupts()). With IGNORE_MST_FAULT, only
 * prints. Called by FakeMstInputs().
 *
 * Side effects: may set g_fault_flags.mst_fault.
 */
void FakeMstFaultCall(void)
{
    static bool fault_sent = false;

    if (!fault_sent &&
        (TimeSince(mst_fake_start_ms) >= FAKE_MST_INIT_HOLD_MS + FAKE_MST_FAULT_AFTER_MS))
    {
        fault_sent = true;
#if IGNORE_MST_FAULT
        PRINT_BENCH("fake MST: fault pin high (ignored: IGNORE_MST_FAULT)");
#else // !IGNORE_MST_FAULT
        PRINT_BENCH("fake MST: fault pin high");
        MstFaultCallback(g_shared.mst_irq_armed);
#endif // IGNORE_MST_FAULT
    }
}
#endif // (FAKE_MST_FAULT_AFTER_MS > 0)
#endif // FAKE_MST_FAULT_PIN

#if FAKE_MST_HEARTBEAT
/**
 * @brief Passes a fake MST heartbeat to CanMessagesHandleRx() every FAKE_MST_HEARTBEAT_PERIOD_MS.
 *
 * Laid out like the real MST's heartbeat. Reports healthy, or a problem if FAKE_MST_UNHEALTHY.
 * Stops for good FAKE_MST_HEARTBEAT_STOP_AFTER_MS after the fake MST starts, if that is not 0.
 * Called by FakeMstInputs().
 *
 * Side effects: through CanMessagesHandleRx(), sets g_shared.mst_heartbeat_received and writes
 * g_shared.mst_healthy.
 */
void FakeMstHeartbeat(void)
{
    static uint32_t last_send_ms = 0;
    static uint32_t heartbeat_count = 0;

#if (FAKE_MST_HEARTBEAT_STOP_AFTER_MS > 0)
    static bool stop_printed = false;

    if (TimeSince(mst_fake_start_ms) >= FAKE_MST_HEARTBEAT_STOP_AFTER_MS)
    {
        if (!stop_printed)
        {
            stop_printed = true;
            PRINT_BENCH("fake MST: heartbeat stopped");
        }
        return;
    }
#endif // (FAKE_MST_HEARTBEAT_STOP_AFTER_MS > 0)

    if (!TimerElapsed(FAKE_MST_HEARTBEAT_PERIOD_MS, &last_send_ms))
    {
        return;
    }

    CAN_RxMessage_t heartbeat = {0};
    heartbeat.rx_header.StdId = MST_HEARTBEAT_ID;
    heartbeat.rx_header.IDE = CAN_ID_STD;
    heartbeat.rx_header.RTR = CAN_RTR_DATA;
    heartbeat.rx_header.DLC = 8;
    heartbeat.data[0] = (uint8_t)(heartbeat_count);         // Counter, little-endian
    heartbeat.data[1] = (uint8_t)(heartbeat_count >> 8);
    heartbeat.data[2] = (uint8_t)(heartbeat_count >> 16);
    heartbeat.data[3] = (uint8_t)(heartbeat_count >> 24);
    heartbeat.data[4] = FAKE_MST_UNHEALTHY ? 1U : 0U;       // Status: 0 = healthy
    heartbeat.data[5] = 0;                                  // Status: 0 = healthy
    heartbeat.timestamp = HAL_GetTick();
    CanMessagesHandleRx(&heartbeat);
    heartbeat_count++;
}
#endif // FAKE_MST_HEARTBEAT

#if FAKE_MST_PACK_VOLTAGE
/**
 * @brief Passes a fake MST voltage summary to CanMessagesHandleRx() every
 *        FAKE_MST_PACK_VOLTAGE_PERIOD_MS.
 *
 * Laid out like the real MST's voltage summary, with FAKE_MST_PACK_VOLTAGE_MV as the pack
 * voltage. Called by FakeMstInputs().
 *
 * Side effects: through CanMessagesHandleRx(), writes g_shared.mst_pack_voltage_mv.
 */
void FakeMstPackVoltage(void)
{
    static uint32_t last_send_ms = 0;

    if (!TimerElapsed(FAKE_MST_PACK_VOLTAGE_PERIOD_MS, &last_send_ms))
    {
        return;
    }

    uint32_t pack_mv = (uint32_t)FAKE_MST_PACK_VOLTAGE_MV;
    CAN_RxMessage_t volt_summary = {0};
    volt_summary.rx_header.StdId = MST_VOLT_SUMMARY_ID;
    volt_summary.rx_header.IDE = CAN_ID_STD;
    volt_summary.rx_header.RTR = CAN_RTR_DATA;
    volt_summary.rx_header.DLC = 6;
    volt_summary.data[0] = (uint8_t)(pack_mv);              // Pack voltage (mV), little-endian
    volt_summary.data[1] = (uint8_t)(pack_mv >> 8);
    volt_summary.data[2] = (uint8_t)(pack_mv >> 16);
    volt_summary.data[3] = (uint8_t)(pack_mv >> 24);
    volt_summary.data[4] = 1;                               // Min cell number (not used by HVC)
    volt_summary.data[5] = 1;                               // Max cell number (not used by HVC)
    volt_summary.timestamp = HAL_GetTick();
    CanMessagesHandleRx(&volt_summary);
}
#endif // FAKE_MST_PACK_VOLTAGE

/**
 * @brief MST hook: true if id is the CAN ID of a faked MST part.
 *
 * Called by BenchRemoveFakedCanIds().
 *
 * @param id CAN ID to check.
 * @return true if FAKE_MST_HEARTBEAT or FAKE_MST_PACK_VOLTAGE fakes this ID.
 */
bool IsFakedMstCanId(uint16_t id)
{
#if FAKE_MST_HEARTBEAT
    if (id == MST_HEARTBEAT_ID)
    {
        return true;
    }
#endif // FAKE_MST_HEARTBEAT
#if FAKE_MST_PACK_VOLTAGE
    if (id == MST_VOLT_SUMMARY_ID)
    {
        return true;
    }
#endif // FAKE_MST_PACK_VOLTAGE
    (void)id; // Not used when no MST CAN part is faked
    return false;
}

/**
 * @brief MST hook: turns off the real MASTERBOARD_FAULT interrupt when it is faked or ignored.
 *
 * FAKE_MST_FAULT_PIN: only the fake pin counts. IGNORE_MST_FAULT: a real MST fault never reaches
 * the HVC; the startup handshake still reads the real pin, only the fault is lost.
 * Called by BenchBlockFakedPinInterrupts().
 */
void BlockMstPinInterrupts(void)
{
#if FAKE_MST_FAULT_PIN || IGNORE_MST_FAULT
    CLEAR_BIT(EXTI->IMR, MASTERBOARD_FAULT_Pin); // EXTI line n is pin n
#endif // FAKE_MST_FAULT_PIN || IGNORE_MST_FAULT
}

/**
 * @brief MST hook: runs the fake MST parts switched on. Called by BenchFakeInputs().
 *
 * The fake MST starts when the FSM first reaches STATE_MST_HANDSHAKE, close to when the real MST
 * would power up. All fake MST times are measured from then.
 * FAKE_MST_FAULT_PIN: every pass, even before the start, writes the fake pin's level over the
 * real one in g_shared.pins.mst_fault, so the FSM reads the fake pin.
 * After the start, each pass:
 *   FAKE_MST_FAULT_PIN:    makes the fault call if the pin goes high again.
 *   FAKE_MST_HEARTBEAT:    sends the heartbeat when its period is up.
 *   FAKE_MST_PACK_VOLTAGE: sends the voltage summary when its period is up.
 * "Sends" means passed to CanMessagesHandleRx(): nothing goes on the real CAN bus.
 *
 * Side effects: may write g_shared.pins.mst_fault.
 */
void FakeMstInputs(void)
{
#if FAKE_MST_ANY
    if (!mst_fake_started && (g_state == STATE_MST_HANDSHAKE))
    {
        mst_fake_started = true;
        mst_fake_start_ms = HAL_GetTick();
        PRINT_BENCH("fake MST: started");
    }

#if FAKE_MST_FAULT_PIN
    g_shared.pins.mst_fault = FakeMstFaultPin(); // Replaces the real pin's level
#endif // FAKE_MST_FAULT_PIN

    if (!mst_fake_started)
    {
        return;
    }

#if FAKE_MST_FAULT_PIN && (FAKE_MST_FAULT_AFTER_MS > 0)
    FakeMstFaultCall();
#endif // FAKE_MST_FAULT_PIN && (FAKE_MST_FAULT_AFTER_MS > 0)
#if FAKE_MST_HEARTBEAT
    FakeMstHeartbeat();
#endif // FAKE_MST_HEARTBEAT
#if FAKE_MST_PACK_VOLTAGE
    FakeMstPackVoltage();
#endif // FAKE_MST_PACK_VOLTAGE
#endif // FAKE_MST_ANY
}

/**
 * @brief MST hook: prints a [BENCH] line, with its settings, for each MST override that is on.
 *        Called by BenchPrintOverrides().
 *
 * @return true if any MST override is on.
 */
bool PrintMstOverrides(void)
{
    bool any_on = false;

    if (FAKE_MST_FAULT_PIN)
    {
        any_on = true;
        PRINT_BENCH("FAKE_MST_FAULT_PIN on (high %d ms at start, fault %d ms later, 0 = never)",
                    (int)FAKE_MST_INIT_HOLD_MS, (int)FAKE_MST_FAULT_AFTER_MS);
    }
    if (FAKE_MST_HEARTBEAT)
    {
        any_on = true;
        PRINT_BENCH("FAKE_MST_HEARTBEAT on (%s, stops after %d ms, 0 = never)",
                    FAKE_MST_UNHEALTHY ? "unhealthy" : "healthy",
                    (int)FAKE_MST_HEARTBEAT_STOP_AFTER_MS);
    }
    if (FAKE_MST_PACK_VOLTAGE)
    {
        any_on = true;
        PRINT_BENCH("FAKE_MST_PACK_VOLTAGE on (%d mV)", (int)FAKE_MST_PACK_VOLTAGE_MV);
    }
    if (IGNORE_MST_FAULT)
    {
        any_on = true;
        PRINT_BENCH("IGNORE_MST_FAULT on");
    }
    return any_on;
}

// --- TEL ---
// TODO: FAKE_TEL_HEARTBEAT - fake CAN (heartbeat). Add IsFakedTelCanId(), FakeTelInputs(), and
// PrintTelOverrides(), then call them from the entry points.

// --- DIST ---
// TODO: FAKE_DIST_DATA - fake CAN (heartbeat + LV power-up reply). IGNORE_DIST_FAULT - block
// DIST_FAULT_ID. Add IsFakedDistCanId(), FakeDistInputs(), and PrintDistOverrides(), then call
// them from the entry points.

// --- HVC ---
// Fakes:   FAKE_PRECHARGE_VOLTAGE
// TODO: IGNORE_ESTOP, IGNORE_IMD, IGNORE_DCDC_FAULT, IGNORE_SUPP_FAULT, IGNORE_HV_CURRENT,
// IGNORE_DCDC_TEMP (see hvc_defs.h). The pin ones also need BlockHvcPinInterrupts().

#if FAKE_PRECHARGE_VOLTAGE
/**
 * @brief Fakes the precharge voltage as charged (FAKE_PRECHARGE_VOLTAGE).
 *
 * Only in STATE_MOTOR_PRECHARGE and STATE_MPPT_PRECHARGE: STATE_OPEN_DISCHARGE_RELAY needs the
 * real (low) motor voltage. Writes exactly the voltage the precharge check needs, so each
 * precharge state exits after its minimum time. CAN also sends this fake voltage, since it reads
 * g_shared.adc.
 * Called by FakeHvcInputs().
 *
 * Side effects: may write g_shared.adc.motor_precharge_mv or mppt_precharge_mv.
 */
void FakePrechargeVoltage(void)
{
    int32_t charged_mv =
        (int32_t)((int64_t)g_shared.mst_pack_voltage_mv * PRECHARGE_PCT_OF_PACK / 100);

    if (g_state == STATE_MOTOR_PRECHARGE)
    {
        g_shared.adc.motor_precharge_mv = charged_mv;
    }
    if (g_state == STATE_MPPT_PRECHARGE)
    {
        g_shared.adc.mppt_precharge_mv = charged_mv;
    }
}
#endif // FAKE_PRECHARGE_VOLTAGE

/**
 * @brief HVC hook: runs the fake HVC parts switched on. Called by BenchFakeInputs().
 */
void FakeHvcInputs(void)
{
#if FAKE_PRECHARGE_VOLTAGE
    FakePrechargeVoltage();
#endif // FAKE_PRECHARGE_VOLTAGE
}

/**
 * @brief HVC hook: prints a [BENCH] line for each HVC override that is on. Called by
 *        BenchPrintOverrides().
 *
 * @return true if any HVC override is on.
 */
bool PrintHvcOverrides(void)
{
    bool any_on = false;

    if (FAKE_PRECHARGE_VOLTAGE)
    {
        any_on = true;
        PRINT_BENCH("FAKE_PRECHARGE_VOLTAGE on");
    }
    return any_on;
}
