# Melexis Compact Master LIN

## General

This project holds the frontend and firmware implementations for the Melexis Compact Master for LIN applications.

## Getting started

### Build Frontend

Read relevant [documentation](frontend\README.md)

### Build Firmware

Read relevant [documentation](firmware\README.md)

### Install Udev Rules

```
$ sudo apt install libusb-1.0-0-dev
$ python3 - m pip install pyusb
$ sudo vi /lib/udev/rules.d/90-mcm-lin.rules
SUBSYSTEMS=="usb", ATTRS{idVendor}=="03e9", ATTRS{idProduct}=="6f09", GROUP="plugdev", MODE="660", ENV{MODALIAS}="ignore"
```

## Application Interfaces

### REST API

See [REST API documentation](REST_API.md)

### Websocket

See [WebSocket API documentation](WSS_API.md)

## License

Copyright (C) 2025 Melexis N.V.

The Software is being delivered 'AS IS' and Melexis, whether explicitly or implicitly, makes no warranty as to its Use or performance.
The user accepts the Melexis Firmware License Agreement.

Melexis confidential & proprietary
