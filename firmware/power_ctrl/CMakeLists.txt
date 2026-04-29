idf_component_register(
    SRCS "vn7040astr.c"
    INCLUDE_DIRS "include"
    REQUIRES driver
             esp_adc
    PRIV_REQUIRES
)
