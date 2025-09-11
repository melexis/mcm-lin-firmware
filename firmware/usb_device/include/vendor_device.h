/**
 * @file
 * @brief vendor device module definitions.
 * @internal
 *
 * @copyright (C) 2025 Melexis N.V.
 *
 * Melexis N.V. is supplying this code for use with Melexis N.V. processor based microcontrollers only.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY,
 * INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.  MELEXIS N.V. SHALL NOT IN ANY CIRCUMSTANCES,
 * BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 * @endinternal
 *
 * @ingroup application
 *
 * @details This file contains the definitions of the vendor device module.
 */

#ifndef VENDOR_DEVICE_H_
    #define VENDOR_DEVICE_H_

#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"

#include "tusb.h"

/* Invoked when a control transfer occurred on an interface of this class
 * Driver response accordingly to the request and the transfer stage (setup/data/ack)
 *
 * @retval false to stall control endpoint (e.g unsupported request)
 */
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request);

/** Initialize the vendor device module
 *
 * @returns error code representing the status of the operation.
 */
esp_err_t vendor_device_init(void);

#endif  /* VENDOR_DEVICE_H_ */
