/**
 * @file
 * @brief The power control module.
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
 * @details This file contains the implementations of the power control module.
 */
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

#include "sdkconfig.h"

#include "power_ctrl.h"

#if (CONFIG_SLAVE_POWER_SENSE != 4)
#error "slave power adc channel is wrong"
#endif

adc_oneshot_unit_handle_t adc1_handle;
adc_oneshot_unit_handle_t adc2_handle;
#define ADC_CHANNEL_CUR_SENSE ADC_CHANNEL_3
#define ADC_CHANNEL_VSUPPLY ADC_CHANNEL_7
#define ADC_CHANNEL_VBUS ADC_CHANNEL_6

void powerctrl_init(void) {
    gpio_reset_pin((gpio_num_t)CONFIG_SLAVE_POWER_CTRL);
    gpio_set_level((gpio_num_t)CONFIG_SLAVE_POWER_CTRL, 0u);
    gpio_set_direction((gpio_num_t)CONFIG_SLAVE_POWER_CTRL, GPIO_MODE_INPUT_OUTPUT);

    adc_oneshot_unit_init_cfg_t init_adc1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_adc1, &adc1_handle));

    adc_oneshot_unit_init_cfg_t init_adc2 = {
        .unit_id = ADC_UNIT_2,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_adc2, &adc2_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_CUR_SENSE, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc2_handle, ADC_CHANNEL_VSUPPLY, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc2_handle, ADC_CHANNEL_VBUS, &config));
}

void powerctrl_slaveEnable(void) {
    gpio_set_level((gpio_num_t)CONFIG_SLAVE_POWER_CTRL, 1u);
}

void powerctrl_slaveDisable(void) {
    gpio_set_level((gpio_num_t)CONFIG_SLAVE_POWER_CTRL, 0u);
}

bool powerctrl_slaveEnabled(void) {
    return gpio_get_level((gpio_num_t)CONFIG_SLAVE_POWER_CTRL) == 1u;
}

int32_t powerctrl_getOutputCurrent(void) {
    int32_t voltage = -1;
    int adc_raw;
    if (adc_oneshot_read(adc1_handle, ADC_CHANNEL_CUR_SENSE, &adc_raw) == ESP_OK) {
        voltage = adc_raw * 3100 / 4095;   /* TODO use eFuse calibrations */
    }
    return voltage;
}

int32_t powerctrl_getSupplyVoltage(void) {
    int32_t voltage = -1;
    int adc_raw;
    if (adc_oneshot_read(adc2_handle, ADC_CHANNEL_VSUPPLY, &adc_raw) == ESP_OK) {
        voltage = adc_raw * 3100 / 4095;   /* TODO use eFuse calibrations */
        voltage *= 13;
    }
    return voltage;
}

int32_t powerctrl_getBusVoltage(void) {
    int32_t voltage = -1;
    int adc_raw;
    if (adc_oneshot_read(adc2_handle, ADC_CHANNEL_VBUS, &adc_raw) == ESP_OK) {
        voltage = adc_raw * 3100 / 4095;   /* TODO use eFuse calibrations */
        voltage *= 13;
    }
    return voltage;
}

void ppmbtl_chipPower(bool enable) {
    if (enable) {
        powerctrl_slaveEnable();
    } else {
        powerctrl_slaveDisable();
    }
}

bool ppmbtl_chipPowered(void) {
    return powerctrl_slaveEnabled();
}
