/**
 * @file
 * @brief The LIN error code definitions.
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
 * @details This file contains the definitions of the LIN error codes.
 */

#ifndef LIN_ERRORS_H_
    #define LIN_ERRORS_H_

/** frame error code enum */
typedef enum lin_error_e {
    ERROR_LIN_NONE = 0,                         /**< operation was successful */
    ERROR_LIN_TX_COLLISION = 1,                 /**< collision was detected */
    ERROR_LIN_RX_STOPBIT = 2,                   /**< stopbit error was detected */
    ERROR_LIN_RX_TIMEOUT = 3,                   /**< timeout occured during receiving */
    ERROR_LIN_CHECKSUM = 4,                     /**< checksum error was detected */
    ERROR_LIN_TX_BUS_DRV_ERROR = 5,             /**< bus did not change to low level */
    ERROR_LIN_INVALID_RESP_ID = 10,             /**< incorrect response frame id was received */
    ERROR_LIN_SERVER_ERR = 11,                  /**< internal server error occurred during handling */
    ERROR_LIN_TL_NOT_EXPECTED = 0x81,           /**< tl error: an unexpected response was received (cf in sf) */
    ERROR_LIN_TL_INV_NAD = 0x82,                /**< tl error: an invalid nad was received */
    ERROR_LIN_TL_INV_PCI = 0x83,                /**< tl error: an invalid pci was received */
    ERROR_LIN_TL_INV_FRAMECOUNTER = 0x84,       /**< tl error: an invalid frame counter was received */
    ERROR_LIN_TL_INV_DATALEN = 0x85,            /**< tl error: invalid data length */
    ERROR_LIN_TL_INTERNAL = 0x86,               /**< tl error: internal error */
    ERROR_LIN_UNKNOWN = 0x87,                   /**< tl error: unknown error */
} lin_error_code_t;                             /**< lin error code type */

#endif /* LIN_ERRORS_H_ */
