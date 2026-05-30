#pragma once

#include "mst_defs.h"
#include "mst_types.h"


/**
 * Pack state 
 * MARK: globals
 */
// extern module_t pack_modules[NUM_MODULES];
// extern faults_t pack_faults;
// extern warnings_t pack_warnings;
// extern pack_state_t pack_state;

// extern slave_t slaves[SLAVE_NUM_DEVICES];


/**
 * Function definitions
 * MARK: f(x)
 */
void Initialize();
void CollectBoardData();
void CollectModuleData();
void AnalyzeModuleData();
void DriveOutputs();
void SendCanMessages();

#if (UNIT_TEST_MCU == RUN)
void Debug_McuTestCycle();
#endif // UNIT_TEST_MCU

#if (UNIT_TEST_IO == RUN)
void Debug_DigitalIoTestCycle();
#endif // UNIT_TEST_IO

#if (UNIT_TEST_CAN == RUN)
void Debug_CanTestCycle();
#endif // UNIT_TEST_CAN

#if (UNIT_TEST_ISOSPI == RUN)
void Debug_IsoSpiTestCycle();
#endif // UNIT_TEST_ISOSPI

#if (UNIT_TEST_SLAVE == RUN)
void Debug_SlaveTestCommsCycle();
void Debug_SlaveTestBalanceCycle();
void Debug_SlaveTestMuxCycle();
#endif // UNIT_TEST_ISOSPI

