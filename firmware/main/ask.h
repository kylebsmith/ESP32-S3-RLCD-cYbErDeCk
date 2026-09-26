/*
 * Asking for a secret without a document ever seeing it.
 *
 * docs/NEXT.md ground rule 6: nothing secret ever enters a document. Documents
 * are journalled, mirrored to the SD card and copied to the owner's DGX - and a
 * command line IS a document line. '>wifi home hunter2' put the password into
 * the journal and onto the card the moment autosave ran, and '>ssh' and '>host'
 * did the same. No command takes a password on its line any more.
 *
 * Instead a command that needs one asks, on the status line. The next keys go
 * HERE, not to the document: shown as one star a character, handed to the
 * command on Enter, and wiped - on Enter, on Esc, and before every new question.
 * Pure, so tools/test_ask.c runs this exact code.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "kbd.h"

/* WPA2 passphrases are 8 to 63 characters; ssh passwords fit too. */
#define ASK_MAX 63

typedef struct {
    bool active;
    char label[40];
    char buf[ASK_MAX + 1];
    int  len;
} ask_t;

enum { ASK_KEEP = 0, ASK_SUBMIT, ASK_CANCEL };

/* Through a volatile pointer, so the compiler cannot decide a buffer nobody
 * reads again is not worth clearing. */
static inline void ask_wipe(ask_t *a)
{
    volatile char *p = a->buf;
    for (size_t i = 0; i < sizeof a->buf; i++) {
        p[i] = '\0';
    }
    a->len = 0;
}

static inline void ask_start(ask_t *a, const char *label)
{
    ask_wipe(a);
    snprintf(a->label, sizeof a->label, "%s", label);
    a->active = true;
}

static inline void ask_end(ask_t *a)
{
    ask_wipe(a);
    a->active = false;
}

/* One key. Enter, with or without a modifier, gives the secret to the command;
 * Esc, Ctrl-C and Ctrl-G take it back. Every other chord does nothing: Ctrl-O
 * must not switch documents under a half-typed password. */
static inline int ask_feed(ask_t *a, const kbd_event_t *ev)
{
    switch (ev->type) {
    case KBD_EV_ENTER:
        return ASK_SUBMIT;
    case KBD_EV_ESC:
        return ASK_CANCEL;
    case KBD_EV_BACKSPACE:
        if (a->len > 0) {
            a->buf[--a->len] = '\0';
        }
        return ASK_KEEP;
    case KBD_EV_CHAR:
        if (ev->mods & KBD_COMMAND_MODS) {
            const char c = (char)(ev->ch | 0x20);
            return (c == 'c' || c == 'g') ? ASK_CANCEL : ASK_KEEP;
        }
        if (a->len < ASK_MAX && ev->ch >= 32 && ev->ch < 127) {
            a->buf[a->len++] = ev->ch;
            a->buf[a->len] = '\0';
        }
        return ASK_KEEP;
    default:
        return ASK_KEEP;
    }
}

/* What the status line shows: the question, and a star for each character -
 * never a character. */
static inline void ask_render(const ask_t *a, char *out, size_t max)
{
    if (max == 0) {
        return;
    }
    int k = snprintf(out, max, "%s: ", a->label);
    if (k < 0) {
        out[0] = '\0';
        return;
    }
    for (int i = 0; i < a->len && (size_t)k + 1 < max; i++) {
        out[k++] = '*';
    }
    out[(size_t)k < max ? (size_t)k : max - 1] = '\0';
}
