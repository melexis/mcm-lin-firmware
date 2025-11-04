/**
 * @file
 * @brief vendor device class - ppm bootloader interface.
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
 * @details Implementations of the vendor device class for the ppm bootloader interface.
 */
#include <stdbool.h>
#include <stdint.h>

#include "esp_log.h"
#include "esp_system.h"

#include "tinyusb.h"

#include "sdkconfig.h"
#include "bus_manager.h"
#include "mlx_err.h"
#include "ppm_err.h"
#include "ppm_bootloader.h"
#include "usb_vendor_bulk.h"
#include "usb_vendor_hex_transfer.h"

#include "usb_vendor_btl_ppm.h"

static const char *TAG = "usb-vendor-btl-ppm";

typedef enum vendor_request_bulk_msg_e {
    /* (MCM_VENDOR_REQUEST_BOOTLOADER_PPM << 8) + [0x00..0xFF] */
    PPM_DO_BTL_ACTION = 0x3300,
    /* MCM_BULK_MSG_ERROR_REPORT = 0xFFFF */
} vendor_request_bulk_msg_t;

typedef struct vendor_btl_request_s {
    uint32_t bitrate;         /**< baudrate to be used during bootloader operations */
    uint8_t manpow;           /**< 1: manual power cycling */
    uint8_t broadcast;        /**< 1: bootloading shall be done in broadcast mode */
    uint8_t memory;           /**< memory type to perform action on (0: NVRAM; 1: flash) */
    uint8_t action;           /**< action type to perform (0: program; 1: verify) */
} vendor_btl_request_t;


static bool bulk_btl_command_handler(uint16_t command, const uint8_t * data, uint16_t datalen) {
    bool handled = false;
    mlx_err_t result = MLX_FAIL_COMMAND_UNKNOWN;

    if (busmngr_ClaimInterface(USER_USB_VENDOR, MODE_BOOTLOADER) == ESP_OK) {
        switch ((vendor_request_bulk_msg_t)command) {
            case PPM_DO_BTL_ACTION:
                if (datalen == sizeof(vendor_btl_request_t)) {
                    vendor_btl_request_t *req_data = (vendor_btl_request_t*)data;

                    ppm_memory_t memory = PPM_MEM_INVALID;
                    if (req_data->memory == 0) {
                        memory = PPM_MEM_NVRAM;
                    } else if (req_data->memory == 1) {
                        memory = PPM_MEM_FLASH;
                    }

                    ppm_action_t action = PPM_ACT_INVALID;
                    if (req_data->action == 0) {
                        action = PPM_ACT_PROGRAM;
                    } else if (req_data->action == 1) {
                        action = PPM_ACT_VERIFY;
                    }

                    ppm_err_t ppmstat = ppmbtl_doAction(req_data->manpow != 0,
                                                        req_data->broadcast != 0,
                                                        req_data->bitrate,
                                                        memory,
                                                        action,
                                                        usb_vendor_hex_transfer_get_container());
                    if (ppmstat == PPM_OK) {
                        usb_vendor_bulk_write_response(command, NULL, 0u);
                    } else {
                        usb_vendor_bulk_write_error(command,
                                                    ppmstat,
                                                    ppm_err_to_string(ppmstat));
                    }

                    handled = true;
                    result = MLX_OK;
                } else {
                    result = MLX_FAIL_INV_DATA_LEN;
                }
                break;

            default:
                break;
        }
    } else {
        result = MLX_FAIL_INTERFACE_NOT_FREE;
    }

    if (result < MLX_OK) {
        usb_vendor_bulk_write_error(command,
                                    result,
                                    mlxerr_ErrorCodeToName(result));
        handled = true;
    }

    return handled;
}


bool vendor_handle_class_control_request_btl_ppm(uint8_t rhport,
                                                 uint8_t stage,
                                                 tusb_control_request_t const * request,
                                                 uint8_t * buffer) {
    (void)buffer;

    if (request->bmRequestType_bit.direction == TUSB_DIR_OUT) {
        if ((stage == CONTROL_STAGE_SETUP) && (request->wLength == 0)) {
            if (request->wValue == 1) {
                ESP_LOGI(TAG, "request btl ppm mode");
                (void)usb_vendor_bulk_start_command(bulk_btl_command_handler);
                return tud_control_xfer(rhport, request, NULL, 0);
            } else {
                ESP_LOGI(TAG, "stop btl ppm mode");
                (void)usb_vendor_bulk_stop();
                (void)busmngr_ReleaseInterface(USER_USB_VENDOR, MODE_BOOTLOADER);
                return tud_control_status(rhport, request);
            }
        }
    }

    /* stall unknown request */
    return false;
}

