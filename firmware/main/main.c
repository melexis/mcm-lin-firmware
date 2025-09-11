/**
 * @file
 * @brief The application main routines.
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
 * @details This file contains the implementations of the application main routine.
 */
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_vfs_fat.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_err.h"
#include "esp_log.h"

#include "bus_manager.h"
#include "device_info.h"
#include "device_status.h"
#include "networking.h"
#include "http_webserver.h"
#include "lin_master.h"
#include "ota_support.h"
#include "ppm_bootloader.h"
#include "power_ctrl.h"
#include "usb_device.h"
#include "webserver.h"

static const char *TAG = "main";


void app_main(void) {
    devstat_init();

    powerctrl_init();

    devinfo_init();

    /* Initialize NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    usbdevice_init();

    ESP_ERROR_CHECK(networking_init());

    busmngr_Init();

    linmaster_init();

    ppmbtl_init();

    (void)otasupport_ImageBootSuccess();

    while (1) {
        usbdevice_task();

        networking_tick();

        devstat_tick();

        vTaskDelay(pdMS_TO_TICKS(250));
    }
}
