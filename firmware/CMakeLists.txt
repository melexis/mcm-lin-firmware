cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS
    bus_manager
    device_info
    device_status
    mlx_err
    networking
    ota_support
    power_ctrl
    usb_device
    webserver
    www_bin
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(mcm-lin)
