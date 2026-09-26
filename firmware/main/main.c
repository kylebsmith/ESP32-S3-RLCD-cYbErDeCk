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
#include "soc/rtc_cntl_reg.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "dinmidi.h"
#include "ensemble.h"
#include "vitals.h"
#include "view.h"
#include "ssh.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "cmd.h"
#include "blemidi.h"
#include "seq.h"
#include "docstore.h"
#include "editor.h"
#include "ui_text.h"
#include "net.h"
#include "usbdev.h"
#include "viz.h"
#include "usbmux.h"
#include "kbd.h"
#include "selftest.h"
#include "battery.h"
#include "serialkbd.h"
#include "splash.h"
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

/* WHAT A CELL COSTS TO DRAW, with nothing else running - both ways, in the
 * same boot, so the before and the after are one board's numbers. The editor's
 * heartbeat reports render time by the wall clock, which counts every
 * preemption by the radio and the keyboard scan as drawing; this holds the
 * scheduler. A full grid of mixed glyphs, a third of them inverted, drawn into
 * the framebuffer (no panel push). */
static void bench_render(void)
{
    const int cols = tg_cols(), rows = tg_rows();
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            tg_put(c, r, (char)(32 + (r * cols + c) % 124),
                   ((c + r) % 3 == 0) ? TG_INVERSE : TG_NORMAL);
        }
    }
    const int runs = 4;
    int64_t per[2] = { 0, 0 }, first = 0;
    int cells = 0;
    for (int turned = 0; turned < 2; turned++) {
        tg_set_turned(turned != 0);
        vTaskSuspendAll();
        /* The first turned frame turns every glyph it meets, once. */
        int64_t t0 = esp_timer_get_time();
        tg_invalidate();
        cells = tg_render();
        if (turned) {
            first = esp_timer_get_time() - t0;
        }
        t0 = esp_timer_get_time();
        for (int i = 0; i < runs; i++) {
            tg_invalidate();
            tg_render();
        }
        per[turned] = (esp_timer_get_time() - t0) * 1000 / (runs * cells);
        xTaskResumeAll();
    }
    /* and what damage bookkeeping alone costs a cell, which the turned path
     * still pays in full */
    vTaskSuspendAll();
    const int64_t t2 = esp_timer_get_time();
    for (int i = 0; i < runs; i++) {
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                st7305_damage(20 + c * 12, 12 + r * 24, 12, 24);
            }
        }
    }
    const int64_t dmg = (esp_timer_get_time() - t2) * 1000 / (runs * cells);
    xTaskResumeAll();
    ESP_LOGI(TAG, "BENCH cell render: per-row %lld ns, turned %lld ns a cell "
             "(%d cells, scheduler held)", per[0], per[1], cells);
    ESP_LOGI(TAG, "BENCH   first turned frame %lld us, damage %lld ns a cell",
             first, dmg);
    tg_clear();
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

/* The destinations. Each is a function and a name the player can type; none
 * of them is privileged and none is compiled in as "the" output.
 *
 * 'mon' was a rate limit - the first 64 note-ons went to the console and the
 * rest were dropped, silently, forever. That is fine for proving the thing
 * boots and useless the moment you want to check what you are actually
 * sending, which is the second thing anyone wants. It is a destination now,
 * off by default, and it can be turned on for as long as it is wanted. */
/* '>flash' needs the panel to say so before the chip goes away. The editor
 * owns the status line and the draw, so the app is the only layer that can
 * do this - which is why cmd takes it as a hook. */
static void announce(const char *line)
{
    editor_message(line);
    editor_draw();
    st7305_flush(NULL);
}

static void dest_ble(const char *lane, uint8_t status, uint8_t d1, uint8_t d2,
                     uint32_t when_us)
{
    (void)lane;
    blemidi_send(status, d1, d2, when_us);
}

static void dest_usb(const char *lane, uint8_t status, uint8_t d1, uint8_t d2,
                     uint32_t when_us)
{
    (void)lane;
    /* USB MIDI carries no timestamp: the host renders on arrival, and a
     * full-speed frame is 1 ms wide. That is the whole reason USB is the
     * answer to jitter rather than a second way to have the same problem. */
    (void)when_us;
    usbdev_midi_send(status, d1, d2);
}

static void dest_mon(const char *lane, uint8_t status, uint8_t d1, uint8_t d2,
                     uint32_t when_us)
{
    /* Notes on AND off, and controllers, each with its lane and the moment the
     * clock decided it. Offs are here because a tie is a note that ends later,
     * and nothing else on the device can show how long a note was. Clock and
     * the step marker stay out: they are fifty messages a second and would bury
     * the thing the player is looking for. */
    const unsigned ms = (unsigned)(when_us / 1000u);
    const uint8_t kind = status & 0xF0;
    if (kind == 0x90 && d2 > 0) {
        ESP_LOGI("midi", "%-5s on  %3u v%-3u ch%-2u t%u", lane, d1, d2,
                 (status & 0x0F) + 1, ms);
    } else if (kind == 0x80 || (kind == 0x90 && d2 == 0)) {
        ESP_LOGI("midi", "%-5s off %3u      ch%-2u t%u", lane, d1,
                 (status & 0x0F) + 1, ms);
    } else if (kind == 0xB0 && d1 != 123) {
        ESP_LOGI("midi", "%-5s cc%-3u = %3u   ch%-2u t%u", lane, d1, d2,
                 (status & 0x0F) + 1, ms);
    }
}

/* THE DESTINATIONS THIS FIRMWARE REGISTERS. Counted here so the table cannot
 * be outgrown silently again: ble, mon, din, osc are unconditional and usb is
 * added when USB MIDI comes up. If a transport is added without room for it,
 * this fails the build instead of failing a performance. */
#define DECK_DESTS 5
_Static_assert(DECK_DESTS <= SEQ_MAX_DESTS,
               "more destinations than seq can hold - raise SEQ_MAX_DESTS");

static uint32_t s_boot_loops;
static bool     s_crashed_last_boot;

static int64_t now_ms(void) { return esp_timer_get_time() / 1000; }

/* The menu is a text file (docs/SUBSTRATE.md). If there is no guide yet,
 * write one - a device whose commands are undiscoverable has, in practice,
 * no commands. The owner can edit it like any other document, which is the
 * whole point: adding a menu item costs typing a line. */
/* Create the boot document if it is missing, then run it. */
static void run_boot_document(void)
{
    const int was = doc_buf_current();
    int idx = doc_buf_find("boot");
    if (idx < 0) {
        if (doc_buf_new() != ESP_OK) {
            return;
        }
        doc_set_text(BOOT_TEXT);
        doc_buf_set_kind(DOC_KIND_GUIDE);
        if (doc_buf_rename("boot") != ESP_OK) {
            doc_buf_select(was);
            return;
        }
        doc_save();
        idx = doc_buf_current();
        ESP_LOGI(TAG, "wrote a starter boot document");
    }
    if (doc_buf_select(idx) != ESP_OK) {
        doc_buf_select(was);
        return;
    }
    /* A BOOT DOCUMENT FROM BEFORE THE NAMES WERE LINES has no names in it, and
     * without them '>kick' means nothing. So the names go in at the TOP - they
     * must exist before any lane line below them runs - and everything the owner
     * wrote stays exactly as it was, underneath. The mark is the first line of
     * the block; a document that has it is left alone. */
    {
        const size_t mlen = sizeof BOOT_MARK - 1;
        bool marked = false;
        for (size_t i = 0; i + mlen <= doc_len() && !marked; i++) {
            size_t j = 0;
            while (j < mlen && doc_at(i + j) == BOOT_MARK[j]) { j++; }
            marked = (j == mlen);
        }
        if (!marked) {
            doc_move_to(0);
            for (const char *q = BOOT_NAMES; *q != '\0'; q++) {
                doc_insert(*q);
            }
            doc_save();
            ESP_LOGW(TAG, "boot document: the lane names were added at its top");
        }
    }
    char line[128];
    size_t k = 0;
    const size_t n = doc_len();
    int ran = 0;
    for (size_t i = 0; i <= n; i++) {
        const char ch = (i < n) ? doc_at(i) : '\n';
        if (ch == '\n' || k == sizeof line - 1) {
            line[k] = '\0';
            if (k > 0 && cmd_run_line(line, CMD_BY_GUIDE, NULL, 0) == CMD_DONE) {
                ran++;
            }
            k = 0;
            continue;
        }
        line[k++] = ch;
    }
    doc_buf_select(was);
    ESP_LOGI(TAG, "boot document: %d line(s) ran", ran);
}

static void ensure_guide_buffer(void)
{
    for (int i = 0; i < DOC_MAX_BUFFERS; i++) {
        if (strcmp(doc_buf_name(i), "guide") == 0) {
            /* A guide written before the sigil existed holds lines that will
             * never run again. Rewrite it once rather than leaving a menu
             * whose buttons silently do nothing. */
            const int was = doc_buf_current();
            if (doc_buf_select(i) != ESP_OK) {
                return;
            }
            /* Has this guide seen the music layer? '>play' is the marker.
             *
             * PREPEND, DO NOT REPLACE. The guide is the owner's menu - the
             * whole claim of docs/SUBSTRATE.md is that they write their own
             * interface - so firmware that overwrites it destroys exactly the
             * thing the design is for. The new track goes on top, where it is
             * read first, and whatever was there stays underneath. */
            bool has_play = false, has_mark = false;
            const size_t mlen = sizeof GUIDE_MARK - 1;
            for (size_t k = 0; k + 5 <= doc_len(); k++) {
                if (doc_at(k) == '>' && doc_at(k+1) == 'p' && doc_at(k+2) == 'l' &&
                    doc_at(k+3) == 'a' && doc_at(k+4) == 'y') {
                    has_play = true;
                }
                if (k + mlen <= doc_len()) {
                    size_t j = 0;
                    while (j < mlen && doc_at(k + j) == GUIDE_MARK[j]) { j++; }
                    if (j == mlen) { has_mark = true; }
                }
            }
            /* AND HAS IT SEEN THIS GRAMMAR? A guide from before docs/MANIFESTO.md
             * §3.6 teaches 'X...' and 'x,x?' - lines that are now refused - so it
             * gets the new text too, on top, with everything the owner wrote kept
             * underneath. */
            if (!has_play || !has_mark) {
                const size_t had = doc_len();
                char *keep = malloc(had + 1);
                if (keep != NULL) {
                    doc_read(keep, had);
                    keep[had] = '\0';
                }
                doc_set_text(GUIDE_TEXT);
                doc_move_to(doc_len());     /* append, not prepend */
                if (keep != NULL && had > 0) {
                    static const char sep[] = "\n-- previously in this guide --\n";
                    for (const char *q = sep; *q != '\0'; q++) { doc_insert(*q); }
                    for (const char *q = keep; *q != '\0'; q++) { doc_insert(*q); }
                }
                free(keep);
                doc_buf_set_kind(DOC_KIND_GUIDE);
                doc_save();
                ESP_LOGW(TAG, "guide updated; the old text is kept below it");
            }
            doc_buf_select(was);
            return;
        }
    }
    const int was = doc_buf_current();
    if (doc_buf_new() != ESP_OK) {
        return;
    }
    doc_set_text(GUIDE_TEXT);
    doc_buf_set_kind(DOC_KIND_GUIDE);
    if (doc_buf_rename("guide") == ESP_OK) {
        doc_save();
        ESP_LOGI(TAG, "wrote a starter guide buffer");
    }
    doc_buf_select(was);
}

int64_t editor_now_ms(void) { return now_ms(); }

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
    /* Belt and braces after '>flash'. The force-download bit lives in the RTC
     * domain and survives a CPU reset, so it can only be cleared by code that
     * is running - which means here, at the first opportunity the app gets.
     * Without this, a system reset that happened to preserve the domain would
     * send the deck back into download mode with no explanation. */
    /* A BOOT LOOP MUST SAY SO.
     *
     * Panics now reboot silently, which is right for recovery and wrong for
     * diagnosis: a deck crashing every two seconds looks exactly like a deck
     * sitting quietly doing nothing. This counts reboots that were NOT a
     * power-on, and says so on the panel - the one surface that still works
     * when the console does not. Cleared by power loss, which is also the
     * gesture that fixes most of what causes it. */
    {
        static RTC_NOINIT_ATTR uint32_t loop_magic;
        static RTC_NOINIT_ATTR uint32_t loop_n;
        const esp_reset_reason_t rr = esp_reset_reason();
        if (loop_magic != 0xB0070009u) { loop_magic = 0xB0070009u; loop_n = 0; }
        if (rr == ESP_RST_POWERON) {
            loop_n = 0;
        } else if (rr == ESP_RST_PANIC || rr == ESP_RST_TASK_WDT ||
                   rr == ESP_RST_INT_WDT || rr == ESP_RST_WDT) {
            loop_n++;
            s_crashed_last_boot = true;
        }
        s_boot_loops = loop_n;
    }

    /* WHY DID WE JUST BOOT? Logged first, because after a silent panic reboot
     * this is the only surviving evidence that anything went wrong - and a
     * deck that reboots itself and says nothing is indistinguishable from one
     * the owner power-cycled. */
    {
        const esp_reset_reason_t r = esp_reset_reason();
        static const char *why[] = {
            "unknown", "power on", "external pin", "software", "panic",
            "interrupt watchdog", "task watchdog", "other watchdog",
            "deep sleep", "brownout", "sdio", "usb", "jtag",
        };
        ESP_LOGW(TAG, "boot: %s",
                 ((unsigned)r < sizeof why / sizeof why[0]) ? why[r] : "?");
    }

    REG_WRITE(RTC_CNTL_OPTION1_REG, 0);

    /* And hand the USB PHY back to USB-Serial-JTAG, for the same reason and
     * with one difference that matters: a panic reset does NOT run shutdown
     * handlers - panic_restart() calls esp_restart_noos() directly - so this
     * line, here, is the only restore that runs after a crash. See usbmux.h
     * for why nothing else puts these bits back, including the ROM. */
    usbmux_release_to_usj();

    ESP_LOGI(TAG, "cYbErDeCk OS  build %s", BUILD_ID);
    report_memory("boot");

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    /* WHAT THE LAST RUN WAS DOING WHEN IT STOPPED. Read as soon as NVS is up and
     * before anything can overwrite it, and logged loudly, because the console
     * that would otherwise have reported the failure is one of the things that
     * dies in the hang this exists to catch - docs/OS.md. */
    vitals_begin();

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
    bench_render();
    testcard_draw(BUILD_ID);
    bench();
    vTaskDelay(pdMS_TO_TICKS(2500));

    /* Until a transport exists, notes go to the log. The sequencer does not
     * know the difference, which is the point of the sink being a function
     * pointer: BLE MIDI, USB MIDI and a UART all plug in here without the
     * musical core changing. */
    cmd_set_announce(announce);
    /* ONE HOOK, AND IT ONLY RECORDS. seq owns every lane now, including the
     * drawing ones, so it no longer needs telling when a step happened or what
     * a lane played - it knows both. What it does not know is what a primitive
     * is, so a drawing lane hands viz an index and an amount and the main loop
     * turns that into a picture. */
    seq_set_draw_hook(viz_mark);
    seq_set_param_hook(viz_mark_param);
    /* CHECKED, NOT ASSUMED. Every one of these used to throw its return value
     * away, and when the table filled up the fifth transport vanished with no
     * message anywhere - see SEQ_MAX_DESTS. A destination that fails to
     * register is a transport the owner will spend an evening debugging with a
     * cable and a DAW, so it says so here, loudly, on the one surface that
     * still works when the console does not. */
    seq_dest_add("ble", dest_ble, blemidi_flush, "BLE MIDI (off by default)");
    seq_dest_add("mon", dest_mon, NULL, "echo notes to console");
    /* DIN/TRS MIDI. Registered always, so '>send' lists it and the owner can
     * see the option exists; it emits nothing until '>din <gpio>' says which
     * pin, because a pin is a fact about a physical object. */
    seq_dest_add("din", dinmidi_send, NULL, "DIN/TRS MIDI - set >din <gpio>");
    /* OSC is registered always and enabled by '>osc <ip> <port>'. It is the
     * visual half of the same lane grammar: one pattern, and whether it is a
     * drum or a frame trigger is the destination's business. */
    seq_dest_add("osc", net_osc_send, net_osc_flush, "OSC /deck/<lane>");
    /* THE PICTURE, to an HDMI node - docs/VIEW.md. A destination like the rest,
     * off until '>send view on'. */
    view_init();
    /* BLE MIDI is OFF by default. It is quantised to the connection interval
     * and shares one radio with the keyboard link, so typing contends with
     * the notes - which is exactly when the owner heard the timing go loose.
     * USB MIDI is the native path. This stays a feature; it is not the
     * default. */
    if (seq_init() != ESP_OK) {
        ESP_LOGE(TAG, "sequencer init failed");
    }
    cmd_init();
    if (doc_init() != ESP_OK) {
        ESP_LOGE(TAG, "docstore init FAILED");
    }
    /* THE JOURNAL SELF-TEST IS OFF ON A REAL DEVICE, and that is the default
     * rather than a safety margin.
     *
     * selftest_run() proves the journal survives a corrupted record and a wiped
     * partition, which it can only do by corrupting a record and wiping the
     * partition. It runs across three reboots and erases the journal on the way.
     * It armed itself on any device with an EMPTY journal - which is exactly
     * what a brand new deck is. So flashing a second board produced a deck that
     * refused input, ate its first document and printed FAIL, and every reason
     * the owner had to read that as a broken board was a good one. A first boot
     * has to be an instrument.
     *
     * Flip this to 1 to bench a board, reset it three times, read the log, and
     * flip it back. A compile-time switch rather than a command because
     * docs/MAP.md refuses verbs that delete nothing, and a destructive test
     * should not sit one keystroke from a performance. */
#define DECK_JOURNAL_SELFTEST 0
    if (DECK_JOURNAL_SELFTEST && selftest_run()) {
        ESP_LOGW(TAG, "a self-test stage ran; reset to advance it");
    }
    sdmirror_init();                 /* a missing card is not fatal */
    report_memory("after docstore");

    ensure_guide_buffer();

    kbd_set_ble_hooks(blemidi_register, blemidi_start);
    if (kbd_init() != ESP_OK) {
        ESP_LOGE(TAG, "BLE keyboard init FAILED");
    }
    /* Configure KEY BEFORE reading it. This was nineteen lines lower, after
     * the read below and after usbdev_boot() - so the escape hatch sampled an
     * unconfigured pin with no pull-up enabled, and the comment claiming
     * otherwise was simply wrong. The consequence was not subtle: the branch
     * only ever turns USB OFF, so a pin reading low would have cancelled
     * '>usb on' on every boot forever, and a floating pin would have done it
     * at random. Never flashed; caught by an audit of the unflashed code. */
    const gpio_config_t key = {
        .pin_bit_mask = 1ULL << PIN_KEY,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&key);

    /* HOLD KEY AT BOOT TO FORCE USB MIDI OFF.
     *
     * The last resort that needs no console, no keyboard and no host - and
     * that is the point. Every other way out of USB MIDI mode requires being
     * able to type, which requires the very interface that mode replaces. If
     * the composite ever comes up mute again, this is the way back, and it is
     * a button the owner already knows because it cycles the orientation.
     *
     * Read here, after the GPIO is configured and before usbdev_boot(), and
     * only ever used to turn something OFF - so a stuck button can cost the
     * owner a feature but can never cost them the deck. */
    /* Let the internal pull-up actually pull. It is ~45 kOhm against the pad
     * and trace capacitance, so sampling immediately after gpio_config can
     * read the pre-config level rather than the pulled one. */
    vTaskDelay(pdMS_TO_TICKS(2));
    ESP_LOGI(TAG, "KEY reads %d at boot", gpio_get_level(PIN_KEY));
    if (gpio_get_level(PIN_KEY) == 0) {
        if (usbdev_wanted()) {
            usbdev_want(false);
            ESP_LOGW(TAG, "KEY held at boot - USB MIDI forced off");
            editor_message("KEY held - USB MIDI off");
        }
    }

    /* A CRASH ALWAYS HANDS THE CONSOLE BACK.
     *
     * USB MIDI mode moves the console onto the composite device's CDC and the
     * intent is held in RTC memory, which survives a reboot on purpose - so
     * that a restart keeps the mode the owner asked for. The hole that leaves
     * is exactly the one that bit: the deck hung in USB MIDI mode, the task
     * watchdog panicked it, it rebooted, it came back in USB MIDI mode, and it
     * hung again. Unreachable, in a loop, with no way in from the cable and no
     * way out but pulling the battery.
     *
     * So a reboot caused by a panic or a watchdog clears the mode. The owner
     * loses USB MIDI and gets back a deck they can talk to and flash, which is
     * the right way round every time: the rule this violates otherwise -
     * docs/OS.md, the device must never become unreachable - outranks keeping
     * a transport across a crash it may itself have caused.
     *
     * An ordinary '>usb on' reboot is ESP_RST_SW and is untouched. Only a
     * crash spends the mode, and only once: the flag lives in RTC memory
     * beside the loop counter and a clean boot does not set it. */
    if (s_crashed_last_boot && usbdev_wanted()) {
        usbdev_want(false);
        ESP_LOGW(TAG, "crashed in USB MIDI mode - back to serial");
        editor_message("crashed - USB MIDI off");
    }

    /* USB MIDI, if it is wanted and has not just failed three times. When it
     * comes up it owns the USB peripheral, so the USB-Serial-JTAG keyboard
     * must NOT also be started - the console moves to the CDC interface and
     * reaches the editor through the same key mapper. */
    const bool usb_midi = usbdev_boot();
    vitals_started(usb_midi);
    if (usb_midi) {
        if (seq_dest_add("usb", dest_usb, usbdev_midi_flush,
                         "USB MIDI (native)") != ESP_OK) {
            /* The deck is in USB MIDI mode with a host attached and cannot
             * route to it. Silence here is the worst outcome: everything looks
             * connected. */
            ESP_LOGE(TAG, "FAULT: no room for the usb destination");
            editor_message("usb sink did not register");
        }
        seq_dest_enable("usb", true);
    } else {
        serialkbd_init();            /* the cable is a keyboard too */
    }
    report_memory("after BLE");


    splash_show();                       /* the boot screen, and a real test
                                          * of the whole draw path */

    /* Re-join the network the deck was last told about. It is remembered when
     * typed rather than when a join succeeds, so this works even if the first
     * attempt failed - and a deck that knows where it lives should not have to
     * be told again on every power-on. */
    if (net_rejoin() == ESP_OK) {
        ESP_LOGI(TAG, "rejoining the remembered network");
    }

    run_boot_document();                 /* settings, as a document */

    if (s_boot_loops > 0) {
        char m[40];
        snprintf(m, sizeof m, "crashed %ux - unplug to clear",
                 (unsigned)s_boot_loops);
        editor_message(m);
    }

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
    int     shown_pos = -1;   /* last playhead step drawn; -1 = stopped */
    bool    force_save = false;
    size_t  saved_len = doc_len();

    /* Arm the watchdog at the editor loop. It was configured and subscribed to
     * nothing, so a hang anywhere in the main pass was invisible - the same
     * failure shape as the BLE link that stayed "connected" while silent, and
     * the display path that rendered without ever pushing.
     *
     * Subscribing can fail if the timer was never initialised, and calling
     * reset from an unsubscribed task prints "task not found" on EVERY pass -
     * which floods the console and is itself a way to lose a real message. */
    bool wdt = esp_task_wdt_add(NULL) == ESP_OK;
    if (!wdt) {
        const esp_task_wdt_config_t wcfg = {
            .timeout_ms = 10000,
            .idle_core_mask = 0,
            .trigger_panic = true,
        };
        if (esp_task_wdt_init(&wcfg) == ESP_OK) {
            wdt = esp_task_wdt_add(NULL) == ESP_OK;
        }
    }
    ESP_LOGI(TAG, "task watchdog %s",
             wdt ? "armed on the editor loop"
                 : "UNAVAILABLE - hangs will be silent");

    /* WHERE THE LOOP'S TIME GOES, by the wall clock, reported and zeroed with
     * each heartbeat. docs/NEXT.md 9: no optimization without a number, and
     * profile first - the render time alone once said the drawing was the
     * budget, and was right, and after it was fixed nothing said what was. */
    int64_t prof_viz = 0, prof_view = 0, prof_draw = 0, prof_push = 0;
    uint32_t prof_loops = 0;

    while (1) {
        if (wdt) {
            esp_task_wdt_reset();
        }
        prof_loops++;

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

        /* Anything a USB timer callback decided, carried out here in task
         * context where blocking is allowed. See usbdev.h - this separation
         * is why the deck can no longer wedge itself into unreachability. */
        usbdev_poll();
        st7305_service();

        /* Keep the radio in step with the destination. '>send ble on' must
         * actually bring the peripheral up, and '>send ble off' must stop it
         * ADVERTISING - not merely drop notes - or the radio keeps contending
         * with the keyboard for nothing. */
        blemidi_set_enabled(seq_dest_is_on("ble"));

        /* The playhead moves on the sequencer's clock, so the document has to
         * be redrawn on it - need_draw is otherwise set only by keys, the
         * keyboard reset and the orientation button.
         *
         * No new task, no new timer, no poll loop. kbd_poll below already
         * blocks 5 ms on a queue, so this loop turns at roughly 200 Hz and the
         * worst-case lag is 5 ms against a 121 ms step at 124 bpm.
         *
         * pos becomes -1 on stop, which differs from whatever was last shown,
         * so the final redraw clears every mark. A stale inverted cell left
         * after a stop reads as "still running", which is a lie the screen
         * must not tell. */
        const int pos = seq_running() ? seq_position() : -1;
        if (pos != shown_pos) {
            shown_pos = pos;
            need_draw = true;
        }

        /* THE PICTURE IS COMPUTED HERE, NOT IN THE CLOCK CALLBACK.
         *
         * viz_tick only writes down which step happened; this is where the
         * frame is actually drawn. Generating one is a pass over the whole
         * picture for every visual lane that fired, and doing that inside an
         * esp_timer callback - which dispatches on a task shared with every
         * other timer - delayed the next tick. That is the coupling
         * docs/OS.md's two-core split exists to forbid, and this is the
         * fourth place in this firmware it has had to be undone.
         *
         * A frame that arrives while this loop is busy replaces the one
         * waiting rather than queueing behind it, so the picture can drop a
         * frame but can never lag the music. */
        /* One increment and one comparison on almost every pass; a flash write
         * once a minute and ONLY while USB MIDI is active, which is the only
         * place the hang has ever been seen. A deck in serial mode pays nothing
         * at all. */
        vitals_loop(usbdev_wanted(), seq_running(), seq_position());
        /* The ensemble broadcast, from the main loop and never the clock
         * callback: a radio send is exactly what docs/OS.md keeps out of there. */
        ensemble_service();
        /* The keyboard scan gives up most of the radio while Wi-Fi or ESP-NOW
         * needs it - see kbd_share_radio(). */
        kbd_share_radio(net_radio_on() || ensemble_role() != ENSEMBLE_OFF);

        int64_t prof_t = esp_timer_get_time();
        const bool frame = viz_service();
        prof_viz += esp_timer_get_time() - prof_t;
        if (frame) {
            need_draw = true;
            prof_t = esp_timer_get_time();
            view_frame();
            prof_view += esp_timer_get_time() - prof_t;
        }

        /* An ssh session runs in a task of its own (ssh.c); what it says is
         * moved into '+out' here, in the task that owns the documents. */
        {
            size_t from = 0;
            const int sv = ssh_service(&from);
            if (sv != SSH_QUIET) {
                need_draw = true;
            }
            if (sv == SSH_FINISHED) {
                char st[40];
                ssh_status(st, sizeof st);
                editor_show_output(from, st);
            }
        }

        if (need_draw) {
            prof_t = esp_timer_get_time();
            editor_draw();
            prof_draw += esp_timer_get_time() - prof_t;
            const uint32_t before = editor_cells_drawn();
            prof_t = esp_timer_get_time();
            editor_present(&bytes);
            prof_push += esp_timer_get_time() - prof_t;
            /* An exact invariant, not a threshold: if cells were rendered into
             * the framebuffer and nothing went out on the wire, the panel is
             * showing something other than the document. That is precisely
             * the bug that shipped for a day, and it has no false positive -
             * tg_put returns early when nothing changed, so a render count
             * above zero means the framebuffer really did change. */
            if (editor_cells_drawn() > before && bytes == 0) {
                ESP_LOGE(TAG, "FAULT: rendered %u cells, pushed 0 bytes",
                         (unsigned)(editor_cells_drawn() - before));
            }
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
            uint32_t pushes = 0, pbytes = 0, rus = 0, cells = 0;
            editor_vitals(&pushes, &pbytes, &rus, &cells);
            ESP_LOGI(TAG, "alive: doc %u%s, undo %d, kbd %s, %u push/%u B, "
                          "render %u us/%u cells, heap %u",
                     (unsigned)doc_len(), doc_dirty() ? "*" : "",
                     doc_undo_depth(),
                     kbd_connected() ? "up" : kbd_state_name(),
                     (unsigned)pushes, (unsigned)pbytes,
                     (unsigned)rus, (unsigned)cells,
                     (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
            char ust[48];
            usbdev_status(ust, sizeof ust);
            ESP_LOGI(TAG, "usb: %s", ust);
            /* The 5 ms kbd_poll wait is the loop's idle time, and is in none
             * of these. */
            ESP_LOGI(TAG, "loop: %u/s; us in 10 s: pictures %lld, view %lld "
                          "(%u sent, %u dropped), draw %lld, push %lld",
                     (unsigned)(prof_loops / 10), prof_viz, prof_view,
                     (unsigned)view_frames, (unsigned)view_dropped,
                     prof_draw, prof_push);
            prof_viz = prof_view = prof_draw = prof_push = 0;
            prof_loops = 0;
        }

        /* Autosave: on newline, or once typing has paused. Never per
         * keystroke - docs/HANDOFF.md trap 6. */
        /* A '+out' buffer is never written, so an autosave of one would log a
         * save that did not happen - and did, until a mirror of command
         * output showed up on the card. Clear the flag and say nothing. */
        /* The instant the transport stops, write. The autosave above is
         * suppressed while playing, so this is what bounds how long an edit
         * can live only in RAM: one keystroke of latency after '>stop'. */
        {
            static bool was_running;
            const bool now_running = seq_running();
            if (was_running && !now_running) {
                force_save = true;
            }
            was_running = now_running;
        }

        if (doc_dirty() && doc_current_is_transient()) {
            doc_save();               /* marks clean, writes nothing */
            saved_len = doc_len();
        } else if (doc_dirty()) {
            const int64_t idle = now_ms() - last_edit_ms;
            const size_t len = doc_len();
            const size_t delta = len > saved_len ? len - saved_len
                                                 : saved_len - len;
            /* NOT WHILE THE TRANSPORT IS RUNNING.
             *
             * A journal write was MEASURED at 13,000-18,600 us, and it
             * suspends both cores' schedulers with the instruction cache off,
             * so the sequencer's clock callback - which lives in flash -
             * cannot run. It is the only term this device produces that is
             * above the ~6 ms at which a listener hears a percussive onset as
             * displaced (Friberg & Sundberg 1995). Every other CPU-side
             * source measured here is individually inaudible.
             *
             * So the deck does not write to flash while it is playing. The
             * cost is bounded and stated: edits made during a performance are
             * held in RAM until the transport stops, and seq_stop() forces
             * the save. The document is still journalled on every other
             * path - naming, closing, '>save', '>flash'. */
            const bool worth_it = (force_save ||
                                   delta >= AUTOSAVE_MIN_CHARS ||
                                   idle >= AUTOSAVE_IDLE_MS) &&
                                  !seq_running();
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
                    /* A failed save must be visible on the panel, not only in
                     * a log nobody is reading. */
                    ESP_LOGE(TAG, "save failed: %s", esp_err_to_name(se));
                    editor_message(se == ESP_ERR_NO_MEM
                                   ? "JOURNAL FULL - free a document"
                                   : "SAVE FAILED");
                }
                force_save = false;
                last_edit_ms = now_ms();
                need_draw = true;
            }
        }
    }
}
