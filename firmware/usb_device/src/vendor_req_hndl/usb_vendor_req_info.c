/**
 * @file
 * @brief vendor device class - device info interface.
 * @internal
 *
 * @copyright (C) 2025 Melexis N.V.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @endinternal
 *
 * @ingroup lib_usb_device
 *
 * @details Implementations of the vendor device class for the device info interface.
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "esp_err.h"
#include "esp_timer.h"

#include "tinyusb.h"

#include "sdkconfig.h"
#include "device_info.h"

#include "usb_vendor_req_info.h"

typedef enum vendor_request_info_e {
    MCM_INFO_VERSION = 0x00,
    MCM_INFO_RESET_REASON = 0x01,
    MCM_INFO_UP_TIME = 0x02,
    MCM_INFO_UNKNOWN = 0xFF,
} vendor_request_info_t;

bool vendor_handle_class_control_request_info(uint8_t rhport,
                                              uint8_t stage,
                                              tusb_control_request_t const * request,
                                              uint8_t * buffer) {
    (void)buffer;
    switch ((vendor_request_info_t)request->wValue) {
        case MCM_INFO_VERSION:
            if ((stage == CONTROL_STAGE_SETUP) && (request->bmRequestType_bit.direction == TUSB_DIR_IN)) {
                const char * version = devinfo_firmwareVersion();
                uint16_t total_len = strlen(version);
                return tud_control_xfer(rhport, request, (void*)(uintptr_t) version, total_len);
            } else if (stage == CONTROL_STAGE_DATA) {
                return true;
            }
            break;

        case MCM_INFO_RESET_REASON:
            if ((stage == CONTROL_STAGE_SETUP) && (request->bmRequestType_bit.direction == TUSB_DIR_IN)) {
                uint8_t reset = esp_reset_reason();
                return tud_control_xfer(rhport, request, &reset, sizeof(reset));
            } else if (stage == CONTROL_STAGE_DATA) {
                return true;
            }
            break;

        case MCM_INFO_UP_TIME:
            if ((stage == CONTROL_STAGE_SETUP) && (request->bmRequestType_bit.direction == TUSB_DIR_IN)) {
                int64_t uptime = esp_timer_get_time();
                return tud_control_xfer(rhport, request, &uptime, sizeof(uptime));
            } else if (stage == CONTROL_STAGE_DATA) {
                return true;
            }
            break;

        default:
            break;
    }

    /* stall unknown request */
    return false;
}

