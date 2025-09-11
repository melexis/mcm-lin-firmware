/**
 * @file
 * @brief The networking module definitions.
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
 * @details This file contains the definitions of the networking module.
 */

#ifndef DNSINFO_H_
    #define DNSINFO_H_

/** initialize the networking module.
 *
 * @returns  error code representing the progress of the operation.
 */
esp_err_t networking_init(void);

/** background handler for the networking module */
void networking_tick(void);

/** update the hostname in the NVS
 *
 * @param[in]  hostname  new hostname to set/store.
 * @param[in]  immediate  immediate apply the new settings.
 * @returns  error code representing the progress of the operation.
 */
esp_err_t networking_set_hostname(const char * hostname, bool immediate);

/** get the currenty configured hostname
 *
 * @param[in]  hostname  current configured hostname.
 * @param[in]  hostname_length  length of the array pointed by hostname.
 * @returns  error code representing the progress of the operation.
 */
esp_err_t networking_get_hostname(char * hostname, size_t * hostname_length);

#endif /* DNSINFO_H_ */
