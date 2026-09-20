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
#include <string.h>

#include "docstore.h"
#include "textgrid.h"
#include "seq.h"

#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/rtc_cntl_reg.h"

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

static cmd_status_t c_drum(cmd_ctx_t *ctx)
{
    uint8_t note = 36;
    for (size_t i = 0; i < sizeof s_drums / sizeof s_drums[0]; i++) {
        if (strcmp(s_drums[i].name, ctx->name) == 0) {
            note = s_drums[i].note;
            break;
        }
    }
    if (ctx->arg[0] == '\0') {
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
    if (ctx->arg[0] == '\0') {
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
static cmd_status_t c_flash(cmd_ctx_t *ctx)
{
    if (!doc_current_is_transient()) {
        doc_save();
        doc_mirror_sd();
    }
    seq_stop();
    cmd_out(ctx, "download mode. flash now:");
    cmd_out(ctx, "  idf.py -p PORT flash");
    cmd_out(ctx, "no button, no paperclip. the deck STAYS in");
    cmd_out(ctx, "download mode until it is flashed or unplugged:");
    cmd_out(ctx, "RTC_CNTL_FORCE_DOWNLOAD_BOOT survives a CPU reset.");
    cmd_announce("DOWNLOAD MODE - flash now");
    /* Long enough for the panel to show it and the log to drain. */
    vTaskDelay(pdMS_TO_TICKS(600));
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
    const int was = doc_buf_current();
    if (want != was && doc_buf_select(want) != ESP_OK) {
        return CMD_ERROR;
    }
    char line[96];
    size_t k = 0;
    int    ln = 1;
    for (size_t i = 0; i <= doc_len(); i++) {
        const char ch = (i < doc_len()) ? doc_at(i) : '\n';
        if (ch == '\n' || k == sizeof line - 1) {
            line[k] = '\0';
            if (i < doc_len() || k > 0) {
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
    { "flash", c_flash, CMD_CAP_SYSTEM,"reboot into the ROM loader" },
    { "dump",  c_dump,  CMD_CAP_READ,  "print a document to the console" },
    { "play",  c_play,  CMD_CAP_EDIT,  "start the clock" },
    { "stop",  c_stop,  CMD_CAP_EDIT,  "stop the clock" },
    { "lanes", c_lanes, CMD_CAP_READ,  "what is playing" },
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
