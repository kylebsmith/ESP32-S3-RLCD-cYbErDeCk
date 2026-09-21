#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "kbd.h"

esp_err_t editor_init(void);

/* 0 low (12x24, 30 cols), 2 high (6x12, 60). Both faces are in the binary;
 * the panel's addressing quanta afford no middle size - see editor.c. */
esp_err_t editor_set_density(int level);
void editor_draw(void);

/* Forget cached chrome so the next draw repaints all of it. */
void editor_invalidate(void);

/* Monotonic count of cells drawn, for the progress invariant. */
uint32_t editor_cells_drawn(void);

/* Put a message on the status row for a few seconds. */
void editor_message(const char *m);

/* Pushes and bytes since the last call, for the liveness heartbeat. */
void editor_vitals(uint32_t *pushes, uint32_t *bytes,
                   uint32_t *render_us, uint32_t *cells);

/* Push what editor_draw/editor_blink rendered. Rendering does not push. */
void editor_present(size_t *bytes);
void editor_handle(const kbd_event_t *ev);
void editor_blink(bool on);
void editor_cursor_solid(void);
