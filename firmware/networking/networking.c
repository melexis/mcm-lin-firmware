/**
 * @file
 * @brief The networking module.
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
 * @details This file contains the implementations of the networking module.
 */
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"

#include "lwip/apps/netbiosns.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "mdns.h"

#include "sdkconfig.h"
#include "device_info.h"
#include "wifi.h"

#include "networking.h"

static const char *TAG = "networking";

static void initialise_mdns(const char * hostname) {
    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set(hostname));
    ESP_ERROR_CHECK(mdns_instance_name_set(devinfo_deviceDescription()));

    ESP_ERROR_CHECK(mdns_service_add(NULL, "_https", "_tcp", 443, NULL, 0));

    const char * shortName = devinfo_deviceShortName();
    char * mdnsName;
    size_t mdnsNameLen = strlen(shortName) + 11;
    mdnsName = malloc(mdnsNameLen);
    snprintf(mdnsName, mdnsNameLen, "%s-WebServer", shortName);
    ESP_ERROR_CHECK(mdns_service_instance_name_set("_https", "_tcp", mdnsName));
    free(mdnsName);

//    const esp_app_desc_t* app_desc = esp_app_get_description();
    mdns_txt_item_t serviceTxtData[] = {
        {"board", "{mcm}"},
        {"path", "/"},
        //{"firmware_version", app_desc->version},
        {"manufacturer", devinfo_manufacturerName()},
        {"type", devinfo_deviceDescription()}
    };

    ESP_ERROR_CHECK(mdns_service_txt_set("_https", "_tcp", serviceTxtData,
                                         sizeof(serviceTxtData) / sizeof(serviceTxtData[0])));
}

esp_err_t networking_init(void) {
    char hostname[32];
    size_t hostname_size = sizeof(hostname);

    (void)networking_get_hostname(hostname, &hostname_size);
    ESP_LOGI(TAG, "Set hostname '%s'", hostname);

    netbiosns_init();
    netbiosns_set_name(hostname);

    ESP_ERROR_CHECK(wifi_init());
    wifi_set_hostname(hostname);

    initialise_mdns(hostname);

    return ESP_OK;
}

void networking_tick(void) {
    wifi_tick();
}

esp_err_t networking_set_hostname(const char * hostname, bool immediate) {
    nvs_handle dns_handle;
    esp_err_t err = nvs_open(TAG, NVS_READWRITE, &dns_handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
    } else {
        err = nvs_set_str(dns_handle, "hostname", hostname);
    }
    if (err == ESP_OK) {
        err = nvs_commit(dns_handle);
    }
    nvs_close(dns_handle);
    if ((err == ESP_OK) && immediate) {
        netbiosns_set_name(hostname);
        mdns_hostname_set(hostname);
        wifi_set_hostname(hostname);
    }
    return err;
}

esp_err_t networking_get_hostname(char * hostname, size_t * hostname_length) {
    nvs_handle dns_handle;
    esp_err_t err = nvs_open(TAG, NVS_READONLY, &dns_handle);
    if (err == ESP_OK) {
        err = nvs_get_str(dns_handle, "hostname", hostname, hostname_length);
    } else {
        ESP_LOGI(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
    }
    nvs_close(dns_handle);

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        strcpy(hostname, CONFIG_LWIP_LOCAL_HOSTNAME);
        err = ESP_OK;
    }

    return err;
}
