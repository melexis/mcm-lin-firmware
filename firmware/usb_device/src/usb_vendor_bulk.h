/**
 * @file
 * @brief vendor device class - bulk interface.
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
 * @details Definitions of the vendor device class for the bulk interface.
 * @{
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "freertos/ringbuf.h"
#include "tinyusb.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BULK_TASK_BUFFER_LEN 204800

extern RingbufHandle_t bulk_rx_buf_handle;
extern RingbufHandle_t bulk_tx_buf_handle;

/** Bulk task handler
 *
 * @param[in]  buffer  buffer used for storing temp data.
 * @param[in]  buffer_wr_ptr  pointer to the current writing location in buffer.
 * @returns  new write pointer in buffer (negative number for task stop).
 */
typedef int32_t (* bulk_task_handle_t)(char *buffer, int32_t buffer_wr_ptr);

typedef bool (* mlx_command_handle_t)(uint16_t command, const uint8_t * data, uint16_t datalen);

#define MCM_BULK_MSG_ERROR_REPORT 0xFFFF


esp_err_t usb_vendor_bulk_init(void);

esp_err_t usb_vendor_bulk_start_raw(bulk_task_handle_t handle);

esp_err_t usb_vendor_bulk_start_command(mlx_command_handle_t handle);

esp_err_t usb_vendor_bulk_stop(void);

/**
 *
 */
void usb_vendor_bulk_write_raw(const char *buffer, uint32_t length);

void usb_vendor_bulk_write_string(const char *buffer);

bool usb_vendor_bulk_write_response(uint16_t command, const uint8_t * data, uint16_t datalen);

bool usb_vendor_bulk_write_error(uint16_t command, int error, const char *error_msg);

/** @} */

#ifdef __cplusplus
}
#endif
