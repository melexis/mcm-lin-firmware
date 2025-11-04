/**
 * @file
 * @brief The Melexis error module.
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
 * @details This file contains the implementations of the Melexis error module.
 */
#include <string.h>

#include "mlx_err.h"

const struct {
    mlx_err_t code;
    const char *name;
} errorCodesDict[] = {
    {MLX_OK, "operation was successful"},
    {MLX_FAIL_TX_COLLISION, "Physical layer error: collision was detected"},
    {MLX_FAIL_RX_STOPBIT, "Physical layer error: stopbit error was detected"},
    {MLX_FAIL_RX_TIMEOUT, "Physical layer error: timeout occured during receiving"},
    {MLX_FAIL_CHECKSUM, "Physical layer error: checksum error was detected"},
    {MLX_FAIL_TX_BUS_DRV_ERROR, "Physical layer error: bus did not change to low level"},
    {MLX_FAIL_INVALID_RESP_ID, "Physical layer error: incorrect response frame id was received"},
    {MLX_FAIL_SERVER_ERR, "Physical layer error: internal server error occurred during handling"},
    {MLX_FAIL_INCORRECT_MODE, "Physical layer error: module is not in correct mode to handle request"},
    {MLX_FAIL_UNKNOWN_BTL_VERSION, "Physical layer error: not supported bootloader protocol version"},
    {MLX_FAIL_INTERFACE_NOT_FREE, "interface is not available at the moment"},
    {MLX_FAIL_INV_DATA_LEN, "invalid message data length"},
    {MLX_FAIL_UNKNOWN_ERROR, "unknown error"},
    {MLX_FAIL_INTERNAL, "internal error"},
    {MLX_FAIL_COMMAND_UNKNOWN, "command is not supported"},
    /* transport layer */
    {MLX_FAIL_TL_NOT_EXPECTED, "Transport Layer error: an unexpected response was received (cf in sf)"},
    {MLX_FAIL_TL_INV_NAD, "Transport Layer error: an invalid nad was received"},
    {MLX_FAIL_TL_INV_PCI, "Transport Layer error: an invalid pci was received"},
    {MLX_FAIL_TL_INV_FRAMECOUNTER, "Transport Layer error: an invalid frame counter was received"},
    {MLX_FAIL_TL_INV_DATALEN, "Transport Layer error: invalid data length"},
    {MLX_FAIL_TL_INTERNAL, "Transport Layer error: internal error"},
    /* uart bootloader chip responses : -0x100..-0x1FF */
    {MLX_FAIL_BTL_INV_FRAME_ID, "Bootloader error: invalid frame identifier"},
    {MLX_FAIL_BTL_INV_PL_LEN, "Bootloader error: invalid payload length"},
    {MLX_FAIL_BTL_INV_PAGE_NR, "Bootloader error: invalid page number"},
    {MLX_FAIL_BTL_INV_CRC, "Bootloader error: invalid checksum"},
    {MLX_FAIL_BTL_INVALID_KEY, "Bootloader error: invalid key received"},
    {MLX_FAIL_BTL_INVALID_SEED, "Bootloader error: seed is expired or not generated yet"},
    {MLX_FAIL_BTL_MEM_PROTECTED, "Bootloader error: memory is protected"},
    {MLX_FAIL_BTL_FLASH_ERASE, "Bootloader error: flash erase failed"},
    {MLX_FAIL_BTL_FLASH_WRITE, "Bootloader error: flash write failed"},
    {MLX_FAIL_BTL_FLASH_CRC, "Bootloader error: flash crc failed"},
    {MLX_FAIL_BTL_NVRAM_ERASE, "Bootloader error: NVRAM erase failed"},
    {MLX_FAIL_BTL_NVRAM_WRITE, "Bootloader error: NVRAM write failed"},
    {MLX_FAIL_BTL_NVRAM_CRC, "Bootloader error: NVRAM crc failed"},
    {MLX_FAIL_BTL_UNKNOWN, "Bootloader error: unknown error"},
    /* generic bootloader : -0x200..-0x2FF */
    {MLX_FAIL_BTL_INV_RESP_CMD, "Bootloader error: chip returned an unexpected command"},
    {MLX_FAIL_BTL_INV_RESP_LEN, "Bootloader error: chip returned an unexpected payload length"},
    {MLX_FAIL_BTL_INV_HEX_FILE, "Bootloader error: hex file could not be read"},
    {MLX_FAIL_BTL_MISSING_DATA, "Bootloader error: no data for the memory in the hex file"},
    {MLX_FAIL_BTL_DATA_NOT_ALIGNED, "Bootloader error: data in hex file is not page aligned"},
    {MLX_FAIL_BTL_VERIFY_FAILED, "Bootloader error: verification failed"},
    {MLX_FAIL_BTL_SET_BAUD, "Bootloader error: failed setting new baudrate"},
    {MLX_FAIL_BTL_ENTER_PROG_MODE, "Bootloader error: failed entering programming mode"},
    {MLX_FAIL_BTL_PROGRAMMING_FAILED, "Bootloader error: programming failed"},
    {MLX_FAIL_BTL_CHIP_NOT_SUPPORTED, "Bootloader error: connected chip is not supported"},
    {MLX_FAIL_BTL_ACTION_NOT_SUPPORTED, "Bootloader error: requested action is not supported by connected chip"},
    /* application master : -0x300..-0x3FF */
    {MLX_FAIL_APP_INV_DATA_LEN, "App error: invalid message data length"},
};

const char *mlxerr_ErrorCodeToName(mlx_err_t code) {
    size_t i;

    for (i = 0; i < sizeof(errorCodesDict) / sizeof(errorCodesDict[0]); ++i) {
        if (errorCodesDict[i].code == code) {
            return errorCodesDict[i].name;
        }
    }

    return NULL;
}
