/* The minimum of esp_err.h needed to include a firmware header - or, for
 * viz.c, a whole firmware SOURCE file - on the host.
 *
 * kbd.h includes esp_err.h for one typedef. Without this shim the constants
 * check below could not compile off-target, and the alternative - copying
 * kbd.h's enum into the test - is exactly the drift the test exists to catch.
 *
 * The error codes here are only the ones the code under test returns. Adding
 * one is fine; copying the whole of IDF's table would not be, because then a
 * test could pass against a constant the firmware never sees.
 */
#ifndef HOSTSHIM_ESP_ERR_H
#define HOSTSHIM_ESP_ERR_H
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_NOT_FOUND       0x105
#define ESP_ERR_INVALID_ARG     0x102
#define ESP_ERR_INVALID_STATE   0x103
#endif
