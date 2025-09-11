/**
 * @file
 * @brief Websocket handlers definitions.
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
 * @details This file contains the definitions of the websocket handlers.
 */

#ifndef WEBSOCKET_H
    #define WEBSOCKET_H

#include "esp_err.h"
#include "esp_http_server.h"

/** number of uri handlers consumed by websocket */
#define WSS_NR_OF_URI_HANDLERS 1

/** Initialize the websocket module
 *
 * @param[out]  httpd  httpd configuration to update.
 * @returns  error code representing the success of the operation.
 */
esp_err_t wss_init(httpd_config_t *httpd);

/** Register all websocket handlers
 *
 * @param[in]  server  handle to the http(s) server to register with.
 * @param[in]  user_ctx  httpd user context.
 * @returns  error code representing the success of the operation.
 */
esp_err_t wss_register_uri(httpd_handle_t server, void *user_ctx);

/** Start the websocket handler
 *
 * @param[in]  server  handle to the http(s) server with which the websocket will work.
 * @returns  error code representing the success of the operation.
 */
esp_err_t wss_start(httpd_handle_t server);

/** Stop and deinit the websocket module
 *
 * @param[in]  server  handle to the http(s) server with which the websocket works.
 * @returns  error code representing the success of the operation.
 */
esp_err_t wss_stop(httpd_handle_t server);

#endif /* WEBSOCKET_H */
