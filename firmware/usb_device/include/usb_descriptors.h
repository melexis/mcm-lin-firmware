/**
 * @file
 * @brief USB descriptors module definitions.
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
 * @details This file contains the definitions of the USB descriptors module.
 */

#ifndef USB_DESCRIPTORS_H_
    #define USB_DESCRIPTORS_H_

#include <stdint.h>

enum {
    ITF_NUM_CDC,
    ITF_NUM_CDC_DATA,
    ITF_NUM_VENDOR,
    ITF_NUM_TOTAL
};

enum {
    VENDOR_REQUEST_WEBUSB = 1,
    VENDOR_REQUEST_MICROSOFT = 2
};

/** Microsoft OS 2.0 platform capability descriptor length */
#define MS_OS_20_DESC_LEN 0xB2

#define LANDING_PAGE_DESCRIPTOR_INDEX 1

extern uint8_t const ms_os_20_descriptor[];

esp_err_t usbdesc_install_driver(void);

#endif  /* USB_DESCRIPTORS_H_ */
