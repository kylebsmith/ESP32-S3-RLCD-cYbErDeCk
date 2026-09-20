/*
 * ST7305 reflective-LCD driver for the Waveshare ESP32-S3-RLCD-4.2.
 *
 * Derived from the ST7305 driver in SolarOS (github.com/nilseuropa/solar_os,
 * src/drivers/rlcd_st7305.c), Copyright 2026 nilseuropa, Apache License 2.0.
 * See ../../NOTICE. The power-on register sequence, the CASET mirroring rule
 * and the 4x2 bit-packing table are taken from that work; the framebuffer
 * layout, the landscape mapping and the damage model here are not.
 *
 * Geometry, in two frames, because confusing them is the whole difficulty:
 *
 *   NATIVE (the controller's own frame)   300 wide (x) x 400 tall (y)
 *     - one RAM byte  = 4 x-pixels  x 2 y-pixels
 *     - one CASET unit = 12 x-pixels = 3 bytes,  addresses 0x12..0x2A
 *     - one RASET unit =  2 y-pixels,            addresses 0x00..0xC7
 *
 *   LOGICAL (what the device presents)    400 wide x 300 tall, landscape
 *     - logical x -> native y   (2 px quantum)
 *     - logical y -> native x  (12 px quantum)
 *
 * So 12 px is the hardware's own line height and a 6x12 character cell costs
 * exactly 9 bytes with no read-modify-write. docs/HARDWARE.md derives this.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Geometry and the address arithmetic live in st7305_addr.h, which has no
 * dependencies so the host-side check in tools/ can exercise the same code. */
#include "st7305_addr.h"

/* The landscape mapping has a two-way ambiguity per axis that no datasheet
 * settles: it depends on how the glass sits in the module. All four are
 * selectable and KEY cycles them, so a wrong guess costs a button press and
 * not a reflash. The choice is persisted in NVS.
 *
 *   0   nx = y          ny = x              (no flip)
 *   1   nx = y          ny = 399 - x        (horizontal mirror)
 *   2   nx = 299 - y    ny = 399 - x        (180 rotation)
 *   3   nx = 299 - y    ny = x              (vertical mirror)
 */
typedef enum {
    ST7305_ORIENT_0 = 0,
    ST7305_ORIENT_1 = 1,
    ST7305_ORIENT_2 = 2,
    ST7305_ORIENT_3 = 3,
} st7305_orient_t;

void st7305_set_orientation(st7305_orient_t o);
st7305_orient_t st7305_orientation(void);

typedef enum {
    ST7305_POWER_AUTO = 0,  /* HPM while changing, LPM after an idle timeout */
    ST7305_POWER_HPM  = 1,
    ST7305_POWER_LPM  = 2,
} st7305_power_policy_t;

typedef struct {
    /* Damage rectangle in LOGICAL coordinates, inclusive. Empty when x0 > x1. */
    int16_t x0, y0, x1, y1;
} st7305_damage_t;

esp_err_t st7305_init(void);

/* The framebuffer, in controller order, in internal DMA SRAM. */
uint8_t *st7305_framebuffer(void);

/* Set/clear one logical pixel and extend the damage rectangle. */
void st7305_pixel(int x, int y, bool on);

/* Fill a logical rectangle. */
void st7305_fill(int x, int y, int w, int h, bool on);

void st7305_clear(bool on);

/* Mark a logical rectangle damaged without drawing (for callers that write
 * the framebuffer directly through st7305_framebuffer()). */
void st7305_damage(int x, int y, int w, int h);

/* Push the damaged region. Quantises out to the controller's 12px/2px grid.
 * Returns ESP_OK and sets *bytes_sent when there was something to send. */
esp_err_t st7305_flush(size_t *bytes_sent);

/* Push everything, ignoring damage. */
esp_err_t st7305_flush_full(void);

esp_err_t st7305_set_power_policy(st7305_power_policy_t policy);
esp_err_t st7305_set_hpm(void);

/* INVON/INVOFF - which way round ink and paper are. */
esp_err_t st7305_set_inverted(bool inverted);
esp_err_t st7305_set_lpm(void);

/* Read the controller ID (command 0x04) at the slow read clock. This is the
 * only evidence available that the panel is physically present and answering,
 * as opposed to writes vanishing into an unconnected bus. */
esp_err_t st7305_read_id(uint8_t out[3]);

#ifdef __cplusplus
}
#endif
