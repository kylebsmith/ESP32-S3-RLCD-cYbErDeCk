idf_component_register(SRCS "main.c" "testcard.c" "editor.c" "selftest.c" "serialkbd.c"
                       INCLUDE_DIRS "."
                       REQUIRES st7305 textgrid docstore kbd driver esp_timer nvs_flash)
