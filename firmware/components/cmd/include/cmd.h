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
    const char  *arg;        /* rest of the line, unparsed, never NULL */
    cmd_caller_t caller;
    char         msg[96];    /* short human-readable result            */
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

const cmd_t *cmd_table(int *count);

/* Register the built-in commands. Called once at start-up. */
void cmd_init(void);
