/**
 * @file
 * @brief The LIN transport layer definitions.
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
 * @details This file contains the definitions of the LIN transport layer module.
 */

#ifndef LIN_TL_H_
    #define LIN_TL_H_

#include <stdint.h>
#include "lin_errors.h"

lin_error_code_t ld_send_message(uint8_t nad,
                                 const uint8_t * data,
                                 size_t data_length,
                                 int baudrate,
                                 int inter_frame);
lin_error_code_t ld_receive_message(uint8_t * nad,
                                    uint8_t ** data,
                                    size_t * data_length,
                                    int baudrate,
                                    int inter_frame);

#endif /* LIN_TL_H_ */
