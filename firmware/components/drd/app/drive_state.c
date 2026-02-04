/*
 *  drive_state.c
 *
 *  @brief Handles the drive state for the car. Takes ADC, GPIO and CAN inputs and
 *         outputs DAC values, MDI flags, and CAN data messages
 * 
 *  Created: Jan 28, 2026
 *  Author:  Tony Chen
*/

#include "drive_state.h"

/* DEFINES */

/* DRIVE STATE FINITE STATE MACHINE */

void drive_state_fms_handler() {

    switch(g_drive_state) {
        case FORWARD:

    }
}

/* DRIVE STATE DATA COLLECTION */

void update_drive_flags() {
    
}