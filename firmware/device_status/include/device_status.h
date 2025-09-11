/**
 * @file
 * @brief The device status definitions.
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
 * @details This file contains the definitions of the device status module.
 */

#ifndef DEVICE_STATUS_H_
    #define CHIP_UART_ITF_H_

/** initialize the device status module */
void devstat_init(void);

/** perform background handling for device status */
void devstat_tick(void);

/** trigger a device identification */
void devstat_startIdentify(void);

void devstat_stopIdentify(void);

#endif  /* DEVICE_STATUS_H_ */
