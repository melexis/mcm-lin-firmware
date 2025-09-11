idf_component_register(SRCS bus_manager.c
                       INCLUDE_DIRS include
                       REQUIRES driver
                                ppm_bootloader
                                lin_master)
