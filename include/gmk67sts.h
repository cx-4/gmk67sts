#ifndef GMK67STS_H
#define GMK67STS_H

/**
 * @file gmk67sts.h
 * @brief Public API for GMK67STS time synchronization library
 * 
 * This header provides the public interface for the GMK67STS library.
 * For most use cases, include this header and link against the compiled binary.
 */

/* Version */
#define GMK67STS_VERSION "1.5.1"

/* USB IDs */
#define GMK_VENDOR_ID  0x320f
#define GMK_PRODUCT_ID 0x5055

/* Include core module headers */
#include "../src/core/device.h"
#include "../src/core/protocol.h"
#include "../src/core/config.h"
#include "../src/core/timesync.h"

#endif /* GMK67STS_H */