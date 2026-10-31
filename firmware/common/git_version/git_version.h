/**
 * @file    git_version.h
 * @brief   Git commit the firmware was built from, packed for a CAN message.
 *
 * GIT_VERSION_HASH (first 8 hex digits of the commit SHA) and GIT_VERSION_DIRTY come from
 * git_version_generated.h, which the build regenerates every time (see git_version.cmake).
 * A board gets this header by including git_version.cmake from its CMakeLists.txt.
 */

#ifndef __GIT_VERSION_H__
#define __GIT_VERSION_H__

/* INCLUDES */
#include <stdint.h>

#include "git_version_generated.h"

/* DEFINES */
#define GIT_VERSION_CAN_DLC 5U

/* FUNCTION DEFINITIONS */
/**
 * @brief Packs the commit hash CAN payload.
 *
 * data[0..3] are the hash bytes in order (big-endian), so a raw dump reads the same as
 * `git rev-parse --short=8 HEAD`: commit 1a2b3c4d is sent as 1A 2B 3C 4D. data[4] is 1 if the
 * build had uncommitted changes to tracked files. A hash of 0 means git wasn't available.
 *
 * @param data Payload buffer, at least GIT_VERSION_CAN_DLC bytes.
 */
static inline void GitVersionPackCan(uint8_t* data)
{
    data[0] = (uint8_t)((GIT_VERSION_HASH >> 24) & 0xFFU);
    data[1] = (uint8_t)((GIT_VERSION_HASH >> 16) & 0xFFU);
    data[2] = (uint8_t)((GIT_VERSION_HASH >> 8) & 0xFFU);
    data[3] = (uint8_t)(GIT_VERSION_HASH & 0xFFU);
    data[4] = (uint8_t)GIT_VERSION_DIRTY;
}

#endif /* __GIT_VERSION_H__ */
