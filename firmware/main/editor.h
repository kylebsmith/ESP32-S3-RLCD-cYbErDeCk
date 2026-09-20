#pragma once
#include <stdbool.h>
#include "esp_err.h"
#include "kbd.h"

esp_err_t editor_init(void);
void editor_draw(void);
void editor_handle(const kbd_event_t *ev);
void editor_blink(bool on);
void editor_cursor_solid(void);
