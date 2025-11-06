/**
 * @file
 * @brief The Melexis error code definitions.
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
 * @details This file contains the definitions of the Melexis error codes.
 */

#ifndef MLX_ERR_H_
    #define MLX_ERR_H_

/** Melexis error code enum */
typedef enum mlx_error_e {
    MLX_OK = 0,                                 /**< operation was successful */
    MLX_FAIL_TX_COLLISION = -1,                 /**< collision was detected */
    MLX_FAIL_RX_STOPBIT = -2,                   /**< stopbit error was detected */
    MLX_FAIL_RX_TIMEOUT = -3,                   /**< timeout occured during receiving */
    MLX_FAIL_CHECKSUM = -4,                     /**< checksum error was detected */
    MLX_FAIL_TX_BUS_DRV_ERROR = -5,             /**< bus did not change to low level */
    MLX_FAIL_INVALID_RESP_ID = -10,             /**< incorrect response frame id was received */
    MLX_FAIL_SERVER_ERR = -11,                  /**< internal server error occurred during handling */
    MLX_FAIL_INCORRECT_MODE = -12,              /**< uart module is not in correct mode to handle request */
    MLX_FAIL_UNKNOWN_BTL_VERSION = -13,         /**< not supported bootloader protocol version */
    MLX_FAIL_INTERFACE_NOT_FREE = -14,          /**< interface is not available at the moment */
    MLX_FAIL_INV_DATA_LEN = -0x7C,              /**< invalid message data length */
    MLX_FAIL_UNKNOWN_ERROR = -0x7D,             /**< unknown error */
    MLX_FAIL_INTERNAL = -0x7E,                  /**< internal error */
    MLX_FAIL_COMMAND_UNKNOWN = -0x7F,           /**< command is not supported */
    /* transport layer */
    MLX_FAIL_TL_NOT_EXPECTED = -0x81,           /**< tl error: an unexpected response was received (cf in sf) */
    MLX_FAIL_TL_INV_NAD = -0x82,                /**< tl error: an invalid nad was received */
    MLX_FAIL_TL_INV_PCI = -0x83,                /**< tl error: an invalid pci was received */
    MLX_FAIL_TL_INV_FRAMECOUNTER = -0x84,       /**< tl error: an invalid frame counter was received */
    MLX_FAIL_TL_INV_DATALEN = -0x85,            /**< tl error: invalid data length */
    MLX_FAIL_TL_INTERNAL = -0x86,               /**< tl error: internal error */
    /* uart bootloader chip responses : -0x100..-0x1FF */
    MLX_FAIL_BTL_INV_FRAME_ID = -0x100 - 1,     /**< btl error: invalid frame identifier */
    MLX_FAIL_BTL_INV_PL_LEN = -0x100 - 2,       /**< btl error: invalid payload length */
    MLX_FAIL_BTL_INV_PAGE_NR = -0x100 - 3,      /**< btl error: invalid page number */
    MLX_FAIL_BTL_INV_CRC = -0x100 - 4,          /**< btl error: invalid checksum */
    MLX_FAIL_BTL_INVALID_KEY = -0x100 - 16,     /**< btl error: invalid key received */
    MLX_FAIL_BTL_INVALID_SEED = -0x100 - 17,    /**< btl error: seed is expired or not generated yet */
    MLX_FAIL_BTL_MEM_PROTECTED = -0x100 - 18,   /**< btl error: memory is protected */
    MLX_FAIL_BTL_FLASH_ERASE = -0x100 - 32,     /**< btl error: flash erase failed */
    MLX_FAIL_BTL_FLASH_WRITE = -0x100 - 33,     /**< btl error: flash write failed */
    MLX_FAIL_BTL_FLASH_CRC = -0x100 - 34,       /**< btl error: flash crc failed */
    MLX_FAIL_BTL_NVRAM_ERASE = -0x100 - 48,     /**< btl error: NVRAM erase failed */
    MLX_FAIL_BTL_NVRAM_WRITE = -0x100 - 49,     /**< btl error: NVRAM write failed */
    MLX_FAIL_BTL_NVRAM_CRC = -0x100 - 50,       /**< btl error: NVRAM crc failed */
    MLX_FAIL_BTL_UNKNOWN = -0x100 - 239,        /**< btl error: unknown error */
    /* generic bootloader : -0x200..-0x2FF */
    MLX_FAIL_BTL_INV_RESP_CMD = -0x200,         /**< btl error: chip returned an unexpected command */
    MLX_FAIL_BTL_INV_RESP_LEN = -0x201,         /**< btl error: chip returned an unexpected pl length */
    MLX_FAIL_BTL_INV_HEX_FILE = -0x202,         /**< btl error: hex file could not be read */
    MLX_FAIL_BTL_MISSING_DATA = -0x203,         /**< btl error: no data for the memory in the hex file */
    MLX_FAIL_BTL_DATA_NOT_ALIGNED = -0x204,     /**< btl error: data in hex file is not page aligned */
    MLX_FAIL_BTL_VERIFY_FAILED = -0x205,        /**< btl error: verification failed */
    MLX_FAIL_BTL_SET_BAUD = -0x206,             /**< btl error: failed setting new baudrate */
    MLX_FAIL_BTL_ENTER_PROG_MODE = -207,        /**< btl error: failed entering programming mode */
    MLX_FAIL_BTL_PROGRAMMING_FAILED = -0x208,   /**< btl error: programming failed */
    MLX_FAIL_BTL_CHIP_NOT_SUPPORTED = -0x209,   /**< btl error: connected chip is not supported */
    MLX_FAIL_BTL_ACTION_NOT_SUPPORTED = -0x20A, /**< btl error: requested action is not supported by connected chip */
    /* application master : -0x300..-0x3FF */
    MLX_FAIL_APP_INV_DATA_LEN = -0x300          /**< app error: invalid message data length */
} mlx_err_t;                                    /**< Melexis error code type */

const char *mlxerr_ErrorCodeToName(mlx_err_t code);

#endif /* MLX_ERR_H_ */
