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
 * @details Implementations of the vendor device class bulk interface.
 */
#include <stdbool.h>
#include <stdint.h>

#include "esp_log.h"
#include "esp_system.h"

#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "freertos/task.h"
#include "tinyusb.h"

#include "sdkconfig.h"
#include "mlx_crc.h"
#include "mlx_err.h"

#include "usb_vendor_bulk.h"

static const char *TAG = "usb-vendor-bulk";

RingbufHandle_t bulk_rx_buf_handle;
RingbufHandle_t bulk_tx_buf_handle;

static TaskHandle_t taskHandle;
static bulk_task_handle_t bulk_task_handle = NULL;
static mlx_command_handle_t command_handle = NULL;

#define USB_PACKET_HEADER 0xAA55AA55
typedef struct bulk_msg_header_s {
    uint32_t header;
    uint16_t length;
    uint16_t command;
    uint32_t reserved;
} bulk_msg_header_t;

/** Flush the Vendor device ring buffers */
static void usb_vendor_bulk_flush_buffers(void);

/** USB vendor bulk endpoint handler task
 *
 * @param[in]  arg  arguments for the task.
 */
static void usb_vendor_bulk_task(void *arg);

/** Melexis command bulk USB communication handler
 *
 * @param[in]  buffer  buffer used for storing temp data.
 * @param[in]  buffer_wr_ptr  pointer to the current writing location in buffer.
 * @retval  new write pointer in buffer.
 */
static int32_t usb_vendor_bulk_command_handler(char *buffer, int32_t buffer_wr_ptr);


static void usb_vendor_bulk_flush_buffers(void) {
    size_t item_size;
    char *item = (char *)xRingbufferReceive(bulk_tx_buf_handle,
                                            &item_size,
                                            0);
    if (item != NULL) {
        vRingbufferReturnItem(bulk_tx_buf_handle, (void *)item);
    }

    item = (char *)xRingbufferReceive(bulk_rx_buf_handle,
                                      &item_size,
                                      0);
    if (item != NULL) {
        vRingbufferReturnItem(bulk_rx_buf_handle, (void *)item);
    }
}

static void usb_vendor_bulk_task(void *arg) {
    (void)arg;
    char *buffer = (char*) malloc(BULK_TASK_BUFFER_LEN);
    int32_t buffer_wr_ptr = 0;

    while (1) {
        if (bulk_task_handle != NULL) {
            buffer_wr_ptr = bulk_task_handle(buffer, buffer_wr_ptr);
            if (buffer_wr_ptr < 0) {
                bulk_task_handle = NULL;
            }
        } else {
            vTaskSuspend(NULL);
            buffer_wr_ptr = 0;
        }
    }

    free(buffer);
}

static int32_t usb_vendor_bulk_command_handler(char *buffer, int32_t buffer_wr_ptr) {
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
    } else {
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    /* TODO add timeout ? */
    if (buffer_wr_ptr >= (sizeof(bulk_msg_header_t) + 2)) {
        bulk_msg_header_t * test_header = NULL;
        while ((buffer_rd_ptr + sizeof(bulk_msg_header_t) + 2) <= buffer_wr_ptr) {
            test_header = (bulk_msg_header_t*)&buffer[buffer_rd_ptr];
            if ((test_header->header != USB_PACKET_HEADER) ||
                (test_header->length > (4096 + sizeof(bulk_msg_header_t) + 2))) {
                buffer_rd_ptr++;
            } else {
                break;
            }
        }

        if ((test_header != NULL) &&
            (test_header->header == USB_PACKET_HEADER) &&
            ((buffer_wr_ptr - buffer_rd_ptr) >= test_header->length)) {
            /* handle message */
            uint16_t calc_crc = crc_calc16bitCrc((const uint8_t*)&buffer[buffer_rd_ptr],
                                                 test_header->length - 2,
                                                 0x1D0Fu);
            uint16_t mess_crc = *((uint16_t*)&buffer[buffer_rd_ptr + test_header->length - 2u]);
            if (mess_crc == calc_crc) {
                bool handled = false;
                if (command_handle != NULL) {
                    handled = command_handle(test_header->command,
                                             (const uint8_t*)&buffer[buffer_rd_ptr + sizeof(bulk_msg_header_t)],
                                             test_header->length - sizeof(bulk_msg_header_t) - 2);
                }
                if (handled == false) {
                    usb_vendor_bulk_write_error(test_header->command,
                                                MLX_FAIL_COMMAND_UNKNOWN,
                                                mlxerr_ErrorCodeToName(MLX_FAIL_COMMAND_UNKNOWN));
                }
                /* frame handled, drop it */
                buffer_rd_ptr += test_header->length;
            } else {
                buffer_rd_ptr++;
            }
        }

        if (buffer_rd_ptr != 0) {
            memcpy(&buffer[0], &buffer[buffer_rd_ptr], buffer_wr_ptr - buffer_rd_ptr);
            buffer_wr_ptr -= buffer_rd_ptr;
        }
    }

    return buffer_wr_ptr;
}

esp_err_t usb_vendor_bulk_init(void) {
    bulk_task_handle = NULL;

    bulk_rx_buf_handle = xRingbufferCreate(BULK_TASK_BUFFER_LEN, RINGBUF_TYPE_BYTEBUF);
    if (bulk_rx_buf_handle == NULL) {
        return ESP_FAIL;
    }
    bulk_tx_buf_handle = xRingbufferCreate(BULK_TASK_BUFFER_LEN, RINGBUF_TYPE_BYTEBUF);
    if (bulk_tx_buf_handle == NULL) {
        return ESP_FAIL;
    }

    xTaskCreate(usb_vendor_bulk_task, "usb_vendor_bulk_task", 2048 * 2, NULL, configMAX_PRIORITIES - 2, &taskHandle);
    return ESP_OK;
}

esp_err_t usb_vendor_bulk_start_raw(bulk_task_handle_t handle) {
    esp_err_t retval = ESP_FAIL;

    if (bulk_task_handle == NULL) {
        usb_vendor_bulk_flush_buffers();
        bulk_task_handle = handle;
        vTaskResume(taskHandle);
        retval = ESP_OK;
    }

    return retval;
}

esp_err_t usb_vendor_bulk_start_command(mlx_command_handle_t handle) {
    esp_err_t retval = ESP_FAIL;

    if (bulk_task_handle == NULL) {
        command_handle = handle;
        retval = usb_vendor_bulk_start_raw(usb_vendor_bulk_command_handler);
    }

    return retval;
}

esp_err_t usb_vendor_bulk_stop(void) {
    bulk_task_handle = NULL;
    command_handle = NULL;
    return ESP_OK;
}

void usb_vendor_bulk_write_raw(const char *buffer, uint32_t length) {
    (void)xRingbufferSend(bulk_tx_buf_handle, buffer, length, pdMS_TO_TICKS(20));
    if (tud_vendor_write_available()) {
        tud_vendor_tx_cb(0u, 0u);
    }
}

void usb_vendor_bulk_write_string(const char *buffer) {
    usb_vendor_bulk_write_raw(buffer, strlen(buffer));
}

bool usb_vendor_bulk_write_response(uint16_t command, const uint8_t * data, uint16_t datalen) {
    bool retval = false;
    uint16_t messlen = sizeof(bulk_msg_header_t) + datalen + 2u;
    uint8_t *message = calloc(messlen, sizeof(uint8_t));
    if (message != NULL) {
        bulk_msg_header_t *messheader = (bulk_msg_header_t*)message;
        messheader->header = USB_PACKET_HEADER;
        messheader->length = messlen;
        messheader->command = command;
        messheader->reserved = 0u;
        memcpy(&message[sizeof(bulk_msg_header_t)], data, datalen);
        *((uint16_t*)&message[messlen - 2u]) = crc_calc16bitCrc(message, messlen - 2u, 0x1D0Fu);
        usb_vendor_bulk_write_raw((const char*)message, messlen);
        retval = true;
    }
    return retval;
}

bool usb_vendor_bulk_write_error(uint16_t command, int error, const char *error_msg) {
    uint16_t datalen = 2u + 2u + strlen(error_msg);
    uint8_t *data = calloc(datalen, sizeof(uint8_t));
    if (data != NULL) {
        *(uint16_t*)(&data[0]) = command;
        *(uint16_t*)(&data[2]) = error;
        memcpy(&data[4], error_msg, strlen(error_msg));
        return usb_vendor_bulk_write_response(MCM_BULK_MSG_ERROR_REPORT, (const uint8_t*)data, datalen);
    }
    return false;
}

void tud_vendor_rx_cb(uint8_t itf, uint8_t const* buffer, uint16_t bufsize) {
    (void)itf;
    BaseType_t pxHigherPriorityTaskWoken;
    if (xRingbufferSendFromISR(bulk_rx_buf_handle, buffer, (size_t)bufsize,
                               &pxHigherPriorityTaskWoken) == pdFALSE) {
        ESP_LOGE(TAG, "not enough room in buffer");
    }
    /** seems CFG_TUD_TASK_QUEUE_SZ is 16 as such this callback shall not take to much time to not drop frames */

    /* if using RX buffered is enabled, we need to flush the buffer to make room for new data */
#if CFG_TUD_VENDOR_RX_BUFSIZE > 0
    tud_vendor_read_flush();
#endif
}

void tud_vendor_tx_cb(uint8_t itf, uint32_t sent_bytes) {
    (void)itf;
    (void)sent_bytes;
    size_t item_size;
    char *item = (char *)xRingbufferReceiveUpTo(bulk_tx_buf_handle,
                                                &item_size,
                                                0,
                                                tud_vendor_write_available());
    if (item != NULL) {
        (void)tud_vendor_write(item, item_size);
        tud_vendor_write_flush();
        vRingbufferReturnItem(bulk_tx_buf_handle, (void *)item);
    }
}
