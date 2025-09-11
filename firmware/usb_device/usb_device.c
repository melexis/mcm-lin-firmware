/**
 * @file
 * @brief USB HID device module
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
 * @details This file contains the implementations of the USB HID device module.
 */
#include "esp_err.h"
#include "esp_log.h"

#include "usb_descriptors.h"
#include "vendor_device.h"
#include "sdkconfig.h"

#include "usb_device.h"

static const char *TAG = "usb-device";


void usbdevice_init(void) {
    ESP_LOGI(TAG, "USB initialization");

    ESP_ERROR_CHECK(usbdesc_install_driver());

    ESP_ERROR_CHECK(vendor_device_init());
}

void usbdevice_task(void) {
#if CONFIG_TINYUSB_NO_DEFAULT_TASK
    tud_task();
#endif
}

/** Invoked when device is mounted */
void tud_mount_cb(void) {
    ESP_LOGI(TAG, "mounted");
}

/** Invoked when device is unmounted */
void tud_umount_cb(void) {
    ESP_LOGI(TAG, "unmounted");
    /* release slave interface claims */
}

/** Invoked when usb bus is suspended
 *
 * remote_wakeup_en: if host allow us to perform remote wakeup
 * Within 7ms, device must draw an average of current less than 2.5 mA from bus
 */
void tud_suspend_cb(bool remote_wakeup_en) {
    (void)remote_wakeup_en;
    ESP_LOGI(TAG, "suspend");
    /* release slave interface claims */
}

/** Invoked when usb bus is resumed */
void tud_resume_cb(void) {
    ESP_LOGI(TAG, "resume");
}
