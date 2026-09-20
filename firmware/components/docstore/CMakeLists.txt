idf_component_register(SRCS "buffer.c" "journal.c" "sdmirror.c" "undo.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_partition fatfs esp_driver_sdmmc sdmmc vfs)
