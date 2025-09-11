/**
 * @file
 * @brief The OTA support definitions.
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
 * @details This file contains the definitions of the OTA support module.
 */

#ifndef OTA_SUPPORT_H_
    #define OTA_SUPPORT_H_

#include "esp_err.h"

/** Indicate in ota data that the current partition started correctly
 *
 * Using this method the new partition is marked final and any roll back
 * after bootloading is discarded.
 * @returns error code representing the status of the operation.
 */
esp_err_t otasupport_ImageBootSuccess(void);

/** Start the next partition programming
 *
 * Calling this function will setup for writing of the next writeable partition, it needs
 * to be called before any subsequent calls to `otasupport_Write()`.
 * @returns error code representing the status of the operation.
 */
esp_err_t otasupport_Start(void);

/** Write a chunk of data to the next partition
 *
 * Data shall be provided in sequential order, no gaps are allowed.
 * @param[in]  data  data to be written in the memory.
 * @param[in]  size  length of the data to be written.
 * @returns error code representing the status of the operation.
 */
esp_err_t otasupport_Write(const void *data, size_t size);

/** Finish writing and validate the content of the next partition
 *
 * @returns error code representing the status of the operation.
 */
esp_err_t otasupport_ValidatePartition(void);

/** Update ota data to indicate next partition as bootable
 *
 * @returns error code representing the status of the operation.
 */
esp_err_t otasupport_UpdateBootPartition(void);

#endif /* OTA_SUPPORT_H_ */
