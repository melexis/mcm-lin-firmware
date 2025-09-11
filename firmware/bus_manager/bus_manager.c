/**
 * @file
 * @brief Bus manager module.
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
 * @details This file contains the implementations of the bus manager module.
 */
#include <string.h>

#include "driver/gpio.h"
#include "esp_log.h"

#include "ppm_bootloader.h"
#include "lin_master.h"

#include "bus_manager.h"

static const char *TAG = "bus-mngr";

static BusUser_t bus_user = USER_UNKNOWN;
static BusMode_t bus_mode = MODE_UNKNOWN;

void busmngr_Init(void) {
    gpio_reset_pin((gpio_num_t)CONFIG_BUS_VOLTAGE_5V_CTRL);
    gpio_set_level((gpio_num_t)CONFIG_BUS_VOLTAGE_5V_CTRL, 0u);
    gpio_set_direction((gpio_num_t)CONFIG_BUS_VOLTAGE_5V_CTRL, GPIO_MODE_INPUT_OUTPUT);

    gpio_reset_pin((gpio_num_t)CONFIG_BUS_VOLTAGE_VOUT_CTRL);
    gpio_set_level((gpio_num_t)CONFIG_BUS_VOLTAGE_VOUT_CTRL, 0u);
    gpio_set_direction((gpio_num_t)CONFIG_BUS_VOLTAGE_VOUT_CTRL, GPIO_MODE_INPUT_OUTPUT);
}

esp_err_t busmngr_ClaimInterface(BusUser_t user, BusMode_t mode) {
    esp_err_t retval = ESP_FAIL;
    ESP_LOGD(TAG, "claim %d %d while we have %d %d", user, mode, bus_user, bus_mode);
    if (busmngr_CheckClaim(user, mode)) {
        /* already claimed */
        retval = ESP_OK;
    } else if (bus_mode == MODE_UNKNOWN) {
        switch (mode) {
            case MODE_BOOTLOADER:
                gpio_set_level((gpio_num_t)CONFIG_BUS_VOLTAGE_VOUT_CTRL, 1u); /* TODO make configurable */
                retval = ppmbtl_enable();
                break;
            case MODE_APPLICATION:
                gpio_set_level((gpio_num_t)CONFIG_BUS_VOLTAGE_VOUT_CTRL, 1u);
                retval = linmaster_enable();
                break;
            default:
                retval = ESP_OK;
                break;
        }
        if (retval == ESP_OK) {
            bus_user = user;
            bus_mode = mode;
        }
    }
    return retval;
}

esp_err_t busmngr_ReleaseInterface(BusUser_t user, BusMode_t mode) {
    esp_err_t retval = ESP_FAIL;
    ESP_LOGD(TAG, "release %d %d while we have %d %d", user, mode, bus_user, bus_mode);
    if (busmngr_CheckClaim(user, mode)) {
        switch (bus_mode) {
            case MODE_BOOTLOADER:
                gpio_set_level((gpio_num_t)CONFIG_BUS_VOLTAGE_VOUT_CTRL, 0u);
                (void)ppmbtl_disable();
                break;
            case MODE_APPLICATION:
                gpio_set_level((gpio_num_t)CONFIG_BUS_VOLTAGE_VOUT_CTRL, 0u);
                (void)linmaster_disable();
                break;
            default:
                break;
        }
        bus_mode = MODE_UNKNOWN;
        retval = ESP_OK;
    }
    return retval;
}

bool busmngr_CheckClaim(BusUser_t user, BusMode_t mode) {
    return (bus_user == user) && (bus_mode == mode);
}

bool busmngr_CheckModeClaim(BusMode_t mode) {
    return bus_mode == mode;
}
