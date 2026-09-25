idf_component_register(SRCS "main.c" "testcard.c" "editor.c" "selftest.c"
                            "serialkbd.c" "splash.c" "battery.c" "vitals.c"
                       INCLUDE_DIRS "."
                       REQUIRES st7305 textgrid docstore cmd seq kbd blemidi
                                usbmux usbdev net viz dinmidi ensemble driver esp_timer nvs_flash esp_adc)
