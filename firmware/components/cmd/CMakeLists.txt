idf_component_register(SRCS "cmd.c" "builtins.c"
                       INCLUDE_DIRS "include"
                       REQUIRES docstore textgrid seq usbmux blemidi usbdev net ssh viz dinmidi kbd main)
