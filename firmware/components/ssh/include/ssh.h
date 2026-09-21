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

/* Run one command on `host` as `user` and put its output in a document named
 * '+ssh'. Transient, so it is never journalled and never reaches the SD
 * mirror - a command's output is not the owner's writing, and a password
 * prompt or a directory listing has no business in the corpus that gets copied
 * to their DGX.
 *
 * Blocking, and called from the editor task rather than any timer - see the
 * note in ssh.c about why that matters on this device. */
esp_err_t ssh_run(const char *user, const char *host, int port,
                  const char *pass, const char *cmd);

/* One line for the status bar. */
void ssh_status(char *out, size_t max);
