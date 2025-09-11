/**
 * @file
 * @brief The wifi networking module definitions.
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
 * @details This file contains the definitions of the wifi networking module.
 */

#ifndef WIFI_H_
    #define WIFI_H_

/** initialize the WiFi networkign module.
 *
 * @returns  error code representing the progress of the operation.
 */
esp_err_t wifi_init(void);

/** background handler for WiFi networking module */
void wifi_tick(void);

/** update the ssid for the wifi interface.
 *
 * @param[in]  ssid  new ssid to set/store.
 * @param[in]  immediate  immediate apply the new settings.
 * @returns  error code representing the progress of the operation.
 */
esp_err_t wifi_set_ssid(const char * ssid, bool immediate);

/** get the currenty configured ssid.
 *
 * @param[in]  ssid  current configured ssid.
 * @param[in]  ssid_len  length of the array pointed by ssid.
 * @returns  error code representing the progress of the operation.
 */
esp_err_t wifi_get_ssid(char * ssid, size_t ssid_len);

/** update the password for the wifi interface.
 *
 * @param[in]  password  new password to set/store.
 * @param[in]  immediate  immediate apply the new settings.
 * @returns  error code representing the progress of the operation.
 */
esp_err_t wifi_set_password(const char * password, bool immediate);

/** get the currenty configured password.
 *
 * @param[in]  password  current configured password.
 * @param[in]  password_len  length of the array pointed by password.
 * @returns  error code representing the progress of the operation.
 */
esp_err_t wifi_get_password(char * password, size_t password_len);

/** update the hostname for the wifi interface.
 *
 * @param[in]  hostname  new hostname to set/store.
 * @returns  error code representing the progress of the operation.
 */
esp_err_t wifi_set_hostname(const char * hostname);

/** get the currenty configured hostname.
 *
 * @param[in]  hostname  current configured hostname.
 * @returns  error code representing the progress of the operation.
 */
esp_err_t wifi_get_hostname(const char ** hostname);

/** get the wifi mac address
 *
 * @param[out]  mac  mac address of the interface.
 * @returns  error code representing the progress of the operation.
 */
esp_err_t wifi_get_mac(uint8_t *mac);

/** get the currently used ip information
 *
 * @param[out]  ip  ip address currently in use.
 * @param[out]  netmask  netmask currently in use.
 * @param[out]  gateway  gateway address currently in use.
 * @retval  ESP_OK  information is returned.
 * @retval  ESP_FAIL  interface is not up.
 */
esp_err_t wifi_get_ip_info(uint32_t *ip, uint32_t *netmask, uint32_t *gateway);

/** check whether the wifi link is up or not
 *
 * @retval  true  WiFi link is up.
 * @retval  false  otherwise.
 */
bool wifi_link_up(void);

#endif /* WIFI_H_ */
