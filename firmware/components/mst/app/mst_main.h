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
void Initialize();
void CollectBoardData();
void CollectModuleData();
void AnalyzeModuleData();
void DriveOutputs();
void SendCanMessages();

#if (UNIT_TEST_MCU == RUN)
void Debug_McuTestCycle(void);
#endif // (UNIT_TEST_MCU == RUN)

#if (UNIT_TEST_IO == RUN)
void Debug_DigitalIoTestCycle(void);
#endif // (UNIT_TEST_IO == RUN)

#if (UNIT_TEST_CAN == RUN)
void Debug_CanTestCycle(void);
#endif // (UNIT_TEST_CAN == RUN)

#if (UNIT_TEST_ISOSPI == RUN)
void Debug_IsoSpiTestCycle(void);
#endif // (UNIT_TEST_ISOSPI == RUN)

#if (INT_TEST_SLAVE == RUN)
void Debug_SlaveTestCommsCycle(void);
#endif // (INT_TEST_SLAVE == RUN)

#if (INT_TEST_SLAVE_BAL_VOLT == RUN)
void Debug_SlaveTestBalancingVoltageDrop(void);
#endif // (INT_TEST_SLAVE_BAL_VOLT == RUN)

#if (INT_TEST_SLAVE_BAL_SCRUT == RUN)
void Debug_SlaveTestBalanceScrutCycle(void);
#endif // (INT_TEST_SLAVE_BAL_SCRUT == RUN)

#if (INT_TEST_SLAVE_MUX == RUN)
void Debug_SlaveTestMuxCycle(void);
#endif // (INT_TEST_SLAVE_MUX == RUN)
