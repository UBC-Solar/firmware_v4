#pragma once

#include "mst_defs.h"

void Initialize();

#ifdef UNIT_TEST_MCU
void Debug_McuTestCycle();
#endif // UNIT_TEST_MCU

#ifdef UNIT_TEST_IO
void Debug_DigitalIoTestCycle();
#endif // UNIT_TEST_IO

#ifdef UNIT_TEST_CAN
void Debug_IsoSpiTestCycle();
#endif // UNIT_TEST_CAN

#ifdef UNIT_TEST_ISOSPI
void Debug_CanTestCycle();
#endif // UNIT_TEST_ISOSPI
