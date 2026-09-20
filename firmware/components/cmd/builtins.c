/*
 * The built-in commands.
 *
 * Every one of these is reachable by name, which is the discipline
 * docs/SUBSTRATE.md imposes and the reason the palette, the guide file and a
 * future agent all work without new plumbing.
 */
#include "cmd.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "docstore.h"
#include "textgrid.h"
#include "seq.h"
#include "blemidi.h"
#include "seq_pattern.h"

#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/rtc_cntl_reg.h"
#include "usbdev.h"
#include "usbmux.h"

static cmd_status_t c_help(cmd_ctx_t *ctx)
{
    int n = 0;
    const cmd_t *t = cmd_table(&n);
    for (int i = 0; i < n; i++) {
        cmd_out(ctx, "%-8s %s", t[i].name, t[i].help);
    }
    snprintf(ctx->msg, sizeof ctx->msg, "%d commands", n);
    return CMD_DONE;
}

static cmd_status_t c_list(cmd_ctx_t *ctx)
{
    int shown = 0;
    for (int i = 0; i < DOC_MAX_BUFFERS; i++) {
        const char *nm = doc_buf_name(i);
        if (nm[0] == '\0' && doc_buf_len(i) == 0 && i != doc_buf_current()) {
            continue;
        }
        cmd_out(ctx, "%c%d %-16s %5u%s",
                i == doc_buf_current() ? '*' : ' ', i,
                nm[0] ? nm : "(scratch)",
                (unsigned)doc_buf_len(i),
                doc_buf_is_dirty(i) ? " *" : "");
        shown++;
    }
    snprintf(ctx->msg, sizeof ctx->msg, "%d buffer%s", shown, shown == 1 ? "" : "s");
    return CMD_DONE;
}

static cmd_status_t c_new(cmd_ctx_t *ctx)
{
    if (doc_buf_new() != ESP_OK) {
        cmd_out(ctx, "no free buffer");
        return CMD_ERROR;
    }
    snprintf(ctx->msg, sizeof ctx->msg, "new scratch buffer %d", doc_buf_current());
    return CMD_DONE;
}

/* Promote the current scratch buffer into the archive. This is the whole of
 * "save": the text was already durable, naming it is what files it. */
static cmd_status_t c_name(cmd_ctx_t *ctx)
{
    if (ctx->arg[0] == '\0') {
        cmd_out(ctx, "name what? e.g. name lullaby");
        return CMD_ERROR;
    }
    if (doc_buf_rename(ctx->arg) != ESP_OK) {
        cmd_out(ctx, "cannot use that name");
        return CMD_ERROR;
    }
    doc_save();
    snprintf(ctx->msg, sizeof ctx->msg, "filed as %s", ctx->arg);
    return CMD_DONE;
}

static cmd_status_t c_open(cmd_ctx_t *ctx)
{
    /* >list already prints an index against every buffer, and an unnamed
     * scratch buffer has no name to match - so the index has to work, or the
     * listing is showing something that cannot be acted on. */
    if (ctx->arg[0] >= '0' && ctx->arg[0] <= '9') {
        const int n = ctx->arg[0] - '0';
        if (doc_buf_select(n) == ESP_OK) {
            const char *nm = doc_buf_name(n);
            snprintf(ctx->msg, sizeof ctx->msg, "%s", nm[0] ? nm : "scratch");
            return CMD_DONE;
        }
        cmd_out(ctx, "no buffer %d", n);
        return CMD_ERROR;
    }
    for (int i = 0; i < DOC_MAX_BUFFERS; i++) {
        if (strcmp(doc_buf_name(i), ctx->arg) == 0 && ctx->arg[0] != '\0') {
            if (doc_buf_select(i) != ESP_OK) {
                cmd_out(ctx, "cannot open %s", ctx->arg);
                return CMD_ERROR;
            }
            snprintf(ctx->msg, sizeof ctx->msg, "%s", ctx->arg);
            return CMD_DONE;
        }
    }
    cmd_out(ctx, "no document called '%s'", ctx->arg);
    return CMD_ERROR;
}

static cmd_status_t c_save(cmd_ctx_t *ctx)
{
    /* Say so rather than reporting a write that cannot happen. A '+' buffer
     * is machine-written: doc_save() marks it clean and returns ESP_OK, so
     * without this the status line read "saved 209 bytes" for a buffer that
     * was never written anywhere. */
    if (doc_current_is_transient()) {
        snprintf(ctx->msg, sizeof ctx->msg, "%s is output, not a document",
                 doc_buf_name(doc_buf_current()));
        return CMD_DONE;
    }
    const esp_err_t e = doc_save();
    if (e != ESP_OK) {
        cmd_out(ctx, "save failed");
        return CMD_ERROR;
    }
    doc_mirror_sd();
    snprintf(ctx->msg, sizeof ctx->msg, "saved %u bytes", (unsigned)doc_len());
    return CMD_DONE;
}

static cmd_status_t c_close(cmd_ctx_t *ctx)
{
    if (doc_buf_close(doc_buf_current()) != ESP_OK) {
        cmd_out(ctx, "cannot close the last buffer");
        return CMD_ERROR;
    }
    snprintf(ctx->msg, sizeof ctx->msg, "closed");
    return CMD_DONE;
}

/* The kind no longer decides whether Enter runs a line - the '>' sigil marks
 * a command and Ctrl+Enter runs it, in any buffer. The kind is kept because
 * it still has to decide prose-versus-grid reflow for Orca patches, which is
 * the distinction docs/SUBSTRATE.md actually cares about. */
static cmd_status_t c_guide(cmd_ctx_t *ctx)
{
    doc_buf_set_kind(DOC_KIND_GUIDE);
    snprintf(ctx->msg, sizeof ctx->msg, "kind: guide");
    return CMD_DONE;
}

static cmd_status_t c_prose(cmd_ctx_t *ctx)
{
    doc_buf_set_kind(DOC_KIND_PROSE);
    snprintf(ctx->msg, sizeof ctx->msg, "kind: prose");
    return CMD_DONE;
}

static cmd_status_t c_out(cmd_ctx_t *ctx)
{
    const int i = doc_buf_ensure("+out");
    if (i < 0 || doc_buf_select(i) != ESP_OK) {
        cmd_out(ctx, "no output buffer");
        return CMD_ERROR;
    }
    snprintf(ctx->msg, sizeof ctx->msg, "output");
    return CMD_DONE;
}

esp_err_t editor_set_density(int dense);

/* Both faces are compiled in, so this costs no flash and no assets. Dense is
 * also CHEAPER to draw - a 6x12 cell is 9 bytes on the wire against 36 for a
 * 12x24 one - so the readable default is the expensive one, which is the
 * right way round. */
static cmd_status_t c_density(cmd_ctx_t *ctx)
{
    /* Strict, not defaulting. A command that silently does the opposite of
     * what a typo asked for is worse than one that refuses. */
    int dense;
    if (ctx->arg[0] == 'd' || ctx->arg[0] == '6') {
        dense = 1;
    } else if (ctx->arg[0] == 'c' || ctx->arg[0] == '1') {
        dense = 0;
    } else {
        cmd_out(ctx, "density chunky | density dense");
        return CMD_ERROR;
    }
    if (editor_set_density(dense) != ESP_OK) {
        cmd_out(ctx, "density: layout refused");
        return CMD_ERROR;
    }
    /* Derived, not hardcoded: the string said 60x20 while the layout computed
     * 60x24. A status message that disagrees with the machine is a small lie
     * that costs someone an afternoon later. */
    snprintf(ctx->msg, sizeof ctx->msg, "%s %dx%d", dense ? "dense" : "chunky",
             tg_cols(), tg_rows());
    return CMD_DONE;
}

/* ------------------------------------------------------------------ music
 *
 * A drum IS a command. "kick x...x...x...x..." is twenty-two keystrokes for a
 * four-on-the-floor, and nothing has to be declared, named or wired up first.
 * That is the whole design brief for a thumb keyboard: the shortest path from
 * a musical idea to a sound, with no ceremony in between.
 */
static const struct { const char *name; uint8_t note; } s_drums[] = {
    { "kick",  36 }, { "snare", 38 }, { "hat",   42 }, { "ohat",  46 },
    { "clap",  39 }, { "tom",   45 }, { "rim",   37 }, { "crash", 49 },
};

/* RE-RUNNING A LINE YOU HAVE NOT TOUCHED SILENCES THE LANE.
 *
 * The rule in one sentence: Ctrl+Enter always compiles the line and starts
 * the lane, EXCEPT when the transport is running and the line is byte-for-byte
 * what you last ran for that lane, in which case it silences it instead.
 *
 * Why that does not surprise anyone on stage:
 *
 *  - The only way in is pressing Run on a line you did not edit. The obvious
 *    objection to a toggle - "I tweaked it and re-ran it and it went silent" -
 *    cannot happen, because an edited line is excluded by definition.
 *  - It is self-inverse. The lane is now muted, so the guard fails on the next
 *    press and the same key brings it back. A fumbled double-press in a loud
 *    room is a no-op, not a coin flip.
 *  - It is unreachable while stopped, so it can never leave the deck in a
 *    state the screen is not showing.
 *  - The result is visible immediately: the playhead stops sweeping that line.
 *    That is why this shipped WITH the playhead and not before it - an
 *    invisible toggle is the one version of this that is genuinely bad.
 *
 * "Changed" means the CHARACTERS changed, not the compiled bitmask. Comparing
 * compiled form would treat "x...x...x...x..." and "x... x... x... x..." as
 * the same line and silence the lane the moment the player re-spaced it for
 * readability. The rule fails toward compiling, never toward silence.
 *
 * BY_HANDS only. A guide replayed by an agent must not silence every lane on
 * its second pass. */
static bool rerun_silences(const cmd_ctx_t *ctx)
{
    if (ctx->caller != CMD_BY_HANDS || !seq_running()) {
        return false;
    }
    const seq_lane_t *l = seq_lane_find(ctx->name, -1);
    return l != NULL && !l->muted && l->src == seq_pattern_hash(ctx->arg);
}

static cmd_status_t c_drum(cmd_ctx_t *ctx)
{
    uint8_t note = 36;
    for (size_t i = 0; i < sizeof s_drums / sizeof s_drums[0]; i++) {
        if (strcmp(s_drums[i].name, ctx->name) == 0) {
            note = s_drums[i].note;
            break;
        }
    }
    if (ctx->arg[0] == '\0' || rerun_silences(ctx)) {
        seq_mute(ctx->name, true);
        snprintf(ctx->msg, sizeof ctx->msg, "%s silent", ctx->name);
        return CMD_DONE;
    }
    seq_lane_note(ctx->name, note, 9);
    if (seq_lane(ctx->name, ctx->arg) != ESP_OK) {
        cmd_out(ctx, "no room for another lane");
        return CMD_ERROR;
    }
    seq_mute(ctx->name, false);
    snprintf(ctx->msg, sizeof ctx->msg, "%s %s", ctx->name, ctx->arg);
    return CMD_DONE;
}

/* The melodic lanes. Four names, because four is how many parts a person can
 * actually hold in their head while performing, and because each one carries
 * a default octave and gate that make it sound like what it is called before
 * anything is configured. A bass that arrives an octave too high is a bass
 * nobody uses. */
static const struct {
    const char *name; int8_t oct; uint8_t chan; uint16_t gate;
} s_voices[] = {
    { "bass", 2, 0, 180 },
    { "lead", 4, 1, 120 },
    { "pad",  3, 2, 420 },
    { "arp",  5, 3,  90 },
};

static cmd_status_t c_voice(cmd_ctx_t *ctx)
{
    const int8_t *oct = NULL;
    size_t v = 0;
    for (; v < sizeof s_voices / sizeof s_voices[0]; v++) {
        if (strcmp(s_voices[v].name, ctx->name) == 0) {
            oct = &s_voices[v].oct;
            break;
        }
    }
    if (oct == NULL) {
        return CMD_ERROR;
    }
    if (ctx->arg[0] == '\0' || rerun_silences(ctx)) {
        seq_mute(ctx->name, true);
        snprintf(ctx->msg, sizeof ctx->msg, "%s silent", ctx->name);
        return CMD_DONE;
    }
    seq_lane_melodic(ctx->name, s_voices[v].oct, s_voices[v].chan,
                     s_voices[v].gate);
    if (seq_lane(ctx->name, ctx->arg) != ESP_OK) {
        cmd_out(ctx, "no room for another lane");
        return CMD_ERROR;
    }
    seq_mute(ctx->name, false);
    snprintf(ctx->msg, sizeof ctx->msg, "%s %s in %s", ctx->name, ctx->arg,
             seq_scale_name());
    return CMD_DONE;
}

static cmd_status_t c_scale(cmd_ctx_t *ctx)
{
    if (ctx->arg[0] != '\0' && seq_scale(ctx->arg) != ESP_OK) {
        cmd_out(ctx, "a root a-g, then # or b, then one of:");
        cmd_out(ctx, "  maj min dor phr lyd mix loc");
        cmd_out(ctx, "  pent maj5 blues chrom");
        cmd_out(ctx, "e.g. dmin  c  f#mix  apent  ebblues");
        return CMD_ERROR;
    }
    snprintf(ctx->msg, sizeof ctx->msg, "key of %s", seq_scale_name());
    return CMD_DONE;
}

static cmd_status_t c_swing(cmd_ctx_t *ctx)
{
    if (ctx->arg[0] != '\0') {
        seq_swing(atoi(ctx->arg));
    }
    const int s = seq_get_swing();
    snprintf(ctx->msg, sizeof ctx->msg, "swing %d%s", s,
             s == 50 ? " (straight)" : s >= 66 && s <= 68 ? " (triplet)" : "");
    return CMD_DONE;
}

static cmd_status_t c_sync(cmd_ctx_t *ctx)
{
    if (strcmp(ctx->arg, "on") == 0)       { seq_sync(true); }
    else if (strcmp(ctx->arg, "off") == 0) { seq_sync(false); }
    else if (ctx->arg[0] != '\0') {
        cmd_out(ctx, "sync on | sync off");
        return CMD_ERROR;
    }
    snprintf(ctx->msg, sizeof ctx->msg, "midi clock %s",
             seq_get_sync() ? "out" : "off");
    return CMD_DONE;
}

/* '>send' with no argument lists the destinations and their state; with a
 * name and on/off it switches one. One command, because "what are my outputs"
 * and "turn that output off" are the same question asked twice. */
/* '>flash' - hand the chip to the ROM loader without touching a button.
 *
 * THIS COMMAND IS A PRECONDITION FOR EVERYTHING ELSE THIS DEVICE MIGHT DO
 * WITH ITS USB PORT, and it is written before any of it.
 *
 * The one hard rule on this project is that the owner is never asked to hold
 * BOOT. Today that holds because the ESP32-S3's USB-Serial-JTAG has reset
 * logic in hardware and esptool drives it over DTR/RTS. Any firmware that
 * reconfigures the USB peripheral - a USB MIDI device, say - takes that
 * hardware away, and the first bad flash after that point bricks the deck
 * into needing a paperclip.
 *
 * Setting RTC_CNTL_FORCE_DOWNLOAD_BOOT and restarting reaches the ROM loader
 * from software, through no peripheral at all. It is the same register
 * ESP-IDF's own USB console uses for its reboot-to-bootloader command, so it
 * is not a trick - it is the supported route.
 *
 * The buffer is written first. A command that reboots the machine and loses
 * the document is not a convenience.
 *
 * ONE PROPERTY THE OWNER HAS TO KNOW, found by testing it rather than by
 * reading about it. RTC_CNTL_OPTION1_REG is in the RTC power domain, and
 * esp_restart() is a CPU reset - rst:0xc, RTC_SW_CPU_RST - which does not
 * touch that domain. So the bit SURVIVES, and the deck re-enters download
 * mode on every subsequent reset until something clears it. Nothing in
 * ESP-IDF clears it; the only writers in the whole tree are IDF's own USB
 * console and this file.
 *
 * What clears it is a full system reset, which is what esptool's
 * '--before default_reset' performs and therefore what a plain
 * 'idf.py flash' does. That was verified both ways on hardware: flashing
 * with '--before no_reset' left the deck silent in download mode, and a
 * plain 'idf.py flash' brought it straight back up.
 *
 * So the deck is never stuck - but it does WAIT, and a deck waiting silently
 * in download mode looks exactly like a dead one. The panel says so before
 * it goes. */
static cmd_status_t flash_now(cmd_ctx_t *ctx);

static cmd_status_t c_flash(cmd_ctx_t *ctx)
{
    /* CONFIRMATION, because the cost of a mistake here is the whole session.
     *
     * '>help' prints a row reading "flash   reboot into the ROM loader", and
     * running that row is one typed '>' away. With the deck live on stage
     * over BLE MIDI, an accidental '>flash' ends the performance and needs a
     * power cycle to come back. Every other command on this device is
     * recoverable; this one is not, so it is the one command that asks. */
    if (strcmp(ctx->arg, "now") != 0) {
        cmd_out(ctx, "this reboots the deck into the");
        cmd_out(ctx, "ROM loader and ends the session.");
        cmd_out(ctx, "type:  >flash now");
        snprintf(ctx->msg, sizeof ctx->msg, "flash needs: >flash now");
        return CMD_ERROR;
    }

    return flash_now(ctx);
}

/* The reboot-into-the-ROM-loader body, shared so that '>usbtest flash' takes
 * exactly the same route rather than a lookalike. A test of a lookalike is
 * a test of the lookalike. */
static cmd_status_t flash_now(cmd_ctx_t *ctx)
{
    /* Save EVERY dirty document, not just the current one - see
     * doc_save_all_dirty(). This used to save only the current buffer and
     * skip even that when it was transient, so running '>flash' from '+out'
     * rebooted having written nothing while claiming otherwise. */
    doc_save_all_dirty();
    seq_stop();
    cmd_out(ctx, "download mode. flash now:");
    cmd_out(ctx, "  idf.py -p PORT flash");
    cmd_out(ctx, "no button, no paperclip. the deck STAYS in");
    cmd_out(ctx, "download mode until it is flashed or unplugged:");
    cmd_out(ctx, "RTC_CNTL_FORCE_DOWNLOAD_BOOT survives a CPU reset.");
    cmd_announce("DOWNLOAD MODE - flash now");
    /* Long enough for the panel to show it and the log to drain. */
    vTaskDelay(pdMS_TO_TICKS(600));
    /* Hand the PHY back before rebooting, or the ROM loader comes up on a PHY
     * that USB-Serial-JTAG does not own and the familiar port never appears.
     * The ROM does not do this for us - see usbmux.h. Without this line the
     * escape hatch stops being one the moment USB MIDI exists. */
    usbmux_release_to_usj();
    REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
    esp_restart();
    return CMD_DONE;                 /* not reached */
}

/* Put a document on the console. The deck has a 30-column window onto a
 * buffer and no other way to see the whole of one; when something is wrong
 * with a document - and something was, because testing over the cable types
 * into whatever is open - there was no way to look at it. */
static cmd_status_t c_dump(cmd_ctx_t *ctx)
{
    const int want = (ctx->arg[0] != '\0') ? doc_buf_find(ctx->arg) : doc_buf_current();
    if (want < 0) {
        cmd_out(ctx, "no document called '%s'", ctx->arg);
        return CMD_ERROR;
    }
    /* '>dump' WRITES INTO +out THROUGH cmd_out, so dumping +out feeds its own
     * input: the loop appends a line, the buffer grows, and the bound - read
     * afresh every pass - grows with it. Replayed on the host, the old loop
     * ran 131,073 iterations and emitted 3,311 log lines before the 128 KB
     * buffer capped it.
     *
     * That is not merely slow. Every one of those cmd_out calls is a
     * synchronous ESP_LOGI on the main task, and for that entire stretch the
     * loop never reaches esp_task_wdt_reset() - which is armed at 10 s with
     * trigger_panic = true. So the deck freezes and then very plausibly
     * panic-reboots, losing whatever was not saved.
     *
     * Two guards. The name test is the courtesy: it says what went wrong. The
     * length test is the INVARIANT - it catches any writer that grows the
     * document mid-walk, whatever route it arrived by, including ones that do
     * not exist yet. */
    const int out = doc_buf_find("+out");
    if (out >= 0 && want == out) {
        cmd_out(ctx, "dump writes into %s", doc_buf_name(out));
        return CMD_ERROR;
    }
    const int was = doc_buf_current();
    if (want != was && doc_buf_select(want) != ESP_OK) {
        return CMD_ERROR;
    }
    char line[96];
    size_t k = 0;
    int    ln = 1;
    const size_t n = doc_len();          /* ONCE, before the walk */
    for (size_t i = 0; i <= n; i++) {
        if (doc_len() != n) {
            cmd_out(ctx, "document grew under dump");
            break;
        }
        const char ch = (i < n) ? doc_at(i) : '\n';
        if (ch == '\n' || k == sizeof line - 1) {
            line[k] = '\0';
            if (i < n || k > 0) {
                cmd_out(ctx, "%3d|%s", ln++, line);
            }
            k = 0;
            continue;
        }
        line[k++] = ch;
    }
    const size_t len = doc_len();
    if (want != was) {
        doc_buf_select(was);
    }
    snprintf(ctx->msg, sizeof ctx->msg, "%u bytes", (unsigned)len);
    return CMD_DONE;
}

/* '>usbtest' - prove the PHY restore BEFORE anything depends on it.
 *
 * This is the gate on the whole USB MIDI migration, and it is deliberately
 * the first thing built. It puts the PHY mux into exactly the state TinyUSB
 * will leave it in - without TinyUSB - and then puts it back.
 *
 * ON THE FIRMWARE THIS REPLACES, THE TEST CANNOT PASS. There was no
 * usbmux_release_to_usj(); the console would go and only a power cycle would
 * bring it back. That is the check being "proven to fail on the old state".
 *
 *   >usbtest now    take the PHY, wait, give it back. The console dies for
 *                   two seconds and returns. If it does not return, the
 *                   restore does not work and no USB work should proceed.
 *   >usbtest flash  take the PHY, then run the '>flash' path with the mux
 *                   dirty. The deck must land in download mode on the SAME
 *                   port it uses today. If it does not, STOP - the escape
 *                   hatch does not survive USB MIDI and nothing else on the
 *                   plan should be built.
 *
 * Worst case either way is a power cycle: the mux is in the RTC domain and a
 * cold boot restores the hardware default. */
static void usbtest_cb(void *arg)
{
    (void)arg;
    usbmux_release_to_usj();
}

static cmd_status_t c_usbtest(cmd_ctx_t *ctx)
{
    const bool to_flash = (strcmp(ctx->arg, "flash") == 0);
    if (!to_flash && strcmp(ctx->arg, "now") != 0) {
        cmd_out(ctx, "usbtest now   - drop the console 2s");
        cmd_out(ctx, "usbtest flash - reboot to ROM loader");
        cmd_out(ctx, "both recover without a button.");
        snprintf(ctx->msg, sizeof ctx->msg, "usbtest now | usbtest flash");
        return CMD_ERROR;
    }

    cmd_announce(to_flash ? "USB TEST - flashing route"
                          : "USB TEST - console back in 2s");
    vTaskDelay(pdMS_TO_TICKS(400));

    if (to_flash) {
        usbmux_take_for_otg();
        return flash_now(ctx);
    }

    /* A one-shot timer, not a delay in this task: if the restore is broken we
     * want it attempted from a context that is still running even if this
     * command's task were somehow stuck. */
    const esp_timer_create_args_t args = {
        .callback = usbtest_cb, .name = "usbtest",
        .dispatch_method = ESP_TIMER_TASK,
    };
    esp_timer_handle_t t = NULL;
    if (esp_timer_create(&args, &t) != ESP_OK) {
        return CMD_ERROR;
    }
    usbmux_take_for_otg();
    esp_timer_start_once(t, 2000000);
    snprintf(ctx->msg, sizeof ctx->msg, "PHY taken - back in 2s");
    return CMD_DONE;
}

/* '>jitter' - what the clock is actually doing, in microseconds.
 *
 * The owner reports perceptible timing jitter. Before optimising anything,
 * measure it: the sequencer knows when each tick should have fired and when
 * it did. CLOCK is dispatch deviation from the ideal grid; XPORT is how long
 * a note waited between the clock queueing it and the transport being handed
 * it. Different causes, different fixes, so they are never added together. */
static void jitter_line(cmd_ctx_t *ctx, const char *what, const seq_stat_t *s)
{
    if (s->n == 0) {
        cmd_out(ctx, "%-5s no samples yet", what);
        return;
    }
    const double n    = (double)s->n;
    const double mean = (double)s->sum / n;
    double var = (double)s->sumsq / n - mean * mean;
    if (var < 0) { var = 0; }
    /* Everything relative to the tightest sample seen. That removes the
     * constant phase offset and leaves only the spread, which is the part a
     * listener can actually hear. */
    cmd_out(ctx, "%-5s n%-6u sd%4d late%u", what, (unsigned)s->n,
            (int)(var > 0 ? sqrt(var) : 0), (unsigned)s->late);
    cmd_out(ctx, "      spread %d us  worst @%us",
            (int)(s->max - s->min), (unsigned)(s->worst_ms / 1000));
    /* The shape, not just the extremes. A single bad tick in two thousand is
     * a different instrument from fifty a second, and min/max cannot tell
     * them apart. */
    static const char *lbl[SEQ_NBUCKETS] = {
        "<.1", "<.25", "<.5", "<1", "<2", "<5", ">5"
    };
    char bar[64] = {0};
    for (int i = 0; i < SEQ_NBUCKETS; i++) {
        char one[16];
        snprintf(one, sizeof one, "%s:%u ", lbl[i], (unsigned)s->bucket[i]);
        strncat(bar, one, sizeof bar - strlen(bar) - 1);
    }
    cmd_out(ctx, "      %s", bar);
}

static cmd_status_t c_jitter(cmd_ctx_t *ctx)
{
    if (strcmp(ctx->arg, "reset") == 0) {
        seq_stats_reset();
        snprintf(ctx->msg, sizeof ctx->msg, "jitter counters cleared");
        return CMD_DONE;
    }
    seq_stat_t clk, xp;
    seq_stats(&clk, &xp);
    jitter_line(ctx, "clock", &clk);
    jitter_line(ctx, "xport", &xp);
    /* The packing ratio. A step with three lanes should cost ONE BLE
     * notification, not three - and a notification is quantised to the
     * connection interval, so this ratio is a jitter number wearing a
     * different hat. */
    uint32_t msgs = 0, pkts = 0;
    blemidi_packing(&msgs, &pkts);
    if (pkts > 0) {
        cmd_out(ctx, "ble   %u msgs in %u packets (%u.%02ux)",
                (unsigned)msgs, (unsigned)pkts,
                (unsigned)(msgs / pkts), (unsigned)((msgs * 100 / pkts) % 100));
    }
    cmd_out(ctx, "clock = tick vs the ideal grid");
    cmd_out(ctx, "xport = queue wait before sending");
    const uint32_t lost = seq_dropped();
    if (lost > 0) {
        cmd_out(ctx, "%u events dropped", (unsigned)lost);
    }
    snprintf(ctx->msg, sizeof ctx->msg, "clock sd %d us over %u ticks",
             (int)(clk.n ? sqrt((double)clk.sumsq / clk.n -
                   ((double)clk.sum / clk.n) * ((double)clk.sum / clk.n)) : 0),
             (unsigned)clk.n);
    return CMD_DONE;
}

/* '>usb' - become a USB MIDI device, or stop being one.
 *
 * Both directions reboot, and neither can lose a document: every dirty buffer
 * is written first. Turning it ON is the risky direction and is ARMED rather
 * than set - if no host mounts the deck within eight seconds it reverts by
 * itself, and three failed boots give up permanently. The owner is never
 * asked to confirm anything, because the confirmation that matters is a host
 * actually attaching, and the deck can see that for itself. */
static cmd_status_t c_usb(cmd_ctx_t *ctx)
{
    const bool on  = (strcmp(ctx->arg, "on") == 0);
    const bool off = (strcmp(ctx->arg, "off") == 0);

    if (!on && !off) {
        cmd_out(ctx, "usb is %s%s", usbdev_wanted() ? "on" : "off",
                usbdev_mounted() ? ", host attached" : "");
        cmd_out(ctx, "usb on   one cable: MIDI + console");
        cmd_out(ctx, "usb off  back to serial only");
        if (usbdev_tries() > 0) {
            cmd_out(ctx, "%u failed attempt(s) this power cycle",
                    usbdev_tries());
        }
        snprintf(ctx->msg, sizeof ctx->msg, "usb %s",
                 usbdev_wanted() ? "on" : "off");
        return CMD_DONE;
    }

    if (on && usbdev_want(true) != ESP_OK) {
        cmd_out(ctx, "USB failed 3x this power cycle.");
        cmd_out(ctx, "unplug the deck and try again.");
        snprintf(ctx->msg, sizeof ctx->msg, "usb: too many failures");
        return CMD_ERROR;
    }
    if (off) {
        usbdev_want(false);
    }

    doc_save_all_dirty();
    seq_stop();
    cmd_announce(on ? "USB MIDI - back in a moment"
                    : "serial console - back in a moment");
    vTaskDelay(pdMS_TO_TICKS(600));
    esp_restart();
    return CMD_DONE;                 /* not reached */
}

static cmd_status_t c_send(cmd_ctx_t *ctx)
{
    if (ctx->arg[0] == '\0') {
        for (int i = 0; i < seq_dest_count(); i++) {
            cmd_out(ctx, "%-5s %-3s %s", seq_dest_name(i),
                    seq_dest_on(i) ? "on" : "off", seq_dest_help(i));
        }
        snprintf(ctx->msg, sizeof ctx->msg, "%d destination%s",
                 seq_dest_count(), seq_dest_count() == 1 ? "" : "s");
        return CMD_DONE;
    }
    char name[16] = {0};
    char state[8] = {0};
    if (sscanf(ctx->arg, "%15s %7s", name, state) < 1) {
        return CMD_ERROR;
    }
    if (state[0] == '\0') {
        snprintf(ctx->msg, sizeof ctx->msg, "%s is %s", name,
                 seq_dest_is_on(name) ? "on" : "off");
        return CMD_DONE;
    }
    const bool on = strcmp(state, "on") == 0;
    if (!on && strcmp(state, "off") != 0) {
        cmd_out(ctx, "send <name> on | off");
        return CMD_ERROR;
    }
    if (seq_dest_enable(name, on) != ESP_OK) {
        cmd_out(ctx, "no destination called '%s'. try just: send", name);
        return CMD_ERROR;
    }
    snprintf(ctx->msg, sizeof ctx->msg, "%s %s", name, on ? "on" : "off");
    return CMD_DONE;
}

static cmd_status_t c_bpm(cmd_ctx_t *ctx)
{
    if (ctx->arg[0] != '\0') {
        seq_bpm(atoi(ctx->arg));
    }
    snprintf(ctx->msg, sizeof ctx->msg, "%d bpm", seq_get_bpm());
    return CMD_DONE;
}

static cmd_status_t c_play(cmd_ctx_t *ctx)
{
    seq_play();
    snprintf(ctx->msg, sizeof ctx->msg, "playing at %d", seq_get_bpm());
    return CMD_DONE;
}

static cmd_status_t c_stop(cmd_ctx_t *ctx)
{
    seq_stop();
    snprintf(ctx->msg, sizeof ctx->msg, "stopped");
    return CMD_DONE;
}

static cmd_status_t c_lanes(cmd_ctx_t *ctx)
{
    int n = 0;
    const seq_lane_t *l = seq_lanes(&n);
    for (int i = 0; i < SEQ_MAX_LANES; i++) {
        if (!l[i].used) {
            continue;
        }
        /* Print what was typed, not a normalised version of it. A listing
         * that silently rewrites 'X' as 'x' teaches the player that accents
         * did not register. */
        char bar[SEQ_MAX_STEPS + 1];
        int k = 0;
        for (; k < l[i].steps && k < SEQ_MAX_STEPS; k++) {
            const uint32_t b = 1u << k;
            if (!(l[i].mask & b))            { bar[k] = '.'; }
            else if (l[i].accent & b)        { bar[k] = 'X'; }
            else if (l[i].ghost & b)         { bar[k] = ','; }
            else if (l[i].melodic && l[i].deg[k] != 0xFF) {
                bar[k] = (char)('0' + l[i].deg[k]);
            } else                           { bar[k] = 'x'; }
        }
        bar[k] = '\0';
        cmd_out(ctx, "%c%-5s %s", l[i].muted ? '-' : ' ', l[i].name, bar);
    }
    cmd_out(ctx, "%d bpm  swing %d  key %s  clock %s", seq_get_bpm(),
            seq_get_swing(), seq_scale_name(), seq_get_sync() ? "out" : "off");
    char dests[64] = {0};
    for (int i = 0; i < seq_dest_count(); i++) {
        if (seq_dest_on(i)) {
            strncat(dests, seq_dest_name(i), sizeof dests - strlen(dests) - 2);
            strncat(dests, " ", sizeof dests - strlen(dests) - 1);
        }
    }
    cmd_out(ctx, "to: %s", dests[0] ? dests : "nowhere - try: send ble on");
    const uint32_t lost = seq_dropped();
    if (lost > 0) {
        cmd_out(ctx, "%u events dropped - the transport is behind",
                (unsigned)lost);
    }
    snprintf(ctx->msg, sizeof ctx->msg, "%d lane%s %s", n, n == 1 ? "" : "s",
             seq_running() ? "playing" : "stopped");
    return CMD_DONE;
}

static cmd_status_t c_panic(cmd_ctx_t *ctx)
{
    seq_stop();
    seq_all_notes_off();
    snprintf(ctx->msg, sizeof ctx->msg, "all notes off");
    return CMD_DONE;
}

static const cmd_t s_builtins[] = {
    { "bpm",   c_bpm,   CMD_CAP_EDIT,  "tempo" },
    { "scale", c_scale, CMD_CAP_EDIT,  "dmin | c | f#mix | apent" },
    { "swing", c_swing, CMD_CAP_EDIT,  "50 straight, 67 triplet" },
    { "sync",  c_sync,  CMD_CAP_EDIT,  "midi clock out on | off" },
    { "send",  c_send,  CMD_CAP_SYSTEM,"where events go; send mon on" },
    { "usb",   c_usb,   CMD_CAP_SYSTEM,"usb on | off - MIDI over the cable" },
    { "flash", c_flash, CMD_CAP_SYSTEM,"flash now - reboot to ROM loader" },
    { "usbtest", c_usbtest, CMD_CAP_SYSTEM, "prove the USB PHY restore" },
    { "dump",  c_dump,  CMD_CAP_READ,  "print a document to the console" },
    { "play",  c_play,  CMD_CAP_EDIT,  "start the clock" },
    { "stop",  c_stop,  CMD_CAP_EDIT,  "stop the clock" },
    { "lanes", c_lanes, CMD_CAP_READ,  "what is playing" },
    { "jitter",c_jitter,CMD_CAP_READ,  "timing, measured in us" },
    { "panic", c_panic, CMD_CAP_EDIT,  "silence everything" },
    { "kick",  c_drum,  CMD_CAP_EDIT,  "x...x...x...x..." },
    { "snare", c_drum,  CMD_CAP_EDIT,  "....x.......x..." },
    { "hat",   c_drum,  CMD_CAP_EDIT,  "x.x.x.x.x.x.x.x." },
    { "ohat",  c_drum,  CMD_CAP_EDIT,  "open hat" },
    { "clap",  c_drum,  CMD_CAP_EDIT,  "clap" },
    { "tom",   c_drum,  CMD_CAP_EDIT,  "tom" },
    { "rim",   c_drum,  CMD_CAP_EDIT,  "rim" },
    { "crash", c_drum,  CMD_CAP_EDIT,  "crash" },
    { "bass",  c_voice, CMD_CAP_EDIT,  "0..0..3..0..5..." },
    { "lead",  c_voice, CMD_CAP_EDIT,  "degrees 0-9, 0 is the root" },
    { "pad",   c_voice, CMD_CAP_EDIT,  "long notes" },
    { "arp",   c_voice, CMD_CAP_EDIT,  "short notes, high" },
    { "help",  c_help,  CMD_CAP_READ,                   "list the commands" },
    { "list",  c_list,  CMD_CAP_READ,                   "list open buffers" },
    { "new",   c_new,   CMD_CAP_EDIT,                   "a fresh scratch buffer" },
    { "name",  c_name,  CMD_CAP_EDIT | CMD_CAP_STORE,   "file this buffer under a name" },
    { "open",  c_open,  CMD_CAP_READ,                   "switch to a named document" },
    { "save",  c_save,  CMD_CAP_STORE,                  "write this buffer now" },
    { "close", c_close, CMD_CAP_EDIT,                   "forget this buffer" },
    { "guide", c_guide, CMD_CAP_EDIT,                   "mark as a guide" },
    { "prose", c_prose, CMD_CAP_EDIT,                   "mark as prose" },
    { "out",   c_out,   CMD_CAP_READ,                   "read command output" },
    { "density", c_density, CMD_CAP_SYSTEM,             "chunky | dense" },
};

void cmd_register(const cmd_t *table, int count);

void cmd_init(void)
{
    cmd_register(s_builtins, (int)(sizeof s_builtins / sizeof s_builtins[0]));
}
