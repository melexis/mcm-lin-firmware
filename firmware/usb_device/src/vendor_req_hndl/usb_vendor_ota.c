/**
 * @file
 * @brief vendor device class - ota interface.
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
 * @details Implementations of the vendor device class for the ota interface.
 */
#include <stdbool.h>
#include <stdint.h>

#include "esp_log.h"
#include "esp_system.h"

#include "freertos/ringbuf.h"
#include "tinyusb.h"

#include "sdkconfig.h"
#include "ota_support.h"
#include "usb_vendor_bulk.h"

#include "usb_vendor_ota.h"

static const char *TAG = "usb-vendor-ota";

static bool ota_transfer_mode = false;

/** OTA bulk USB communication handler
 *
 * @param[in]  buffer  buffer used for storing temp data.
 * @param[in]  buffer_wr_ptr  pointer to the current writing location in buffer.
 * @returns  new write pointer in buffer (negative number for task stop).
 */
static int32_t usb_vendor_bulk_ota_task_handler(char *buffer, int32_t buffer_wr_ptr);


static int32_t usb_vendor_bulk_ota_task_handler(char *buffer, int32_t buffer_wr_ptr) {
    size_t item_size = 0;
    const char *item = (const char *)xRingbufferReceiveUpTo(bulk_rx_buf_handle,
                                                            &item_size,
                                                            pdMS_TO_TICKS(25),
                                                            BULK_TASK_BUFFER_LEN / 4);
    if (item != NULL) {
        if (item_size > 0) {
            if (otasupport_Write(item, item_size) != ESP_OK) {
                buffer_wr_ptr = -1;
                ota_transfer_mode = false;
                usb_vendor_bulk_write_string("FAIL\n");
                ESP_LOGI(TAG, "ota transfer failed while writing");
            }
        }
        vRingbufferReturnItem(bulk_rx_buf_handle, (void *)item);
    }

    if (item_size == 0) {
        if (ota_transfer_mode) {
            usb_vendor_bulk_write_string("EMPTY\n");
            vTaskDelay(pdMS_TO_TICKS(50));
        } else {
            buffer_wr_ptr = -1;
            if (otasupport_ValidatePartition() == ESP_OK) {
                usb_vendor_bulk_write_string("VALID\n");
                ESP_LOGI(TAG, "ota transfer done and image valid");
            } else {
                usb_vendor_bulk_write_string("FAIL\n");
                ESP_LOGI(TAG, "ota transfer done and image invalid");
            }
        }
    }

    return buffer_wr_ptr;
}

bool vendor_handle_class_control_request_ota_transfer(uint8_t rhport,
                                                      uint8_t stage,
                                                      tusb_control_request_t const * request,
                                                      uint8_t * buffer) {
    (void)buffer;

    if (request->bmRequestType_bit.direction == TUSB_DIR_OUT) {
        if ((stage == CONTROL_STAGE_SETUP) && (request->wLength == 0)) {
            if (request->wValue == 1) {
                ESP_LOGI(TAG, "do ota transfer");
                if (otasupport_Start() == ESP_OK) {
                    ota_transfer_mode = true;
                    (void)usb_vendor_bulk_start_raw(usb_vendor_bulk_ota_task_handler);
                    return tud_control_status(rhport, request);
                }
            } else {
                ESP_LOGI(TAG, "stop ota transfer");
                ota_transfer_mode = false;
                return tud_control_status(rhport, request);
            }
        }
    }

    /* stall unknown request */
    return false;
}

bool vendor_handle_class_control_request_ota_boot(uint8_t rhport,
                                                  uint8_t stage,
                                                  tusb_control_request_t const * request,
                                                  uint8_t * buffer) {
    (void)buffer;

    if (request->bmRequestType_bit.direction == TUSB_DIR_OUT) {
        if ((stage == CONTROL_STAGE_SETUP) && (request->wLength == 0)) {
            if (otasupport_UpdateBootPartition() == ESP_OK) {
                return tud_control_status(rhport, request);
            }
        }
    }

    /* stall unknown request */
    return false;
}

