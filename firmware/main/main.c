/*
 * cYbErDeCk OS - entry point.
 *
 * Steps 1-4 of the build order in docs/OS.md: display, text grid, BLE HID
 * keyboard, and a text buffer that survives power loss.
 *
 * The acceptance test is: power on, the keyboard connects by itself, type a
 * paragraph, pull the power, power on, the paragraph is still there.
 */
#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "docstore.h"
#include "editor.h"
#include "kbd.h"
#include "selftest.h"
#include "serialkbd.h"
#include "st7305.h"
#include "testcard.h"
#include "textgrid.h"

static const char *TAG = "cyberdeck";

#define PIN_KEY        18
#define KEY_LONG_MS  2000        /* >= 180 ms thresholds, per the BLE caveat */
#define AUTOSAVE_MS  1000
/* A save costs a whole flash sector, and the idle timer alone fires once per
 * character for a slow typist - 128 characters cycles the entire partition.
 * A save is therefore also gated on enough having changed, unless a newline
 * or a long pause says the thought is finished. */
#define AUTOSAVE_MIN_CHARS  24
#define AUTOSAVE_IDLE_MS  6000
/* The cursor blinks while you are typing and for a while after, then goes
 * solid so the idle-LPM policy can fire. docs/OS.md bans blink outright for a
 * slow panel; the cost is not the 36 bytes a blink puts on the wire, it is
 * that an endless blink keeps the panel pinned in HPM. Bounding it keeps both
 * the affordance and the power behaviour. */
#define BLINK_MS       500
#define BLINK_WINDOW  15000
#define BUILD_ID (__DATE__ " " __TIME__)
#define NVS_NS "deck"

esp_err_t sdmirror_init(void);

static void report_memory(const char *when)
{
    ESP_LOGI(TAG, "%s: internal free %u B, largest %u B, PSRAM free %u B", when,
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

static void orient_save(uint8_t v);

/* The shipped default, used only when nothing has been chosen yet. An
 * orientation the owner set by hand is ALWAYS honoured - a stored value is
 * evidence about the physical build, which is knowledge this firmware does
 * not have and must not overwrite. */
#define ORIENT_DEFAULT  ST7305_ORIENT_3

static uint8_t orient_load(void)
{
    nvs_handle_t h;
    uint8_t v = ORIENT_DEFAULT;
    bool stored = false;

    if (nvs_open(NVS_NS, NVS_READONLY, &h) == ESP_OK) {
        stored = nvs_get_u8(h, "orient", &v) == ESP_OK;
        nvs_close(h);
    }
    if (v > 3) {
        v = ORIENT_DEFAULT;
        stored = false;
    }
    ESP_LOGI(TAG, "orientation %d (%s)", v,
             stored ? "chosen on this device" : "shipped default");
    if (!stored) {
        orient_save(v);
    }
    return v;
}

static void orient_save(uint8_t v)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_u8(h, "orient", v);
        nvs_commit(h);
        nvs_close(h);
    }
}

static void bench(void)
{
    const int runs = 20;
    int64_t t0 = esp_timer_get_time();
    for (int i = 0; i < runs; i++) {
        st7305_flush_full();
    }
    ESP_LOGI(TAG, "BENCH full frame: %lld us (%d B)",
             (esp_timer_get_time() - t0) / runs, ST7305_FB_SIZE);
}

static int64_t now_ms(void) { return esp_timer_get_time() / 1000; }

/* The passkey arrives on the NimBLE host task. It is only RECORDED here; the
 * main task draws it on its next pass.
 *
 * Drawing it directly raced the main task over the text grid, the framebuffer
 * and the SPI bus - three pieces of shared state with no lock between them -
 * and would have produced a torn screen at exactly the moment the owner most
 * needs to read six digits correctly. */
static volatile uint32_t s_passkey;
static volatile bool     s_passkey_pending;

void kbd_on_passkey(uint32_t passkey)
{
    s_passkey = passkey;
    s_passkey_pending = true;
}

static void draw_passkey(uint32_t passkey)
{
    char line[32];
    snprintf(line, sizeof line, "%06u", (unsigned)passkey);

    tg_clear();
    tg_puts(1, 2, "PAIR THE KEYBOARD", TG_NORMAL);
    tg_fill(1, 4, 12, ' ', TG_INVERSE);
    tg_puts(4, 4, line, TG_INVERSE);
    tg_puts(1, 6, "Type that on the", TG_NORMAL);
    tg_puts(1, 7, "keyboard, then press", TG_NORMAL);
    tg_puts(1, 8, "Enter.", TG_NORMAL);
    tg_render();
    st7305_flush_full();
}

void app_main(void)
{
    ESP_LOGI(TAG, "cYbErDeCk OS  build %s", BUILD_ID);
    report_memory("boot");

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    if (st7305_init() != ESP_OK) {
        ESP_LOGE(TAG, "display init FAILED - stopping");
        return;
    }
    uint8_t id[3] = {0};
    if (st7305_read_id(id) == ESP_OK) {
        ESP_LOGI(TAG, "panel RDDID -> %02X %02X %02X%s", id[0], id[1], id[2],
                 (id[0] | id[1] | id[2]) == 0 ? "  (all zero - the bus may not"
                 " read back on this board; nothing depends on it)" : "");
    } else {
        ESP_LOGW(TAG, "panel RDDID read failed - nothing depends on it");
    }

    uint8_t orient = orient_load();
    st7305_set_orientation((st7305_orient_t)orient);
    ESP_ERROR_CHECK(tg_set_font(&tg_font_12x24, 1));     /* test card: flush grid */

    /* Show the card briefly so a boot is visibly a boot, then get out of the
     * way. If the text reads mirrored, KEY cycles the orientation. */
    testcard_draw(BUILD_ID);
    bench();
    vTaskDelay(pdMS_TO_TICKS(2500));

    if (doc_init() != ESP_OK) {
        ESP_LOGE(TAG, "docstore init FAILED");
    }
    if (selftest_run()) {
        ESP_LOGW(TAG, "a self-test stage ran; reset to advance it");
    }
    sdmirror_init();                 /* a missing card is not fatal */
    report_memory("after docstore");

    if (kbd_init() != ESP_OK) {
        ESP_LOGE(TAG, "BLE keyboard init FAILED");
    }
    serialkbd_init();                /* the cable is a keyboard too */
    report_memory("after BLE");

    const gpio_config_t key = {
        .pin_bit_mask = 1ULL << PIN_KEY,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&key);

    ESP_ERROR_CHECK(editor_init());      /* margins; 30 x 10 inside them */
    tg_invalidate();
    st7305_clear(false);
    editor_draw();
    size_t bytes = 0;
    editor_present(&bytes);
    ESP_LOGI(TAG, "editor up - KEY taps cycle orientation, KEY held %d ms "
                  "forgets all keyboard bonds", KEY_LONG_MS);

    int64_t key_down_at = 0;
    bool    key_was_down = false;
    bool    long_fired = false;
    int64_t last_edit_ms = now_ms();
    int64_t last_blink_ms = now_ms();
    int64_t last_beat_ms  = now_ms();
    bool    blink_on = true;
    bool    need_draw = false;
    bool    force_save = false;
    size_t  saved_len = doc_len();

    while (1) {
        kbd_event_t ev;
        bool acted = false;

        /* Input is drained tightly so a burst of keystrokes costs one redraw
         * rather than one redraw each. */
        while (kbd_poll(&ev, 5)) {
            if (ev.type == KBD_EV_CONNECTED || ev.type == KBD_EV_DISCONNECTED) {
                need_draw = true;
                continue;
            }
            editor_handle(&ev);
            acted = true;
            need_draw = true;
            if (ev.type == KBD_EV_ENTER) {
                force_save = true;         /* a finished line is worth flash */
            }
        }
        if (acted) {
            last_edit_ms  = now_ms();
            last_blink_ms = now_ms();
            blink_on = true;
            editor_cursor_solid();   /* never blink away mid-keystroke */
        }

        if (s_passkey_pending) {
            s_passkey_pending = false;
            draw_passkey(s_passkey);
            tg_invalidate();
            editor_invalidate();
            need_draw = true;        /* restore the editor afterwards */
        }

        /* KEY: tap cycles orientation, long hold forgets bonds. */
        const bool down = gpio_get_level(PIN_KEY) == 0;
        if (down && !key_was_down) {
            key_down_at = now_ms();
            long_fired = false;
        } else if (down && !long_fired && now_ms() - key_down_at >= KEY_LONG_MS) {
            kbd_forget_all();
            long_fired = true;
            need_draw = true;
        } else if (!down && key_was_down && !long_fired) {
            orient = (uint8_t)((orient + 1) & 3);
            st7305_set_orientation((st7305_orient_t)orient);
            orient_save(orient);
            ESP_LOGI(TAG, "orientation -> %d (saved)", orient);
            tg_invalidate();
            editor_invalidate();
            st7305_clear(false);
            need_draw = true;
        }
        key_was_down = down;

        if (need_draw) {
            editor_draw();
            editor_present(&bytes);
            need_draw = false;
        }

        /* Blink, bounded. One cell, 36 bytes. */
        const int64_t since_edit = now_ms() - last_edit_ms;
        if (since_edit < BLINK_WINDOW) {
            if (now_ms() - last_blink_ms >= BLINK_MS) {
                last_blink_ms = now_ms();
                blink_on = !blink_on;
                editor_blink(blink_on);
                editor_present(&bytes);
            }
        } else if (!blink_on) {
            blink_on = true;
            editor_cursor_solid();
            editor_present(&bytes);
        }

        /* Liveness. The main task printing this is proof the loop is
         * turning; a silent log used to be ambiguous between idle and hung,
         * which cost real debugging time. */
        if (now_ms() - last_beat_ms >= 10000) {
            last_beat_ms = now_ms();
            uint32_t pushes = 0, pbytes = 0;
            editor_vitals(&pushes, &pbytes);
            ESP_LOGI(TAG, "alive: doc %u%s, undo %d, kbd %s, %u push/%u B, "
                          "heap %u",
                     (unsigned)doc_len(), doc_dirty() ? "*" : "",
                     doc_undo_depth(),
                     kbd_connected() ? "up" : kbd_state_name(),
                     (unsigned)pushes, (unsigned)pbytes,
                     (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
        }

        /* Autosave: on newline, or once typing has paused. Never per
         * keystroke - docs/HANDOFF.md trap 6. */
        if (doc_dirty()) {
            const int64_t idle = now_ms() - last_edit_ms;
            const size_t len = doc_len();
            const size_t delta = len > saved_len ? len - saved_len
                                                 : saved_len - len;
            const bool worth_it = force_save ||
                                  delta >= AUTOSAVE_MIN_CHARS ||
                                  idle >= AUTOSAVE_IDLE_MS;
            if (worth_it && idle >= AUTOSAVE_MS) {
                const int64_t t0 = esp_timer_get_time();
                const esp_err_t se = doc_save();
                const int64_t dt = esp_timer_get_time() - t0;
                if (se == ESP_OK) {
                    saved_len = doc_len();
                    ESP_LOGI(TAG, "saved %u bytes, seq %u, %lld us",
                             (unsigned)doc_len(), (unsigned)doc_save_seq(), dt);
                    doc_mirror_sd();
                } else {
                    ESP_LOGE(TAG, "save failed: %s", esp_err_to_name(se));
                }
                force_save = false;
                last_edit_ms = now_ms();
                need_draw = true;
            }
        }
    }
}
