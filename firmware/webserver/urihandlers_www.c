/**
 * @file
 * @brief WWW URI handlers.
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
 * @details This file contains the implementations of the WWW URI handlers.
 */
#include <string.h>
#include <fcntl.h>

#include "cJSON.h"
#include "esp_chip_info.h"
#include "esp_err.h"
#include "esp_https_server.h"
#include "esp_log.h"
#include "esp_system.h"

#include "sdkconfig.h"
#include "webserver.h"
#include "www_bin.h"

#include "urihandlers_www.h"

static const char *WWW_TAG = "www-uri";

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath) {
    const char *type = "text/plain";
    if (CHECK_FILE_EXTENSION(filepath, ".html")) {
        type = "text/html";
    } else if (CHECK_FILE_EXTENSION(filepath, ".js")) {
        type = "application/javascript";
    } else if (CHECK_FILE_EXTENSION(filepath, ".css")) {
        type = "text/css";
    } else if (CHECK_FILE_EXTENSION(filepath, ".png")) {
        type = "image/png";
    } else if (CHECK_FILE_EXTENSION(filepath, ".ico")) {
        type = "image/x-icon";
    } else if (CHECK_FILE_EXTENSION(filepath, ".svg")) {
        type = "image/svg+xml";
    }
    return httpd_resp_set_type(req, type);
}

/** Send a file in a http response */
static esp_err_t send_file(httpd_req_t *req, const char *filepath) {
    www_item_t * item = NULL;
    for (int index = 0; index < sizeof(www_bin_files) / sizeof(www_bin_files[0]); index++) {
        if (strcasecmp(www_bin_files[index].path, filepath) == 0) {
            item = &www_bin_files[index];
        }
    }

    if (item == NULL) {
        ESP_LOGE(WWW_TAG, "Failed to open file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Page Not Found");
        return ESP_FAIL;
    }

    ESP_LOGI(WWW_TAG, "Sending file");
    set_content_type_from_file(req, filepath);
    return httpd_resp_send(req, (const char *)item->start, *item->length);
}

/** URI Handler: assets content */
static esp_err_t assets_get_handler(httpd_req_t *req) {
    if (req->uri[strlen(req->uri) - 1] == '/') {
        return send_file(req, "/index.html");
    }
    /* todo if html or no extension send index.html */
    return send_file(req, req->uri);
}

/** URI Handler: index <> webapplication */
static esp_err_t index_get_handler(httpd_req_t *req) {
    return send_file(req, "/index.html");
}

esp_err_t www_register_uri(httpd_handle_t server, void *user_ctx) {
    esp_err_t retval;

    httpd_uri_t assets_get_uri = {
        .uri = "/assets/*",
        .method = HTTP_GET,
        .handler = assets_get_handler,
        .user_ctx = user_ctx,
        .is_websocket = false,
        .handle_ws_control_frames = false
    };
    retval = httpd_register_uri_handler(server, &assets_get_uri);
    if (retval != ESP_OK) {
        return retval;
    }

    httpd_uri_t index_get_uri = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = index_get_handler,
        .user_ctx = user_ctx,
        .is_websocket = false,
        .handle_ws_control_frames = false
    };
    retval = httpd_register_uri_handler(server, &index_get_uri);
    if (retval != ESP_OK) {
        return retval;
    }

    return retval;
}
