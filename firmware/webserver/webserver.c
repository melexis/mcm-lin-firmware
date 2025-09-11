/**
 * @file
 * @brief The webserver module.
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
 * @details This file contains the implementations of the webserver module.
 */
#include <string.h>
#include <fcntl.h>

#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_https_server.h"
#include "esp_log.h"

#include "sdkconfig.h"
#include "urihandlers_rest.h"
#include "urihandlers_wss.h"
#include "urihandlers_www.h"

#include "webserver.h"

static const char *TAG = "webserver";

static www_server_data_t * www_data = NULL;

static httpd_handle_t start_webserver(void) {
    if (www_data) {
        ESP_LOGE(TAG, "Webserver already started");
        return NULL;
    }

    /* Allocate memory for server data */
    www_data = calloc(1, sizeof(www_server_data_t));
    if (!www_data) {
        ESP_LOGE(TAG, "Failed to allocate memory for webserver data");
        return NULL;
    }

    ESP_LOGI(TAG, "Starting server");

    httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();
    conf.httpd.max_open_sockets = MAX_WWW_CLIENTS;
    conf.httpd.max_uri_handlers = WSS_NR_OF_URI_HANDLERS + REST_NR_OF_URI_HANDLERS + WWW_NR_OF_URI_HANDLERS;
    conf.httpd.uri_match_fn = httpd_uri_match_wildcard;   /* enable wildcard uri matching */

    conf.httpd.keep_alive_enable = true;  /* keep alive handled by httpd */

    extern const unsigned char servercert_start[] asm ("_binary_servercert_pem_start");
    extern const unsigned char servercert_end[]   asm ("_binary_servercert_pem_end");
    conf.servercert = servercert_start;
    conf.servercert_len = servercert_end - servercert_start;

    extern const unsigned char prvtkey_pem_start[] asm ("_binary_prvtkey_pem_start");
    extern const unsigned char prvtkey_pem_end[]   asm ("_binary_prvtkey_pem_end");
    conf.prvtkey_pem = prvtkey_pem_start;
    conf.prvtkey_len = prvtkey_pem_end - prvtkey_pem_start;

    /* Initialize the websocket module */
    wss_init(&conf.httpd);

    /* Initialize and start the webserver */
    httpd_handle_t server = NULL;
    ESP_ERROR_CHECK(httpd_ssl_start(&server, &conf));

    /* Start websocket module */
    ESP_ERROR_CHECK(wss_start(server));

    /* Register all URIs */
    ESP_ERROR_CHECK(wss_register_uri(server, www_data));
    ESP_ERROR_CHECK(rest_register_uri(server, www_data));
    ESP_ERROR_CHECK(www_register_uri(server, www_data));

    return server;
}

/** Stop a running webserver
 *
 * @param[in]  server  handle of the server to stop.
 * @returns  error code representing the success of the operation.
 */
static esp_err_t stop_webserver(httpd_handle_t server) {
    esp_err_t retval = ESP_FAIL;

    /* Stop the websocket */
    wss_stop(server);

    /* Stop the httpd server */
    retval = httpd_ssl_stop(server);

    free(www_data);
    www_data = NULL;

    return retval;
}

void webserver_disconnect_handler(httpd_handle_t* server) {
    if (*server != NULL) {
        ESP_ERROR_CHECK(stop_webserver(*server));
        *server = NULL;
    }
}

void webserver_connect_handler(httpd_handle_t* server) {
    if (*server == NULL) {
        *server = start_webserver();
    }
}
