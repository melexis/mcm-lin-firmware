/**
 * @file
 * @brief The device status routines.
 * @internal
 *
 * @copyright (C) 2024-2025 Melexis N.V.
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
 * @details This file contains the implementations of the device status routine.
 */
#include "driver/gpio.h"

#include "sdkconfig.h"

#include "wifi.h"

#include "device_status.h"

static bool booting = true;
static uint8_t identify_cnt = 0u;
static uint8_t heartbeat_cnt = 0u;

static void devstat_enableHeartbeat(void) {
    gpio_set_level((gpio_num_t)CONFIG_LED_HEARTBEAT, 0u);
}

static void devstat_disableHeartbeat(void) {
    gpio_set_level((gpio_num_t)CONFIG_LED_HEARTBEAT, 1u);
}

static void devstat_enableStatus(void) {
    gpio_set_level((gpio_num_t)CONFIG_LED_STATUS, 0u);
}

static void devstat_disableStatus(void) {
    gpio_set_level((gpio_num_t)CONFIG_LED_STATUS, 1u);
}

void devstat_init(void) {
    gpio_reset_pin((gpio_num_t)CONFIG_LED_HEARTBEAT);
    gpio_set_direction((gpio_num_t)CONFIG_LED_HEARTBEAT, GPIO_MODE_OUTPUT);
    devstat_enableHeartbeat();

    gpio_reset_pin((gpio_num_t)CONFIG_LED_STATUS);
    gpio_set_direction((gpio_num_t)CONFIG_LED_STATUS, GPIO_MODE_OUTPUT);
    devstat_enableStatus();
}

void devstat_tick(void) {
    if (booting) {
        booting = false;
        devstat_disableHeartbeat();
        devstat_disableStatus();
    }

    if (identify_cnt == 0u) {
        switch (heartbeat_cnt) {
            case 0:
                devstat_enableHeartbeat();
                break;
            case 1:
                devstat_disableHeartbeat();
                break;
            default:
                break;
        }

        heartbeat_cnt++;
        uint8_t hb_mask = 0x1;
        if (wifi_link_up()) {
            hb_mask = 0x3;
        }
        heartbeat_cnt &= hb_mask;
    } else {
        /* identification ongoing */
        if ((identify_cnt % 2) == 0) {
            devstat_disableHeartbeat();
            devstat_enableStatus();
        } else {
            devstat_enableHeartbeat();
            devstat_disableStatus();
        }

        identify_cnt--;
        if (identify_cnt == 0u) {
            devstat_stopIdentify();
        }
    }
}

void devstat_startIdentify(void) {
    identify_cnt = 20u;
}

void devstat_stopIdentify(void) {
    identify_cnt = 0u;
    heartbeat_cnt = 0u;
    devstat_disableHeartbeat();
    devstat_disableStatus();
}
