idf_component_register(SRCS "gapbuf.c" "journal.c" "sdmirror.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_partition fatfs esp_driver_sdmmc sdmmc vfs)
