/**
 * @file
 * @brief Bus manager definitions.
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
 * @details This file contains the definitions of the bus manager module.
 */

#ifndef BUS_MANAGER_H_
    #define BUS_MANAGER_H_

#include <stdbool.h>

#include "esp_err.h"

typedef enum BusUser_e {
    USER_UNKNOWN = 0,
    USER_WIFI,
    USER_USB_VENDOR,
} BusUser_t;

typedef enum UartMode_e {
    MODE_UNKNOWN = 0,
    MODE_BOOTLOADER,
    MODE_APPLICATION,
    MODE_OTA,
} BusMode_t;

/** initialize the bus manager module */
void busmngr_Init(void);

esp_err_t busmngr_ClaimInterface(BusUser_t user, BusMode_t mode);
esp_err_t busmngr_ReleaseInterface(BusUser_t user, BusMode_t mode);
bool busmngr_CheckClaim(BusUser_t user, BusMode_t mode);

bool busmngr_CheckModeClaim(BusMode_t mode);

#endif /* BUS_MANAGER_H_ */
