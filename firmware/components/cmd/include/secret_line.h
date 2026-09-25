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

#include <stddef.h>

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
