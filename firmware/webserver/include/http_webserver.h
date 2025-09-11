/**
 * @file
 * @brief The http webserver routines definitions.
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
 * @details This file contains the definitions of the http webserver module.
 */

#ifndef HTTP_WEBSERVER_H_
    #define HTTP_WEBSERVER_H_

#include "esp_http_server.h"

/** TCP disconnect handler
 *
 * @param[in]  server  handle of the server which will get stopped.
 */
void http_webserver_disconnect_handler(httpd_handle_t *server);

/** TCP connect handler
 *
 * @param[out]  server  handle of the server which is started.
 */
void http_webserver_connect_handler(httpd_handle_t *server);

#endif /* HTTP_WEBSERVER_H_ */
