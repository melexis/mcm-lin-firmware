idf_component_register(
    SRCS "http_webserver.c"
         "urihandlers_rest.c"
         "urihandlers_wss.c"
         "urihandlers_www.c"
         "webserver.c"
    INCLUDE_DIRS "include"
    EMBED_TXTFILES "certs/servercert.pem"
                  "certs/prvtkey.pem"
    REQUIRES bus_manager
             device_info
             device_status
             esp_http_server
             esp_https_server
             esp_timer
             json
             lin_master
             mlx_err
             networking
             power_ctrl
             ppm_bootloader
             www_bin
)
