/**
 * @file
 * @brief The LIN transport layer.
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
 * @details This file contains the implementations of the LIN transport layer.
 */
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"
#include "lin_errors.h"
#include "lin_master.h"

#include "lin_tl.h"

static const char *TAG = "lin-tl";

typedef enum {
    sf_nad = 0,
    sf_pci,
    sf_sid,
    sf_data_0,
    sf_data_1,
    sf_data_2,
    sf_data_3,
    sf_data_4,
    sf_max_len
} single_frame_bits;

typedef enum {
    ff_nad = 0,
    ff_pci,
    ff_len,
    ff_sid,
    ff_data_0,
    ff_data_1,
    ff_data_2,
    ff_data_3,
    ff_max_len
} first_frame_bits;

typedef enum {
    cf_nad = 0,
    cf_pci,
    cf_data_0,
    cf_data_1,
    cf_data_2,
    cf_data_3,
    cf_data_4,
    cf_data_5,
    cf_max_len
} cons_frame_bits;

/** Requests the transmission of 8 bytes of data in one single diagnostic request frame
 *
 * @param[in]  data         data to sent.
 * @param[in]  data_length  number of bytes to sent.
 * @param[in]  baudrate     baudrate to be used for this frame (in bps).
 *
 * @return  an error code representing the result of the operation.
 */
static lin_error_code_t ld_put_raw(const uint8_t * data, int baudrate) {
    return linmaster_sendM2S(baudrate, false, 0x3Cu, data, 8);
}

/** Reads a single diagnostic response frame data on the bus
 *
 * @param[out]  data         pointer to buffer to store received data (to be deleted by caller).
 * @param[out]  data_length  number of bytes to received.
 * @param[in]   baudrate     baudrate to be used for this frame (in bps).
 *
 * @return  an error code representing the result of the operation.
 */
static lin_error_code_t ld_get_raw(uint8_t * data, int baudrate) {
    return linmaster_sendS2M(baudrate, false, 0x3Du, data, 8);
}

/** Send a master diagnostic request frame to a slave
 *
 * The call packs the information specified by data into one or multiple diagnostic
 * frames. The frames are transmitted to the slave node with the address NAD.
 *
 * The value of the service identifier (SID) shall be the first byte in the data area.
 * Length of data must be in the range of 1 to 4095 bytes. The length shall also include
 * the SID value, i.e. message length plus one.
 *
 * @param[in]  nad          slave node address (nad) to be used.
 * @param[in]  data         data to be transfered (1..4095 bytes).
 * @param[in]  data_length  number of bytes to sent.
 * @param[in]  baudrate     baudrate to be used for this communication (in bps).
 * @param[in]  inter_frame  inter-frame time for this segmented messages (in us).
 *
 * @return  an error code representing the result of the operation.
 */
lin_error_code_t ld_send_message(uint8_t nad,
                                 const uint8_t * data,
                                 size_t data_length,
                                 int baudrate,
                                 int inter_frame) {
    lin_error_code_t retval;

    if ((data_length <= 4095) && (data != NULL)) {
        lin_error_code_t lin_stat;

        if (data_length > 6) {
            /* First Frame (FF) + Consecutive Frame (CF) */
            uint8_t pci = 0x10 + ((data_length / 256) % 16);

            /* Sent FF */
            uint8_t ff_data[8];
            ff_data[ff_nad] = nad;
            ff_data[ff_pci] = pci;
            ff_data[ff_len] = (uint8_t)(data_length % 256);

            for (uint8_t ctr = 0u; ctr < 5u; ctr++) {
                ff_data[ff_sid + ctr] = data[ctr];
            }

            lin_stat = ld_put_raw(ff_data, baudrate);

            /* Rest of data is transmitted in cf frames */
            uint16_t max_cf = (data_length - 5) % 6;
            uint16_t current_byte = 5u;
            uint8_t cf_data[8];

            for (uint16_t cf_ctr = 0u; cf_ctr < max_cf; cf_ctr++) {
                if (lin_stat == ERROR_LIN_NONE) {
                    /* Prepare CF */
                    pci = 0x20 + ((cf_ctr + 1) & 0xF);

                    uint8_t nr_bytes = 6u;
                    if ((current_byte + 6u) > data_length) {
                        nr_bytes = data_length - current_byte;
                    }

                    cf_data[cf_nad] = nad;
                    cf_data[cf_pci] = pci;

                    for (uint8_t ctr = 0u; ctr < 6u; ctr++) {
                        if (ctr < nr_bytes) {
                            cf_data[cf_data_0 + ctr] = data[current_byte];
                        } else {
                            cf_data[cf_data_0 + ctr] = 0xFFu;
                        }
                        current_byte++;
                    }

                    if (inter_frame != 0) {
                        vTaskDelay(inter_frame / portTICK_PERIOD_MS);
                    }

                    /* Sent CF */
                    lin_stat = ld_put_raw(cf_data, baudrate);
                } else {
                    break;
                }
            }
        } else {
            /* Single Frame (SF) */
            uint8_t pci = data_length;

            uint8_t sf_data[8];
            sf_data[sf_nad] = nad;
            sf_data[sf_pci] = pci;

            for (uint8_t ctr = 0u; ctr < 6; ctr++) {
                if (ctr < data_length) {
                    sf_data[sf_sid + ctr] = data[ctr];
                } else {
                    sf_data[sf_sid + ctr] = 0xFFu;
                }
            }

            lin_stat = ld_put_raw(sf_data, baudrate);
        }

        /* report eventual LIN frame transmission issues */
        retval = lin_stat;
    } else {
        retval = ERROR_LIN_TL_INV_DATALEN;
    }

    return retval;
}

/** Receive a diagnostic slave response frame from the slave
 *
 * Receive a slave response frame from a slave node.
 * RSID will be the first byte in the data area.
 * Length will be in the range of 1 to 4095 bytes. RSID is included in the length.
 *
 * @param[in,out]  nad       expected nad, nad of responding device is returned in case input was 0x7F.
 * @param[out]  data         data bytes to write (the caller needs to take care to delete the object pointed by).
 * @param[out]  data_length  number of bytes to write.
 * @param[in]   baudrate     baudrate to be used for this communication (in bps).
 * @param[in]   inter_frame  inter-frame time for this segmented messages (in us).
 *
 * @return  an error code representing the result of the operation.
 */
lin_error_code_t ld_receive_message(uint8_t * nad,
                                    uint8_t ** data,
                                    size_t * data_length,
                                    int baudrate,
                                    int inter_frame) {
    lin_error_code_t retval = ERROR_LIN_UNKNOWN;
    uint16_t cf_ctr = 0u;
    size_t byte_ctr = 0u;

    if ((data != NULL) && (data_length != NULL) && (nad != NULL)) {
        *data = NULL;
        *data_length = 0u;

        while (retval == ERROR_LIN_UNKNOWN) {
            uint8_t frame_data[8];

            esp_err_t status = ld_get_raw(frame_data, baudrate);

            if (status == ERROR_LIN_NONE) {
                if (*nad == 0x7Fu) {
                    *nad = frame_data[sf_nad];
                }

                if (frame_data[sf_nad] == *nad) {
                    uint8_t pci_r = frame_data[sf_pci];

                    switch (pci_r & 0xF0) {
                        case 0x00:
                            /* Single Frame (SF) */

                            /* frame_data[0] = nad
                             * frame_data[1] = pci (1..6)
                             * frame_data[2] = rsid = sid + 0x40
                             * frame_data[3] = data
                             * frame_data[4] = data
                             * frame_data[5] = data
                             * frame_data[6] = data
                             * frame_data[7] = data
                             */
                            if (*data == NULL) {
                                uint8_t * payload = calloc(sizeof(uint8_t), pci_r);

                                if (payload != NULL) {
                                    memcpy(&payload[0], &frame_data[sf_sid], pci_r);
                                    *data = payload;
                                    *data_length = (size_t)pci_r;

                                    retval = ERROR_LIN_NONE;
                                } else {
                                    retval = ERROR_LIN_TL_INTERNAL;
                                    ESP_LOGE(TAG, "SF mem allocation failed");
                                }
                            } else {
                                /* we did not expect to get a SF */
                                retval = ERROR_LIN_TL_NOT_EXPECTED;
                                ESP_LOGE(TAG, "SF received while in multi frame mode");
                            }
                            break;

                        case 0x10:
                            /* First Frame (FF) */

                            /* frame_data[0] = nad
                             * frame_data[1] = pci (0x10 + len/256)
                             * frame_data[2] = len
                             * frame_data[3] = rsid
                             * frame_data[4] = data
                             * frame_data[5] = data
                             * frame_data[6] = data
                             * frame_data[7] = data
                             */
                            if (*data == NULL) {
                                *data_length = (((size_t)pci_r % 16) * 256) + (size_t)frame_data[2];
                                uint8_t * payload = calloc(sizeof(uint8_t), *data_length);

                                if (payload != NULL) {
                                    memcpy(&payload[0], &frame_data[ff_sid], ff_max_len - ff_sid);
                                    byte_ctr = ff_max_len - ff_sid;
                                    *data = payload;
                                } else {
                                    retval = ERROR_LIN_TL_INTERNAL;
                                    ESP_LOGE(TAG, "FF mem allocation failed");
                                }
                            } else {
                                /* we did not expect to get a FF */
                                retval = ERROR_LIN_TL_NOT_EXPECTED;
                                ESP_LOGE(TAG, "FF received while in multi frame mode");
                            }

                            break;

                        case 0x20:
                            /* Consecutive Frame (CF) */

                            /* dev_resp[0] = nad
                             * dev_resp[1] = pci (0x20 + cf_frame_ctr)
                             * dev_resp[2] = data
                             * dev_resp[3] = data
                             * dev_resp[4] = data
                             * dev_resp[5] = data
                             * dev_resp[6] = data
                             * dev_resp[7] = data
                             */
                            if (*data != NULL) {
                                cf_ctr = (cf_ctr + 1u) & 0xF;
                                if ((pci_r & 0xFu) == cf_ctr) {
                                    for (uint8_t ctr = cf_data_0; ctr < cf_max_len; ctr++) {
                                        if (byte_ctr < *data_length) {
                                            (*data)[byte_ctr++] = frame_data[ctr];
                                        }
                                    }

                                    if (byte_ctr >= *data_length) {
                                        retval = ERROR_LIN_NONE;
                                    }
                                } else {
                                    retval = ERROR_LIN_TL_INV_FRAMECOUNTER;
                                    ESP_LOGE(TAG, "Invalid frame counter received");
                                }
                            } else {
                                /* we did not expect to get a CF */
                                retval = ERROR_LIN_TL_NOT_EXPECTED;
                                ESP_LOGE(TAG, "CF received while not in multi frame mode");
                            }

                            break;

                        default:
                            /* invalid pci */
                            retval = ERROR_LIN_TL_INV_PCI;
                            ESP_LOGE(TAG, "Invalid pci received");
                            break;
                    }
                } else {
                    retval = ERROR_LIN_TL_INV_NAD;
                    ESP_LOGE(TAG, "Unexpected NAD in response");
                }

                if ((retval == ERROR_LIN_UNKNOWN) && (inter_frame != 0)) {
                    vTaskDelay(inter_frame / portTICK_PERIOD_MS);
                }
            } else {
                /* something went wrong during LIN frame transmission */
                retval = status;
                ESP_LOGE(TAG, "No slave response received");
            }
        }

        if (retval != ERROR_LIN_NONE) {
            if (*data != NULL) {
                free(*data);
                *data = NULL;
                *data_length = 0u;
            }
        }
    } else {
        /* no pointer provided, this makes no sense */
    }

    return retval;
}
