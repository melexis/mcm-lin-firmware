/**
 * @file
 * @brief The OTA support module.
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
 * @details This file contains the implementations of the OTA support module.
 */
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"

#include "ota_support.h"

static const char *TAG = "ota-support";
static esp_ota_handle_t update_handle;
static const esp_partition_t *update_partition;

esp_err_t otasupport_ImageBootSuccess(void) {
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    esp_err_t err = esp_ota_get_state_partition(running, &ota_state);
    if ((err == ESP_OK) && (ota_state == ESP_OTA_IMG_PENDING_VERIFY)) {
        ESP_LOGI(TAG, "new image marked as valid");
        err = esp_ota_mark_app_valid_cancel_rollback();
    }
    return err;
}

esp_err_t otasupport_Start(void) {
    update_partition = esp_ota_get_next_update_partition(NULL);
    esp_err_t err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
    }
    return err;
}

esp_err_t otasupport_Write(const void *data, size_t size) {
    return esp_ota_write(update_handle, data, size);
}

esp_err_t otasupport_ValidatePartition(void) {
    esp_err_t err = esp_ota_end(update_handle);
    update_handle = (esp_ota_handle_t)NULL;
    if (err != ESP_OK) {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
        } else {
            ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
        }
    }
    return err;
}

esp_err_t otasupport_UpdateBootPartition(void) {
    esp_err_t err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
    }
    return err;
}
