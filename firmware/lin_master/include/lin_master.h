/**
 * @file
 * @brief The LIN master routines definitions.
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
 * @details This file contains the definitions of the LIN master module.
 */

#ifndef LIN_MASTER_H_
    #define LIN_MASTER_H_

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#include "lin_errors.h"

void linmaster_init(void);
esp_err_t linmaster_enable(void);
esp_err_t linmaster_disable(void);

/** Generate a LIN wake up pulse
 *
 * @param[in]  pulse_time  LIN dominant bus level time for the wake up pulse [us] (default 200us).
 * @returns  Error code representing the result of the action.
 */
lin_error_code_t linmaster_sendWakeUp(int pulse_time);
lin_error_code_t linmaster_sendM2S(int baudrate, bool enhanced_crc, uint8_t fid, const uint8_t* data, size_t data_len);
lin_error_code_t linmaster_sendS2M(int baudrate, bool enhanced_crc, uint8_t fid, uint8_t* data, size_t data_len);

#endif /* LIN_MASTER_H_ */
