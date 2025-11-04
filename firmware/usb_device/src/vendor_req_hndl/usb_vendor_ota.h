/**
 * @file
 * @brief vendor device class - ota interface.
 * @internal
 *
 * @copyright (C) 2025 Melexis N.V.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @endinternal
 *
 * @ingroup lib_usb_device
 *
 * @details Definitions of the vendor device class for the ota interface.
 * @{
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "tinyusb.h"

#ifdef __cplusplus
extern "C" {
#endif

/** vendor device control request handler for the ota transfer interface
 *
 * @param[in]  rhport  root hub port number on which the request was received.
 * @param[in]  stage  stage of the control transfer.
 * @param[in]  request  pointer to the TinyUSB control request structure.
 * @param[in|out]  buffer  temporary buffer which can be used for data transfers (64 bytes max).
 * @retval  true  requested was recognized and handled successfully.
 * @retval  false  stall control endpoint (e.g unsupported request).
 */
bool vendor_handle_class_control_request_ota_transfer(uint8_t rhport,
                                                      uint8_t stage,
                                                      tusb_control_request_t const * request,
                                                      uint8_t * buffer);

/** vendor device control request handler for the ota update boot partition interface
 *
 * @param[in]  rhport  root hub port number on which the request was received.
 * @param[in]  stage  stage of the control transfer.
 * @param[in]  request  pointer to the TinyUSB control request structure.
 * @param[in|out]  buffer  temporary buffer which can be used for data transfers (64 bytes max).
 * @retval  true  requested was recognized and handled successfully.
 * @retval  false  stall control endpoint (e.g unsupported request).
 */
bool vendor_handle_class_control_request_ota_boot(uint8_t rhport,
                                                  uint8_t stage,
                                                  tusb_control_request_t const * request,
                                                  uint8_t * buffer);

/** @} */

#ifdef __cplusplus
}
#endif
