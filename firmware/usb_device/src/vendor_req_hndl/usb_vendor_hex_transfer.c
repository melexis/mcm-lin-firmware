/**
 * @file
 * @brief vendor device class - intelhex transfer interface.
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
 * @details Implementations of the vendor device class for the intelhex transfer interface.
 */
#include <stdbool.h>
#include <stdint.h>

#include "esp_log.h"
#include "esp_system.h"

#include "tinyusb.h"

#include "sdkconfig.h"
#include "intelhex.h"
#include "usb_vendor_bulk.h"

#include "usb_vendor_hex_transfer.h"

static const char *TAG = "usb-vendor-hex";

static bool btl_transfer_mode = false;

static uint32_t btl_ihex_ext_address = 0u;
static ihexContainer_t * btl_ihex_first_container = NULL;
static ihexContainer_t * btl_ihex_prev_container = NULL;

/** Intelhex transfer bulk USB communication handler
 *
 * @param[in]  buffer  buffer used for storing temp data.
 * @param[in]  buffer_wr_ptr  pointer to the current writing location in buffer.
 * @returns  new write pointer in buffer (negative number for task stop).
 */
static int32_t usb_vendor_hex_transfer_handler(char *buffer, int32_t buffer_wr_ptr);


static int32_t usb_vendor_hex_transfer_handler(char *buffer, int32_t buffer_wr_ptr) {
    int32_t buffer_rd_ptr = 0;
    size_t item_size = 0;
    char *item = (char *)xRingbufferReceiveUpTo(bulk_rx_buf_handle,
                                                &item_size,
                                                pdMS_TO_TICKS(25),
                                                BULK_TASK_BUFFER_LEN / 4);
    if (item != NULL) {
        if (item_size > 0) {
            memcpy(&buffer[buffer_wr_ptr], item, item_size);
            buffer_wr_ptr += item_size;
            buffer[buffer_wr_ptr] = 0;
        }
        vRingbufferReturnItem(bulk_rx_buf_handle, (void *)item);

        /* replace newline chars for later strlen usage */
        size_t temp_ptr = 0;
        while (temp_ptr < buffer_wr_ptr) {
            if ((buffer[temp_ptr] == '\n') || (buffer[temp_ptr] == '\r')) {
                buffer[temp_ptr] = 0;
            }
            temp_ptr++;
        }
    }

    if (buffer_wr_ptr > 9) {
        /* one line of intelhex has at least 9 chars for the start */
        while (buffer_rd_ptr < buffer_wr_ptr) {
            /* todo add a sleep every x loops to allow other processes to trigger? */
            while ((buffer[buffer_rd_ptr] == '\0') && (buffer_rd_ptr < buffer_wr_ptr)) {
                buffer_rd_ptr++;
            }
            ihexContainer_t * container = NULL;
            if (intelhex_readLine(&buffer[buffer_rd_ptr], &btl_ihex_ext_address, &container) == true) {
                buffer_rd_ptr += strlen(&buffer[buffer_rd_ptr]);
                if (container != NULL) {
                    container->prev = btl_ihex_prev_container;
                    if (btl_ihex_prev_container != NULL) {
                        btl_ihex_prev_container->next = container;
                    } else {
                        btl_ihex_first_container = container;
                    }
                    btl_ihex_prev_container = container;
                } else {
                    break;
                }
            } else {
                break;
            }
        }

        if (buffer_rd_ptr != 0) {
            buffer_wr_ptr -= buffer_rd_ptr;
            memcpy(&buffer[0], &buffer[buffer_rd_ptr], buffer_wr_ptr);
            buffer[buffer_wr_ptr] = 0;
        }
    } else if (!btl_transfer_mode) {
        /* done transferring and processing */
        buffer_wr_ptr = -1;
        usb_vendor_bulk_write_string("OK\n");
    }

    return buffer_wr_ptr;
}

bool vendor_handle_class_control_request_hex_transfer(uint8_t rhport,
                                                      uint8_t stage,
                                                      tusb_control_request_t const * request,
                                                      uint8_t * buffer) {
    (void)buffer;

    if (request->bmRequestType_bit.direction == TUSB_DIR_OUT) {
        if ((stage == CONTROL_STAGE_SETUP) && (request->wLength == 0)) {
            if (request->wValue == 1) {
                ESP_LOGI(TAG, "do hex transfer");
                /* clear current intelhex object if it exists */
                btl_ihex_ext_address = 0u;
                if (btl_ihex_first_container != NULL) {
                    intelhex_free(btl_ihex_first_container);
                }
                btl_ihex_first_container = NULL;
                btl_ihex_prev_container = NULL;
                btl_transfer_mode = true;
                (void)usb_vendor_bulk_start_raw(usb_vendor_hex_transfer_handler);
            } else {
                ESP_LOGI(TAG, "stop hex transfer");
                btl_transfer_mode = false;
            }
            return tud_control_status(rhport, request);
        }
    }

    /* stall unknown request */
    return false;
}

ihexContainer_t * usb_vendor_hex_transfer_get_container(void) {
    return btl_ihex_first_container;
}

