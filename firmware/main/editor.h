#pragma once
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"
#include "kbd.h"

esp_err_t editor_init(void);
void editor_draw(void);

/* Forget cached chrome so the next draw repaints all of it. */
void editor_invalidate(void);

/* Push what editor_draw/editor_blink rendered. Rendering does not push. */
void editor_present(size_t *bytes);
void editor_handle(const kbd_event_t *ev);
void editor_blink(bool on);
void editor_cursor_solid(void);
