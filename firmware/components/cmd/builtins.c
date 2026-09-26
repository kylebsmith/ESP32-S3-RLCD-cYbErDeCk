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
#include "lane_name.h"
#include "secret_line.h"

#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/rtc_cntl_reg.h"
#include "battery.h"
#include "net.h"
#include "ssh.h"
#include "kbd.h"
#include "viz.h"
#include "dinmidi.h"
#include "ensemble.h"
#include "esp_rom_sys.h"
#include "vitals.h"
#include "usbdev.h"
#include "usbmux.h"

static void list_names(cmd_ctx_t *ctx);

static cmd_status_t c_help(cmd_ctx_t *ctx)
{
    int n = 0;
    const cmd_t *t = cmd_table(&n);
    for (int i = 0; i < n; i++) {
        cmd_out(ctx, "%-8s %s", t[i].name, t[i].help);
    }
    /* THE NAMES ARE NOT COMMANDS ANY MORE, so they get their own lines: the
     * defined ones and the pictures, and how to make another. */
    list_names(ctx);
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

/* '>run [name]' - RUN EVERY COMMAND LINE IN A DOCUMENT.
 *
 * Until this existed, the only document that ever ran as a whole was 'boot',
 * at startup. So a piece could be written, named and saved, and then the only
 * way to hear it again was to stand on each line in turn and press Ctrl+Enter
 * twenty times - which is menu diving with extra steps, and it is the one
 * gesture live coding cannot be missing: load the piece, play the piece.
 *
 * With no argument it runs the document you are looking at, which is what
 * 'run' means when you are already standing in it. With a name it opens that
 * one first, because '>open kilroy1' then '>run' is two commands for one
 * thought.
 *
 * IT CANNOT RUN ITSELF. A page holding '>run' would otherwise re-enter here
 * for every line of itself, forever - the same shape as the '>dump' that fed
 * its own output back in and spun 131,073 times. One flag, checked first. */
static cmd_status_t c_run(cmd_ctx_t *ctx)
{
    static bool running;
    if (running) {
        cmd_out(ctx, "run cannot run itself");
        return CMD_ERROR;
    }

    const int was = doc_buf_current();
    if (ctx->arg[0] != '\0') {
        const int idx = doc_buf_find(ctx->arg);
        if (idx < 0 || doc_buf_select(idx) != ESP_OK) {
            cmd_out(ctx, "no document '%.20s'", ctx->arg);
            return CMD_ERROR;
        }
    }

    /* The line buffer is bounded and the document length is read ONCE, before
     * the loop: a command that lengthens the document it is being read from
     * must not extend the walk. */
    running = true;
    const size_t n = doc_len();
    char line[128];
    size_t k = 0;
    int ran = 0, failed = 0;
    for (size_t i = 0; i <= n; i++) {
        const char ch = (i < n) ? doc_at(i) : '\n';
        if (ch == '\n' || k == sizeof line - 1) {
            line[k] = '\0';
            if (k > 0) {
                switch (cmd_run_line(line, CMD_BY_GUIDE, NULL, 0)) {
                case CMD_DONE:  ran++;    break;
                case CMD_ERROR: failed++; break;
                default: break;
                }
            }
            k = 0;
            continue;
        }
        line[k++] = ch;
    }
    running = false;

    /* ALWAYS COME BACK TO WHERE THE OWNER WAS STANDING, named or not.
     *
     * Running a page is not navigation. This did leave you on the named page,
     * on the reasoning that naming it was half a request to see it - and the
     * consequence was immediate: the next thing typed went INTO the piece,
     * because the page you were working on had silently been swapped out from
     * under the cursor. That is the teleport trap wearing a different hat.
     * '>open kilroy' is how you go there; '>run kilroy' is how you hear it. */
    doc_buf_select(was);
    if (failed > 0) {
        snprintf(ctx->msg, sizeof ctx->msg, "%d ran, %d refused", ran, failed);
        return CMD_ERROR;
    }
    snprintf(ctx->msg, sizeof ctx->msg, "%d line%s ran", ran, ran == 1 ? "" : "s");
    return CMD_DONE;
}

/* '>din <gpio>' - MIDI on a wire.
 *
 * THIS IS THE ONE THAT NEEDS NO COMPUTER. Every other transport makes the deck
 * a USB device, a BLE peripheral or a network client, so all of them need
 * something else to be the host - and an SP404, a Mutant Brain, a MIDI
 * interface and every other box in a studio is a USB device too, or has no USB
 * at all. Two devices cannot talk. DIN MIDI is one wire and has been the same
 * wire since 1983, so this is what lets the deck drive hardware directly.
 *
 * THE PIN IS THE OWNER'S TO DECLARE. Guessing one on this board has already
 * cost an afternoon - the battery sense line - and the reason has not changed:
 * a pin is a fact about a physical object and only the owner can see it.
 *
 * The wiring is in dinmidi.h and it is not optional: a MIDI output is a current
 * loop, not a logic level. */
static cmd_status_t c_din(cmd_ctx_t *ctx)
{
    if (strcmp(ctx->arg, "off") == 0) {
        dinmidi_stop();
        seq_dest_enable("din", false);
        snprintf(ctx->msg, sizeof ctx->msg, "din off");
        return CMD_DONE;
    }
    if (ctx->arg[0] == '\0') {
        if (!dinmidi_running()) {
            cmd_out(ctx, "din <gpio>  MIDI out, no host");
            cmd_out(ctx, "drives an SP404, a eurorack");
            cmd_out(ctx, "brain, any MIDI IN at all.");
            cmd_out(ctx, "needs a resistor loop - see");
            cmd_out(ctx, "docs/HARDWARE.md before you");
            cmd_out(ctx, "trust it. e.g. >din 17");
            snprintf(ctx->msg, sizeof ctx->msg, "din <gpio>");
            return CMD_DONE;
        }
        /* BYTES, NOT A BOOLEAN. "din on GPIO17" is true of a deck with nothing
         * attached and of one driving a drum machine, and those are the two
         * cases the owner needs to tell apart without a scope. */
        const uint32_t n = dinmidi_bytes();
        cmd_out(ctx, "din GPIO%d, %u bytes sent", dinmidi_pin(), (unsigned)n);
        cmd_out(ctx, n ? "the wire is busy" : "nothing sent since last asked");
        snprintf(ctx->msg, sizeof ctx->msg, "din GPIO%d %uB",
                 dinmidi_pin(), (unsigned)n);
        return CMD_DONE;
    }

    const int gpio = atoi(ctx->arg);
    if (gpio <= 0 || gpio > 48) {
        cmd_out(ctx, "a GPIO number, 1-48");
        return CMD_ERROR;
    }
    /* THE PINS THIS BOARD HAS ALREADY SPOKEN FOR. Taking one of these would
     * kill the panel or the button and look like a MIDI fault. */
    static const struct { int pin; const char *what; } taken[] = {
        { 11, "panel SCK" }, { 12, "panel MOSI" }, { 5, "panel DC" },
        { 40, "panel CS" },  { 41, "panel RST" },  { 18, "the KEY button" },
    };
    for (size_t i = 0; i < sizeof taken / sizeof taken[0]; i++) {
        if (taken[i].pin == gpio) {
            cmd_out(ctx, "GPIO%d is %s", gpio, taken[i].what);
            return CMD_ERROR;
        }
    }
    /* SAY WHICH ERROR. "GPIO17 refused" was true of a pin the chip cannot use
     * and of a driver call that failed for a reason having nothing to do with
     * the pin - and it was the second one. A refusal that does not name its
     * cause sends the owner to rewire hardware that was never wrong. */
    const esp_err_t de = dinmidi_start(gpio);
    if (de != ESP_OK) {
        cmd_out(ctx, "GPIO%d refused: %s", gpio, esp_err_to_name(de));
        return CMD_ERROR;
    }
    seq_dest_enable("din", true);
    snprintf(ctx->msg, sizeof ctx->msg, "din on GPIO%d", gpio);
    return CMD_DONE;
}

/* '>kbd' - what is typing, and how to change it.
 *
 * WHY THIS IS A VERB. docs/MAP.md refuses names that delete nothing, and this
 * one earns its place by deleting a GESTURE: dropping a keyboard bond was
 * reachable only by holding KEY for two seconds - undiscoverable, silent,
 * all-or-nothing, and on a board whose switch identities are still an open item
 * in docs/ASSEMBLY.md, a hold the owner could not reliably perform. The project
 * has one rule about buttons and it is that the owner should never be asked to
 * hold one.
 *
 * It also answers a question nothing else could: a deck with no keyboard and a
 * deck with a bonded keyboard out of range look identical, and the difference
 * decides whether you go and fetch the keyboard or pair a new one.
 *
 * The hold stays, because it is the way in when there is no keyboard to type
 * with - which is exactly the case a new deck is in. */
static cmd_status_t c_kbd(cmd_ctx_t *ctx)
{
    if (strcmp(ctx->arg, "forget") == 0) {
        kbd_forget_all();
        cmd_out(ctx, "bonds dropped, scanning again.");
        cmd_out(ctx, "put the keyboard in pairing");
        cmd_out(ctx, "mode now. any HID keyboard");
        cmd_out(ctx, "works - full size included.");
        snprintf(ctx->msg, sizeof ctx->msg, "forgotten - pair one now");
        return CMD_DONE;
    }
    if (ctx->arg[0] != '\0') {
        cmd_out(ctx, "kbd          what is typing");
        cmd_out(ctx, "kbd forget   pair a different one");
        return CMD_ERROR;
    }

    const int bonds = kbd_bond_count();
    cmd_out(ctx, "%s", kbd_connected() ? "a keyboard is connected"
                                       : "no keyboard connected");
    cmd_out(ctx, "state: %.20s", kbd_state_name());
    if (bonds < 0) {
        cmd_out(ctx, "bonds: the store did not say");
    } else {
        cmd_out(ctx, "bonds: %d remembered", bonds);
    }
    /* THE TWO CASES THAT LOOK THE SAME. Saying which one this is, is most of
     * the value of the command. */
    if (!kbd_connected()) {
        cmd_out(ctx, bonds > 0 ? "it is paired but not in range"
                               : "nothing has ever paired here");
        cmd_out(ctx, "the cable is a keyboard too");
        cmd_out(ctx, "kbd forget to pair another");
    }
    snprintf(ctx->msg, sizeof ctx->msg, "%s, %d bond%s",
             kbd_connected() ? "connected" : "not connected",
             bonds < 0 ? 0 : bonds, bonds == 1 ? "" : "s");
    return CMD_DONE;
}

static cmd_status_t c_new(cmd_ctx_t *ctx)
{
    if (doc_buf_new() != ESP_OK) {
        cmd_out(ctx, "no free buffer");
        return CMD_ERROR;
    }
    /* A NEW DOCUMENT IS A BLANK SLATE, in the sequencer as well as on screen.
     *
     * It used not to be, and the result was the worst kind of wrong: an empty
     * page that went on playing the last page's lanes, with eight slots full
     * of names the text did not mention, so '>lanes' described a piece nobody
     * could see and the ninth lane was refused. What plays is what the
     * document says. */
    seq_forget_all();
    viz_forget_all();
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
static cmd_status_t c_out(cmd_ctx_t *ctx) __attribute__((unused));
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
    /* TWO DENSITIES, AND THE PANEL DECIDES THAT, NOT TASTE.
     *
     * A middle size was built and thrown away. Cell height must be a multiple
     * of 12 (the CASET quantum) and width must be even (RASET), so the only
     * cells at a legible height are 24 tall - and fitting more columns means
     * narrowing the body, which at 24 tall means a face that reads thin and
     * barely monospaced. Every body column of the chunky art carries ink, so
     * there is no lossless crop either: narrowing collapses a 2 px stem
     * somewhere, and 2 px stems are why the chunky face is readable on a
     * reflective panel with no backlight at all.
     *
     * So: low is 12x24 at 30 columns, high is 6x12 at 60. For visual work use
     * high - a third-width split leaves 39 columns of code, which fits any
     * pattern line without wrapping. */
    int level;
    const char a = ctx->arg[0];
    if (a == 'h' || a == 'd' || a == '6') {
        level = 2;                            /* high / dense  - 6x12,  60 */
    } else if (a == 'l' || a == 'c' || a == '1') {
        level = 0;                            /* low / chunky - 12x24,  30 */
    } else {
        cmd_out(ctx, "density low | high");
        return CMD_ERROR;
    }
    if (editor_set_density(level) != ESP_OK) {
        cmd_out(ctx, "density: layout refused");
        return CMD_ERROR;
    }
    /* Derived, not hardcoded: the string said 60x20 while the layout computed
     * 60x24. A status message that disagrees with the machine is a small lie
     * that costs someone an afternoon later. */
    static const char *names[3] = { "low", "-", "high" };
    snprintf(ctx->msg, sizeof ctx->msg, "%s %dx%d", names[level],
             tg_cols(), tg_rows());
    return CMD_DONE;
}

/* ------------------------------------------------------------------ names
 *
 * A LANE IS A NAME AND A PATTERN, AND THE NAME IS DEFINED, NOT BUILT IN.
 *
 * "kick x...x...x...x..." is still twenty-two keystrokes for a four-on-the-floor
 * with nothing declared first - the boot document declares it, at startup:
 *
 *     >kick = note 36
 *     >bass = voice 2 ch 1 gate 180
 *     >cut = cc 74
 *
 * The seventeen sound names used to be verbs with their numbers compiled in, so
 * a player could add neither a conga nor a second controller without a
 * firmware build, and could not move a kick to the note their drum machine
 * wants. The picture names were verbs too, sixteen more. Now there is ONE lane
 * command for all of them, a table of defined names, and the pictures by their
 * own names - and thirty-three verbs are gone. docs/MANIFESTO.md §3.8; the
 * grammar of the name and the definition is lane_name.h.
 */
typedef struct {
    char     name[LANE_BASE_MAX + 1];
    uint8_t  kind;          /* LD_NOTE, LD_VOICE, LD_CC or LD_DRAW          */
    uint8_t  num;           /* note, octave, controller, or picture index   */
    uint8_t  chan;          /* 0-15                                         */
    uint16_t gate;          /* ms                                           */
} alias_t;

/* THIRTY-TWO NAMES. Sixteen come in the boot document, which leaves room for a
 * player's own twice over; a definition past that is refused with the number,
 * not dropped. */
#define ALIAS_MAX 32
static alias_t s_alias[ALIAS_MAX];

_Static_assert(LANE_NAME_MAX <= SEQ_NAME_MAX, "a lane's address fits its name");

static alias_t *alias_find(const char *name)
{
    for (int i = 0; i < ALIAS_MAX; i++) {
        if (s_alias[i].name[0] != '\0' && strcmp(s_alias[i].name, name) == 0) {
            return &s_alias[i];
        }
    }
    return NULL;
}

/* Is this word the exact name of a command? */
static bool is_verb(const char *w)
{
    int n = 0;
    const cmd_t *t = cmd_table(&n);
    for (int i = 0; i < n; i++) {
        if (strcmp(t[i].name, w) == 0) {
            return true;
        }
    }
    return false;
}

bool cmd_lane_known(const char *word, size_t n)
{
    char base[LANE_BASE_MAX + 1];
    size_t k = 0;
    while (k < n && word[k] != ':' && k < LANE_BASE_MAX) {
        base[k] = word[k];
        k++;
    }
    if (k < n && word[k] != ':') {
        return false;                  /* longer than a name can be */
    }
    base[k] = '\0';
    return alias_find(base) != NULL || viz_prim_index(base) >= 0;
}

/* WHAT A NAME IS BOUND TO: a defined name, or a picture by its own name, plus
 * the part the address selects. False, with the reason, when it is neither -
 * or when it names a part the binding does not have. */
static bool binding_of(const lane_name_t *ln, seq_binding_t *b, char *why,
                       size_t wn)
{
    memset(b, 0, sizeof *b);
    const alias_t *a = alias_find(ln->base);
    int prim = -1;
    if (a != NULL) {
        switch (a->kind) {
        case LD_NOTE:
            b->bind = SEQ_BIND_NOTE;
            b->note = a->num;
            b->chan = a->chan;
            b->gate_ms = a->gate;
            break;
        case LD_VOICE:
            b->bind = SEQ_BIND_NOTE;
            b->melodic = true;
            b->octave = (int8_t)a->num;
            b->chan = a->chan;
            b->gate_ms = a->gate;
            break;
        case LD_CC:
            b->bind = SEQ_BIND_CC;
            b->cc = a->num;
            b->chan = a->chan;
            break;
        default:
            prim = a->num;
            break;
        }
    } else {
        prim = viz_prim_index(ln->base);
        if (prim < 0) {
            snprintf(why, wn, "%s? try: help", ln->base);
            return false;
        }
    }
    if (prim >= 0) {
        b->bind  = SEQ_BIND_VIZ;
        b->prim  = (uint8_t)prim;
        b->param = (uint8_t)viz_param_index(ln->part);
        /* A part that does nothing is a lane that silently does nothing, so it
         * is refused - the rule '>box[z]' was the one loud case of, before. */
        if (ln->part[0] != '\0' && b->param == VIZ_PARAM_NONE) {
            snprintf(why, wn, "no :%s - a picture has x, y", ln->part);
            return false;
        }
        return true;
    }
    /* A SOUND'S PARTS: how hard, and - on a voice - which octave. The same
     * sentence as a circle's position, pointed at a sound: '>bass:vel 9..3' and
     * '>bass:oct <2 3>' are lanes like any other (docs/MANIFESTO.md §3.7,
     * §3.11). A controller has none; its digit already is its value. */
    if (ln->part[0] != '\0') {
        if (b->bind == SEQ_BIND_NOTE && strcmp(ln->part, "vel") == 0) {
            b->param = SEQ_PART_VEL;
        } else if (b->bind == SEQ_BIND_NOTE && b->melodic &&
                   strcmp(ln->part, "oct") == 0) {
            b->param = SEQ_PART_OCT;
        } else if (b->bind == SEQ_BIND_NOTE) {
            snprintf(why, wn, b->melodic ? "a voice has :vel and :oct"
                                         : "a drum has :vel");
            return false;
        } else {
            snprintf(why, wn, "%s has no parts", ln->base);
            return false;
        }
    }
    return true;
}

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
static bool rerun_silences_named(const char *name, const char *pat)
{
    if (!seq_running()) {
        return false;
    }
    const seq_lane_t *l = seq_lane_find(name, -1);
    return l != NULL && !l->muted && l->src == seq_pattern_hash(pat);
}

static bool rerun_silences(const cmd_ctx_t *ctx)
{
    if (ctx->caller != CMD_BY_HANDS) {
        return false;
    }
    return rerun_silences_named(ctx->name, ctx->arg);
}

/* A LANE THAT WAS NOT COMPILED, AND WHY.
 *
 * This printed "16 lanes is all there is" for every refusal - including a
 * pattern too long to hold, which then read as a full lane table to a performer
 * with three lanes playing. Now each reason is its own sentence, and a pattern
 * refused because of one character says which, marks it in the document
 * (cmd_ctx.err_at), and fits the status bar in ONE line - so a typo does not
 * throw the performer into the output page mid-song. */
static cmd_status_t lane_refused(cmd_ctx_t *ctx, esp_err_t e)
{
    if (e == ESP_ERR_NO_MEM) {
        cmd_out(ctx, "%d lanes is all there is.", SEQ_MAX_LANES);
        cmd_out(ctx, "free one: type its name alone");
        return CMD_ERROR;
    }
    int at = -1;
    const char *why = seq_lane_error(&at);
    ctx->err_at = at;
    cmd_out(ctx, "%s", why[0] ? why : "not a pattern");
    return CMD_ERROR;
}

/* ONE COMMAND FOR EVERY LANE - a drum, a voice, a controller, a picture - because
 * a lane is a lane and the name only says where it goes. `word` is the address
 * as typed: 'kick', 'disc:2', 'disc:2:x'. */
cmd_status_t cmd_lane(cmd_ctx_t *ctx, const char *word, size_t n)
{
    lane_name_t ln;
    char why[40];
    const int e = lane_name_parse(word, n, &ln);
    if (e != LN_OK) {
        lane_name_error_text(e, why, sizeof why);
        cmd_out(ctx, "%s", why);
        return CMD_ERROR;
    }
    seq_binding_t b;
    if (!binding_of(&ln, &b, why, sizeof why)) {
        cmd_out(ctx, "%s", why);
        return CMD_ERROR;
    }
    /* THE LANE'S NAME IS ITS CANONICAL ADDRESS - 'disc:1' is 'disc' - so two
     * spellings of one lane are one lane. */
    static char name[SEQ_NAME_MAX];
    snprintf(name, sizeof name, "%s", ln.canon);
    ctx->name = name;
    const char *pat = ctx->arg;

    /* A BARE NAME MEANS THE LANE IS GONE, not muted. Muting has a word -
     * '>mute kick' - and leaves the slot allocated, which is how a session once
     * filled every lane with names the document no longer mentioned. Re-running
     * the same line still silences, because that is a performance gesture on a
     * lane that is still part of the piece. */
    if (pat[0] == '\0') {
        const bool had = seq_forget(name) == ESP_OK;
        snprintf(ctx->msg, sizeof ctx->msg, had ? "%s gone" : "no %s", name);
        return CMD_DONE;
    }
    if (rerun_silences(ctx)) {
        seq_mute(name, true);
        snprintf(ctx->msg, sizeof ctx->msg, "%s silent", name);
        return CMD_DONE;
    }
    /* A WAY WHERE NOTHING TURNS IS REFUSED (docs/MANIFESTO.md §3.10). 'u d l r'
     * are steps for move, warp, ramp and turn; on a drum, a controller or a
     * circle they did nothing, so '>kick x..u' played a hit where the performer
     * had typed something else - exactly the silent kind of typo §3.2 closed. */
    {
        static seq_comp_t c;
        const bool turns = (b.bind == SEQ_BIND_VIZ && b.param == VIZ_PARAM_NONE &&
                            viz_prim_turns(b.prim));
        if (!turns && seq_pattern_compile(pat, &c) == SEQ_PAT_OK) {
            int at = -1;
            if (c.dir != 0) {
                const char *q = pat;
                while (*q == ' ') { q++; }
                at = (int)(q - pat);
            }
            for (int i = 0; i < c.n && at < 0; i++) {
                if (c.leaf[i].dir != 0) { at = c.leaf[i].at; }
            }
            if (at >= 0) {
                ctx->err_at = at;
                cmd_out(ctx, "u d l r: move warp ramp turn");
                return CMD_ERROR;
            }
        }
    }
    if (seq_lane_bind(name, &b) != ESP_OK) {
        return lane_refused(ctx, ESP_ERR_NO_MEM);
    }
    const esp_err_t le = seq_lane(name, pat);
    if (le != ESP_OK) {
        return lane_refused(ctx, le);
    }
    seq_mute(name, false);
    if (b.bind == SEQ_BIND_VIZ) {
        viz_split(true);
    }
    if (b.melodic) {
        snprintf(ctx->msg, sizeof ctx->msg, "%s %s in %s", name, pat,
                 seq_scale_name());
    } else {
        snprintf(ctx->msg, sizeof ctx->msg, "%s %s", name, pat);
    }
    return CMD_DONE;
}

/* Rebind every live lane of this name, so redefining a name changes what is
 * already playing: '>kick = note 35' moves the running kick, which is the whole
 * reason to redefine it mid-set. */
static void rebind_lanes(const char *base)
{
    int n = 0;
    const seq_lane_t *l = seq_lanes(&n);
    for (int i = 0; i < SEQ_MAX_LANES; i++) {
        if (!l[i].used) {
            continue;
        }
        lane_name_t ln;
        char why[40];
        seq_binding_t b;
        if (lane_name_parse(l[i].name, strlen(l[i].name), &ln) == LN_OK &&
            strcmp(ln.base, base) == 0 && binding_of(&ln, &b, why, sizeof why)) {
            seq_lane_bind(l[i].name, &b);
        }
    }
}

/* '>conga = note 63' - DEFINE A NAME. See lane_name.h for the grammar. */
cmd_status_t cmd_define(cmd_ctx_t *ctx, const char *word, size_t n)
{
    lane_name_t ln;
    char why[40];
    const int e = lane_name_parse(word, n, &ln);
    if (e != LN_OK) {
        lane_name_error_text(e, why, sizeof why);
        cmd_out(ctx, "%s", why);
        return CMD_ERROR;
    }
    if (ln.inst != 1 || ln.part[0] != '\0') {
        cmd_out(ctx, "define the plain name: %s", ln.base);
        return CMD_ERROR;
    }
    /* A name may not shadow a command or a picture. Shadowing 'play' would make
     * a line in some document mean something different from the page it came
     * from; shadowing 'disc' would leave no way back to the circle. An alias to a
     * picture is fine - '>circle = disc' - it is a second name, not a replacement. */
    if (is_verb(ln.base)) {
        cmd_out(ctx, "%s is a command", ln.base);
        return CMD_ERROR;
    }
    if (viz_prim_index(ln.base) >= 0) {
        cmd_out(ctx, "%s is a picture already", ln.base);
        return CMD_ERROR;
    }
    const char *arg = ctx->arg;
    if (*arg == '=') {
        arg++;
    }
    lane_def_t d;
    lane_def_parse(arg, &d);
    if (d.kind == LD_ERROR) {
        cmd_out(ctx, "%s", d.why);
        return CMD_ERROR;
    }
    alias_t *a = alias_find(ln.base);
    if (d.kind == LD_REMOVE) {
        if (a == NULL) {
            snprintf(ctx->msg, sizeof ctx->msg, "no name %s", ln.base);
            return CMD_DONE;
        }
        /* A NAME THAT GOES TAKES ITS LANES WITH IT. Leaving them playing would
         * leave lanes nothing can name - not to change, not to drop - which is a
         * stuck note with extra steps. */
        int n2 = 0;
        const seq_lane_t *l = seq_lanes(&n2);
        for (int i = 0; i < SEQ_MAX_LANES; i++) {
            lane_name_t o;
            if (l[i].used &&
                lane_name_parse(l[i].name, strlen(l[i].name), &o) == LN_OK &&
                strcmp(o.base, ln.base) == 0) {
                seq_forget(l[i].name);
            }
        }
        a->name[0] = '\0';
        snprintf(ctx->msg, sizeof ctx->msg, "%s is not a name now", ln.base);
        return CMD_DONE;
    }
    int prim = -1;
    if (d.kind == LD_DRAW) {
        prim = viz_prim_index(d.draw);
        if (prim < 0) {
            cmd_out(ctx, "no picture called %s", d.draw);
            return CMD_ERROR;
        }
    }
    if (a == NULL) {
        for (int i = 0; i < ALIAS_MAX && a == NULL; i++) {
            if (s_alias[i].name[0] == '\0') {
                a = &s_alias[i];
            }
        }
        if (a == NULL) {
            cmd_out(ctx, "%d names is all there is", ALIAS_MAX);
            return CMD_ERROR;
        }
    }
    snprintf(a->name, sizeof a->name, "%s", ln.base);
    a->kind = (uint8_t)d.kind;
    a->num  = (uint8_t)(d.kind == LD_DRAW ? prim : d.num);
    a->chan = (uint8_t)((d.chan > 0 ? d.chan : 1) - 1);
    a->gate = (uint16_t)d.gate;
    rebind_lanes(ln.base);
    snprintf(ctx->msg, sizeof ctx->msg, "%s =%s", ln.base, arg);
    return CMD_DONE;
}

/* Every name there is, for '>help': the defined ones, then the pictures. */
static void list_names(cmd_ctx_t *ctx)
{
    char line[40] = "names:";
    for (int pass = 0; pass < 2; pass++) {
        const int count = (pass == 0) ? ALIAS_MAX : viz_prim_count();
        for (int i = 0; i < count; i++) {
            const char *w = (pass == 0) ? s_alias[i].name : viz_prim_name(i);
            if (w == NULL || w[0] == '\0') {
                continue;
            }
            if (strlen(line) + 1 + strlen(w) > 29) {
                cmd_out(ctx, "%s", line);
                snprintf(line, sizeof line, "      ");
            }
            strncat(line, " ", sizeof line - strlen(line) - 1);
            strncat(line, w, sizeof line - strlen(line) - 1);
        }
    }
    cmd_out(ctx, "%s", line);
    cmd_out(ctx, "your own: >conga = note 63");
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

/* A whole number in [lo, hi] and nothing after it, or false. The performance
 * parameters used atoi(), which reads '>swing fast' as 0 and lets the clamp make
 * it 50 - a change nobody asked for, reported back as though they had. */
static bool whole_number(const char *s, long lo, long hi, long *out)
{
    char *end = NULL;
    const long v = strtol(s, &end, 10);
    if (end == s || end == NULL) {
        return false;
    }
    while (*end == ' ' || *end == '\t') { end++; }
    if (*end != '\0' || v < lo || v > hi) {
        return false;
    }
    *out = v;
    return true;
}

static cmd_status_t c_swing(cmd_ctx_t *ctx)
{
    if (ctx->arg[0] != '\0') {
        long v = 0;
        if (!whole_number(ctx->arg, 50, 75, &v)) {
            cmd_out(ctx, "swing is 50-75: 67 is triplet");
            return CMD_ERROR;
        }
        seq_swing((int)v);
    }
    const int s = seq_get_swing();
    snprintf(ctx->msg, sizeof ctx->msg, "swing %d%s", s,
             s == 50 ? " (straight)" : s >= 66 && s <= 68 ? " (triplet)" : "");
    return CMD_DONE;
}

/* '>sync' - SHARING TIME, in both directions it can be shared.
 *
 * One verb because it is one idea. 'on' and 'off' put MIDI clock on the wire for a
 * DAW or a drum machine to follow; 'lead' and 'follow' share a clock with the
 * other decks in the room. Both are this deck agreeing about time with something
 * else, and docs/MAP.md refuses a second verb for the second half of an idea.
 *
 * The ensemble runs over ESP-NOW, which needs no router, no password and no
 * association - so two decks play together by typing one word each, which is the
 * only interaction rate a performance tolerates. See ensemble.h for why not mesh.
 */
static cmd_status_t c_sync(cmd_ctx_t *ctx)
{
    const char *a = ctx->arg;

    if (strcmp(a, "lead") == 0 || strcmp(a, "follow") == 0) {
        const ensemble_role_t r = (a[0] == 'l') ? ENSEMBLE_LEAD : ENSEMBLE_FOLLOW;
        if (ensemble_set(r) != ESP_OK) {
            cmd_out(ctx, "the radio would not start");
            return CMD_ERROR;
        }
        cmd_out(ctx, "%s the ensemble.", (r == ENSEMBLE_LEAD) ? "leading"
                                                             : "following");
        cmd_out(ctx, "no network needed - the decks");
        cmd_out(ctx, "talk to each other directly.");
        cmd_out(ctx, "one deck leads, the rest follow.");
        snprintf(ctx->msg, sizeof ctx->msg, "sync %s", a);
        return CMD_DONE;
    }
    if (strcmp(a, "alone") == 0) {
        ensemble_set(ENSEMBLE_OFF);
        snprintf(ctx->msg, sizeof ctx->msg, "sync alone");
        return CMD_DONE;
    }

    if (a[0] == '\0') {
        int peers = 0; int32_t err = 0; uint32_t heard = 0;
        cmd_out(ctx, "midi clock out: %s", seq_get_sync() ? "on" : "off");
        if (ensemble_state(&peers, &err, &heard)) {
            cmd_out(ctx, "%s, %d other deck%s",
                    ensemble_role() == ENSEMBLE_LEAD ? "leading" : "following",
                    peers, peers == 1 ? "" : "s");
            /* THE ERROR IS THE WHOLE POINT OF ASKING. "following" is true of a
             * deck in phase and of one that has heard nothing for a minute, and
             * those are the two a player needs to tell apart on stage. */
            if (ensemble_role() == ENSEMBLE_FOLLOW) {
                cmd_out(ctx, "off by %d us, %u packets",
                        (int)err, (unsigned)heard);
                /* The floor is what this link can do; the skip count is how often
                 * the air was too busy to trust. Together they say whether a bad
                 * error is the clock or the room. */
                cmd_out(ctx, "best trip %d us, %u skipped",
                        (int)ensemble_floor_rtt(), (unsigned)ensemble_skipped());
                /* And how much the probes behind that number agreed with each
                 * other, which is the estimator grading its own work. */
                cmd_out(ctx, "probes agreed within %d us",
                        (int)ensemble_spread());
                uint32_t rep = 0, stale = 0, lost = 0, dup = 0, win = 0;
                ensemble_counts(&rep, &stale, &lost, &dup, &win);
                cmd_out(ctx, "%u replies, %u twice, %u lost ack",
                        (unsigned)rep, (unsigned)dup, (unsigned)lost);
                cmd_out(ctx, "%u corrections, %u stale",
                        (unsigned)win, (unsigned)stale);
            } else {
                cmd_out(ctx, "%u packets since asked", (unsigned)heard);
            }
        } else {
            cmd_out(ctx, "playing alone");
        }
        cmd_out(ctx, "sync on | off   midi clock");
        cmd_out(ctx, "sync lead | follow | alone");
        snprintf(ctx->msg, sizeof ctx->msg, "clock %s",
                 seq_get_sync() ? "on" : "off");
        return CMD_DONE;
    }

    const bool on = (strcmp(a, "on") == 0);
    if (!on && strcmp(a, "off") != 0) {
        cmd_out(ctx, "sync on | off | lead | follow | alone");
        return CMD_ERROR;
    }
    seq_sync(on);
    snprintf(ctx->msg, sizeof ctx->msg, "midi clock %s", on ? "on" : "off");
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

/* REBOOT WITHOUT RUNNING SHUTDOWN HANDLERS, and this is not belt-and-braces -
 * esp_restart() does not come back from USB MIDI mode.
 *
 * Observed three times and finally caught with the console open: '>usb off' runs,
 * doc_save_all_dirty() works through every buffer, the SD mirror completes - and
 * then nothing. No announce, no reset. The deck stops inside the restart, with the
 * USB device still enumerated because the PHY was never handed back, and with
 * every task gone except TinyUSB's own. That is the hang docs/OS.md describes, and
 * every occurrence of it followed a deliberate restart from USB MIDI mode. A deck
 * left alone in that mode ran five minutes with the clock exact.
 *
 * esp_restart() runs registered shutdown handlers first. usbdev registers one, and
 * TinyUSB installs its own teardown; a deadlock in there never completes, and
 * because it happens with the scheduler still up, the task watchdog's own
 * reporting path is gone too - which is why nothing ever rescued it.
 *
 * So: reset the chip and skip the handlers. Nothing is lost by doing so. Every
 * document is already saved above, and the one thing the handler does - handing
 * the PHY back to USB-Serial-JTAG - is done unconditionally at boot in main.c
 * before anything else can want it. The orderly path was buying a teardown this
 * device does not need and cannot survive.
 *
 * Same reasoning as CONFIG_ESP_SYSTEM_PANIC_SILENT_REBOOT: a restart that cannot
 * complete is a restart that never happens. */
static void deck_reboot(void)
{
    esp_rom_software_reset_system();
    for (;;) { }                     /* not reached */
}

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
    /* Close the vitals record, so the next boot knows this restart was chosen
     * rather than suffered - see vitals.h. */
    vitals_goodbye(usbdev_wanted(), false, seq_position());
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
static void usbtest_cb(void *arg) __attribute__((unused));
static void usbtest_cb(void *arg)
{
    (void)arg;
    usbmux_release_to_usj();
}

static cmd_status_t c_usbtest(cmd_ctx_t *ctx) __attribute__((unused));
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
    cmd_out(ctx, "%-5s n%-6u sd%4dus late%u", what, (unsigned)s->n,
            (int)(var > 0 ? sqrt(var) : 0), (unsigned)s->late);
    /* TWO NUMBERS, TWO LINES, AND NEITHER CAN BE READ AS THE OTHER.
     *
     * This was "spread %d us  worst @%us" on one line, which put a duration in
     * microseconds next to a MOMENT in seconds with nothing between them but
     * two spaces - and it was read, reasonably, as a hundred-and-ten-second
     * latency. A worst case of two minutes would be a catastrophe; the actual
     * worst case was eleven milliseconds, at the instant play was pressed.
     * A number nobody can parse is worse than no number, because it is
     * indistinguishable from a disaster. */
    cmd_out(ctx, "      spread %d us", (int)(s->max - s->min));
    /* WHERE THE TICKS SIT, not only how much they wander. On a following deck
     * this is the distance between when its ticks fire and where the ensemble
     * says they belong - which sd and spread cannot show, because a constant
     * offset has neither. */
    cmd_out(ctx, "      mean %+d us", (int)mean);
    cmd_out(ctx, "      widest one at t+%us",
            (unsigned)(s->worst_ms / 1000));
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
    cmd_out(ctx, "mean = where ticks sit on it");
    cmd_out(ctx, "sd/spread are MICROseconds.");
    cmd_out(ctx, "t+ is when, not how long.");
    cmd_out(ctx, "first 8 ticks after play skipped");
    /* THE PREVIOUS RUN'S LAST WORDS. Reported here rather than under a new verb,
     * because docs/MAP.md refuses names that delete nothing and this is a
     * diagnostic - which is what '>jitter' already is. It is how the hang in
     * docs/OS.md gets caught: the console dies with the deck, so the evidence has
     * to arrive on the next boot instead. */
    {
        const char *v[VITALS_LINES];
        const int n = vitals_report(v);
        for (int i = 0; i < n; i++) {
            cmd_out(ctx, "%s", v[i]);
        }
    }
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
    /* THE MODE THIS RUN WAS IN, captured before it is changed. The vitals record
     * is written further down, after usbdev_want() has already flipped the flag,
     * so reading it there recorded the mode the deck was going TO - and a record
     * of a hang that names the wrong mode sends the next investigation to the
     * wrong place. */
    const bool was_usb = usbdev_wanted();

    if (!on && !off) {
        cmd_out(ctx, "usb is %s%s", usbdev_wanted() ? "on" : "off",
                usbdev_mounted() ? ", host attached" : "");
        cmd_out(ctx, "usb on   MIDI + console, 1 cable");
        cmd_out(ctx, "usb off  back to serial");
        cmd_out(ctx, "a power cycle always returns");
        cmd_out(ctx, "to serial. put >usb on in boot");
        cmd_out(ctx, "to have it every time.");
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
    /* Close the vitals record, so the next boot knows this restart was chosen
     * rather than suffered - see vitals.h. */
    vitals_goodbye(was_usb, false, seq_position());
    /* Say REBOOTING. This command deliberately restarts the deck, which cuts
     * the console off mid-sentence - and the owner reported '>usb on' as a
     * crash, because that is exactly what a deliberate reboot looks like from
     * a serial terminal. */
    cmd_announce(on ? "USB MIDI - rebooting now"
                    : "serial console - rebooting now");
    vTaskDelay(pdMS_TO_TICKS(600));
    deck_reboot();
    return CMD_DONE;                 /* not reached */
}

/* '>wifi', '>host' and '>osc' - the network, as three commands that each say
 * exactly what they do.
 *
 * All three are CMD_CAP_NET, so a guide file may use them but the boot
 * document - which runs with guide authority - can bring the deck onto a
 * network at startup for an installation. What a guide may NOT do is change
 * pairing or power, which is where the line sits. */
static void two_words(const char *arg, char *a, size_t an, char *b, size_t bn)
{
    size_t i = 0;
    a[0] = b[0] = '\0';
    while (*arg == ' ') { arg++; }
    while (*arg != '\0' && *arg != ' ' && i < an - 1) { a[i++] = *arg++; }
    a[i] = '\0';
    while (*arg == ' ') { arg++; }
    i = 0;
    while (*arg != '\0' && i < bn - 1) { b[i++] = *arg++; }
    b[i] = '\0';
}

/* '>battery' - find the sense pin, because nothing documents it.
 *
 * Prints every free ADC1 channel. Unplug USB and run it again: the channel
 * that MOVES with the cell is the battery, and once it is known the reading
 * can be wired up and this command deleted. Until then battery_percent()
 * returns -1 and the status bar shows nothing rather than a number somebody
 * made up. */
static cmd_status_t c_battery(cmd_ctx_t *ctx)
{
    /* '>battery use 4' once the owner knows which channel moved, optionally
     * with the divider ratio x10 - 'use 4 20' is a 2:1 network. Persisted, so
     * it is a one-time act, and 'use 0' forgets it. */
    if (strncmp(ctx->arg, "use", 3) == 0) {
        int g = 0, d = 20;
        sscanf(ctx->arg + 3, "%d %d", &g, &d);
        battery_use(g, d);
        const int mv = battery_mv();
        if (mv > 0) {
            cmd_out(ctx, "GPIO%d: %dmV = %d%%", g, mv, battery_percent());
        } else {
            cmd_out(ctx, "GPIO%d reads nothing", g);
        }
        snprintf(ctx->msg, sizeof ctx->msg, "battery on GPIO%d", g);
        return CMD_DONE;
    }

    const int mv = battery_mv();
    if (mv > 0) {
        cmd_out(ctx, "cell %dmV = %d%%", mv, battery_percent());
    }
    char s[120];
    battery_scan(s, sizeof s);
    /* Chunked to the grid, because a 120-character line on a 30-column screen
     * is the same mistake twice. */
    for (size_t i = 0; i < strlen(s); i += 28) {
        cmd_out(ctx, "%.28s", s + i);
    }
    cmd_out(ctx, "unplug USB, run again: the one");
    cmd_out(ctx, "that moves is it. then:");
    cmd_out(ctx, "  battery use <gpio>");
    snprintf(ctx->msg, sizeof ctx->msg, "scanned ADC1");
    return CMD_DONE;
}

/* '>ssh user@host pass command...' - run something on another machine and get
 * the answer back as a document.
 *
 * The password is on the line, which is a real trade-off stated plainly: it is
 * typed on a thumb keyboard by someone holding the device, it goes into a
 * document, and '+ssh' is TRANSIENT so that document is never journalled and
 * never reaches the SD mirror or the owner's DGX. The alternative - a key in
 * NVS - is better and is the next step; this is the version that works today
 * without a key-management design nobody has agreed yet.
 *
 * Runs on the editor task, blocking, with a 15-second library timeout. An SSH
 * server that never answers must not become a deck that never redraws. */
/* '>frame' - send a document to the renderer as ASCII.
 *
 * THIS IS THE VISUAL PRIMITIVE, and it is deliberately not a graphics API. The
 * deck's medium is a rectangle of characters; the most useful thing it can hand
 * a projector is that rectangle. A receiver renders it however it likes - as
 * text, as a bitmap, as geometry driven by the glyphs - and the deck stays the
 * text and control brain, which is what the pogo-pin coprocessor plan assumes.
 *
 * With no argument it sends the current document, so an ASCII drawing IS a
 * frame and editing it live IS visual coding - the same Ctrl+Enter, the same
 * buffer, no second environment. */
static cmd_status_t c_split(cmd_ctx_t *ctx)
{
    /* '>split 20' sets how many columns the visual gets and turns it on. The
     * default is a third, because the code is what is being edited and the
     * preview is a monitor - a half-and-half split at 30 columns wrapped every
     * pattern line and made the document unnavigable.
     *
     * ON AND OFF ARE SPELLED OUT BECAUSE A DOCUMENT LINE HAS TO BE IDEMPOTENT.
     * A bare '>split' toggles, which is right for a hand at the keyboard and
     * wrong in a document: '>disc 9' turns the preview on by itself, so
     * a '>split' further down the page turned it back OFF, and re-running the
     * page flipped it again. Running a document twice has to leave the device
     * in the same state both times, or the text is not a description of the
     * piece. So a page says 'split on' and a performer says 'split'. */
    if (strcmp(ctx->arg, "on") == 0) {
        viz_split(true);
    } else if (strcmp(ctx->arg, "off") == 0) {
        viz_split(false);
    } else if (ctx->arg[0] != '\0') {
        const int n = atoi(ctx->arg);
        if (n <= 0) {
            cmd_out(ctx, "split on | off | <rows>");
            return CMD_ERROR;
        }
        viz_split_rows(n);
        viz_split(true);
    } else {
        viz_split(!viz_split_on());
    }
    tg_invalidate();
    if (!viz_split_on()) {
        snprintf(ctx->msg, sizeof ctx->msg, "split off");
        return CMD_DONE;
    }
    /* Say the SHAPE, not just a number. The direction is decided by arithmetic
     * on the grid - see viz_pane() - so the one thing the owner cannot work out
     * for themselves is which way it went and how big the picture ended up. */
    snprintf(ctx->msg, sizeof ctx->msg, "view %dx%d below", viz_cols(),
             viz_rows());
    return CMD_DONE;
}

/* '>route <gen> <lane>' - the sidechain, generalised.
 *
 * The visual lane says WHEN and the named music lane says HOW MUCH. It is one
 * idea - a lane may read another lane's output - and it works between any two,
 * which is why it is not called sidechaining: the same mechanism sends a kick
 * to a bar height or a filter sweep to a wave amplitude. */
static cmd_status_t c_route(cmd_ctx_t *ctx)
{
    if (ctx->arg[0] == '\0') {
        cmd_out(ctx, "route <lane> <lane it follows>");
        cmd_out(ctx, "route disc kick");
        cmd_out(ctx, "route grow disc   viz drives viz");
        cmd_out(ctx, "route disc:x bass a part follows");
        cmd_out(ctx, "route disc       unroutes");
        snprintf(ctx->msg, sizeof ctx->msg, "route disc kick");
        return CMD_DONE;
    }
    char gw[SEQ_NAME_MAX], sw[SEQ_NAME_MAX];
    two_words(ctx->arg, gw, sizeof gw, sw, sizeof sw);
    /* BOTH SIDES ARE ADDRESSES, taken to their canonical spelling, so 'disc:1'
     * and 'disc' route the same lane - the name is the lane. */
    lane_name_t g, src;
    char why[40];
    int e = lane_name_parse(gw, strlen(gw), &g);
    if (e == LN_OK && sw[0] != '\0') {
        e = lane_name_parse(sw, strlen(sw), &src);
    }
    if (e != LN_OK) {
        lane_name_error_text(e, why, sizeof why);
        cmd_out(ctx, "%s", why);
        return CMD_ERROR;
    }
    const char *gen = g.canon;
    const char *from = (sw[0] != '\0') ? src.canon : "";
    /* A ROUTE IS A PATTERN, AND THAT HAS TO SURVIVE THE COLLAPSE.
     *
     * seq_route works on a lane that exists, and seq deliberately does not know
     * what a name is bound to - so '>route grow disc' failed on a fresh
     * document with "no lane 'grow'". Binding here is the command layer's job,
     * because this is the only layer that knows a picture from a drum. */
    if (seq_lane_find(gen, -1) == NULL) {
        seq_binding_t b;
        if (!binding_of(&g, &b, why, sizeof why)) {
            cmd_out(ctx, "%s", why);
            return CMD_ERROR;
        }
        seq_lane_bind(gen, &b);
    }
    const esp_err_t re = seq_route(gen, from);
    if (re == ESP_ERR_INVALID_ARG) {
        cmd_out(ctx, "%s cannot follow itself", gen);
        return CMD_ERROR;
    }
    if (re != ESP_OK) {
        /* ANY LANE, NOT JUST A PICTURE. Routing used to live in the visual
         * half only, so a kick could drive a circle and a circle could drive
         * nothing. One table means one mechanism for every pair. */
        cmd_out(ctx, "no lane '%.18s' to drive", gen);
        cmd_out(ctx, "write it first, then route it");
        return CMD_ERROR;
    }
    /* A SOURCE THAT DOES NOT EXIST IS THE SILENT FAILURE HERE. The route is
     * set, the primitive stops drawing because nothing ever triggers it, and
     * nothing anywhere says why. Typing it is still allowed - the lane may be
     * written on the next line - but it says so. */
    /* 'intro:end' is the moment the lane called intro finishes; the lane to
     * look for is intro, and it only ever finishes if it has a count. */
    if (from[0] != '\0' && strcmp(src.part, "end") == 0) {
        char base[SEQ_NAME_MAX];
        snprintf(base, sizeof base, "%.*s", (int)(strlen(from) - 4), from);
        const seq_lane_t *sl = seq_lane_find(base, -1);
        if (sl == NULL) {
            cmd_out(ctx, "no lane '%s' yet - it will", base);
            cmd_out(ctx, "stay silent until there is one");
        } else if (sl->count == 0) {
            cmd_out(ctx, "%s never ends - give it !n", base);
        }
    } else if (from[0] != '\0' && seq_lane_find(from, -1) == NULL) {
        cmd_out(ctx, "no lane '%s' yet - it will", from);
        cmd_out(ctx, "stay silent until there is one");
    }
    snprintf(ctx->msg, sizeof ctx->msg, from[0] ? "%s <- %s" : "%s unrouted",
             gen, from);
    return CMD_DONE;
}

static cmd_status_t c_frame(cmd_ctx_t *ctx)
{
    /* If visuals are running, THE FRAME IS THE FRAME. Sending the document
     * instead would be sending the source when the owner asked for the
     * picture. With no visual lanes it falls back to the document, which is
     * how a hand-drawn ASCII frame gets out. */
    if (ctx->arg[0] == '\0' && viz_active()) {
        static char vt[VIZ_H * (VIZ_W + 2) + 4];
        const int n = viz_text(vt, (int)sizeof vt);
        const esp_err_t ve = net_osc_frame(vt);
        if (ve == ESP_ERR_INVALID_STATE) {
            cmd_out(ctx, "no osc target. try: osc <ip> <port>");
            return CMD_ERROR;
        }
        snprintf(ctx->msg, sizeof ctx->msg, "sent a %d-byte frame", n);
        return (ve == ESP_OK) ? CMD_DONE : CMD_ERROR;
    }
    const int want = (ctx->arg[0] != '\0') ? doc_buf_find(ctx->arg)
                                           : doc_buf_current();
    if (want < 0) {
        cmd_out(ctx, "no document called '%s'", ctx->arg);
        return CMD_ERROR;
    }
    const int was = doc_buf_current();
    if (want != was && doc_buf_select(want) != ESP_OK) {
        return CMD_ERROR;
    }
    static char text[1024];
    size_t n = doc_len();
    if (n > sizeof text - 1) {
        n = sizeof text - 1;
    }
    doc_read(text, n);
    text[n] = '\0';
    if (want != was) {
        doc_buf_select(was);
    }

    const esp_err_t e = net_osc_frame(text);
    if (e == ESP_ERR_INVALID_STATE) {
        cmd_out(ctx, "no osc target. try: osc <ip> <port>");
        return CMD_ERROR;
    }
    if (e != ESP_OK) {
        cmd_out(ctx, "frame too large or send failed");
        return CMD_ERROR;
    }
    snprintf(ctx->msg, sizeof ctx->msg, "sent %u bytes as a frame",
             (unsigned)n);
    return CMD_DONE;
}

/* '>ssh user@host <command>'. THE PASSWORD IS ASKED FOR, NEVER READ FROM THE
 * LINE: the line is a document line, and ground rule 6 keeps secrets out of
 * documents. What the command needs to remember until the answer comes is kept
 * here; the answer itself goes straight to ssh_start and is wiped. */
static struct {
    char user[33], host[64], cmd[128];
    int  port;
} s_ssh;

static void ssh_answer(const char *secret, char *msg, size_t max)
{
    /* THE OLD HABIT, CAUGHT. The syntax was '>ssh me@host hunter2 ls'; typed
     * now, the old password would be sent to the host as part of the command,
     * and it is already in the document. So if the answer appears in the
     * command, nothing is sent at all. */
    if (secret[0] != '\0' && strstr(s_ssh.cmd, secret) != NULL) {
        snprintf(msg, max, "that password is on the line");
        return;
    }
    const esp_err_t e = ssh_start(s_ssh.user, s_ssh.host, s_ssh.port, secret,
                                  s_ssh.cmd);
    snprintf(msg, max, "%s", e == ESP_OK ? "ssh: connecting" :
                             e == ESP_ERR_INVALID_STATE ? "one ssh at a time" :
                                                          "ssh could not start");
}

/* user@host[:port] into s_ssh. False when it is not that shape. */
static bool ssh_address(const char *who)
{
    const char *at = strchr(who, '@');
    if (at == NULL || at == who || at[1] == '\0') {
        return false;
    }
    snprintf(s_ssh.user, sizeof s_ssh.user, "%.*s", (int)(at - who), who);
    snprintf(s_ssh.host, sizeof s_ssh.host, "%s", at + 1);
    s_ssh.port = 22;
    char *colon = strchr(s_ssh.host, ':');
    if (colon != NULL) {
        *colon = '\0';
        long pn;
        if (!whole_number(colon + 1, 1, 65535, &pn)) {
            return false;
        }
        s_ssh.port = (int)pn;
    }
    return true;
}

static cmd_status_t c_ssh(cmd_ctx_t *ctx)
{
    if (ctx->arg[0] == '\0') {
        cmd_out(ctx, "ssh user@host <command>");
        cmd_out(ctx, "asks for the password: it");
        cmd_out(ctx, "never goes on the line.");
        cmd_out(ctx, "the reply lands in +out.");
        cmd_out(ctx, "a host's key is kept the");
        cmd_out(ctx, "first time, and a changed");
        cmd_out(ctx, "key is refused before any");
        cmd_out(ctx, "password is sent. if you");
        cmd_out(ctx, "changed it: ssh forget <host>");
        snprintf(ctx->msg, sizeof ctx->msg, "ssh user@host ls");
        return CMD_DONE;
    }

    char word[80];
    size_t wl = 0;
    const int rest = first_word_rest(ctx->arg, word, sizeof word, &wl);
    if (strcmp(word, "forget") == 0) {
        char host[64];
        size_t hl = 0;
        if (rest < 0 ||
            first_word_rest(ctx->arg + rest, host, sizeof host, &hl) >= 0 ||
            hl == 0 || hl >= sizeof host) {
            snprintf(ctx->msg, sizeof ctx->msg, "ssh forget <host>");
            return CMD_ERROR;
        }
        int port = 22;
        char *colon = strchr(host, ':');
        if (colon != NULL) {
            *colon = '\0';
            long pn;
            if (!whole_number(colon + 1, 1, 65535, &pn)) {
                snprintf(ctx->msg, sizeof ctx->msg, "a port is 1-65535");
                return CMD_ERROR;
            }
            port = (int)pn;
        }
        const esp_err_t e = ssh_forget(host, port);
        snprintf(ctx->msg, sizeof ctx->msg, "%s %.20s",
                 e == ESP_OK ? "key forgotten:" : "no key kept for", host);
        return CMD_DONE;
    }

    if (!net_up()) {
        cmd_out(ctx, "no network. try: wifi <ssid>");
        snprintf(ctx->msg, sizeof ctx->msg, "ssh needs wifi");
        return CMD_ERROR;
    }
    if (ssh_busy()) {
        snprintf(ctx->msg, sizeof ctx->msg, "one ssh at a time");
        return CMD_ERROR;
    }
    if (rest < 0 || wl >= sizeof word || !ssh_address(word)) {
        snprintf(ctx->msg, sizeof ctx->msg, "ssh user@host <command>");
        return CMD_ERROR;
    }
    const char *cmd = ctx->arg + rest;
    while (*cmd == ' ') {
        cmd++;
    }
    snprintf(s_ssh.cmd, sizeof s_ssh.cmd, "%s", cmd);

    char q[40];
    snprintf(q, sizeof q, "%.24s password", s_ssh.host);
    if (!cmd_ask_secret(q, ssh_answer)) {
        snprintf(ctx->msg, sizeof ctx->msg, "nothing here can ask");
        return CMD_ERROR;
    }
    snprintf(ctx->msg, sizeof ctx->msg, "type it, Enter. Esc stops");
    return CMD_PENDING;
}

/* '>wifi <ssid>' and '>host <ssid>'. ONE WORD, AND THE PASSWORD IS ASKED FOR.
 * Anything after the name is what used to be the password, so it is treated as
 * one: refused, and cut from the line before autosave can keep it (secret_at).
 * The answer goes to the radio, which keeps it in NVS - never a document. */
static char s_net_ssid[33];

static void wifi_answer(const char *secret, char *msg, size_t max)
{
    if (net_join(s_net_ssid, secret) != ESP_OK) {
        snprintf(msg, max, "could not start the radio");
        return;
    }
    snprintf(msg, max, "joining %.20s", s_net_ssid);
}

static void host_answer(const char *secret, char *msg, size_t max)
{
    if (net_host(s_net_ssid, secret) != ESP_OK) {
        snprintf(msg, max, "could not start the radio");
        return;
    }
    /* net_host hosts OPEN below eight characters; say so here too. */
    snprintf(msg, max, strlen(secret) >= 8 ? "hosting %.12s 192.168.4.1"
                                           : "hosting %.12s OPEN", s_net_ssid);
}

/* The one word, or a refusal. Returns false with ctx->msg set. */
static bool net_name(cmd_ctx_t *ctx, const char *usage)
{
    size_t n = 0;
    const int rest = first_word_rest(ctx->arg, s_net_ssid, sizeof s_net_ssid, &n);
    if (rest >= 0) {
        ctx->secret_at = rest;
        snprintf(ctx->msg, sizeof ctx->msg, "no passwords on a line - cut");
        return false;
    }
    if (n == 0 || n >= sizeof s_net_ssid) {
        snprintf(ctx->msg, sizeof ctx->msg, "%s", usage);
        return false;
    }
    return true;
}

static cmd_status_t ask_for(cmd_ctx_t *ctx, cmd_secret_fn fn)
{
    char q[40];
    snprintf(q, sizeof q, "%.24s password", s_net_ssid);
    if (!cmd_ask_secret(q, fn)) {
        snprintf(ctx->msg, sizeof ctx->msg, "nothing here can ask");
        return CMD_ERROR;
    }
    snprintf(ctx->msg, sizeof ctx->msg, "type it, Enter. Esc stops");
    return CMD_PENDING;
}

static cmd_status_t c_wifi(cmd_ctx_t *ctx)
{
    char st[40];
    if (ctx->arg[0] == '\0') {
        net_status(st, sizeof st);
        cmd_out(ctx, "%s", st);
        char kn[34];
        net_remembered(kn, sizeof kn);
        if (kn[0] != '\0') {
            cmd_out(ctx, "remembered: %.17s", kn);
        }
        cmd_out(ctx, "wifi <ssid> - then it asks");
        cmd_out(ctx, "for the password.");
        cmd_out(ctx, "wifi off | wifi forget");
        snprintf(ctx->msg, sizeof ctx->msg, "%s", st);
        return CMD_DONE;
    }
    if (strcmp(ctx->arg, "forget") == 0) {
        net_forget();
        snprintf(ctx->msg, sizeof ctx->msg, "network forgotten");
        return CMD_DONE;
    }
    if (strcmp(ctx->arg, "off") == 0) {
        net_stop();
        snprintf(ctx->msg, sizeof ctx->msg, "wifi off");
        return CMD_DONE;
    }
    if (!net_name(ctx, "wifi <ssid>")) {
        return CMD_ERROR;
    }
    return ask_for(ctx, wifi_answer);
}

static cmd_status_t c_host(cmd_ctx_t *ctx)
{
    if (ctx->arg[0] == '\0') {
        cmd_out(ctx, "host <ssid> - then it asks");
        cmd_out(ctx, "for a password. under 8");
        cmd_out(ctx, "chars, or none, and it hosts");
        cmd_out(ctx, "open, and says so.");
        snprintf(ctx->msg, sizeof ctx->msg, "host <ssid>");
        return CMD_DONE;
    }
    if (!net_name(ctx, "host <ssid>")) {
        return CMD_ERROR;
    }
    return ask_for(ctx, host_answer);
}

static cmd_status_t c_osc(cmd_ctx_t *ctx)
{
    if (ctx->arg[0] == '\0') {
        uint32_t m = 0, p = 0;
        net_osc_counts(&m, &p);
        cmd_out(ctx, "osc <ip> <port>");
        cmd_out(ctx, "sends /deck/<lane> i i");
        cmd_out(ctx, "%u msgs in %u packets", (unsigned)m, (unsigned)p);
        snprintf(ctx->msg, sizeof ctx->msg, "osc 192.168.4.2 9000");
        return CMD_DONE;
    }
    char ip[24], port[8];
    two_words(ctx->arg, ip, sizeof ip, port, sizeof port);
    long pn = 9000;
    if (port[0] != '\0' && !whole_number(port, 1, 65535, &pn)) {
        cmd_out(ctx, "a port is 1-65535");
        return CMD_ERROR;
    }
    if (net_osc_target(ip, (int)pn) != ESP_OK) {
        cmd_out(ctx, "'%s' is not an address", ip);
        return CMD_ERROR;
    }
    seq_dest_enable("osc", pn > 0);
    snprintf(ctx->msg, sizeof ctx->msg, "osc -> %.15s:%ld", ip, pn);
    return CMD_DONE;
}

static cmd_status_t c_send(cmd_ctx_t *ctx)
{
    if (ctx->arg[0] == '\0') {
        /* TWO SHORT LINES, NOT ONE LONG ONE.
         *
         * This printed name, state and help on one line - 39 characters into
         * a 30-column grid, so every line wrapped mid-word and ran into the
         * next. The owner's report was that these pages are "jumbled in a
         * funky way that's not very legible", and they were right: output
         * that does not fit the screen is output nobody can read, which makes
         * every command that produces it useless as a diagnostic. */
        for (int i = 0; i < seq_dest_count(); i++) {
            cmd_out(ctx, "%-4s %s", seq_dest_name(i),
                    seq_dest_on(i) ? "ON" : "off");
            cmd_out(ctx, "     %.25s", seq_dest_help(i));
        }
        /* The USB MIDI chain, in one line, because "no notes arrive" has four
         * possible causes and they need different fixes. */
        char st[48];
        usbdev_status(st, sizeof st);
        cmd_out(ctx, "usb: %s", st);
        cmd_out(ctx, "act=mode dev=host midi=bound");
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
    /* THE VIEW NODE TAKES A SIZE AS WELL: '>send view 53x20' is on, at that
     * many cells. 'on' alone is 53x20 - the whole 640x480 screen in the deck's
     * own 12x24 face - and 'off' gives the size back to the preview pane. The
     * pane keeps its own shape throughout and shows a sample of the output. */
    int vw = 0, vh = 0;
    const bool view = (strcmp(name, "view") == 0);
    if (view && sscanf(state, "%dx%d", &vw, &vh) == 2) {
        if (vw < 4 || vh < 2 || vw > VIZ_W || vh > VIZ_H) {
            cmd_out(ctx, "view is 4x2 to %dx%d cells", VIZ_W, VIZ_H);
            return CMD_ERROR;
        }
        snprintf(state, sizeof state, "on");
    }
    const bool on = strcmp(state, "on") == 0;
    if (!on && strcmp(state, "off") != 0) {
        cmd_out(ctx, view ? "send view on | off | 53x20" : "send <name> on | off");
        return CMD_ERROR;
    }
    if (view) {
        if (!on) {
            viz_out_size(0, 0);
        } else {
            viz_out_size(vw ? vw : 53, vh ? vh : 20);
            viz_split(true);
        }
        tg_invalidate();
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
    /* A TEMPO OR NOTHING. This was atoi(), so '>bpm fast' - or a stray
     * character, or a definition that reached here by mistake - was 0, clamped
     * to 20: the set slowed to a crawl and the deck said "20 bpm" as though it
     * had been asked to. */
    if (ctx->arg[0] != '\0') {
        long v = 0;
        if (!whole_number(ctx->arg, 20, 300, &v)) {
            cmd_out(ctx, "bpm is a number, 20-300");
            return CMD_ERROR;
        }
        seq_bpm((int)v);
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
        /* PRINT WHAT WAS TYPED - THE TEXT ITSELF.
         *
         * The listing used to rebuild the pattern from the compiled bits, to
         * prove it had been read. Once a step can be '9%30' or '[0,4,7]', a
         * rebuild prints something the player did not type, which is exactly
         * what docs/COMMANDS.md forbids. And a pattern is now either compiled
         * exactly as written or refused, so the text IS the proof. (The rebuild
         * also shifted a 32-bit 1 across 64 slots, so a long lane listed wrong
         * past its thirty-second slot.)
         *
         * THE BINDING IS A COLUMN. A lane's name no longer tells you where it
         * goes - '>disc' draws and '>kick' sounds, and both are lanes - so the
         * listing has to say. '<- name' means this lane follows that one and
         * ignores its own steps. */
        /* A COUNT SAYS WHERE IT IS - "2/4", "waits", "done" - because a counted
         * lane that is silent may be waiting, finished, or muted, and those are
         * three different things to do about it. A cue shows its pattern AND its
         * source, since both decide what it plays. */
        char at[16] = "";
        const int pass = seq_lane_pass(&l[i]);
        if (pass >= 0) {
            snprintf(at, sizeof at, " %d/%u", pass + 1, (unsigned)l[i].count);
        } else if (pass == SEQ_PASS_WAITS) {
            snprintf(at, sizeof at, " waits");
        } else if (pass == SEQ_PASS_DONE) {
            snprintf(at, sizeof at, " done");
        }
        const char mute = (l[i].muted && !l[i].done) ? '-' : ' ';
        if (l[i].route[0] != '\0' && l[i].count == 0) {
            cmd_out(ctx, "%c%-5s <- %s", mute, l[i].name, l[i].route);
        } else if (l[i].route[0] != '\0') {
            cmd_out(ctx, "%c%-5s %s <- %s%s", mute, l[i].name, l[i].text,
                    l[i].route, at);
        } else {
            cmd_out(ctx, "%c%-5s %s%s", mute, l[i].name, l[i].text, at);
        }
    }
    /* THIRTY COLUMNS. This was "%d bpm  swing %d  key %s  clock %s", which
     * renders as 38 characters and wraps - on the one screen the player looks
     * at to check their state mid-performance. */
    cmd_out(ctx, "%d %s sw%d%s", seq_get_bpm(), seq_scale_name(),
            seq_get_swing(), seq_get_sync() ? " clk" : "");
    char dests[64] = {0};
    for (int i = 0; i < seq_dest_count(); i++) {
        if (seq_dest_on(i)) {
            strncat(dests, seq_dest_name(i), sizeof dests - strlen(dests) - 2);
            strncat(dests, " ", sizeof dests - strlen(dests) - 1);
        }
    }
    /* ONE LISTING, because there is one table. This used to print the music
     * lanes and then walk a second table for the visual ones, with a rule
     * between them - two loops describing two structs that were the same
     * struct. What a lane is BOUND to is one column now. */
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

/* '>mute hat bass' and '>solo kick' - the two moves a performer makes
 * constantly and which, until now, cost retyping a whole pattern line to
 * silence it. Several names at once, because muting one thing at a time is
 * not how anyone plays.
 *
 * '>mute' with no argument unmutes EVERYTHING - the way back from any state,
 * and the same shape as '>panic' for notes. '>solo' with no argument does the
 * same, so the two keys are safe to hit blind. */
static bool lane_named(const char *arg, const char *name)
{
    const size_t n = strlen(name);
    for (const char *p = arg; *p != '\0'; ) {
        while (*p == ' ') { p++; }
        const char *s = p;
        while (*p != '\0' && *p != ' ') { p++; }
        if ((size_t)(p - s) == n && strncmp(s, name, n) == 0) {
            return true;
        }
    }
    return false;
}

static cmd_status_t c_mute(cmd_ctx_t *ctx)
{
    const bool solo  = (strcmp(ctx->name, "solo") == 0);
    const bool clear = (ctx->arg[0] == '\0');
    int n = 0, touched = 0;
    const seq_lane_t *l = seq_lanes(&n);
    for (int i = 0; i < SEQ_MAX_LANES; i++) {
        if (!l[i].used) {
            continue;
        }
        const bool named = !clear && lane_named(ctx->arg, l[i].name);
        /* solo: everything not named goes quiet. mute: everything named does.
         * With no argument both mean "everything back on". */
        const bool want_mute = clear ? false : (solo ? !named : named);
        if (want_mute != l[i].muted) {
            seq_mute(l[i].name, want_mute);
            touched++;
        }
    }
    snprintf(ctx->msg, sizeof ctx->msg, "%s: %d lane%s changed",
             clear ? "all on" : ctx->name, touched, touched == 1 ? "" : "s");
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
    { "sync",  c_sync,  CMD_CAP_EDIT,  "on | off | lead | follow | alone" },
    { "send",  c_send,  CMD_CAP_SYSTEM,"where events go; send mon on" },
    { "wifi",  c_wifi,  CMD_CAP_NET,   "wifi <ssid> | off - asks the pass" },
    { "battery", c_battery, CMD_CAP_READ, "find the sense pin" },
    { "kbd",   c_kbd,   CMD_CAP_SYSTEM,"what is typing | kbd forget" },
    { "host",  c_host,  CMD_CAP_NET,   "host <ssid> - be the net" },
    { "osc",   c_osc,   CMD_CAP_NET,   "osc <ip> <port> - /deck/<lane>" },
    { "ssh",   c_ssh,   CMD_CAP_NET,   "ssh user@host <command>" },
    { "frame", c_frame, CMD_CAP_NET,   "send the frame over osc" },
    { "split", c_split, CMD_CAP_EDIT,  "split on | off | <rows>" },
    { "route", c_route, CMD_CAP_EDIT,  "route disc kick" },
    { "usb",   c_usb,   CMD_CAP_SYSTEM,"usb on | off - MIDI over the cable" },
    { "din",   c_din,   CMD_CAP_SYSTEM,"din <gpio> - MIDI with no host" },
    { "flash", c_flash, CMD_CAP_SYSTEM,"flash now - reboot to ROM loader" },
    { "dump",  c_dump,  CMD_CAP_READ,  "print a document to the console" },
    { "play",  c_play,  CMD_CAP_EDIT,  "start the clock" },
    { "stop",  c_stop,  CMD_CAP_EDIT,  "stop the clock" },
    { "lanes", c_lanes, CMD_CAP_READ,  "what is playing" },
    { "jitter",c_jitter,CMD_CAP_READ,  "timing, measured in us" },
    { "panic", c_panic, CMD_CAP_EDIT,  "silence everything" },
    { "mute",  c_mute,  CMD_CAP_EDIT,  "mute hat bass | mute = all on" },
    { "solo",  c_mute,  CMD_CAP_EDIT,  "solo kick | solo = all on" },
    { "help",  c_help,  CMD_CAP_READ,                   "list the commands" },
    { "list",  c_list,  CMD_CAP_READ,                   "list open buffers" },
    { "new",   c_new,   CMD_CAP_EDIT,                   "a fresh scratch buffer" },
    { "name",  c_name,  CMD_CAP_EDIT | CMD_CAP_STORE,   "file this buffer under a name" },
    { "open",  c_open,  CMD_CAP_READ,                   "switch to a named document" },
    { "run",   c_run,   CMD_CAP_EDIT,                   "run a document without leaving this one" },
    { "save",  c_save,  CMD_CAP_STORE,                  "write this buffer now" },
    { "close", c_close, CMD_CAP_EDIT,                   "forget this buffer" },
    { "density", c_density, CMD_CAP_EDIT,               "low | high (use high to split)" },
};

void cmd_register(const cmd_t *table, int count);

void cmd_init(void)
{
    cmd_register(s_builtins, (int)(sizeof s_builtins / sizeof s_builtins[0]));
}
