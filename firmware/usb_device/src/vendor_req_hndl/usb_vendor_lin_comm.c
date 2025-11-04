/**
 * @file
 * @brief vendor device class - LIN communication interface.
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
 * @details Implementations of the vendor device class for the LIN communication interface.
 */
#include <stdbool.h>
#include <stdint.h>

#include "esp_log.h"
#include "esp_system.h"

#include "tinyusb.h"

#include "sdkconfig.h"
#include "bus_manager.h"
#include "lin_master.h"
#include "lin_err.h"
#include "power_ctrl.h"
#include "usb_vendor_bulk.h"

#include "usb_vendor_lin_comm.h"

static const char *TAG = "usb-vendor-lin";

typedef enum vendor_request_lin_comm_e {
    /* (MCM_VENDOR_REQUEST_LIN_COMM << 8) + [0x00..0xFF] */
    MCM_LIN_COMM_SEND_WAKEUP = 0x2200,
    MCM_LIN_COMM_HANDLE_MESSAGE = 0x2201,
    /* MCM_BULK_MSG_ERROR_REPORT = 0xFFFF, */
} vendor_request_lin_comm_t;

typedef struct bulk_lin_transfer_message_s {
    uint16_t baudrate;
    uint8_t datalength;
    uint8_t m2s;
    uint8_t enhanced_crc;
    uint8_t frameid;
    uint8_t payload[8];
} bulk_lin_transfer_message_t;

static bool bulk_lin_command_handler(uint16_t command, const uint8_t * data, uint16_t datalen) {
    bool handled = false;

    switch ((vendor_request_lin_comm_t)command) {
        case MCM_LIN_COMM_SEND_WAKEUP:
            lin_err_t error = linmaster_send_wakeup(*((uint16_t*)data));
            if (error == LIN_OK) {
                usb_vendor_bulk_write_response(command, NULL, 0u);
            } else {
                usb_vendor_bulk_write_error(command, error, lin_err_to_string(error));
            }
            handled = true;
            break;

        case MCM_LIN_COMM_HANDLE_MESSAGE:
        {
            bulk_lin_transfer_message_t * message = (bulk_lin_transfer_message_t*)data;
            if (message->m2s != 0u) {
                /* M2S message */
                lin_err_t error = linmaster_send_m2s(message->baudrate,
                                                     message->enhanced_crc != 0u,
                                                     message->frameid,
                                                     message->payload,
                                                     message->datalength);

                if (error == LIN_OK) {
                    usb_vendor_bulk_write_response(command, NULL, 0u);
                } else {
                    usb_vendor_bulk_write_error(command, error, lin_err_to_string(error));
                }
                handled = true;
            } else {
                /* S2M message */
                uint8_t *resp = calloc(message->datalength, sizeof(uint8_t));
                if (resp != NULL) {
                    lin_err_t error = linmaster_send_s2m(message->baudrate,
                                                         message->enhanced_crc != 0u,
                                                         message->frameid,
                                                         resp,
                                                         message->datalength);

                    if (error == LIN_OK) {
                        /* Report the received message */
                        usb_vendor_bulk_write_response(command, resp, message->datalength);
                    } else {
                        usb_vendor_bulk_write_error(command, error, lin_err_to_string(error));
                    }
                    handled = true;
                    free(resp);
                }
            }
            break;
        }

        default:
            break;
    }

    return handled;
}


bool vendor_handle_class_control_request_lin_comm(uint8_t rhport,
                                                  uint8_t stage,
                                                  tusb_control_request_t const * request,
                                                  uint8_t * buffer) {
    (void)buffer;

    if (request->bmRequestType_bit.direction == TUSB_DIR_OUT) {
        if ((stage == CONTROL_STAGE_SETUP) && (request->wLength == 0)) {
            if (request->wValue == 1) {
                ESP_LOGI(TAG, "enable lin mode");
                if (busmngr_ClaimInterface(USER_USB_VENDOR, MODE_APPLICATION) == ESP_OK) {
                    powerctrl_slaveEnable();
                    (void)usb_vendor_bulk_start_command(bulk_lin_command_handler);
                }
                return tud_control_xfer(rhport, request, NULL, 0);
            } else {
                ESP_LOGI(TAG, "disable lin mode");
                (void)usb_vendor_bulk_stop();
                (void)busmngr_ReleaseInterface(USER_USB_VENDOR, MODE_APPLICATION);
                powerctrl_slaveDisable();
                return tud_control_status(rhport, request);
            }
        }
    }

    /* stall unknown request */
    return false;
}

