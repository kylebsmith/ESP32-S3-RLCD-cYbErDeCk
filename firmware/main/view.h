#pragma once
#include <stdint.h>

/* The picture to the view node: see view.c. */
void view_init(void);
void view_frame(void);
/* Frames sent, and frames dropped because the console had no room. */
extern uint32_t view_frames, view_dropped;
