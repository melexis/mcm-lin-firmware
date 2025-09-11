<%
from datetime import datetime

def get_object_name_for_file(file):
    return f"{file.name}".replace("-", "_").replace(".", "_")

def get_path_for_file(dist, file):
    return str(file).replace(str(dist), '')
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
 * @details This file contains the implementations of the webserver webpage binaries module.
 */
#include <sys/types.h>
#include <stdint.h>

#include "www_bin.h"

% for file in files:
extern const uint8_t server_${get_object_name_for_file(file)}[] asm ("${get_object_name_for_file(file)}");
extern const ssize_t server_${get_object_name_for_file(file)}_length[] asm ("${get_object_name_for_file(file)}_length");
extern const uint8_t server_${get_object_name_for_file(file)}_start[] asm ("_binary_${get_object_name_for_file(file)}_start");
extern const uint8_t server_${get_object_name_for_file(file)}_end[] asm ("_binary_${get_object_name_for_file(file)}_end");

% endfor
www_item_t www_bin_files[] =
{
% for file in files:
    {
        .path = "${get_path_for_file(dist, file)}",
        .start = server_${get_object_name_for_file(file)},
        .length = server_${get_object_name_for_file(file)}_length
    },
% endfor
};
