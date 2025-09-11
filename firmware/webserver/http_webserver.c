/**
 * @file
 * @brief The http webserver module.
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
 * @details This file contains the implementations of the http webserver module.
 */
#include <string.h>
#include <fcntl.h>

#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"

#include "sdkconfig.h"

#include "http_webserver.h"

static const char *TAG = "http-webserver";

/** HTTP to HTTPS redirect handler
 *
 * @param[in]  req  current request
 * @returns  error code representing the success of the operation.
 */
static esp_err_t http_redirect_handler(httpd_req_t *req) {
    size_t redir_len = 100;
    char * redir_path = malloc(redir_len);
    redir_path[0] = 0;
    strlcat(redir_path, "https://", redir_len);
    size_t hostlen = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (hostlen > 0) {
        char * host = malloc(hostlen);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", host, hostlen) == ESP_OK) {
            strlcpy(&redir_path[strlen(redir_path)], host, redir_len - strlen(redir_path));
        }
        free(host);
    }
    strlcpy(&redir_path[strlen(redir_path)], req->uri, redir_len - strlen(redir_path));
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_status(req, "301 Moved Permanently");
    httpd_resp_set_hdr(req, "Location", redir_path);
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

/** Start a webserver
 *
 * @returns  handle to the newly created and started server.
 */
static httpd_handle_t start_http_webserver(void) {
    httpd_config_t conf = HTTPD_DEFAULT_CONFIG();
    conf.lru_purge_enable = true;
    conf.uri_match_fn = httpd_uri_match_wildcard;   /* enable wildcard uri matching */

    /* Start the httpd server */
    ESP_LOGI(TAG, "Starting server");

    httpd_handle_t server = NULL;
    ESP_ERROR_CHECK(httpd_start(&server, &conf));

    /* Register all URIs */
    httpd_uri_t http_redirect = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = http_redirect_handler,
        .user_ctx = NULL,
        .is_websocket = false,
        .handle_ws_control_frames = false
    };
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &http_redirect));

    return server;
}

/** Stop a running webserver
 *
 * @param[in]  server  handle of the server to stop.
 * @returns  error code representing the success of the operation.
 */
static esp_err_t stop_http_webserver(httpd_handle_t server) {
    /* Stop the httpd server */
    return httpd_stop(server);
}

void http_webserver_disconnect_handler(httpd_handle_t *server) {
    if (*server != NULL) {
        ESP_ERROR_CHECK(stop_http_webserver(*server));
        *server = NULL;
    }
}

void http_webserver_connect_handler(httpd_handle_t *server) {
    if (*server == NULL) {
        *server = start_http_webserver();
    }
}
