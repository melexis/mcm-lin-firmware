/**
 * @file
 * @brief vendor device module
 * @internal
 *
 * @copyright (C) 2025 Melexis N.V.
 *
 * Melexis N.V. is supplying this code for use with Melexis N.V. processor based microcontrollers only.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY,
 * INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.  MELEXIS N.V. SHALL NOT IN ANY CIRCUMSTANCES,
 * BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 * @endinternal
 *
 * @ingroup application
 *
 * @details This file contains the implementations of the vendor device module.
 */
#include <stdint.h>
#include <string.h>

#include "sdkconfig.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "tusb.h"

#include "bus_manager.h"
#include "device_info.h"
#include "device_status.h"
#include "intelhex.h"
#include "lin_master.h"
#include "mlx_err.h"
#include "networking.h"
#include "ota_support.h"
#include "power_ctrl.h"
#include "ppm_bootloader.h"
#include "usb_descriptors.h"
#include "wifi.h"

#include "vendor_device.h"

static const char *TAG = "vendor-device";

static TaskHandle_t taskHandle;

static BusMode_t cur_mode = MODE_UNKNOWN;
static bool btl_transfer_mode = false;
static bool ota_transfer_mode = false;

#define URL "melexis.github.io/mcm-ui"

const tusb_desc_webusb_url_t desc_url =
{
    .bLength         = 3 + sizeof(URL) - 1,
    .bDescriptorType = 3, /* WEBUSB_URL type */
    .bScheme         = 1, /* 0: 'http://', 1: 'https://', 255: '' */
    .url             = URL
};

typedef enum vendor_request_e {
    MCM_VENDOR_REQUEST_IDENTIFY = 0x00,
    MCM_VENDOR_REQUEST_INFO = 0x01,
    MCM_VENDOR_REQUEST_CONFIG = 0x02,
    MCM_VENDOR_REQUEST_SLAVE_CTRL = 0x10,
    MCM_VENDOR_REQUEST_LIN_COMM = 0x20,
    MCM_VENDOR_REQUEST_BOOTLOADER_DO_TRANSFER = 0x30,
    MCM_VENDOR_REQUEST_BOOTLOADER_DO = 0x31,
    MCM_VENDOR_REQUEST_OTA_DO_TRANSFER = 0x80,
    MCM_VENDOR_REQUEST_OTA_UPDATE_BOOT_PARTITION = 0x81,
    MCM_VENDOR_REQUEST_RESTART = 0xE0,
    MCM_VENDOR_REQUEST_UNKNOWN = 0xFF,
} vendor_request_t;

typedef enum vendor_request_info_e {
    MCM_INFO_VERSION = 0x00,
    MCM_INFO_RESET_REASON = 0x01,
    MCM_INFO_UP_TIME = 0x02,
    MCM_INFO_UNKNOWN = 0xFF,
} vendor_request_info_t;

typedef enum vendor_request_config_e {
    MCM_CONFIG_HOSTNAME = 0x00,
    MCM_CONFIG_WIFI_SSID = 0x01,
    MCM_CONFIG_WIFI_PASS = 0x02,
    MCM_CONFIG_WIFI_MAC = 0x03,
    MCM_CONFIG_WIFI_IP_INFO = 0x04,
    MCM_CONFIG_UNKNOWN = 0xFF,
} vendor_request_config_t;

typedef enum vendor_request_slave_ctrl_e {
    MCM_SLAVE_CTRL_POWER_DOWN = 0x00,
    MCM_SLAVE_CTRL_POWER_UP = 0x01,
    MCM_SLAVE_CTRL_V_SUPPLY = 0x02,
    MCM_SLAVE_CTRL_V_BUS = 0x03,
    MCM_SLAVE_CTRL_C_BUS = 0x04,
} vendor_request_slave_ctrl_t;

typedef enum vendor_request_lin_comm_e {
    MCM_LIN_COMM_SEND_WAKEUP = 0x00,
    MCM_LIN_COMM_HANDLE_MESSAGE = 0x01,
    MCM_LIN_ERROR_REPORT = 0xFFFF,
} vendor_request_lin_comm_t;

vendor_request_t last_request = MCM_VENDOR_REQUEST_UNKNOWN;

static uint8_t control_data[64];

static uint32_t btl_ihex_ext_address = 0u;
static ihexContainer_t * btl_ihex_first_container = NULL;
static ihexContainer_t * btl_ihex_prev_container = NULL;

#define BULK_TASK_BUFFER_LEN 204800
RingbufHandle_t bulk_rx_buf_handle;
RingbufHandle_t bulk_tx_buf_handle;

#define USB_PACKET_HEADER  0xAA55AA55
typedef struct bulk_lin_header_s {
    uint32_t header;
    uint16_t length;
    uint16_t command;
    uint32_t reserved;
} bulk_lin_header_t;

typedef struct bulk_lin_transfer_message_s {
    uint16_t baudrate;
    uint8_t datalength;
    uint8_t m2s;
    uint8_t enhanced_crc;
    uint8_t frameid;
    uint8_t payload[8];
} bulk_lin_transfer_message_t;

typedef struct vendor_btl_request_s {
    uint32_t bitrate;         /**< baudrate to be used during bootloader operations */
    uint8_t manpow;           /**< 1: manual power cycling */
    uint8_t broadcast;        /**< 1: bootloading shall be done in broadcast mode */
    uint8_t memory;           /**< memory type to perform action on (0: NVRAM; 1: flash) */
    uint8_t action;           /**< action type to perform (0: program; 1: verify) */
} vendor_btl_request_t;


/** Flush the Vendor device ring buffers */
static void vendor_flush_ring_buffers(void);

/** LIN bulk USB communication handler
 *
 * @param[in]  buffer  buffer used for storing temp data.
 * @param[in]  buffer_wr_ptr  pointer to the current writing location in buffer.
 * @retval  new write pointer in buffer.
 */
static uint16_t tud_vendor_bulk_task_lin_mode(char *buffer, size_t buffer_wr_ptr);

/** Bootloader bulk USB communication handler
 *
 * @param[in]  buffer  buffer used for storing temp data.
 * @param[in]  buffer_wr_ptr  pointer to the current writing location in buffer.
 * @retval  new write pointer in buffer.
 */
static uint16_t tud_vendor_bulk_task_bootloader_mode(char *buffer, size_t buffer_wr_ptr);

/** OTA bulk USB communication handler
 *
 * @param[in]  buffer  buffer used for storing temp data.
 * @param[in]  buffer_wr_ptr  pointer to the current writing location in buffer.
 * @retval  new write pointer in buffer.
 */
static uint16_t tud_vendor_bulk_task_ota_mode(char *buffer, size_t buffer_wr_ptr);

/** USB vendor bulk endpoint handler task
 *
 * @param[in]  arg  arguments for the task.
 */
static void tud_vendor_bulk_task(void *arg);

static bool vendor_handle_device_control(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request);

static bool vendor_handle_class_control_request_info(uint8_t rhport,
                                                     uint8_t stage,
                                                     tusb_control_request_t const * request);

static bool vendor_handle_class_control_request_config(uint8_t rhport,
                                                       uint8_t stage,
                                                       tusb_control_request_t const * request);

static bool vendor_handle_class_control_request_slave_pwr(uint8_t rhport,
                                                          uint8_t stage,
                                                          tusb_control_request_t const * request);

static bool vendor_handle_class_control(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request);

static void tud_vendor_bulk_write(const char *buffer, uint32_t length);

static void tud_vendor_bulk_write_string(const char *buffer);


static void vendor_flush_ring_buffers(void) {
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

static mlx_err_t tud_vendor_do_btl_action(void) {
    mlx_err_t retval = MLX_FAIL_UNKNOWN;

    if (busmngr_ClaimInterface(USER_USB_VENDOR, MODE_BOOTLOADER) == ESP_OK) {
        vendor_btl_request_t * btl_request = (vendor_btl_request_t*)control_data;

        ppm_memory_t memory = PPM_MEM_INVALID;
        if (btl_request->memory == 0) {
            memory = PPM_MEM_NVRAM;
        } else if (btl_request->memory == 1) {
            memory = PPM_MEM_FLASH;
        }

        ppm_action_t action = PPM_ACT_INVALID;
        if (btl_request->action == 0) {
            action = PPM_ACT_PROGRAM;
        } else if (btl_request->action == 1) {
            action = PPM_ACT_VERIFY;
        }

        ppm_err_t ppmstat = ppmbtl_doAction(btl_request->manpow != 0,
                                            btl_request->broadcast != 0,
                                            btl_request->bitrate,
                                            memory,
                                            action,
                                            btl_ihex_first_container);
        if (ppmstat == PPM_OK) {
            retval = MLX_OK;
        }
    } else {
        retval = MLX_FAIL_INTERFACE_NOT_FREE;
    }
    (void)busmngr_ReleaseInterface(USER_USB_VENDOR, MODE_BOOTLOADER);

    return retval;
}

static bool bulk_lin_response(uint16_t command, const uint8_t * data, uint16_t datalen) {
    bool retval = false;
    uint16_t messlen = sizeof(bulk_lin_header_t) + datalen + 2u;
    uint8_t *message = calloc(messlen, sizeof(uint8_t));
    if (message != NULL) {
        bulk_lin_header_t *messheader = (bulk_lin_header_t*)message;
        messheader->header = USB_PACKET_HEADER;
        messheader->length = messlen;
        messheader->command = command;
        messheader->reserved = 0u;
        memcpy(&message[sizeof(bulk_lin_header_t)], data, datalen);
        *((uint16_t*)&message[messlen - 2u]) = crc_calc16bitCrc(message, messlen - 2u, 0x1D0Fu);
        tud_vendor_bulk_write((const char*)message, messlen);
        retval = true;
    }
    return retval;
}

static bool bulk_lin_report_error(uint16_t command, const char *buffer) {
    return bulk_lin_response(command, (const uint8_t*)buffer, strlen(buffer));
}

static bool bulk_lin_command_handler(uint16_t command, const uint8_t * data, uint16_t datalen) {
    bool retval = false;
    ESP_LOGI(TAG, "LIN command %d", (int)command);
    switch (command) {
        case MCM_LIN_COMM_SEND_WAKEUP:
    ESP_LOGI(TAG, "send wakeup %d", *((uint16_t*)data));
            lin_error_code_t error = linmaster_sendWakeUp(*((uint16_t*)data));
            if (error == ERROR_LIN_NONE) {
                retval = true;
            } else {
                bulk_lin_report_error(command, mlxerr_ErrorCodeToName(MLX_FAIL_SERVER_ERR));
            }
            break;

        case MCM_LIN_COMM_HANDLE_MESSAGE:
        {
            bulk_lin_transfer_message_t * message = (bulk_lin_transfer_message_t*)data;
            if (message->m2s != 0u) {
                /* M2S message */
                lin_error_code_t error = linmaster_sendM2S(message->baudrate,
                                                           message->enhanced_crc != 0u,
                                                           message->frameid,
                                                           message->payload,
                                                           message->datalength);

    ESP_LOGI(TAG, "m2s error %d %d", (int)message->frameid, (int)error);
                if (error == ERROR_LIN_NONE) {
                    retval = true;
                } else {
                    /* report error */
                }
            } else {
                /* S2M message */
                uint8_t *resp = calloc(message->datalength, sizeof(uint8_t));
                if (resp != NULL) {
                    lin_error_code_t error = linmaster_sendS2M(message->baudrate,
                                                               message->enhanced_crc != 0u,
                                                               message->frameid,
                                                               resp,
                                                               message->datalength);

        ESP_LOGI(TAG, "s2m error %d %d", (int)message->frameid, (int)error);
                    if (error == ERROR_LIN_NONE) {
                        /* Report the received message */
                        bulk_lin_response(command, resp, message->datalength);
                        retval = true;
                    } else {
                        /* report error */
                    }
                    free(resp);
                }
            }
            break;
        }

        default:
            break;
    }

    return retval;
}

static uint16_t tud_vendor_bulk_task_lin_mode(char *buffer, size_t buffer_wr_ptr) {
    size_t buffer_rd_ptr = 0;
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

    ESP_LOGI(TAG, "ptr wr %d rd %d size %d", (int)buffer_wr_ptr, (int)buffer_rd_ptr, (int)sizeof(bulk_lin_header_t));

    /* TODO add timeout ? */
    if (buffer_wr_ptr >= (sizeof(bulk_lin_header_t) + 2)) {
        bulk_lin_header_t * test_header = NULL;
        while ((buffer_rd_ptr + sizeof(bulk_lin_header_t) + 2) <= buffer_wr_ptr) {
            test_header = (bulk_lin_header_t*)&buffer[buffer_rd_ptr];
            if ((test_header->header != USB_PACKET_HEADER) || (test_header->length > (4096 + sizeof(bulk_lin_header_t) + 2))) {
                buffer_rd_ptr++;
            } else {
                break;
            }
        }

        if ((test_header != NULL) &&
            (test_header->header == USB_PACKET_HEADER) &&
            ((buffer_wr_ptr - buffer_rd_ptr) >= test_header->length)) {
            /* handle message */
            uint16_t calc_crc = crc_calc16bitCrc((const uint8_t*)&buffer[buffer_rd_ptr], test_header->length - 2, 0x1D0Fu);
            uint16_t mess_crc = *((uint16_t*)&buffer[buffer_rd_ptr + test_header->length - 2u]);
            if (mess_crc == calc_crc) {
                bool result = bulk_lin_command_handler(test_header->command,
                                                       (const uint8_t*)&buffer[buffer_rd_ptr + sizeof(bulk_lin_header_t)],
                                                       test_header->length - sizeof(bulk_lin_header_t) - 2);
                if (result) {
                    (void)bulk_lin_response(test_header->command, NULL, 0u);
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

static uint16_t tud_vendor_bulk_task_bootloader_mode(char *buffer, size_t buffer_wr_ptr) {
    switch (last_request) {
        case MCM_VENDOR_REQUEST_BOOTLOADER_DO_TRANSFER:
        {
            size_t buffer_rd_ptr = 0;
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
                cur_mode = MODE_UNKNOWN;
                tud_vendor_bulk_write_string("OK\n");
            }
            break;
        }

        case MCM_VENDOR_REQUEST_BOOTLOADER_DO:
            tud_vendor_bulk_write_string("STARTED\n");
            mlx_err_t result = tud_vendor_do_btl_action();
            if (result == MLX_OK) {
                tud_vendor_bulk_write_string("OK\n");
            } else {
                tud_vendor_bulk_write_string("FAIL: ");
                tud_vendor_bulk_write_string(mlxerr_ErrorCodeToName(result));
                tud_vendor_bulk_write_string("\n");
            }
            last_request = MCM_VENDOR_REQUEST_UNKNOWN;
            cur_mode = MODE_UNKNOWN;
            buffer_wr_ptr = 0;
            break;

        default:
            vTaskDelay(pdMS_TO_TICKS(50));
            buffer_wr_ptr = 0;
            break;
    }

    return buffer_wr_ptr;
}

static uint16_t tud_vendor_bulk_task_ota_mode(char *buffer, size_t buffer_wr_ptr) {
    switch (last_request) {
        case MCM_VENDOR_REQUEST_OTA_DO_TRANSFER:
        {
            size_t item_size = 0;
            const char *item = (const char *)xRingbufferReceiveUpTo(bulk_rx_buf_handle,
                                                                    &item_size,
                                                                    pdMS_TO_TICKS(25),
                                                                    BULK_TASK_BUFFER_LEN / 4);
            if (item != NULL) {
                if (item_size > 0) {
                    if (otasupport_Write(item, item_size) != ESP_OK) {
                        cur_mode = MODE_UNKNOWN;
                        ota_transfer_mode = false;
                        tud_vendor_bulk_write_string("FAIL\n");
                        ESP_LOGI(TAG, "ota transfer failed while writing");
                    }
                }
                vRingbufferReturnItem(bulk_rx_buf_handle, (void *)item);
            }

            if (item_size == 0) {
                if (ota_transfer_mode) {
                    tud_vendor_bulk_write_string("EMPTY\n");
                    vTaskDelay(pdMS_TO_TICKS(50));
                } else {
                    cur_mode = MODE_UNKNOWN;
                    if (otasupport_ValidatePartition() == ESP_OK) {
                        tud_vendor_bulk_write_string("VALID\n");
                        ESP_LOGI(TAG, "ota transfer done and image valid");
                    } else {
                        tud_vendor_bulk_write_string("FAIL\n");
                        ESP_LOGI(TAG, "ota transfer done and image invalid");
                    }
                }
            }
            break;
        }

        default:
            vTaskDelay(pdMS_TO_TICKS(50));
            buffer_wr_ptr = 0;
            break;
    }

    return buffer_wr_ptr;
}

static void tud_vendor_bulk_task(void *arg) {
    (void)arg;
    char *buffer = (char*) malloc(BULK_TASK_BUFFER_LEN);
    size_t buffer_wr_ptr = 0u;

    while (1) {
        if (cur_mode == MODE_APPLICATION) {
            buffer_wr_ptr = tud_vendor_bulk_task_lin_mode(buffer, buffer_wr_ptr);
        } else if (cur_mode == MODE_BOOTLOADER) {
            buffer_wr_ptr = tud_vendor_bulk_task_bootloader_mode(buffer, buffer_wr_ptr);
        } else if (cur_mode == MODE_OTA) {
            buffer_wr_ptr = tud_vendor_bulk_task_ota_mode(buffer, buffer_wr_ptr);
        } else {
            vTaskSuspend(NULL);
            buffer_wr_ptr = 0u;
        }
    }

    free(buffer);
}

static bool vendor_handle_device_control(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request) {
    if (stage != CONTROL_STAGE_SETUP) {
        /* nothing to do with DATA & ACK stage */
        return true;
    }

    switch (request->bmRequestType_bit.type) {
        case TUSB_REQ_TYPE_VENDOR:
            switch (request->bRequest) {
                case VENDOR_REQUEST_WEBUSB:
                    /* WebUSB device requests */
                    if ((request->wValue == LANDING_PAGE_DESCRIPTOR_INDEX) && (request->wIndex == 2)) {
                        /* GET_URL: This request fetches the landing page URL descriptor */
                        return tud_control_xfer(rhport,
                                                request,
                                                (void*)(uintptr_t) &desc_url,
                                                desc_url.bLength);
                    }
                    break;

                case VENDOR_REQUEST_MICROSOFT:
                    if (request->wIndex == 7) {
                        /* MS_OS_20_DESCRIPTOR_INDEX: Get Microsoft OS 2.0 compatible descriptor */
                        return tud_control_xfer(rhport,
                                                request,
                                                (void*)(uintptr_t) ms_os_20_descriptor,
                                                MS_OS_20_DESC_LEN);
                    }
                    break;

                default:
                    break;
            }
            break;

        case TUSB_REQ_TYPE_CLASS:
        case TUSB_REQ_TYPE_STANDARD:
        /* setup commands which are handled by the tinyusb stack */
        default:
            break;
    }

    /* stall unknown request */
    return false;
}

static bool vendor_handle_class_control_request_info(uint8_t rhport,
                                                     uint8_t stage,
                                                     tusb_control_request_t const * request) {
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

static bool vendor_handle_class_control_request_config(uint8_t rhport,
                                                       uint8_t stage,
                                                       tusb_control_request_t const * request) {
    switch ((vendor_request_config_t)request->wValue) {
        case MCM_CONFIG_HOSTNAME:
            if (stage == CONTROL_STAGE_SETUP) {
                if (request->bmRequestType_bit.direction == TUSB_DIR_OUT) {
                    /* set hostname */
                    return tud_control_xfer(rhport, request, control_data, request->wLength);
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
                    networking_set_hostname((const char *)control_data, true);
                }
                return true;
            }
            break;

        case MCM_CONFIG_WIFI_SSID:
            if (stage == CONTROL_STAGE_SETUP) {
                if (request->bmRequestType_bit.direction == TUSB_DIR_OUT) {
                    if (request->wLength <= 32) {
                        /* set ssid */
                        return tud_control_xfer(rhport, request, control_data, request->wLength);
                    }
                } else {
                    /* get ssid */
                    if (wifi_get_ssid((char*)&control_data, sizeof(control_data)) == ESP_OK) {
                        return tud_control_xfer(rhport,
                                                request,
                                                (void*)(uintptr_t) control_data,
                                                strlen((const char*)control_data));
                    }
                }
            } else if (stage == CONTROL_STAGE_DATA) {
                if (request->bmRequestType_bit.direction == TUSB_DIR_OUT) {
                    wifi_set_ssid((const char *)control_data, true);
                }
                return true;
            }
            break;

        case MCM_CONFIG_WIFI_PASS:
            if (stage == CONTROL_STAGE_SETUP) {
                if (request->bmRequestType_bit.direction == TUSB_DIR_OUT) {
                    if (request->wLength <= 64) {
                        /* set pass */
                        return tud_control_xfer(rhport, request, control_data, request->wLength);
                    }
                } else {
                    /* get pass */
                    if (wifi_get_password((char*)&control_data, sizeof(control_data)) == ESP_OK) {
                        return tud_control_xfer(rhport,
                                                request,
                                                (void*)(uintptr_t) control_data,
                                                strlen((const char*)control_data));
                    }
                }
            } else if (stage == CONTROL_STAGE_DATA) {
                if (request->bmRequestType_bit.direction == TUSB_DIR_OUT) {
                    wifi_set_password((const char *)control_data, true);
                }
                return true;
            }
            break;

        case MCM_CONFIG_WIFI_MAC:
            if (stage == CONTROL_STAGE_SETUP) {
                if (request->bmRequestType_bit.direction == TUSB_DIR_IN) {
                    (void)wifi_get_mac(&control_data[0]);
                    return tud_control_xfer(rhport,
                                            request,
                                            (void*)(uintptr_t) control_data,
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
                    if (wifi_get_ip_info((uint32_t*)&control_data[0],
                                         (uint32_t*)&control_data[4],
                                         (uint32_t*)&control_data[8]) == ESP_OK) {
                        return tud_control_xfer(rhport,
                                                request,
                                                (void*)(uintptr_t) control_data,
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

static bool vendor_handle_class_control_request_slave_pwr(uint8_t rhport,
                                                          uint8_t stage,
                                                          tusb_control_request_t const * request) {
    switch ((vendor_request_slave_ctrl_t)request->wValue) {
        case MCM_SLAVE_CTRL_POWER_DOWN:
        case MCM_SLAVE_CTRL_POWER_UP:
            if (request->bmRequestType_bit.direction == TUSB_DIR_OUT) {
                if ((stage == CONTROL_STAGE_SETUP) && (request->wLength == 0)) {
                    if (request->wValue == MCM_SLAVE_CTRL_POWER_UP) {
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
                    control_data[0] = (uint8_t)powerctrl_slaveEnabled();
                    return tud_control_xfer(rhport,
                                            request,
                                            (void*)(uintptr_t) control_data,
                                            1u);
                } else if (stage == CONTROL_STAGE_DATA) {
                    return true;
                }
            }
            break;

        case MCM_SLAVE_CTRL_V_SUPPLY:
            if (request->bmRequestType_bit.direction == TUSB_DIR_IN) {
                if (stage == CONTROL_STAGE_SETUP) {
                    int32_t * ptr = (int32_t*)&control_data[0];
                    *ptr = powerctrl_getSupplyVoltage();
                    return tud_control_xfer(rhport,
                                            request,
                                            (void*)(uintptr_t) control_data,
                                            sizeof(int32_t));
                } else if (stage == CONTROL_STAGE_DATA) {
                    return true;
                }
            }
            break;

        case MCM_SLAVE_CTRL_V_BUS:
            if (request->bmRequestType_bit.direction == TUSB_DIR_IN) {
                if (stage == CONTROL_STAGE_SETUP) {
                    int32_t * ptr = (int32_t*)&control_data[0];
                    *ptr = powerctrl_getBusVoltage();
                    return tud_control_xfer(rhport,
                                            request,
                                            (void*)(uintptr_t) control_data,
                                            sizeof(int32_t));
                } else if (stage == CONTROL_STAGE_DATA) {
                    return true;
                }
            }
            break;

        case MCM_SLAVE_CTRL_C_BUS:
            if (request->bmRequestType_bit.direction == TUSB_DIR_IN) {
                if (stage == CONTROL_STAGE_SETUP) {
                    int32_t * ptr = (int32_t*)&control_data[0];
                    *ptr = powerctrl_getOutputCurrent();
                    return tud_control_xfer(rhport,
                                            request,
                                            (void*)(uintptr_t) control_data,
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

static bool vendor_handle_class_control(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request) {
    switch (request->bmRequestType_bit.type) {
        case TUSB_REQ_TYPE_CLASS:
            last_request = request->bRequest;
            switch ((vendor_request_t)request->bRequest) {
                case MCM_VENDOR_REQUEST_IDENTIFY:
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
                    break;

                case MCM_VENDOR_REQUEST_INFO:
                    return vendor_handle_class_control_request_info(rhport, stage, request);

                case MCM_VENDOR_REQUEST_SLAVE_CTRL:
                    return vendor_handle_class_control_request_slave_pwr(rhport, stage, request);

                case MCM_VENDOR_REQUEST_LIN_COMM:
                    if (request->bmRequestType_bit.direction == TUSB_DIR_OUT) {
                        if ((stage == CONTROL_STAGE_SETUP) && (request->wLength == 0)) {
                            if (request->wValue == 1) {
                                ESP_LOGI(TAG, "enable lin mode");
                                if (busmngr_ClaimInterface(USER_USB_VENDOR, MODE_APPLICATION) == ESP_OK) {
                                    powerctrl_slaveEnable();
                                    vendor_flush_ring_buffers();
                                    cur_mode = MODE_APPLICATION;
                                    vTaskResume(taskHandle);
                                }
                                return tud_control_xfer(rhport, request, control_data, request->wLength);
                            } else {
                                ESP_LOGI(TAG, "disable lin mode");
                                (void)busmngr_ReleaseInterface(USER_USB_VENDOR, MODE_APPLICATION);
                                powerctrl_slaveDisable();
                                cur_mode = MODE_UNKNOWN;
                                return tud_control_status(rhport, request);
                            }
                        }
                    }
                    break;

                case MCM_VENDOR_REQUEST_BOOTLOADER_DO_TRANSFER:
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
                                vendor_flush_ring_buffers();
                                cur_mode = MODE_BOOTLOADER;
                                vTaskResume(taskHandle);
                                btl_transfer_mode = true;
                            } else {
                                ESP_LOGI(TAG, "stop hex transfer");
                                btl_transfer_mode = false;
                            }
                            return tud_control_status(rhport, request);
                        }
                    }
                    break;

                case MCM_VENDOR_REQUEST_BOOTLOADER_DO:
                    if (request->bmRequestType_bit.direction == TUSB_DIR_OUT) {
                        if (stage == CONTROL_STAGE_SETUP) {
                            if (request->wLength == sizeof(vendor_btl_request_t)) {
                                ESP_LOGI(TAG, "request perform btl action");
                                return tud_control_xfer(rhport, request, control_data, request->wLength);
                            }
                        } else if (stage == CONTROL_STAGE_DATA) {
                            /* control_data is used to set programming specs */
                            ESP_LOGI(TAG, "btl data received");
                            vendor_flush_ring_buffers();
                            cur_mode = MODE_BOOTLOADER;
                            vTaskResume(taskHandle);
                            return true;
                        }
                    }
                    break;

                case MCM_VENDOR_REQUEST_CONFIG:
                    return vendor_handle_class_control_request_config(rhport, stage, request);

                case MCM_VENDOR_REQUEST_OTA_DO_TRANSFER:
                    if (request->bmRequestType_bit.direction == TUSB_DIR_OUT) {
                        if ((stage == CONTROL_STAGE_SETUP) && (request->wLength == 0)) {
                            if (request->wValue == 1) {
                                ESP_LOGI(TAG, "do ota transfer");
                                if (otasupport_Start() == ESP_OK) {
                                    vendor_flush_ring_buffers();
                                    cur_mode = MODE_OTA;
                                    vTaskResume(taskHandle);
                                    ota_transfer_mode = true;
                                    return tud_control_status(rhport, request);
                                }
                            } else {
                                ESP_LOGI(TAG, "stop ota transfer");
                                ota_transfer_mode = false;
                                return tud_control_status(rhport, request);
                            }
                        }
                    }
                    break;

                case MCM_VENDOR_REQUEST_OTA_UPDATE_BOOT_PARTITION:
                    if (request->bmRequestType_bit.direction == TUSB_DIR_OUT) {
                        if ((stage == CONTROL_STAGE_SETUP) && (request->wLength == 0)) {
                            if (otasupport_UpdateBootPartition() == ESP_OK) {
                                return tud_control_status(rhport, request);
                            }
                        }
                    }
                    break;

                case MCM_VENDOR_REQUEST_RESTART:
                    if (request->bmRequestType_bit.direction == TUSB_DIR_OUT) {
                        if ((stage == CONTROL_STAGE_SETUP) && (request->wLength == 0)) {
                            /* reset will be done in ack */
                            return tud_control_status(rhport, request);
                        } else if (stage == CONTROL_STAGE_ACK) {
                            ESP_LOGI(TAG, "Restart system");
                            esp_restart();
                        }
                    }
                    break;

                default:
                    break;
            }
            break;

        case TUSB_REQ_TYPE_VENDOR:
        case TUSB_REQ_TYPE_STANDARD:
        /* setup commands which are handled by the tinyusb stack */
        default:
            break;
    }

    if (stage == CONTROL_STAGE_ACK) {
        /* nothing to do with ACK stage */
        return true;
    }

    /* stall unknown request */
    return false;
}

static void tud_vendor_bulk_write(const char *buffer, uint32_t length) {
    (void)xRingbufferSend(bulk_tx_buf_handle, buffer, length, pdMS_TO_TICKS(20));
    if (tud_vendor_write_available()) {
        tud_vendor_tx_cb(0u, 0u);
    }
}

static void tud_vendor_bulk_write_string(const char *buffer) {
    tud_vendor_bulk_write(buffer, strlen(buffer));
}

bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request) {
    ESP_LOGI(TAG, "control xfer rhport:%d, stage:%d", rhport, stage);

    if (request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_DEVICE) {
        return vendor_handle_device_control(rhport, stage, request);
    } else if (request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_INTERFACE) {
        return vendor_handle_class_control(rhport, stage, request);
    } else if (request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_ENDPOINT) {
        /* no endpoint control handling */
    }

    /* stall unknown request */
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

esp_err_t vendor_device_init(void) {
    bulk_rx_buf_handle = xRingbufferCreate(BULK_TASK_BUFFER_LEN, RINGBUF_TYPE_BYTEBUF);
    if (bulk_rx_buf_handle == NULL) {
        return ESP_FAIL;
    }
    bulk_tx_buf_handle = xRingbufferCreate(BULK_TASK_BUFFER_LEN, RINGBUF_TYPE_BYTEBUF);
    if (bulk_tx_buf_handle == NULL) {
        return ESP_FAIL;
    }

    xTaskCreate(tud_vendor_bulk_task, "tud_vendor_bulk_task", 2048 * 2, NULL, configMAX_PRIORITIES - 2, &taskHandle);
    return ESP_OK;
}
