/**
 * @file
 * @brief The device information definitions.
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
 * @details This file contains the definitions of the device information module.
 */

#ifndef DEVICE_INFO_H_
    #define DEVICE_INFO_H_

/** manufacturer name string */
extern const char manufacturerName[];

/** device description string */
extern const char deviceDescription[];

/** initialize the device info module */
void devinfo_init(void);

/** get the manufacturer name string
 *
 * @returns pointer to the manufacturer name string
 */
const char * devinfo_manufacturerName(void);

/** get the device short name string
 *
 * @returns pointer to the device short name string
 */
const char * devinfo_deviceShortName(void);

/** get the device description string
 *
 * @returns pointer to the device description string
 */
const char * devinfo_deviceDescription(void);

/** get the firmware version string
 *
 * @returns pointer to the firmware string
 */
const char * devinfo_firmwareVersion(void);

/** get the idf version string
 *
 * @returns pointer to the idf string
 */
const char * devinfo_idfVersion(void);

#endif  /* DEVICE_INFO_H_ */
