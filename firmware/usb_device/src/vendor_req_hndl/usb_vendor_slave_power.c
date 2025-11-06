/**
 * @file
 * @brief vendor device class - slave power control interface.
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
 * @details Implementations of the vendor device class for the slave power control interface.
 */
#include <stdbool.h>
#include <stdint.h>

#include "esp_log.h"

#include "tinyusb.h"

#include "sdkconfig.h"
#include "power_ctrl.h"

#include "usb_vendor_slave_power.h"

static const char *TAG = "usb-vendor-slpwr";

typedef enum vendor_request_slave_ctrl_e {
    MCM_SLAVE_CTRL_POWER_DOWN = 0x00,
    MCM_SLAVE_CTRL_POWER_UP = 0x01,
    MCM_SLAVE_CTRL_V_SUPPLY = 0x02,
    MCM_SLAVE_CTRL_V_BUS = 0x03,
    MCM_SLAVE_CTRL_C_BUS = 0x04,
} vendor_request_slave_ctrl_t;

bool vendor_handle_class_control_request_slave_pwr(uint8_t rhport,
                                                   uint8_t stage,
                                                   tusb_control_request_t const * request,
                                                   uint8_t * buffer) {
    switch ((vendor_request_slave_ctrl_t)request->wValue) {
        case MCM_SLAVE_CTRL_POWER_DOWN:
        case MCM_SLAVE_CTRL_POWER_UP:
            if (request->bmRequestType_bit.direction == TUSB_DIR_OUT) {
                if ((stage == CONTROL_STAGE_SETUP) && (request->wLength == 0)) {
                    if ((vendor_request_slave_ctrl_t)request->wValue == MCM_SLAVE_CTRL_POWER_UP) {
                        ESP_LOGI(TAG, "enable slave power");
                        powerctrl_slaveEnable();
                    } else {
                        ESP_LOGI(TAG, "disable slave power");
                        powerctrl_slaveDisable();
                    }
                    /* response with status OK */
                    return tud_control_status(rhport, request);
                }
            } else {
                if (stage == CONTROL_STAGE_SETUP) {
                    buffer[0] = (uint8_t)powerctrl_slaveEnabled();
                    return tud_control_xfer(rhport,
                                            request,
                                            (void*)(uintptr_t) buffer,
                                            1u);
                } else if (stage == CONTROL_STAGE_DATA) {
                    return true;
                }
            }
            break;

        case MCM_SLAVE_CTRL_V_SUPPLY:
            if (request->bmRequestType_bit.direction == TUSB_DIR_IN) {
                if (stage == CONTROL_STAGE_SETUP) {
                    int32_t * ptr = (int32_t*)&buffer[0];
                    *ptr = powerctrl_getSupplyVoltage();
                    return tud_control_xfer(rhport,
                                            request,
                                            (void*)(uintptr_t) buffer,
                                            sizeof(int32_t));
                } else if (stage == CONTROL_STAGE_DATA) {
                    return true;
                }
            }
            break;

        case MCM_SLAVE_CTRL_V_BUS:
            if (request->bmRequestType_bit.direction == TUSB_DIR_IN) {
                if (stage == CONTROL_STAGE_SETUP) {
                    int32_t * ptr = (int32_t*)&buffer[0];
                    *ptr = powerctrl_getBusVoltage();
                    return tud_control_xfer(rhport,
                                            request,
                                            (void*)(uintptr_t) buffer,
                                            sizeof(int32_t));
                } else if (stage == CONTROL_STAGE_DATA) {
                    return true;
                }
            }
            break;

        case MCM_SLAVE_CTRL_C_BUS:
            if (request->bmRequestType_bit.direction == TUSB_DIR_IN) {
                if (stage == CONTROL_STAGE_SETUP) {
                    int32_t * ptr = (int32_t*)&buffer[0];
                    *ptr = powerctrl_getOutputCurrent();
                    return tud_control_xfer(rhport,
                                            request,
                                            (void*)(uintptr_t) buffer,
                                            sizeof(int32_t));
                } else if (stage == CONTROL_STAGE_DATA) {
                    return true;
                }
            }
            break;

        default:
            break;
    }

    /* stall unknown request */
    return false;
}

