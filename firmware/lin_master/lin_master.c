/**
 * @file
 * @brief The LIN master module.
 * @internal
 *
 * @copyright (C) 2023-2025 Melexis N.V.
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
 * @details This file contains the implementations of the LIN master module.
 */
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "hal/uart_hal.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"

#include "lin_errors.h"
#include "sdkconfig.h"

#include "lin_master.h"

static const char *TAG = "lin";

#define BUF_SIZE (128)

/** Update a frame identifier with its parity bits
 *
 * @param[in]  frame_id  the frame id to be converted to protected fid.
 *
 * @return  the protected frame identifier.
 */
uint8_t fid_2_pid(uint8_t frame_id) {
    uint8_t u8Return;
    uint8_t u8P0 = 0u;
    uint8_t u8P1 = 0u;

    /* P0 = ID0 xor ID1 xor ID2 xor ID4 */
    if ((frame_id & (1u << 0)) != 0u) {
        u8P0 = ~u8P0;
    }

    if ((frame_id & (1u << 1)) != 0u) {
        u8P0 = ~u8P0;
    }

    if ((frame_id & (1u << 2)) != 0u) {
        u8P0 = ~u8P0;
    }

    if ((frame_id & (1u << 4)) != 0u) {
        u8P0 = ~u8P0;
    }

    /* P1 = ~(ID1 xor ID3 xor ID4 xor ID5) */
    if ((frame_id & (1u << 1)) != 0u) {
        u8P1 = ~u8P1;
    }

    if ((frame_id & (1u << 3)) != 0u) {
        u8P1 = ~u8P1;
    }

    if ((frame_id & (1u << 4)) != 0u) {
        u8P1 = ~u8P1;
    }

    if ((frame_id & (1u << 5)) != 0u) {
        u8P1 = ~u8P1;
    }

    u8P1 = ~u8P1;

    /* delete msbit's */
    u8Return = 0x3f & frame_id;

    /* add parity bits */
    if (u8P0 != 0u) {
        u8Return |= (1u << 6);
    }

    if (u8P1 != 0u) {
        u8Return |= (1u << 7);
    }

    return u8Return;
}

/** Calculate the checksum for this frame
 *
 * @param[in]  enhanced_crc  use enhanced checksum type.
 * @param[in]  pid  protected frame identifier.
 * @param[in]  frame_data  frame data bytes.
 * @param[in]  data_len  number of frame data bytes.
 *
 * @return  the calculated checksum.
 */
static uint8_t linmaster_calcCrc(bool enhanced_crc, uint8_t pid, uint8_t* frame_data, uint8_t data_len) {
    uint16_t u16Checksum;

    if (enhanced_crc == true) {
        u16Checksum = pid;
    } else {
        u16Checksum = 0u;
    }

    for (uint8_t byte_nr = 0u; byte_nr < data_len; byte_nr++) {
        u16Checksum += frame_data[byte_nr];

        /* cfr LIN spec v2.2A section 2.3.1.5 Checksum
         * ....
         * Eight bit sum with carry is equivalent to sum all values and subtract 255
         * every time the sum is greater or equal to 256
         */
        if (u16Checksum >= 256u) {
            u16Checksum -= 255u;
        }
    }

    u16Checksum = ~u16Checksum;

    return ((uint8_t)u16Checksum);
}

static int linmaster_GenerateBreak(int baudrate) {
    /* See https://esp32.com/viewtopic.php?p=98456#p98456
     * Temporarily change baudrate to slightly lower and send a 0 byte
     * to get the right baudrate, we want 9 bits (1 start + 8 data) to take the time of 13 bits (one break)
     */
    uint8_t dummy = 0;
    uart_set_baudrate(CONFIG_LIN_MASTER_UART_PORT_NUM, (baudrate * 9) / 13);
    uart_write_bytes(CONFIG_LIN_MASTER_UART_PORT_NUM, (char *)&dummy, 1);
    uart_wait_tx_done(CONFIG_LIN_MASTER_UART_PORT_NUM, 2);
    uart_wait_tx_done(CONFIG_LIN_MASTER_UART_PORT_NUM, 2);
    uart_set_baudrate(CONFIG_LIN_MASTER_UART_PORT_NUM, baudrate);

    #if 0
    uart_hal_clr_intsts_mask(&(uart_context[uart_num].hal), UART_INTR_TX_BRK_DONE);
    UART_ENTER_CRITICAL_ISR(&(uart_context[uart_num].spinlock));
    uart_hal_tx_break(&(uart_context[uart_num].hal), p_uart->tx_brk_len);
    uart_hal_ena_intr_mask(&(uart_context[uart_num].hal), UART_INTR_TX_BRK_DONE);
    UART_EXIT_CRITICAL_ISR(&(uart_context[uart_num].spinlock));
    #endif
    return 0;
}

lin_error_code_t linmaster_sendWakeUp(int pulse_time) {
    uint8_t dummy = 0;
    uart_set_baudrate(CONFIG_LIN_MASTER_UART_PORT_NUM, 9000000 / pulse_time);
    uart_write_bytes(CONFIG_LIN_MASTER_UART_PORT_NUM, (char *)&dummy, 1);
    uart_wait_tx_done(CONFIG_LIN_MASTER_UART_PORT_NUM, 2);
    uart_wait_tx_done(CONFIG_LIN_MASTER_UART_PORT_NUM, 2);
    return ERROR_LIN_NONE;
}

lin_error_code_t linmaster_sendM2S(int baudrate, bool enhanced_crc, uint8_t fid, const uint8_t* data, size_t data_len) {
    int frame_len = 2 + data_len + 1;
    uint8_t frame[11];
    frame[0] = 0x55u;
    frame[1] = fid_2_pid(fid);
    memcpy(&frame[2], data, data_len);
    frame[frame_len - 1] = linmaster_calcCrc(enhanced_crc, frame[1], &frame[2], data_len);

    uart_flush_input(CONFIG_LIN_MASTER_UART_PORT_NUM);
    uart_set_baudrate(CONFIG_LIN_MASTER_UART_PORT_NUM, baudrate);
/*    (void)uart_write_bytes_with_break(CONFIG_LIN_MASTER_UART_PORT_NUM, NULL, 0, 19); */
    linmaster_GenerateBreak(baudrate);
    uart_write_bytes(CONFIG_LIN_MASTER_UART_PORT_NUM, (const void *)frame, frame_len);

    /* use RX to read back our full frame and have a kind of uart_wait_tx_done() functionality */

    /* RX also sees the break as 0x00 so adding 1 more byte */
    uint8_t rx_frame[12];
    int rx_len = uart_read_bytes(CONFIG_LIN_MASTER_UART_PORT_NUM, rx_frame, frame_len + 1, 150 / portTICK_PERIOD_MS);

    ESP_LOG_BUFFER_HEX_LEVEL(TAG, rx_frame, rx_len, ESP_LOG_INFO);

    return ERROR_LIN_NONE;
}

lin_error_code_t linmaster_sendS2M(int baudrate, bool enhanced_crc, uint8_t fid, uint8_t* data, size_t data_len) {
    lin_error_code_t retval = ERROR_LIN_UNKNOWN;
    int frame_len = 2 + data_len + 1;
    uint8_t frame[2];
    frame[0] = 0x55u;
    frame[1] = fid_2_pid(fid);

    uart_flush_input(CONFIG_LIN_MASTER_UART_PORT_NUM);
    uart_set_baudrate(CONFIG_LIN_MASTER_UART_PORT_NUM, baudrate);
/*    (void)uart_write_bytes_with_break(CONFIG_LIN_MASTER_UART_PORT_NUM, NULL, 0, 19); */
    linmaster_GenerateBreak(baudrate);
    uart_write_bytes(CONFIG_LIN_MASTER_UART_PORT_NUM, (const void *)frame, 2);

    /* RX also sees the break as 0x00 so adding 1 more byte */
    uint8_t rx_frame[12];
    int rx_len = uart_read_bytes(CONFIG_LIN_MASTER_UART_PORT_NUM, rx_frame, frame_len + 1, 150 / portTICK_PERIOD_MS);

    if (rx_len >= 0) {
        if ((rx_len - 4) == data_len) {
            uint8_t calc_crc = linmaster_calcCrc(enhanced_crc, rx_frame[2], &rx_frame[3], data_len);
            if (calc_crc == rx_frame[rx_len - 1]) {
                memcpy(data, &rx_frame[3], data_len);

                ESP_LOG_BUFFER_HEX_LEVEL(TAG, rx_frame, rx_len, ESP_LOG_INFO);

                retval = ERROR_LIN_NONE;
            } else {
                retval = ERROR_LIN_CHECKSUM;
            }
        } else {
            retval = ERROR_LIN_RX_TIMEOUT;
        }
    } else {
        retval = ERROR_LIN_SERVER_ERR;
    }

    return retval;
}

static void linmaster_setSleepMode(void) {
    gpio_set_level(CONFIG_LIN_MASTER_SLEEP, 0u);
}

static void linmaster_setNormalMode(void) {
    gpio_set_level(CONFIG_LIN_MASTER_SLEEP, 1u);
}

void linmaster_init(void) {
    gpio_reset_pin(CONFIG_LIN_MASTER_SLEEP);
    gpio_set_direction(CONFIG_LIN_MASTER_SLEEP, GPIO_MODE_OUTPUT);

    linmaster_disable();

    /* Configure parameters of an UART driver, communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = 19200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1_5,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    ESP_ERROR_CHECK(uart_driver_install(CONFIG_LIN_MASTER_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(CONFIG_LIN_MASTER_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(CONFIG_LIN_MASTER_UART_PORT_NUM, CONFIG_LIN_MASTER_TXD, CONFIG_LIN_MASTER_RXD,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

esp_err_t linmaster_enable(void) {
    linmaster_setNormalMode();
    return ESP_OK;
}

esp_err_t linmaster_disable(void) {
    linmaster_setSleepMode();
    return ESP_OK;
}
