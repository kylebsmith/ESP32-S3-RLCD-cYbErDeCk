idf_component_register(SRCS "usbdev.c"
                       INCLUDE_DIRS "include"
                       REQUIRES kbd docstore usbmux esp_timer nvs_flash driver
                                soc espressif__esp_tinyusb)
