<%
from datetime import datetime
%>\
/**
 * @file
 * @brief The webserver webpage binaries.
 * @internal
 *
 * @copyright (C) ${datetime.now().year} Melexis N.V.
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
 * @details This file contains the definitions of the webserver webpage binaries module.
 */

#ifndef WWW_BIN_H_
    #define WWW_BIN_H_

#include <sys/types.h>
#include <stdint.h>

typedef struct {
    const char * path;
    const uint8_t * start;
    ssize_t * length;
} www_item_t;

extern www_item_t www_bin_files[${len(files)}];

#endif /* WWW_BIN_H_ */
