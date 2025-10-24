/**
 * @file
 * @brief USB descriptors module
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
 * @details This file contains the implementations of the USB descriptors module.
 */
#include <stdint.h>

#include "esp_mac.h"
#include "tinyusb.h"

#include "device_info.h"

#include "usb_descriptors.h"


/* --------------------------------------------------------------------
 * Device descriptor
 * -------------------------------------------------------------------- */

tusb_desc_device_t const device_descriptor =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0210,

    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = 0x03e9,
    .idProduct          = 0x6F09,
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 0x01
};


/* --------------------------------------------------------------------
 * Configuration descriptor
 * -------------------------------------------------------------------- */

#define EPNUM_VENDOR_IN 0x04
#define EPNUM_VENDOR_OUT 0x05

#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_VENDOR_DESC_LEN)

/** HID configuration descriptor */
uint8_t const configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1,                    /* configuration number */
                          ITF_NUM_TOTAL,        /* interface count */
                          0,                    /* string index */
                          TUSB_DESC_TOTAL_LEN,  /* total length */
                          TUSB_DESC_CONFIG_ATT_SELF_POWERED,  /* attribute */
                          100),                 /* power in mA */
    TUD_VENDOR_DESCRIPTOR(ITF_NUM_VENDOR,                   /* interface number */
                          4,                                /* string index */
                          EPNUM_VENDOR_OUT,                 /* EP Out address */
                          0x80 | EPNUM_VENDOR_IN,           /* EP In address */
                          64)                               /* size */
};


/* --------------------------------------------------------------------
 * BOS descriptor
 * -------------------------------------------------------------------- */

/** Microsoft OS 2.0 registry property descriptor
 *
 * Per MS requirements https://msdn.microsoft.com/en-us/library/windows/hardware/hh450799(v=vs.85).aspx
 * device should create DeviceInterfaceGUIDs. It can be done by driver and
 * in case of real PnP solution device should expose MS "Microsoft OS 2.0
 * registry property descriptor". Such descriptor can insert any record
 * into Windows registry per device/configuration/interface. In our case it
 * will insert "DeviceInterfaceGUIDs" multistring property.
 *
 * https://developers.google.com/web/fundamentals/native-hardware/build-for-webusb/
 * (Section Microsoft OS compatibility descriptors)
 */

#define BOS_TOTAL_LEN (TUD_BOS_DESC_LEN + TUD_BOS_WEBUSB_DESC_LEN + TUD_BOS_MICROSOFT_OS_DESC_LEN)

static uint8_t const bos_descriptor[] = {
    /* Binary device Object Store descriptor */
    TUD_BOS_DESCRIPTOR(BOS_TOTAL_LEN,                       /* total length */
                       2),                                  /* number of device capability descriptors */

    /* WebUSB platform capability descriptor */
    TUD_BOS_WEBUSB_DESCRIPTOR(VENDOR_REQUEST_WEBUSB,        /* bVendorCode */
                              LANDING_PAGE_DESCRIPTOR_INDEX), /* iLandingPage */

    /* Microsoft OS 2.0 platform capability descriptor */
    TUD_BOS_MS_OS_20_DESCRIPTOR(MS_OS_20_DESC_LEN,          /* total Length of descriptor set */
                                VENDOR_REQUEST_MICROSOFT)   /* bVendorCode */
};

/** Invoked when received GET BOS DESCRIPTOR request.
 */
uint8_t const *tud_descriptor_bos_cb(void) {
    return bos_descriptor;
}

uint8_t const ms_os_20_descriptor[] = {
    /* === Microsoft OS 2.0 descriptor set header === */
    U16_TO_U8S_LE(0x000A),                                  /* wLength */
    U16_TO_U8S_LE(MS_OS_20_SET_HEADER_DESCRIPTOR),          /* wDescriptorType */
    U32_TO_U8S_LE(0x06030000),                              /* dwWindowsVersion */
    U16_TO_U8S_LE(MS_OS_20_DESC_LEN),                       /* wTotalLength */

    /* === Microsoft OS 2.0 configuration subset header === */
    U16_TO_U8S_LE(0x0008),                                  /* wLength */
    U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_CONFIGURATION),    /* wDescriptorType */
    0,                                                      /* bConfigurationValue */
    0,                                                      /* bReserved */
    U16_TO_U8S_LE(MS_OS_20_DESC_LEN - 0x0A),                /* wTotalLength */

    /* === Microsoft OS 2.0 function subset header === */
    U16_TO_U8S_LE(0x0008),                                  /* wLength */
    U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_FUNCTION),         /* wDescriptorType */
    ITF_NUM_VENDOR,                                         /* bFirstInterface */
    0,                                                      /* bReserved */
    U16_TO_U8S_LE(MS_OS_20_DESC_LEN - 0x0A - 0x08),         /* wSubsetLength */

    /* === Microsoft OS 2.0 compatible ID descriptor === */
    U16_TO_U8S_LE(0x0014),                                  /* wLength */
    U16_TO_U8S_LE(MS_OS_20_FEATURE_COMPATBLE_ID),           /* wDescriptorType */
    'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00,               /* CompatibileID */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,         /* SubCompatibleID */

    /* === Microsoft OS 2.0 registry property descriptor === */
    U16_TO_U8S_LE(MS_OS_20_DESC_LEN - 0x0A - 0x08 - 0x08 - 0x14), /* wLength */
    U16_TO_U8S_LE(MS_OS_20_FEATURE_REG_PROPERTY),           /* wDescriptorType */
    U16_TO_U8S_LE(0x0007),                                  /* wPropertyDataType */
    U16_TO_U8S_LE(0x002A),                                  /* wPropertyNameLength */
    /* PropertyName ("DeviceInterfaceGUIDs\0" in UTF-16) */
    'D', 0x00, 'e', 0x00, 'v', 0x00, 'i', 0x00, 'c', 0x00, 'e', 0x00, 'I', 0x00, 'n', 0x00, 't', 0x00, 'e', 0x00, 'r',
    0x00, 'f', 0x00, 'a', 0x00, 'c', 0x00, 'e', 0x00, 'G', 0x00, 'U', 0x00, 'I', 0x00, 'D', 0x00, 's', 0x00, 0x00, 0x00,
    U16_TO_U8S_LE(0x0050),                          /* wPropertyDataLength */
    /* PropertyData ("{3A1C31C1-32C8-49D8-8DDD-FB6C1819D30A}\0\0") */
    '{', 0x00, '3', 0x00, 'A', 0x00, '1', 0x00, 'C', 0x00, '3', 0x00, '1', 0x00, 'C', 0x00, '1', 0x00, '-', 0x00, '3',
    0x00, '2', 0x00, 'C', 0x00, '8', 0x00, '-', 0x00, '4', 0x00, '9', 0x00, 'D', 0x00, '8', 0x00, '-', 0x00, '8', 0x00,
    'D', 0x00, 'D', 0x00, 'D', 0x00, '-', 0x00, 'F', 0x00, 'B', 0x00, '6', 0x00, 'C', 0x00, '1', 0x00, '8', 0x00, '1',
    0x00, '9', 0x00, 'D', 0x00, '3', 0x00, '0', 0x00, 'A', 0x00, '}', 0x00,
    0x00, 0x00, 0x00, 0x00
};

TU_VERIFY_STATIC(sizeof(ms_os_20_descriptor) == MS_OS_20_DESC_LEN, "Incorrect size");


/* --------------------------------------------------------------------
 * String Descriptor
 * -------------------------------------------------------------------- */

/** String descriptor index */
enum {
    STRID_LANGID = 0,
    STRID_MANUFACTURER,
    STRID_PRODUCT,
    STRID_SERIAL,
    STRID_WEBUSB_INT
};

/** Device serial number */
char device_serial_nr[13] = {
    "sn"
};

/** Device string descriptor */
char const * string_descriptor[] = {
    (char[]){0x09, 0x04},   /* 0: is supported language is English (0x0409) */
    manufacturerName,       /* 1: Manufacturer */
    deviceDescription,      /* 2: Product */
    device_serial_nr,       /* 3: serial number */
    "MCM WebUSB"            /* 4: Vendor */
};


esp_err_t usbdesc_install_driver(void) {
    /* update serial number using mac address */
    uint8_t base_mac_addr[6];
    (void)esp_efuse_mac_get_default(base_mac_addr);
    snprintf(device_serial_nr, sizeof(device_serial_nr), "%02X%02X%02X%02X%02X%02X", MAC2STR(base_mac_addr));

    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = &device_descriptor,
        .string_descriptor = string_descriptor,
        .string_descriptor_count = sizeof(string_descriptor) / sizeof(string_descriptor[0]),
        .external_phy = false,
#if (TUD_OPT_HIGH_SPEED)
        TODO
        .fs_configuration_descriptor = NULL,
        .hs_configuration_descriptor = NULL,
        .qualifier_descriptor = NULL,
#else
        .configuration_descriptor = configuration_descriptor,
#endif
#if 0
        .self_powered = true,
        .vbus_monitor_io = CONFIG_USB_MEAS_VUSB_PORT_NUM = IO8
#endif
    };
    return tinyusb_driver_install(&tusb_cfg);
}
