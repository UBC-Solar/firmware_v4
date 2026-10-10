#pragma once

#include "mst_defs.h"
#include "mst_types.h"


/**
 * Pack state \
 */
extern module_t pack_modules[NUM_MODULES];
extern faults_t pack_faults;
extern warnings_t pack_warnings;
extern pack_state_t pack_state;

extern slave_t slaves[SLAVE_NUM_DEVICES];

/**
 * Function definitions\
 */

/**
 * @brief Start MST in a safe state with all peripherals ready
 *
 * Holds FAULT asserted while initializing UART, CAN, and slaves so the
 * main loop begins from a known fault-safe condition.
 */
void Initialize();

/**
 * @brief Read balance and scrutineering enable switches
 */
void CollectBoardData();

/**
 * @brief Gather fresh voltages and temperatures for every module
 */
void CollectModuleData();

/**
 * @brief Turn raw measurements into faults, warnings, and stats
 */
void AnalyzeModuleData();

/**
 * @brief Drives contactors, limits, and discharge to match the latest analysis
 */
void DriveOutputs();

/**
 * @brief Keeps the rest of the car informed so HVC and logging can react
 */
void SendCanMessages();

#if (UNIT_TEST_MCU == RUN)
/**
 * @brief Proves code runs, clocks work, and basic IO is alive
 */
void Debug_McuTestCycle(void);
#endif // (UNIT_TEST_MCU == RUN)

#if (UNIT_TEST_IO == RUN)
/**
 * @brief Verifies every disable line can actually be driven
 */
void Debug_DigitalIoTestCycle(void);
#endif // (UNIT_TEST_IO == RUN)

#if (UNIT_TEST_CAN == RUN)
/**
 * @brief Proves the CAN peripheral can get bits on the bus
 */
void Debug_CanTestCycle(void);
#endif // (UNIT_TEST_CAN == RUN)

#if (UNIT_TEST_ISOSPI == RUN)
/**
 * @brief Proves the SPI chain can be addressed
 */
void Debug_IsoSpiTestCycle(void);
#endif // (UNIT_TEST_ISOSPI == RUN)

#if (INT_TEST_SLAVE == RUN)
/**
 * @brief Proves writes survive the round trip to the ADBMS1818 chain
 */
void Debug_SlaveTestCommsCycle(void);
#endif // (INT_TEST_SLAVE == RUN)

#if (INT_TEST_SLAVE_BAL_VOLT == RUN)
/**
 * @brief Compares each module with balancing off versus on to characterize the
 * balancing network
 */
void Debug_SlaveTestBalancingVoltageDrop(void);
#endif // (INT_TEST_SLAVE_BAL_VOLT == RUN)

#if (INT_TEST_SLAVE_BAL_SCRUT == RUN)
/**
 * @brief Verifies the two modes can be enabled together and toggled cleanly
 */
void Debug_SlaveTestBalanceScrutCycle(void);
#endif // (INT_TEST_SLAVE_BAL_SCRUT == RUN)

#if (INT_TEST_SLAVE_MUX == RUN)
/**
 * @brief Lets each thermistor path be observed and measured in turn
 */
void Debug_SlaveTestMuxCycle(void);
#endif // (INT_TEST_SLAVE_MUX == RUN)
