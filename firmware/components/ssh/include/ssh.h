/*
 * An SSH client, on the deck.
 *
 * WHAT THIS IS FOR, in the owner's words: "controlling claude code on my
 * laptop via the device and getting the terminal back". So the useful unit is
 * not an interactive terminal emulator - it is RUN A COMMAND, GET THE OUTPUT
 * AS A DOCUMENT. That fits the substrate exactly: the answer arrives as text
 * in a buffer, editable, searchable, and journalled like everything else, and
 * it needs no terminal emulation, no cursor addressing and no PTY.
 *
 * An interactive session is the harder, later thing. This is the slice that is
 * genuinely useful on the first day.
 *
 * WHY libssh2 AND NOT A LITTLE AGENT ON THE LAPTOP. An agent would be twenty
 * lines each side and would work today - but it would be another thing to
 * install, keep running, and trust on a machine the owner uses for work. SSH
 * is already there, already authenticated, already audited.
 *
 * WHERE THE SESSION LIVES. libssh2's session is around 80 KB; on this part
 * that belongs in PSRAM, not in the 512 KB of internal SRAM the sequencer and
 * the radio are competing for. The socket and the crypto scratch are small.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

/* Run one command on `host` as `user`, in a task of its own, and put what it
 * says into '+out' - transient, so it is never journalled and never reaches the
 * SD mirror: a command's output is not the owner's writing, and a directory
 * listing has no business in the corpus that gets copied to their DGX.
 *
 * Returns at once. ESP_ERR_INVALID_STATE while a session is already running.
 * `pass` is never read from a document: it comes from the deck asking for it
 * (firmware/main/ask.h). It is copied, used once, and wiped; the caller wipes
 * its own copy. */
esp_err_t ssh_start(const char *user, const char *host, int port,
                    const char *pass, const char *cmd);

/* From the editor's loop, every pass: moves what the session has said into
 * '+out' - docstore belongs to that task. SSH_SAID when lines arrived,
 * SSH_FINISHED once when the session has ended and everything it said is in;
 * *reply_from is where this session's lines begin in '+out'. */
enum { SSH_QUIET = 0, SSH_SAID, SSH_FINISHED };
int  ssh_service(size_t *reply_from);
bool ssh_busy(void);

/* One line for the status bar. */
void ssh_status(char *out, size_t max);

/* Forget the key kept for `host` (port 0 = 22), so the next session keeps
 * whatever key it shows - for when the owner changed it themselves. */
esp_err_t ssh_forget(const char *host, int port);
