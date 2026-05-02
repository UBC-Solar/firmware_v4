#ifndef __CAN_DRIVER__H__
#define __CAN_DRIVER__H__

#include "CAN_comms.h"

/**
 * @brief Initializes CAN Comms hardware requirements and configures CAN filters for the TEL subsystem.
 * @return CAN comms configuration structure
 */
CAN_comms_config_t CanDriverInit();

#endif /* __CAN_DRIVER__H__ */