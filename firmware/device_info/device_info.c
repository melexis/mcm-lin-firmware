/**
 * @file
 * @brief The device information routines.
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
 * @details This file contains the implementations of the device information routine.
 */
#include <string.h>

#include "esp_app_desc.h"
#include "esp_idf_version.h"

#include "device_info.h"

const char manufacturerName[] = "Melexis Technologies NV";
const char deviceShortName[] = "MCM-LIN";
const char deviceDescription[] = "Melexis Compact Master LIN";

void devinfo_init(void) {
    /* todo load description from config memory? */
}

const char * devinfo_manufacturerName(void) {
    return manufacturerName;
}

const char * devinfo_deviceShortName(void) {
    return deviceShortName;
}

const char * devinfo_deviceDescription(void) {
    return deviceDescription;
}

const char * devinfo_firmwareVersion(void) {
    const esp_app_desc_t* app_desc = esp_app_get_description();
    return app_desc->version;
}

const char * devinfo_idfVersion(void) {
    return esp_get_idf_version();
}
