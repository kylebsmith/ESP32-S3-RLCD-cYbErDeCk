/*
 * The boot screen.
 *
 * KILROY. The name is the owner's, and it is right for this object: Kilroy was
 * wartime graffiti - a line drawing and four words, scratched on walls by
 * people who had been somewhere. It is the least on-the-nose reference
 * available to an early-computer slab, it is monospace by nature, and it says
 * "someone was here and made this" rather than naming a product category.
 *
 * WHAT AN ANIMATION IS ALLOWED TO BE ON THIS PANEL. A full frame is 4.75 ms
 * and the liquid crystal is only clean to about 23 Hz, so anything trying to
 * be smooth will smear and read as a fault. What DOES read is discrete,
 * deliberate state changes with air between them - closer to a mechanical
 * split-flap than to a fade. So: the name arrives a letter at a time, a rule
 * sweeps under it once, and the numbers land. Nothing moves twice.
 *
 * It is also doing real work. Every frame is one partial flush, so if the
 * panel, the framebuffer, the damage list or the font are wrong, this is where
 * it shows - before any document is loaded and while the failure is still
 * cheap to read. A splash that only decorates would not have earned the
 * milliseconds.
 */
#include "splash.h"

#include <stdio.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "battery.h"
#include "docstore.h"
#include "st7305.h"
#include "textgrid.h"

#define NAME "KILROY"

static void beat(int ms)
{
    tg_render();
    st7305_flush(NULL);
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void splash_show(void)
{
    const int cols = tg_cols();
    const int rows = tg_rows();
    const int y = rows / 2 - 1;
    const int x = (cols - (int)strlen(NAME)) / 2;

    tg_clear();
    tg_render();
    st7305_flush_full();

    /* The name, one letter at a time. 70 ms is slow enough to read as
     * deliberate and fast enough that nobody waits for it. */
    for (size_t i = 0; i < strlen(NAME); i++) {
        tg_put(x + (int)i, y, NAME[i], TG_NORMAL);
        beat(70);
    }

    /* One rule, sweeping outward from under the name. It is drawn with the
     * cell attribute rather than a glyph so it costs nothing extra and lands
     * in the same damage rectangle as the row above it. */
    const int mid = cols / 2;
    for (int d = 0; d <= mid; d++) {
        if (mid - d >= 0)   { tg_put(mid - d, y + 1, '-', TG_NORMAL); }
        if (mid + d < cols) { tg_put(mid + d, y + 1, '-', TG_NORMAL); }
        if ((d % 3) == 0)   { beat(18); }
    }
    beat(120);

    /* What the machine is, in numbers, under the rule. Files, free internal
     * memory, and the battery when the hardware can say. */
    char line[40];
    const size_t heap = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    const int pct = battery_percent();
    if (pct >= 0) {
        snprintf(line, sizeof line, "%d docs  %uk free  %d%%",
                 doc_buf_count(), (unsigned)(heap / 1024), pct);
    } else {
        snprintf(line, sizeof line, "%d docs  %uk free",
                 doc_buf_count(), (unsigned)(heap / 1024));
    }
    const int lx = (cols - (int)strlen(line)) / 2;
    tg_puts(lx > 0 ? lx : 0, y + 3, line, TG_NORMAL);
    beat(650);

    tg_clear();
    tg_render();
    st7305_flush_full();
}
