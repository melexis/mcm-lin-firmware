/**
 * @file
 * @brief The webserver routines definitions.
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
 * @details This file contains the definitions of the webserver module.
 */

#ifndef WEBSERVER_H_
    #define WEBSERVER_H_

#include "esp_https_server.h"

/** HTTP scratch buffer size */
#define SCRATCH_BUFSIZE (10240)

/** Maximum number of webserver clients */
#define MAX_WWW_CLIENTS 4

typedef struct www_server_data {
    char scratch[SCRATCH_BUFSIZE];
} www_server_data_t;

/** TCP disconnect handler
 *
 * @param[in]  server  handle of the server which will get stopped.
 */
void webserver_disconnect_handler(httpd_handle_t *server);

/** TCP connect handler
 *
 * @param[out]  server  handle of the server which is started.
 */
void webserver_connect_handler(httpd_handle_t *server);

#endif /* WEBSERVER_H_ */
