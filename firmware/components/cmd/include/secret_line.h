/*
 * No password on a line - the parsing half. firmware/main/ask.h is the asking
 * half, and cmd.h's cmd_ask_secret() joins them.
 *
 * '>wifi <ssid>' and '>host <ssid>' take ONE word now. What used to follow it
 * was the password, so anything that still does is treated as one: the command
 * is refused, and the line is cut from the end of the word (cmd.h, secret_at)
 * before autosave can write it anywhere. Pure, so tools/test_ask.c and
 * tools/test_ui_text.c run this exact code.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/* The first word of `arg` into `word` (cut to fit), its whole length into
 * *len. Returns the offset in `arg` just past the word when anything follows
 * it, or -1 when the word is all there is. */
static inline int first_word_rest(const char *arg, char *word, size_t max,
                                  size_t *len)
{
    size_t i = 0, k = 0;
    while (arg[i] == ' ') {
        i++;
    }
    const size_t start = i;
    while (arg[i] != '\0' && arg[i] != ' ') {
        if (k + 1 < max) {
            word[k++] = arg[i];
        }
        i++;
    }
    if (max > 0) {
        word[k] = '\0';
    }
    if (len != NULL) {
        *len = i - start;
    }
    size_t j = i;
    while (arg[j] == ' ') {
        j++;
    }
    return arg[j] != '\0' ? (int)i : -1;
}

/* '<HomeNet>' IS 'HomeNet'. The help writes '>wifi <ssid>' and the brackets
 * were typed: the deck then looked for a network called "<HomeNet>" and
 * retried for ever (2026-09-25). A name wrapped in them is unwrapped, and the
 * caller says so. Returns whether it was. */
static inline bool unbracket(char *w)
{
    const size_t n = strlen(w);
    if (n >= 3 && w[0] == '<' && w[n - 1] == '>') {
        memmove(w, w + 1, n - 2);
        w[n - 2] = '\0';
        return true;
    }
    return false;
}

