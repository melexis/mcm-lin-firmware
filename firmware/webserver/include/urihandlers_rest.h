/**
 * @file
 * @brief REST API URI handlers definitions.
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
 * @details This file contains the definitions of the REST API URI handlers.
 */

#ifndef URIHANDLERS_REST_H
    #define URIHANDLERS_REST_H

#include "esp_err.h"
#include "esp_http_server.h"

/** number of uri handlers consumed by REST API */
#define REST_NR_OF_URI_HANDLERS 6

/** Register all REST API URI handlers
 *
 * @param[in]  server  handle to the http(s) server to register with.
 * @param[in]  user_ctx  httpd user context.
 * @returns  error code representing the status.
 */
esp_err_t rest_register_uri(httpd_handle_t server, void *user_ctx);

#endif /* URIHANDLERS_REST_H */
