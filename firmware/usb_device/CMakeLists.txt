idf_component_register(
    SRCS "src/usb_descriptors.c"
         "src/usb_device.c"
         "src/usb_vendor_bulk.c"
         "src/vendor_device.c"
         "src/vendor_req_hndl/usb_vendor_btl_ppm.c"
         "src/vendor_req_hndl/usb_vendor_config.c"
         "src/vendor_req_hndl/usb_vendor_hex_transfer.c"
         "src/vendor_req_hndl/usb_vendor_identify.c"
         "src/vendor_req_hndl/usb_vendor_lin_comm.c"
         "src/vendor_req_hndl/usb_vendor_ota.c"
         "src/vendor_req_hndl/usb_vendor_req_info.c"
         "src/vendor_req_hndl/usb_vendor_reset.c"
         "src/vendor_req_hndl/usb_vendor_slave_power.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src"
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
    PRIV_REQUIRES
)
