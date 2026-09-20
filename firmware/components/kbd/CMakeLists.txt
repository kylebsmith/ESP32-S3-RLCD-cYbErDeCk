idf_component_register(SRCS "ble_kbd.c" "keymap.c"
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS "."
                       REQUIRES bt esp_timer nvs_flash)
