/**
 * @file
 * @brief The wifi networking module.
 * @internal
 *
 * @copyright (C) 2024 Melexis N.V.
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
 * @details This file contains the implementations of the wifi networking module.
 */
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_wifi.h"

#include "http_webserver.h"
#include "webserver.h"

#include "wifi.h"

esp_netif_t *netif;

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "wifi";

static int s_retry_num = 0;
static int64_t discon_millis = -1;

static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data) {
    static httpd_handle_t http_webserver = NULL;
    static httpd_handle_t webserver = NULL;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        http_webserver_disconnect_handler(&http_webserver);
        webserver_disconnect_handler(&webserver);
        if (s_retry_num < CONFIG_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        http_webserver_connect_handler(&http_webserver);
        webserver_connect_handler(&webserver);
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_err_t wifi_init(void) {
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    ESP_ERROR_CHECK(esp_wifi_start());

    return ESP_OK;
}

void wifi_tick(void) {
    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdTRUE,
                                           pdFALSE,
                                           0);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        discon_millis = -1;
        ESP_LOGI(TAG, "Connected to AP");
    } else if (bits & WIFI_FAIL_BIT) {
        discon_millis = esp_timer_get_time();
        ESP_LOGI(TAG, "Failed to connect to AP");
    } else {
        /* nothing to be done */
    }

    if ((discon_millis >= 0) && (esp_timer_get_time() - discon_millis > 30000000)) {
        /* 30sec have passed, lets try to reconnect */
        wifi_config_t wifi_config;
        (void)esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_config);
        ESP_LOGI(TAG, "Try reconnecting to AP with SSID: %s", wifi_config.sta.ssid);
        esp_wifi_connect();
        discon_millis = -1;
        s_retry_num = 0;
    }
}

esp_err_t wifi_set_ssid(const char * ssid, bool immediate) {
    wifi_config_t wifi_config;
    esp_err_t err = esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_config);

    if (err == ESP_OK) {
        strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
        err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    }
    if (immediate) {
        if (err == ESP_OK) {
            err = esp_wifi_stop();
        }
        if (err == ESP_OK) {
            err = esp_wifi_start();
        }
    }

    return err;
}

esp_err_t wifi_get_ssid(char * ssid, size_t ssid_len) {
    wifi_config_t wifi_config;
    esp_err_t err = esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_config);

    if (err == ESP_OK) {
        strncpy(ssid, (char*)wifi_config.sta.ssid, ssid_len);
    }

    return err;
}

esp_err_t wifi_set_password(const char * password, bool immediate) {
    wifi_config_t wifi_config;
    esp_err_t err = esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_config);

    if (err == ESP_OK) {
        strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
        err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    }
    if (immediate) {
        if (err == ESP_OK) {
            err = esp_wifi_stop();
        }
        if (err == ESP_OK) {
            err = esp_wifi_start();
        }
    }

    return err;
}

esp_err_t wifi_get_password(char * password, size_t password_len) {
    wifi_config_t wifi_config;
    esp_err_t err = esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_config);

    if (err == ESP_OK) {
        strncpy(password, (char*)wifi_config.sta.password, password_len);
    }

    return err;
}

esp_err_t wifi_set_hostname(const char * hostname) {
    return esp_netif_set_hostname(netif, hostname);
}

esp_err_t wifi_get_hostname(const char ** hostname) {
    return esp_netif_get_hostname(netif, hostname);
}

esp_err_t wifi_get_mac(uint8_t *mac) {
    return esp_netif_get_mac(netif, mac);
}

esp_err_t wifi_get_ip_info(uint32_t *ip, uint32_t *netmask, uint32_t *gateway) {
    esp_err_t retval = ESP_FAIL;
    if (wifi_link_up() == true) {
        esp_netif_ip_info_t ipInfo;
        (void)esp_netif_get_ip_info(netif, &ipInfo);
        if (ip != NULL) {
            *ip = ipInfo.ip.addr;
        }
        if (netmask != NULL) {
            *netmask = ipInfo.netmask.addr;
        }
        if (gateway != NULL) {
            *gateway = ipInfo.gw.addr;
        }
        retval = ESP_OK;
    }
    return retval;
}

bool wifi_link_up(void) {
    return esp_netif_is_netif_up(netif);
}
