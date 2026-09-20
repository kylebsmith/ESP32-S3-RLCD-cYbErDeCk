/* The minimum of esp_err.h needed to include a firmware header on the host.
 *
 * kbd.h includes esp_err.h for one typedef. Without this shim the constants
 * check below could not compile off-target, and the alternative - copying
 * kbd.h's enum into the test - is exactly the drift the test exists to catch.
 */
#ifndef HOSTSHIM_ESP_ERR_H
#define HOSTSHIM_ESP_ERR_H
typedef int esp_err_t;
#define ESP_OK 0
#endif
