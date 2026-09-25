/*
 * The command table.
 *
 * docs/SUBSTRATE.md: "every action in the system must be reachable by a name
 * in a table, never only by a keybinding." This is that table, and it is the
 * joint every later feature hangs off - the palette, the guide file, the
 * plumber, and eventually an on-device agent all dispatch through here.
 *
 * THE CALLING CONVENTION, fixed deliberately and early, because it is the one
 * thing that cannot be retrofitted: adding quoting or flags later would change
 * the meaning of guide files already written, and guide files are user data.
 *
 *   - A command line BEGINS WITH '>'. Anything else is prose, so a document
 *     can carry explanation and runnable lines together.
 *   - The command is the first word after the sigil.
 *   - The rest of the line is ONE UNPARSED STRING. Commands that want
 *     structure parse it themselves.
 *   - The selection, when there is one, is the implicit input.
 *   - The return is DONE, or PENDING with a job id.
 *
 * PENDING exists from day one even though nothing uses it yet. It is what
 * keeps a network round-trip or an agent call off the main loop later; if
 * commands were assumed synchronous now, the first SSH command would be
 * written blocking and the run loop would have to be torn up to fix it.
 *
 * CAPABILITIES exist for the same reason. An agent given the command table
 * would otherwise hold exactly the authority of the owner's hands on the
 * device. The caller is tagged, each command declares what it touches, and
 * the dispatcher refuses combinations that were never intended.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    CMD_DONE = 0,
    CMD_PENDING,      /* job started; result arrives later */
    CMD_ERROR,
} cmd_status_t;

/* What a command is allowed to touch. */
#define CMD_CAP_READ    0x01u   /* inspects state, changes nothing     */
#define CMD_CAP_EDIT    0x02u   /* mutates a buffer                    */
#define CMD_CAP_STORE   0x04u   /* writes flash or the card            */
#define CMD_CAP_NET     0x08u   /* reaches off the device              */
#define CMD_CAP_SYSTEM  0x10u   /* changes device state: pairing, power */

/* Who is asking. The owner's hands may do anything; a guide line is still the
 * owner, one step removed; an agent is not the owner and is bounded here
 * rather than by hoping its prompt holds. */
typedef enum {
    CMD_BY_HANDS = 0,
    CMD_BY_GUIDE,
    CMD_BY_AGENT,
} cmd_caller_t;

typedef struct cmd_ctx cmd_ctx_t;
typedef cmd_status_t (*cmd_fn_t)(cmd_ctx_t *ctx);

typedef struct {
    const char *name;
    cmd_fn_t    fn;
    uint32_t    caps;
    const char *help;
} cmd_t;

struct cmd_ctx {
    const char  *name;       /* the command word that matched */
    const char  *arg;        /* rest of the line, unparsed, never NULL */
    cmd_caller_t caller;
    char         msg[96];    /* short human-readable result            */
    /* WHICH CHARACTER IS WRONG, as an offset into `arg`, or -1. Set by a
     * command that refuses its argument because of one character, so the
     * editor can mark that character in the document instead of leaving the
     * performer to count columns against a message. */
    int          err_at;
    /* WHERE A SECRET STARTS, as an offset into `arg`, or -1. Set by a command
     * that found a password on its own line - which no command takes any more
     * (firmware/main/ask.h). The editor deletes the line from there to its end
     * at once, so what was typed there reaches no journal, card or backup that
     * had not already caught it. */
    int          secret_at;
};

/* Run one line. Returns CMD_ERROR for an unknown name or a refused
 * capability; msg_out, when given, receives the result text. */
cmd_status_t cmd_run_line(const char *line, cmd_caller_t caller,
                          char *msg_out, size_t msg_max);

/* Command output. Goes to the log today and to an output buffer when one
 * exists; the call site does not change either way. */
void cmd_out(cmd_ctx_t *ctx, const char *fmt, ...);

/* How many lines the last command wrote. A result worth reading should show
 * itself rather than waiting to be found. */
int cmd_last_output_lines(void);

/* The column, in the line the last command was run from, of the character it
 * refused - or -1. See cmd_ctx.err_at. */
int cmd_last_error_col(void);

/* Is this line a command the table actually knows?
 *
 * The editor asks so it can mark a recognised command differently from
 * ordinary text - which answers two questions at a glance without anything
 * being run: is this line addressed to the machine, and does the machine know
 * the word. An unrecognised command looks like prose, which is exactly what
 * it will behave like.
 *
 * Returns the command, or NULL. On success *word_at and *word_len give the
 * span of the command NAME within the line, so only the word that was
 * recognised is marked. */
const cmd_t *cmd_recognise(const char *line, int *word_at, int *word_len);

const cmd_t *cmd_table(int *count);

/* Register the built-in commands. Called once at start-up. */
/* The column, in the line last run, from which it held a secret; -1 if none. */
int cmd_last_secret_col(void);

/* ASKING FOR A SECRET. Ground rule 6: nothing secret ever enters a document -
 * and a command line is a document line, journalled, mirrored to the card and
 * copied to the owner's DGX. So a command that needs a password never reads it
 * from its line. It asks here and returns; whoever holds the keyboard shows the
 * question, takes the next line typed as stars, and calls `fn` with it - in the
 * task commands run in - then wipes it (firmware/main/ask.h). `fn` writes its
 * result, one line, into msg. False when there is nobody to ask. */
typedef void (*cmd_secret_fn)(const char *secret, char *msg, size_t max);
typedef bool (*cmd_asker_t)(const char *question, cmd_secret_fn fn);
void cmd_set_asker(cmd_asker_t fn);
bool cmd_ask_secret(const char *question, cmd_secret_fn fn);

void cmd_init(void);

/* LANES AND NAMES ARE NOT ROWS OF THE TABLE (docs/MANIFESTO.md §3.8). A lane's
 * first word is an address - 'kick', 'disc:2:x' - whose name is DEFINED
 * ('>kick = note 36') or is a picture's own. The dispatcher asks whether a word
 * is one, and hands the line to the one lane command or to a definition. The
 * grammar is lane_name.h. */
bool         cmd_lane_known(const char *word, size_t n);
cmd_status_t cmd_lane(cmd_ctx_t *ctx, const char *word, size_t n);
cmd_status_t cmd_define(cmd_ctx_t *ctx, const char *word, size_t n);

/* The app supplies this. A component cannot reach into main/editor.h, and
 * should not: '>flash' needs the panel to say what is about to happen before
 * the chip reboots, and only the app knows how to draw. Same shape as the
 * sequencer's destinations - the layer that knows sets the hook. */
typedef void (*cmd_announce_t)(const char *line);
void cmd_set_announce(cmd_announce_t fn);
void cmd_announce(const char *line);
