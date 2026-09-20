/*
 * The document: a gap buffer that survives power loss.
 *
 * Storage is an append-only journal of whole-buffer snapshots in a raw flash
 * partition. Each record is CRC-checked, so a write torn by a power cut costs
 * the newest snapshot and never the document - the previous record is still
 * intact and still the newest one that validates.
 *
 * docs/HANDOFF.md trap 6: the SD card is an export medium, never the source of
 * truth. Losing power during a FAT write can corrupt the directory, not merely
 * truncate a record. So the journal is the truth and the card is a mirror,
 * written .tmp-then-rename, on newline or a 1 s timer, never per keystroke.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#define DOC_CAPACITY (128 * 1024)
#define DOC_MAX_BUFFERS 8
#define DOC_NAME_MAX    24

/* ---- buffers --------------------------------------------------------------
 *
 * The document singleton is gone. Four things the deck needs are the SAME
 * primitive and could not exist while there was only one buffer: the archive
 * listing beside the file it opens, a scratch buffer beside a named one, a
 * command's output sink, and later one session buffer per remote host.
 *
 * The save model is SCRATCH BY DEFAULT, SAVE PROMOTES. Every buffer is
 * journalled and therefore crash-safe from the first keystroke, named or not.
 * Naming a buffer is what files it in the archive - it is not what makes it
 * durable. So "I did not want to save this" never costs data, and the archive
 * only ever contains things deliberately put there.
 *
 * A buffer with an empty name is scratch. There is always at least one.
 */

/* A buffer's KIND is one bit of interpretation over the same bytes
 * (docs/SUBSTRATE.md): it decides only what Enter does.
 *
 *   prose  Enter splits the line and what follows reflows
 *   guide  Enter EXECUTES the line under the cursor
 *
 * That is what makes a menu a text file: adding a menu item costs typing a
 * line. Commands live in a guide buffer rather than being typed into prose,
 * which is also the only thing that stops a command line accumulating in the
 * middle of somebody's writing. */
typedef enum {
    DOC_KIND_PROSE = 0,
    DOC_KIND_GUIDE = 1,
} doc_kind_t;

doc_kind_t doc_buf_kind(int i);
void       doc_buf_set_kind(doc_kind_t kind);

/* All of the doc_* calls below act on the CURRENT buffer. */
int         doc_buf_count(void);
int         doc_buf_current(void);
const char *doc_buf_name(int i);       /* "" for a scratch buffer */
size_t      doc_buf_len(int i);
bool        doc_buf_is_dirty(int i);

esp_err_t   doc_buf_select(int i);
esp_err_t   doc_buf_new(void);         /* a fresh scratch buffer           */
esp_err_t   doc_buf_rename(const char *name);  /* promote current to filed */
esp_err_t   doc_buf_close(int i);      /* forget it; the journal keeps it  */


/* Load the newest valid snapshot, or start empty. */
esp_err_t doc_init(void);

void   doc_insert(char c);

/* Move the gap so the cursor sits at pos. Used by undo, which has to apply an
 * edit somewhere other than where the cursor happens to be. */
void   doc_move_to(size_t pos);
void   doc_backspace(void);
void   doc_left(void);
void   doc_right(void);

size_t doc_len(void);
size_t doc_cursor(void);
char   doc_at(size_t i);

/* Copy the document out contiguously. Returns bytes written. */
size_t doc_read(char *out, size_t max);

/* True when there are unsaved changes. */
bool   doc_dirty(void);

/* Append a snapshot to the journal. Synchronous; returns once flash has it. */
esp_err_t doc_save(void);

/* Mirror to /sdcard/notes.txt, .tmp-then-rename. Safe to call with no card. */
esp_err_t doc_mirror_sd(void);

/* ---- undo ----------------------------------------------------------------
 * An operation log rather than a snapshot ring: a 128 KB document makes
 * snapshots far too expensive, and consecutive keystrokes coalesce into one
 * step so undo moves by words rather than by letters.
 */
void doc_undo_reset(void);
bool doc_undo(void);          /* false when there is nothing left to undo */
bool doc_redo(void);
int  doc_undo_depth(void);

/* Replace the whole document (used to seed the self-test). */
void doc_set_text(const char *s);

/* --- self-test hooks. Deliberately damaging; only the self-test calls them. */
esp_err_t journal_erase_all(void);
esp_err_t journal_corrupt_newest(void);

/* Diagnostics for the status line and the log. */
uint32_t doc_save_seq(void);
size_t   doc_journal_used(void);
bool     doc_sd_present(void);
