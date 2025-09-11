idf_component_register(
    SRCS "usb_device.c"
         "usb_descriptors.c"
         "vendor_device.c"
    INCLUDE_DIRS "include"
    REQUIRES bus_manager
             device_info
             device_status
             esp_ringbuf
             esp_tinyusb
             esp_timer
             intelhex
             json
             lin_master
             mlx_err
             networking
             ota_support
             ppm_bootloader
)
