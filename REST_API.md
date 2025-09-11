# REST API Description

## Device Information `/api/v1`

The `/api/v1` endpoint allows you to get relevant information about the device.

This endpoint accepts `GET` requests.

### Parameters

| Data             | Type   | Description                                |
|:----------------:|:------:|:------------------------------------------ |
| firmware_version | String | Firmware version of the device.            |
| model            | String | Name of the MCM device model.              |
| reset_reason     | Number | Reason of the last reset of the device.    |
| up_time          | Number | The uptime of the device in micro seconds. |

### Examples

```shell title="Request"
curl --insecure --include https://<ip_address>/api/v1
```

```shell title="Response"
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 135

{
	"firmware_version":	"v0.13.1-4-gd275fd7-dirty",
	"model":	"Melexis Compact Master LIN",
	"reset_reason":	3,
	"up_time":	58194064
}
```

### Reset Reasons

| Value | Description                                            |
|:-----:|:------------------------------------------------------ |
| 0     | Reset reason can not be determined                     |
| 1     | Reset due to power-on event                            |
| 2     | Reset by external pin                                  |
| 3     | Software reset via esp_restart                         |
| 4     | Software reset due to exception/panic                  |
| 5     | Reset (software or hardware) due to interrupt watchdog |
| 6     | Reset due to task watchdog                             |
| 7     | Reset due to other watchdogs                           |
| 8     | Reset after exiting deep sleep mode                    |
| 9     | Brownout reset (software or hardware)                  |
| 10    | Reset over SDIO                                        |
| 11    | Fast reboot                                            |

## System `/api/v1/system/wifi`

The `/api/v1/system/wifi` endpoint allows you to get and set some system information of the MCM device.

This endpoint accepts `GET` and `PUT` requests.

### Parameters

| Data              | Type    | Access     | Description                            |
|:-----------------:|:-------:|:----------:|:-------------------------------------- |
| ssid              | String  | Read/Write | The SSID of the Wi-Fi network.         |
| password          | String  | Read/Write | The password of the Wi-Fi network.     |
| hostname          | String  | Read/Write | The hostname of the device.            |
| mac               | String  | Read-only  | The MAC address of the device.         |
| link_up           | Boolean | Read-only  | Wether or not the Wi-Fi link is up.    |
| ip                | Number  | Read-only  | The IP address of the device.          |
| netmask           | Number  | Read-only  | The netmask of the device.             |
| gateway           | Number  | Read-only  | The gateway IP address.                |

### Actions

| Action   | Description                   |
|:--------:|:----------------------------- |
| identify | Identification of the device. |
| reboot   | Reboot the device.            |

### WiFi Configuration

#### Examples

```shell title="Request"
curl --insecure --include https://<ip_address>/api/v1/system
```

```shell title="Response"
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 193

{
	"ssid":	"my-wifi-ssid",
	"password":	"my-wifi-pass",
	"hostname":	"my-hostname",
	"mac":	"68:b6:b3:ba:ad:ef",
	"link_up":	true,
	"ip":	833284106,
	"netmask":	15794175,
	"gateway":	27322378
}
```

```shell title="Request"
curl --insecure --include -X PUT -H 'Content-Type: application/json' -d '{"ssid":	"my-wifi-ssid"}' https://<ip_address>/api/v1/system
```

```shell title="Response"
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 193

{
	"ssid":	"my-wifi-ssid",
	"password":	"my-wifi-pass",
	"hostname":	"my-hostname",
	"mac":	"68:b6:b3:ba:ad:ef",
	"link_up":	true,
	"ip":	833284106,
	"netmask":	15794175,
	"gateway":	27322378
}
```

### Identify

The `/api/v1/system/identify` endpoint can be used to let the user identify the device. The status
LEDs will blink for about 20 seconds after calling this endpoint.

#### Examples

```shell title="Request"
curl --insecure --include -X PUT https://<ip_address>/api/v1/system/identify
```

```shell title="Response"
HTTP/1.1 204 No Content
Content-Type: text/html
Content-Length: 0

```

### Reboot

The `/api/v1/system/reboot` endpoint allows you to request the MCM device to perform a reboot.

#### Examples

```shell title="Request"
curl --insecure --include -X PUT https://<ip_address>/api/v1/system/reboot
```

```shell title="Response"
HTTP/1.1 204 No Content
Content-Type: text/html
Content-Length: 0

```
