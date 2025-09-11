/**
 * @file
 * @brief REST API URI handlers.
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
 * @details This file contains the implementations of the REST API URI handlers.
 */
#include <string.h>
#include <fcntl.h>

#include "cJSON.h"
#include "esp_chip_info.h"
#include "esp_err.h"
#include "esp_https_server.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"

#include "sdkconfig.h"
#include "device_info.h"
#include "device_status.h"
#include "networking.h"
#include "webserver.h"
#include "wifi.h"

#include "urihandlers_rest.h"

static const char *TAG = "rest";

static esp_err_t get_post_json_payload(httpd_req_t *req, cJSON **root) {
    int total_len = req->content_len;
    int cur_len = 0;
    char *scratch = ((www_server_data_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, scratch + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "failed to receive post data");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    scratch[total_len] = '\0';

    *root = cJSON_Parse(scratch);

    return ESP_OK;
}

/** Send bad request reponse */
static esp_err_t api_bad_request(httpd_req_t *req) {
    httpd_resp_set_status(req, "400 Bad Request");
    return httpd_resp_send(req, NULL, 0);
}

/** Send method not allowed response */
static esp_err_t api_method_not_allowed(httpd_req_t *req) {
    httpd_resp_set_status(req, "405 Method Not Allowed");
    return httpd_resp_send(req, NULL, 0);
}

/** Send internal server error response */
static esp_err_t api_internal_server_error(httpd_req_t *req) {
    httpd_resp_set_status(req, "500 Internal Server Error");
    return httpd_resp_send(req, NULL, 0);
}

/** Send not implemented response */
static esp_err_t api_not_implemented(httpd_req_t *req) {
    httpd_resp_set_status(req, "501 Not Implemented");
    return httpd_resp_send(req, NULL, 0);
}

/** URI Handler: device information */
static esp_err_t api_device_info_handler(httpd_req_t *req) {
    if (req->method != HTTP_GET) {
        return api_method_not_allowed(req);
    }

    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return api_internal_server_error(req);
    }

    /* add system information */
    cJSON_AddStringToObject(root, "firmware_version", devinfo_firmwareVersion());
    cJSON_AddStringToObject(root, "model", devinfo_deviceDescription());
    cJSON_AddNumberToObject(root, "reset_reason", esp_reset_reason());
    cJSON_AddNumberToObject(root, "up_time", esp_timer_get_time());

    /* create response */
    httpd_resp_set_type(req, "application/json");
    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    free((void *)sys_info);

    cJSON_Delete(root);

    return ESP_OK;
}

/** URI Handler: system wifi */
static esp_err_t api_system_wifi_handler(httpd_req_t *req) {
    if ((req->method != HTTP_PUT) && (req->method != HTTP_GET)) {
        return api_method_not_allowed(req);
    }

    if (req->method == HTTP_PUT) {
        cJSON *root;
        if (get_post_json_payload(req, &root) != ESP_OK) {
            return api_bad_request(req);
        }

        cJSON *ssidObj = cJSON_GetObjectItem(root, "ssid");
        if (ssidObj) {
            char *ssid = ssidObj->valuestring;
            (void)wifi_set_ssid(ssid, false);
        }

        cJSON *passwordObj = cJSON_GetObjectItem(root, "password");
        if (passwordObj) {
            char *password = passwordObj->valuestring;
            (void)wifi_set_password(password, false);
        }

        cJSON *hostnameObj = cJSON_GetObjectItem(root, "hostname");
        if (hostnameObj) {
            char *hostname = hostnameObj->valuestring;
            (void)networking_set_hostname(hostname, false);
        }
        cJSON_Delete(root);
    }

    cJSON *resp = cJSON_CreateObject();
    if (resp == NULL) {
        return api_internal_server_error(req);
    }

    char ssid[32 + 1];
    memset(ssid, 0, sizeof(ssid));
    if (wifi_get_ssid(ssid, sizeof(ssid) - 1) == ESP_OK) {
        cJSON_AddStringToObject(resp, "ssid", ssid);
    }

    char password[64 + 1];
    memset(password, 0, sizeof(password));
    if (wifi_get_password(password, sizeof(password) - 1) == ESP_OK) {
        cJSON_AddStringToObject(resp, "password", password);
    }

    char hostname[32];
    size_t hostname_size = sizeof(hostname);
    if (networking_get_hostname(hostname, &hostname_size) == ESP_OK) {
        if (hostname_size > 0) {
            cJSON_AddStringToObject(resp, "hostname", hostname);
        }
    }

    /* add mac address */
    uint8_t base_mac_addr[6] = {[0 ... 5] = 0};
    (void)wifi_get_mac(base_mac_addr);
    char base_mac_addr_str[20];
    snprintf(base_mac_addr_str, sizeof(base_mac_addr_str), MACSTR, MAC2STR(base_mac_addr));
    cJSON_AddStringToObject(resp, "mac", base_mac_addr_str);

    uint32_t ip;
    uint32_t netmask;
    uint32_t gateway;
    if (wifi_get_ip_info(&ip, &netmask, &gateway) == ESP_OK) {
        cJSON_AddBoolToObject(resp, "link_up", true);
        cJSON_AddNumberToObject(resp, "ip", ip);
        cJSON_AddNumberToObject(resp, "netmask", netmask);
        cJSON_AddNumberToObject(resp, "gateway", gateway);
    } else {
        /* interface is not up */
        cJSON_AddBoolToObject(resp, "link_up", false);
    }

    /* create response */
    httpd_resp_set_type(req, "application/json");
    const char *sys_config = cJSON_Print(resp);
    httpd_resp_sendstr(req, sys_config);
    free((void *)sys_config);

    cJSON_Delete(resp);

    return ESP_OK;
}

/** URI Handler: perform system reboot */
static esp_err_t api_system_reboot_handler(httpd_req_t *req) {
    if (req->method != HTTP_PUT) {
        return api_method_not_allowed(req);
    }

    httpd_resp_set_status(req, "204 No Content");
    esp_err_t err = httpd_resp_send(req, NULL, 0);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "System Reboot");
        /* wait a bit till the response was send to the client */
        vTaskDelay(pdMS_TO_TICKS(1000));
        /* now perform reset */
        esp_restart();
    }

    return err;
}

/** URI Handler: perform system identification */
static esp_err_t api_system_identify_handler(httpd_req_t *req) {
    if (req->method != HTTP_PUT) {
        return api_method_not_allowed(req);
    }

    httpd_resp_set_status(req, "204 No Content");
    esp_err_t err = httpd_resp_send(req, NULL, 0);

    if (err == ESP_OK) {
        devstat_startIdentify();
    }

    return err;
}

esp_err_t rest_register_uri(httpd_handle_t server, void *user_ctx) {
    esp_err_t retval;

    httpd_uri_t info_get_uri = {
        .uri = "/api/v1/?",
        .method = HTTP_ANY,
        .handler = api_device_info_handler,
        .user_ctx = user_ctx,
        .is_websocket = false,
        .handle_ws_control_frames = false
    };
    retval = httpd_register_uri_handler(server, &info_get_uri);
    if (retval != ESP_OK) {
        return retval;
    }

    httpd_uri_t system_uri = {
        .uri = "/api/v1/system/?",
        .method = HTTP_ANY,
        .handler = api_system_wifi_handler,  /* to be deleted */
        .user_ctx = user_ctx,
        .is_websocket = false,
        .handle_ws_control_frames = false
    };
    retval = httpd_register_uri_handler(server, &system_uri);
    if (retval != ESP_OK) {
        return retval;
    }

    httpd_uri_t system_wifi_uri = {
        .uri = "/api/v1/system/wifi/?",
        .method = HTTP_ANY,
        .handler = api_system_wifi_handler,
        .user_ctx = user_ctx,
        .is_websocket = false,
        .handle_ws_control_frames = false
    };
    retval = httpd_register_uri_handler(server, &system_wifi_uri);
    if (retval != ESP_OK) {
        return retval;
    }

    httpd_uri_t system_reboot_put_uri = {
        .uri = "/api/v1/system/reboot/?",
        .method = HTTP_ANY,
        .handler = api_system_reboot_handler,
        .user_ctx = user_ctx,
        .is_websocket = false,
        .handle_ws_control_frames = false
    };
    retval = httpd_register_uri_handler(server, &system_reboot_put_uri);
    if (retval != ESP_OK) {
        return retval;
    }

    httpd_uri_t system_identify_put_uri = {
        .uri = "/api/v1/system/identify/?",
        .method = HTTP_ANY,
        .handler = api_system_identify_handler,
        .user_ctx = user_ctx,
        .is_websocket = false,
        .handle_ws_control_frames = false
    };
    retval = httpd_register_uri_handler(server, &system_identify_put_uri);
    if (retval != ESP_OK) {
        return retval;
    }

    httpd_uri_t api_not_implemented_uri = {
        .uri = "/api/?*",
        .method = HTTP_ANY,
        .handler = api_not_implemented,
        .user_ctx = user_ctx,
        .is_websocket = false,
        .handle_ws_control_frames = false
    };
    retval = httpd_register_uri_handler(server, &api_not_implemented_uri);
    if (retval != ESP_OK) {
        return retval;
    }

    return retval;
}
