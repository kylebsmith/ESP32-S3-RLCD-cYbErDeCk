/* The minimum of esp_log.h needed to compile a firmware SOURCE file on the
 * host - textgrid.c, for tools/test_textgrid.c. The logging macros print, so a
 * layout the firmware refuses says why in the test's output too. */
#ifndef HOSTSHIM_ESP_LOG_H
#define HOSTSHIM_ESP_LOG_H
#include <stdio.h>
#define ESP_LOGE(tag, fmt, ...) printf("  (%s) " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...) printf("  (%s) " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGI(tag, fmt, ...) ((void)(tag))
#endif
