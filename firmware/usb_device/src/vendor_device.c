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
 * @details Implementations of the vendor device module.
 */
#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_log.h"

#include "tinyusb.h"
#include "tusb.h"

#include "sdkconfig.h"
#include "usb_descriptors.h"
#include "usb_vendor_bulk.h"

#include "vendor_req_hndl/usb_vendor_btl_ppm.h"
#include "vendor_req_hndl/usb_vendor_config.h"
#include "vendor_req_hndl/usb_vendor_hex_transfer.h"
#include "vendor_req_hndl/usb_vendor_identify.h"
#include "vendor_req_hndl/usb_vendor_lin_comm.h"
#include "vendor_req_hndl/usb_vendor_ota.h"
#include "vendor_req_hndl/usb_vendor_req_info.h"
#include "vendor_req_hndl/usb_vendor_reset.h"
#include "vendor_req_hndl/usb_vendor_slave_power.h"

#include "vendor_device.h"

static const char *TAG = "vendor-device";

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
    MCM_VENDOR_REQUEST_BARE_UART_MODE = 0x20,
    MCM_VENDOR_REQUEST_PWM_COMM = 0x21,
    MCM_VENDOR_REQUEST_LIN_COMM = 0x22,
    MCM_VENDOR_REQUEST_BOOTLOADER_DO_TRANSFER = 0x30,
    MCM_VENDOR_REQUEST_BOOTLOADER_DO = 0x31,
    MCM_VENDOR_REQUEST_BOOTLOADER_UART = 0x32,
    MCM_VENDOR_REQUEST_BOOTLOADER_PPM = 0x33,
    MCM_VENDOR_REQUEST_OTA_DO_TRANSFER = 0x80,
    MCM_VENDOR_REQUEST_OTA_UPDATE_BOOT_PARTITION = 0x81,
    MCM_VENDOR_REQUEST_RESTART = 0xE0,
    MCM_VENDOR_REQUEST_UNKNOWN = 0xFF,
} vendor_request_t;

/** vendor device control request handler
 *
 * @param[in]  rhport  root hub port number on which the request was received.
 * @param[in]  stage  stage of the control transfer.
 * @param[in]  request  pointer to the TinyUSB control request structure.
 * @param[in|out]  buffer  temporary buffer which can be used for data transfers (64 bytes max).
 * @retval  true  requested was recognized and handled successfully.
 * @retval  false  stall control endpoint (e.g unsupported request).
 */
typedef bool (* vendor_handle_class_control_t)(uint8_t rhport,
                                               uint8_t stage,
                                               tusb_control_request_t const * request,
                                               uint8_t * buffer);

static const struct {
    vendor_request_t id;
    vendor_handle_class_control_t handle;
} request_handlers[] = {
    {MCM_VENDOR_REQUEST_IDENTIFY, vendor_handle_class_control_request_identify},
    {MCM_VENDOR_REQUEST_INFO, vendor_handle_class_control_request_info},
    {MCM_VENDOR_REQUEST_CONFIG, vendor_handle_class_control_request_config},
    {MCM_VENDOR_REQUEST_SLAVE_CTRL, vendor_handle_class_control_request_slave_pwr},
    {MCM_VENDOR_REQUEST_LIN_COMM, vendor_handle_class_control_request_lin_comm},
    {MCM_VENDOR_REQUEST_BOOTLOADER_DO_TRANSFER, vendor_handle_class_control_request_hex_transfer},
    {MCM_VENDOR_REQUEST_BOOTLOADER_PPM, vendor_handle_class_control_request_btl_ppm},
    {MCM_VENDOR_REQUEST_OTA_DO_TRANSFER, vendor_handle_class_control_request_ota_transfer},
    {MCM_VENDOR_REQUEST_OTA_UPDATE_BOOT_PARTITION, vendor_handle_class_control_request_ota_boot},
    {MCM_VENDOR_REQUEST_RESTART, vendor_handle_class_control_request_reset},
};

static uint8_t control_data[64];

static bool vendor_handle_device_control(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request);

static bool vendor_handle_class_control(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request);


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

static bool vendor_handle_class_control(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request) {
    switch (request->bmRequestType_bit.type) {
        case TUSB_REQ_TYPE_CLASS:
            for (size_t i = 0; i < sizeof(request_handlers) / sizeof(request_handlers[0]); i++) {
                if (request_handlers[i].id == (vendor_request_t)request->bRequest) {
                    return request_handlers[i].handle(rhport, stage, request, control_data);
                }
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

esp_err_t vendor_device_init(void) {
    return usb_vendor_bulk_init();
}
