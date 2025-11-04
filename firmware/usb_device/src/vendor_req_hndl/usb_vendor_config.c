/**
 * @file
 * @brief vendor device class - configuration interface.
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
 * @details Implementations of the vendor device class for the configuration interface.
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "tinyusb.h"

#include "sdkconfig.h"
#include "networking.h"
#include "wifi.h"

#include "usb_vendor_config.h"

typedef enum vendor_request_config_e {
    MCM_CONFIG_HOSTNAME = 0x00,
    MCM_CONFIG_WIFI_SSID = 0x01,
    MCM_CONFIG_WIFI_PASS = 0x02,
    MCM_CONFIG_WIFI_MAC = 0x03,
    MCM_CONFIG_WIFI_IP_INFO = 0x04,
    MCM_CONFIG_UNKNOWN = 0xFF,
} vendor_request_config_t;

bool vendor_handle_class_control_request_config(uint8_t rhport,
                                                uint8_t stage,
                                                tusb_control_request_t const * request,
                                                uint8_t * buffer) {
    switch ((vendor_request_config_t)request->wValue) {
        case MCM_CONFIG_HOSTNAME:
            if (stage == CONTROL_STAGE_SETUP) {
                if (request->bmRequestType_bit.direction == TUSB_DIR_OUT) {
                    /* set hostname */
                    return tud_control_xfer(rhport, request, buffer, request->wLength);
                } else {
                    /* get hostname */
                    char hostname[32];
                    size_t hostname_size = sizeof(hostname);
                    if (networking_get_hostname(hostname, &hostname_size) == ESP_OK) {
                        return tud_control_xfer(rhport, request, (void*)(uintptr_t) hostname, strlen(hostname));
                    }
                }
            } else if (stage == CONTROL_STAGE_DATA) {
                if (request->bmRequestType_bit.direction == TUSB_DIR_OUT) {
                    networking_set_hostname((const char *)buffer, true);
                }
                return true;
            }
            break;

        case MCM_CONFIG_WIFI_SSID:
            if (stage == CONTROL_STAGE_SETUP) {
                if (request->bmRequestType_bit.direction == TUSB_DIR_OUT) {
                    if (request->wLength <= 32) {
                        /* set ssid */
                        return tud_control_xfer(rhport, request, buffer, request->wLength);
                    }
                } else {
                    /* get ssid */
                    if (wifi_get_ssid((char *)buffer, 64) == ESP_OK) {
                        return tud_control_xfer(rhport,
                                                request,
                                                (void*)(uintptr_t) buffer,
                                                strlen((const char*)buffer));
                    }
                }
            } else if (stage == CONTROL_STAGE_DATA) {
                if (request->bmRequestType_bit.direction == TUSB_DIR_OUT) {
                    wifi_set_ssid((const char *)buffer, true);
                }
                return true;
            }
            break;

        case MCM_CONFIG_WIFI_PASS:
            if (stage == CONTROL_STAGE_SETUP) {
                if (request->bmRequestType_bit.direction == TUSB_DIR_OUT) {
                    if (request->wLength <= 64) {
                        /* set pass */
                        return tud_control_xfer(rhport, request, buffer, request->wLength);
                    }
                } else {
                    /* get pass */
                    if (wifi_get_password((char *)buffer, 64) == ESP_OK) {
                        return tud_control_xfer(rhport,
                                                request,
                                                (void*)(uintptr_t) buffer,
                                                strlen((const char*)buffer));
                    }
                }
            } else if (stage == CONTROL_STAGE_DATA) {
                if (request->bmRequestType_bit.direction == TUSB_DIR_OUT) {
                    wifi_set_password((const char *)buffer, true);
                }
                return true;
            }
            break;

        case MCM_CONFIG_WIFI_MAC:
            if (stage == CONTROL_STAGE_SETUP) {
                if (request->bmRequestType_bit.direction == TUSB_DIR_IN) {
                    (void)wifi_get_mac(&buffer[0]);
                    return tud_control_xfer(rhport,
                                            request,
                                            (void*)(uintptr_t) buffer,
                                            6);
                }
            } else if (stage == CONTROL_STAGE_DATA) {
                if (request->bmRequestType_bit.direction == TUSB_DIR_IN) {
                    return true;
                }
            }
            break;

        case MCM_CONFIG_WIFI_IP_INFO:
            if (stage == CONTROL_STAGE_SETUP) {
                if (request->bmRequestType_bit.direction == TUSB_DIR_IN) {
                    /* get ip info */
                    if (wifi_get_ip_info((uint32_t*)&buffer[0],
                                         (uint32_t*)&buffer[4],
                                         (uint32_t*)&buffer[8]) == ESP_OK) {
                        return tud_control_xfer(rhport,
                                                request,
                                                (void*)(uintptr_t) buffer,
                                                12);
                    } else {
                        /* interface is not up */
                        return tud_control_xfer(rhport,
                                                request,
                                                NULL,
                                                0);
                    }
                }
            } else if (stage == CONTROL_STAGE_DATA) {
                if (request->bmRequestType_bit.direction == TUSB_DIR_OUT) {
                    /* todo use received data */
                }
                return true;
            }
            break;

        default:
            break;
    }

    /* stall unknown request */
    return false;
}

