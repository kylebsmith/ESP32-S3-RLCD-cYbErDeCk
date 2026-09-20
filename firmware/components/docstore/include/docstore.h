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
