/**
 * @file
 * @brief Websocket handlers.
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
 * @details This file contains the implementations of the websocket handlers.
 */
#include <string.h>
#include <fcntl.h>

#include "sdkconfig.h"

#include "cJSON.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "lwip/sockets.h"

#include "bus_manager.h"
#include "device_info.h"
#include "lin_master.h"
#include "mlx_err.h"
#include "power_ctrl.h"
#include "ppm_bootloader.h"
#include "wifi.h"

#include "webserver.h"

#include "urihandlers_wss.h"

#if !CONFIG_HTTPD_WS_SUPPORT
#error This project cannot be used unless HTTPD_WS_SUPPORT is enabled in esp-http-server component configuration
#endif

static const char *TAG = "wss";

struct async_resp_arg {
    httpd_handle_t hd;
    int fd;
};

typedef struct wss_client_info_s {
    int sockfd;                                 /**< socket fd of the client connection */
    uint8_t *message;                           /**< pointer to buffer where the fragmented message is stored */
    size_t message_len;                         /**< length of the message */
} wss_client_info_t;

wss_client_info_t open_clients[MAX_WWW_CLIENTS];  // todo this should be 2 dimensional including httpd_handle_t

/** wss handler error code enum */
typedef enum wss_error_code_e {
    WSS_ERR_NONE,                               /**< wss handler: task was handled successful */
    WSS_ERR_ENDPOINT_UNKNOWN,                   /**< wss handler: received unknown endpoint */
    WSS_ERR_COMMAND_UNKNOWN,                    /**< wss handler: received unknown command */
    WSS_ERR_ALREADY_SET,                        /**< wss handler: error json already populated */
    WSS_ERR_ITF_NOT_AVAILABLE,                  /**< wss handler: lin interface is not available */
    WSS_ERR_UNKNOWN,                            /**< wss handler: unknown error */
} wss_error_code_t;                             /**< wss handler error code type */

static wss_client_info_t* wss_get_client_connection_info(int sockfd) {
    wss_client_info_t *retval = NULL;
    for (int client = 0; client < MAX_WWW_CLIENTS; client++) {
        if (open_clients[client].sockfd == sockfd) {
            retval = &open_clients[client];
            break;
        }
    }
    return retval;
}

static wss_error_code_t wss_system_handler(const char* function, const cJSON * const params, cJSON * result) {
    wss_error_code_t retval = WSS_ERR_COMMAND_UNKNOWN;

    ESP_LOGI(TAG, "System task received: %s", function);

    if (strcasecmp(function, "wifi") == 0) {
        uint32_t ip;
        uint32_t netmask;
        uint32_t gateway;
        if (wifi_get_ip_info(&ip, &netmask, &gateway) == ESP_OK) {
            cJSON_AddBoolToObject(result, "link_up", true);
            cJSON_AddNumberToObject(result, "ip", ip);
            cJSON_AddNumberToObject(result, "netmask", netmask);
            cJSON_AddNumberToObject(result, "gateway", gateway);
        } else {
            /* interface is not up */
            cJSON_AddBoolToObject(result, "link_up", false);
        }
        retval = WSS_ERR_NONE;
    }

    return retval;
}

static wss_error_code_t wss_lin_ifc_wake_up(const cJSON * const params, cJSON * result) {
    wss_error_code_t retval = WSS_ERR_COMMAND_UNKNOWN;

    if (busmngr_ClaimInterface(USER_WIFI, MODE_APPLICATION) == ESP_OK) {
        int pulse_time = 200;
        cJSON *pulse_time_json = cJSON_GetObjectItem(params, "pulse_time");
        if (pulse_time_json != NULL) {
            pulse_time = (int)cJSON_GetNumberValue(pulse_time_json);
        }

        lin_error_code_t error = linmaster_sendWakeUp(pulse_time);

        if (error == ERROR_LIN_NONE) {
            retval = WSS_ERR_NONE;
        } else {
            cJSON_AddStringToObject(result, "message", mlxerr_ErrorCodeToName(MLX_FAIL_SERVER_ERR));
            retval = WSS_ERR_ALREADY_SET;
        }
    } else {
        cJSON_AddStringToObject(result, "message", mlxerr_ErrorCodeToName(MLX_FAIL_INTERFACE_NOT_FREE));
        retval = WSS_ERR_ALREADY_SET;
    }

    return retval;
}

static wss_error_code_t wss_lin_handle_message_on_bus(const cJSON * const params, cJSON * result) {
    wss_error_code_t retval = WSS_ERR_COMMAND_UNKNOWN;

    if (busmngr_ClaimInterface(USER_WIFI, MODE_APPLICATION) == ESP_OK) {
        cJSON *datalength_json = cJSON_GetObjectItem(params, "datalength");
        cJSON *m2s_json = cJSON_GetObjectItem(params, "m2s");
        cJSON *baudrate_json = cJSON_GetObjectItem(params, "baudrate");
        cJSON *enhanced_crc_json = cJSON_GetObjectItem(params, "enhanced_crc");
        cJSON *frameid_json = cJSON_GetObjectItem(params, "frameid");

        if ((datalength_json != NULL) && (m2s_json != NULL) &&
            (baudrate_json != NULL) && (enhanced_crc_json != NULL) &&
            (frameid_json != NULL)) {
            size_t datalength = (int)cJSON_GetNumberValue(datalength_json);
            bool m2s = cJSON_IsTrue(m2s_json);
            int baudrate = (int)cJSON_GetNumberValue(baudrate_json);
            bool enhanced_crc = cJSON_IsTrue(enhanced_crc_json);
            uint8_t frameid = (int)cJSON_GetNumberValue(frameid_json);

            if (m2s) {
                cJSON *payload_json = cJSON_GetObjectItem(params, "payload");
                if (payload_json != NULL) {
                    uint8_t *payload = calloc(datalength, sizeof(uint8_t));
                    if (payload != NULL) {
                        for (int i = 0; i < cJSON_GetArraySize(payload_json); i++) {
                            payload[i] = cJSON_GetArrayItem(payload_json, i)->valueint;
                        }

                        lin_error_code_t error = linmaster_sendM2S(baudrate,
                                                                   enhanced_crc,
                                                                   frameid,
                                                                   payload,
                                                                   datalength);

                        if (error == ERROR_LIN_NONE) {
                            retval = WSS_ERR_NONE;
                        } else {
                            cJSON_AddStringToObject(result, "message", "LIN Failed");   /* TODO convert to user friendly errors */
                            retval = WSS_ERR_ALREADY_SET;
                        }
                    } else {
                        cJSON_AddStringToObject(result, "message", mlxerr_ErrorCodeToName(MLX_FAIL_INTERNAL));
                        retval = WSS_ERR_ALREADY_SET;
                    }
                    free(payload);
                } else {
                    cJSON_AddStringToObject(result, "message", "Corrupted request");
                    retval = WSS_ERR_ALREADY_SET;
                }
            } else {
                uint8_t *data = calloc(datalength, sizeof(uint8_t));
                if (data != NULL) {
                    lin_error_code_t error = linmaster_sendS2M(baudrate, enhanced_crc, frameid, data, datalength);

                    if (error == ERROR_LIN_NONE) {
                        /* Copy data into json */
                        cJSON *result_json = cJSON_AddArrayToObject(result, "data");
                        for (int i = 0; i < datalength; i++) {
                            cJSON *number_json = cJSON_CreateNumber(data[i]);
                            cJSON_AddItemToArray(result_json, number_json);
                        }
                        retval = WSS_ERR_NONE;
                    } else {
                        cJSON_AddStringToObject(result, "message", "LIN Failed");   /* TODO convert to user friendly errors */
                        retval = WSS_ERR_ALREADY_SET;
                    }
                } else {
                    cJSON_AddStringToObject(result, "message", mlxerr_ErrorCodeToName(MLX_FAIL_INTERNAL));
                    retval = WSS_ERR_ALREADY_SET;
                }
                free(data);
            }
        } else {
            cJSON_AddStringToObject(result, "message", "Corrupted request");
            retval = WSS_ERR_ALREADY_SET;
        }
    } else {
        cJSON_AddStringToObject(result, "message", mlxerr_ErrorCodeToName(MLX_FAIL_INTERFACE_NOT_FREE));
        retval = WSS_ERR_ALREADY_SET;
    }

    return retval;
}

static wss_error_code_t wss_lin_handler(const char* function, const cJSON * const params, cJSON * result) {
    wss_error_code_t retval = WSS_ERR_COMMAND_UNKNOWN;

    ESP_LOGI(TAG, "LIN task received: %s", function);

    if (strcasecmp(function, "l_ifc_wake_up") == 0) {
        retval = wss_lin_ifc_wake_up(params, result);
    } else if (strcasecmp(function, "handle_message_on_bus") == 0) {
        retval = wss_lin_handle_message_on_bus(params, result);
    }
    /* TODO
     * "ld_send_message"
     * "ld_receive_message",
     * "ld_diagnostic"
     */

    return retval;
}

static wss_error_code_t wss_btl_handler(const char* function, const cJSON * const params, cJSON * result) {
    wss_error_code_t retval = WSS_ERR_COMMAND_UNKNOWN;

    ESP_LOGI(TAG, "bootloader task received: %s", function);

    (void)busmngr_ReleaseInterface(USER_WIFI, MODE_APPLICATION);

    if (busmngr_ClaimInterface(USER_WIFI, MODE_BOOTLOADER) == ESP_OK) {
        char *hexfile = cJSON_GetObjectItem(params, "hexfile")->valuestring;
        char *memory_str = cJSON_GetObjectItem(params, "memory")->valuestring;
        cJSON *manpow_json = cJSON_GetObjectItem(params, "manpow");
        cJSON *bitrate_json = cJSON_GetObjectItem(params, "bitrate");
        cJSON *project_json = cJSON_GetObjectItem(params, "project");

        if ((memory_str != NULL) && (hexfile != NULL)) {
            bool manpow = false;
            if (manpow_json != NULL) {
                manpow = cJSON_IsTrue(manpow_json);
            }
            uint32_t bitrate = 300000u;
            if (bitrate_json != NULL) {
                bitrate = (uint32_t)cJSON_GetNumberValue(bitrate_json);
            }
            uint16_t project = 0x0000u;
            if (project_json != NULL) {
                project = (uint16_t)cJSON_GetNumberValue(project_json);
            }
            ppm_memory_t memory = PPM_MEM_INVALID;
            if (strcasecmp(memory_str, "flash") == 0) {
                memory = PPM_MEM_FLASH;
            } else if ((strcasecmp(memory_str, "nvram") == 0) || (strcasecmp(memory_str, "eeprom") == 0)) {
                memory = PPM_MEM_NVRAM;
            }
            ppm_action_t action = PPM_ACT_INVALID;
            if (strcasecmp(function, "program") == 0) {
                action = PPM_ACT_PROGRAM;
            } else if (strcasecmp(function, "verify") == 0) {
                action = PPM_ACT_VERIFY;
            }

            ihexContainer_t * iHex = intelhex_read(hexfile, strlen(hexfile));
            ppm_err_t ppmstat = ppmbtl_doAction(manpow,
                                                project != 0x0000,  /* todo pass id */
                                                bitrate,
                                                memory,
                                                action,
                                                iHex);
            intelhex_free(iHex);

            if (ppmstat == PPM_OK) {
                retval = WSS_ERR_NONE;
            } else {
                cJSON_AddStringToObject(result, "message", ppmerr_ErrorCodeToName(ppmstat));
                retval = WSS_ERR_ALREADY_SET;
            }
        } else {
            cJSON_AddStringToObject(result, "message", "Corrupted request");
            retval = WSS_ERR_ALREADY_SET;
        }
    } else {
        cJSON_AddStringToObject(result, "message", mlxerr_ErrorCodeToName(MLX_FAIL_INTERFACE_NOT_FREE));
        retval = WSS_ERR_ALREADY_SET;
    }

    (void)busmngr_ReleaseInterface(USER_WIFI, MODE_BOOTLOADER);

    return retval;
}

static wss_error_code_t wss_power_out_handler(const char* function, const cJSON * const params, cJSON * result) {
    wss_error_code_t retval = WSS_ERR_COMMAND_UNKNOWN;

    ESP_LOGI(TAG, "power out task received: %s", function);

    if (strcasecmp(function, "control") == 0) {
        cJSON *enable_json = cJSON_GetObjectItem(params, "switch_enable");
        if (enable_json != NULL) {
            if (cJSON_IsTrue(enable_json)) {
                ESP_LOGI(TAG, "enable slave power");
                powerctrl_slaveEnable();
            } else {
                ESP_LOGI(TAG, "disable slave power");
                powerctrl_slaveDisable();
            }
            retval = WSS_ERR_NONE;
        }
    } else if (strcasecmp(function, "status") == 0) {
        cJSON *value_json = cJSON_CreateBool(powerctrl_slaveEnabled());
        cJSON_AddItemToObject(result, "switch_enabled", value_json);
        retval = WSS_ERR_NONE;
    }

    return retval;
}

/** WebSocket Message Handler */
static esp_err_t wss_message_handler(const cJSON * const input, cJSON * output) {
    esp_err_t retval = ESP_FAIL;

    cJSON *ping = cJSON_GetObjectItem(input, "__ping__");
    cJSON *type = cJSON_GetObjectItem(input, "type");
    cJSON *payload = cJSON_GetObjectItem(input, "payload");
    if ((ping != NULL) && cJSON_IsTrue(ping)) {
        cJSON_AddBoolToObject(output, "__pong__", true);
        retval = ESP_OK;
    } else if ((type != NULL) && (strcasecmp(type->valuestring, "info") == 0)) {
        cJSON *result = cJSON_CreateObject();
        cJSON_AddNumberToObject(result, "api_rev", 2);
        cJSON_AddStringToObject(result, "model", devinfo_deviceDescription());
        cJSON_AddStringToObject(result, "firmware_version", devinfo_firmwareVersion());
        cJSON_AddStringToObject(output, "type", "ack");
        cJSON_AddItemToObject(output, "payload", result);
        retval = ESP_OK;
    } else if ((type != NULL) && (strcasecmp(type->valuestring, "command") == 0) && (payload != NULL)) {
        cJSON *result = cJSON_CreateObject();
        cJSON_AddItemToObject(output, "payload", result);

        cJSON *endpoint = cJSON_GetObjectItem(payload, "endpoint");
        cJSON *command = cJSON_GetObjectItem(payload, "command");
        cJSON *params = cJSON_GetObjectItem(payload, "params");
        if ((endpoint != NULL) && (command != NULL)) {
            wss_error_code_t wss_err = WSS_ERR_ENDPOINT_UNKNOWN;

            if (strcasecmp(endpoint->valuestring, "system") == 0) {
                wss_err = wss_system_handler(command->valuestring, params, result);
            } else if (strcasecmp(endpoint->valuestring, "lin") == 0) {
                wss_err = wss_lin_handler(command->valuestring, params, result);
            } else if (strcasecmp(endpoint->valuestring, "bootloader") == 0) {
                wss_err = wss_btl_handler(command->valuestring, params, result);
            } else if (strcasecmp(endpoint->valuestring, "power_out") == 0) {
                wss_err = wss_power_out_handler(command->valuestring, params, result);
            }

            if (wss_err == WSS_ERR_NONE) {
                cJSON_AddStringToObject(output, "type", "ack");
            } else if (wss_err == WSS_ERR_ENDPOINT_UNKNOWN) {
                cJSON_AddStringToObject(output, "type", "error");
                cJSON_AddStringToObject(result, "message", "Endpoint unknown");
            } else if (wss_err == WSS_ERR_COMMAND_UNKNOWN) {
                cJSON_AddStringToObject(output, "type", "error");
                cJSON_AddStringToObject(result, "message", "Command unknown");
            } else if (wss_err == WSS_ERR_ALREADY_SET) {
                cJSON_AddStringToObject(output, "type", "error");
            } else {
                cJSON_AddStringToObject(output, "type", "error");
                cJSON_AddStringToObject(result, "message", "Error unknown");
            }
            retval = ESP_OK;
        } else {
            cJSON_AddStringToObject(output, "type", "error");
            cJSON_AddStringToObject(result, "message", "Protocol unknown");
            retval = ESP_OK;
        }
    } else {
        /* build error response */
        cJSON *result = cJSON_CreateObject();
        cJSON_AddStringToObject(result, "message", "Corrupted request");
        cJSON_AddStringToObject(output, "type", "error");
        cJSON_AddItemToObject(output, "payload", result);
        retval = ESP_OK;
    }

    return retval;
}

static esp_err_t wss_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "handshake done, the new connection was opened");
        return ESP_OK;
    }
    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));

    /* First receive the full ws message */
    /* Set max_len = 0 to get the frame len */
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
        return ret;
    }

    ESP_LOGI(TAG, "frame len is %d", ws_pkt.len);
    if (ws_pkt.len) {
        /* ws_pkt.len + 1 is for NULL termination as we are expecting a string */
        buf = calloc(ws_pkt.len + 1, sizeof(uint8_t));
        if (buf == NULL) {
            ESP_LOGE(TAG, "failed to calloc memory for buf");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;

        /* Set max_len = ws_pkt.len to get the frame payload */
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
            free(buf);
            return ret;
        }
    }

    if ((ws_pkt.type == HTTPD_WS_TYPE_TEXT) || (ws_pkt.type == HTTPD_WS_TYPE_CONTINUE)) {
        ESP_LOGD(TAG, "ws frame received for client %d", httpd_req_to_sockfd(req));

        wss_client_info_t *client_info = wss_get_client_connection_info(httpd_req_to_sockfd(req));
        if (client_info == NULL) {
            ESP_LOGE(TAG, "client %d is unknown", httpd_req_to_sockfd(req));
            free(buf);
            return ESP_FAIL;
        }

        if (ws_pkt.type == HTTPD_WS_TYPE_TEXT) {
            if (client_info->message != NULL) {
                /* drop whatever we buffered as this is a new frame */
                free(client_info->message);
                client_info->message_len = 0;
            }
            client_info->message = buf;
            client_info->message_len = ws_pkt.len + 1;
        } else {
            /* extend buffered frame with extra content */
            client_info->message = realloc(client_info->message,
                                           client_info->message_len + ws_pkt.len);
            memcpy(&client_info->message[client_info->message_len - 1], buf, ws_pkt.len + 1);
            client_info->message_len += ws_pkt.len;
            free(buf);
        }
        ESP_LOGD(TAG, "ws buffered message len now is %d", client_info->message_len - 1);

        if (ws_pkt.final) {
            /* handle fully received websocket message */
            ESP_LOGI(TAG, "ws message received: %.100s", client_info->message);
            cJSON *root = cJSON_Parse((const char *)client_info->message);
            if (root) {
                cJSON *response = cJSON_CreateObject();
                cJSON *id = cJSON_GetObjectItem(root, "id");
                if (id != NULL) {
                    cJSON_AddStringToObject(response, "id", id->valuestring);
                }
                if (wss_message_handler(root, response) == ESP_OK) {
                    /* TODO handling should be moved to another thread so that ping pong can continue
                     * and STR `this.state.isBootloading` commit can be reverted */
                    char *json_resp = NULL;
                    json_resp = cJSON_PrintUnformatted(response);
                    if (json_resp != NULL) {
                        ws_pkt.payload = (uint8_t*)json_resp;
                        ws_pkt.len = strlen(json_resp);
                        ws_pkt.type = HTTPD_WS_TYPE_TEXT;
                        ESP_LOGI(TAG, "ws message response: %.100s", ws_pkt.payload);

                        ret = httpd_ws_send_frame(req, &ws_pkt);
                        if (ret != ESP_OK) {
                            ESP_LOGE(TAG, "httpd_ws_send_frame failed with %d", ret);
                        }
                        ESP_LOGI(TAG, "wss_handler: httpd_handle_t=%p, sockfd=%d, client_info:%d", req->handle,
                                 httpd_req_to_sockfd(req), httpd_ws_get_fd_info(req->handle, httpd_req_to_sockfd(req)));
                    }
                    cJSON_free(json_resp);
                }
                cJSON_Delete(response);
            }
            cJSON_Delete(root);
            free(client_info->message);
            client_info->message = NULL;
            client_info->message_len = 0;
        }
        return ret;
    }
    free(buf);
    return ESP_OK;
}

/** Custom session opening callback.
 *
 * Called on a new session socket just after accept(), but before reading any data.
 *
 * This is an opportunity to set up e.g. SSL encryption using global_transport_ctx
 * and the send/recv/pending session overrides.
 *
 * If a context needs to be maintained between these functions, store it in the session
 * using httpd_sess_set_transport_ctx() and retrieve it later with httpd_sess_get_transport_ctx()
 *
 * Returning a value other than ESP_OK will immediately close the new socket.
 *
 * @param[in]  hd  handle to the http(s) server with which the websocket works.
 * @param[in]  sockfd  file descriptor of the socket
 * @returns  error code representing the success of the operation.
 */
static esp_err_t wss_open_fd(httpd_handle_t hd, int sockfd) {
    ESP_LOGI(TAG, "new client connected %d", sockfd);

    /* keep track of connected clients */
    wss_client_info_t *client_info = wss_get_client_connection_info(0);
    if (client_info != NULL) {
        client_info->sockfd = sockfd;
        client_info->message = NULL;
        client_info->message_len = 0;
    }

    return ESP_OK;
}

/** Custom session closing callback.
 *
 * Called when a session is deleted, before freeing user and transport contexts and
 * before closing the socket. This is a place for custom de-init code common to all sockets.
 *
 * The server will only close the socket if no custom session closing callback is set. If a
 * custom callback is used, close(sockfd) should be called in here for most cases.
 *
 * Set the user or transport context to NULL if it was freed here, so the server does not try
 * to free it again.
 *
 * This function is run for all terminated sessions, including sessions where the socket was
 * closed by the network stack - that is, the file descriptor may not be valid anymore.
 *
 * @param[in]  hd  handle to the http(s) server with which the websocket works.
 * @param[in]  sockfd  file descriptor of the socket
 */
static void wss_close_fd(httpd_handle_t hd, int sockfd) {
    ESP_LOGI(TAG, "client disconnected %d", sockfd);

    /* keep track of connected clients */
    wss_client_info_t *client_info = wss_get_client_connection_info(sockfd);
    if (client_info != NULL) {
        client_info->sockfd = 0;
        if (client_info->message != NULL) {
            free(client_info->message);
        }
        client_info->message = NULL;
        client_info->message_len = 0;
    }

    close(sockfd);

    /* release lin interface if it was taken */
    (void)busmngr_ReleaseInterface(USER_WIFI, MODE_APPLICATION);
}

esp_err_t wss_init(httpd_config_t *httpd) {
    memset(open_clients, 0, sizeof(open_clients));

    httpd->open_fn = wss_open_fd;
    httpd->close_fn = wss_close_fd;

    return ESP_OK;
}

esp_err_t wss_register_uri(httpd_handle_t server, void *user_ctx) {
    (void)user_ctx;
    httpd_uri_t wss_uri = {
        .uri = "/ws/v1/?",
        .method = HTTP_GET,
        .handler = wss_handler,
        .user_ctx = NULL,
        .is_websocket = true,
        .handle_ws_control_frames = false
    };
    return httpd_register_uri_handler(server, &wss_uri);
}

esp_err_t wss_start(httpd_handle_t server) {
    return ESP_OK;
}

esp_err_t wss_stop(httpd_handle_t server) {
    return ESP_OK;
}
