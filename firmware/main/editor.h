#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "kbd.h"

esp_err_t editor_init(void);

/* 0 = 12x24 chunky, 1 = 6x12 dense. Both faces are already in the binary. */
esp_err_t editor_set_density(int dense);
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
