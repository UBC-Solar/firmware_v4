#pragma once

#include "mst_defs.h"


/**
 * Pack state 
 * MARK: globals
 */
extern module_t pack_modules[NUM_MODULES];
extern faults_t pack_faults;
extern warnings_t pack_warnings;
extern pack_state_t pack_state;

extern slave_t slaves[NUM_SLAVES];


/**
 * Function definitions
 * MARK: f(x)
 */
void Initialize();
void CollectPackData();
void DriveOutputs();
void SendCanMMessages();

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
