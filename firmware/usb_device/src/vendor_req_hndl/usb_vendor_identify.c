/**
 * @file
 * @brief vendor device class - device identification interface.
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
 * @details Implementations of the vendor device class for the device identification interface.
 */
#include <stdbool.h>
#include <stdint.h>

#include "esp_log.h"

#include "tinyusb.h"

#include "sdkconfig.h"
#include "device_status.h"

#include "usb_vendor_identify.h"

static const char *TAG = "usb-vendor-id";

bool vendor_handle_class_control_request_identify(uint8_t rhport,
                                                  uint8_t stage,
                                                  tusb_control_request_t const * request,
                                                  uint8_t * buffer) {
    (void)buffer;

    if (request->bmRequestType_bit.direction == TUSB_DIR_OUT) {
        if ((stage == CONTROL_STAGE_SETUP) && (request->wLength == 0)) {
            if (request->wValue == 1) {
                ESP_LOGI(TAG, "enable device identification");
                devstat_startIdentify();
            } else {
                ESP_LOGI(TAG, "disable device identification");
                devstat_stopIdentify();
            }
            /* response with status OK */
            return tud_control_status(rhport, request);
        }
    }

    /* stall unknown request */
    return false;
}

