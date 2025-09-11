idf_component_register(SRCS "networking.c" "wifi.c"
                       INCLUDE_DIRS "include"
                       REQUIRES device_info
                                esp_timer
                                esp_wifi
                                mdns
                                nvs_flash
                                webserver)
