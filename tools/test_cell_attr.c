/*
 * How a cell composes the cursor and the playhead.
 *
 * This existed as an expression written twice - once in editor_draw, once in
 * editor_blink - and the blink copy dropped the playhead bit. So every time
 * the cursor blinked on a cell the playhead was passing through, the bar was
 * erased from that cell. The owner reported losing their cursor while editing
 * a running lane, which is exactly and only the cell where the two meet.
 *
 * Each line below fails against `on ? TG_INVERSE : TG_NORMAL`, the expression
 * that used to be in the blink.
 */
#include <stdio.h>

#include "cell_attr.h"

static int fails;

static void eq(const char *what, int got, int want)
{
    if (got != want) {
        printf("[FAIL] %s: got %d want %d\n", what, got, want);
        fails++;
    }
}

int main(void)
{
    eq("plain",               cell_attr(false, false, false), TG_NORMAL);
    eq("cursor only",         cell_attr(true,  false, false), TG_INVERSE);
    eq("playhead only",       cell_attr(false, true,  false), TG_UNDER);
    eq("command word only",   cell_attr(false, false, true),  TG_OVER);
    /* THE ONE THAT WAS BROKEN. */
    eq("cursor on playhead",  cell_attr(true,  true,  false), TG_INVERSE | TG_UNDER);
    /* AND THE ONE THE OWNER REPORTED: a cursor on a recognised command word.
     * When the word was TG_INVERSE too, the cursor XORed against it and the
     * cell went back to normal - lost. Three separate bits cannot cancel. */
    eq("cursor on command",   cell_attr(true,  false, true),  TG_INVERSE | TG_OVER);
    eq("all three",           cell_attr(true,  true,  true),
       TG_INVERSE | TG_UNDER | TG_OVER);

    /* The bits must be independent, or one treatment can never be layered on
     * the other and the panel has no way to show both. */
    if ((TG_INVERSE & TG_UNDER) || (TG_INVERSE & TG_OVER) ||
        (TG_UNDER & TG_OVER)) {
        printf("[FAIL] the attribute bits overlap; they cannot compose\n");
        fails++;
    }
    if (TG_NORMAL != 0) {
        printf("[FAIL] TG_NORMAL must be zero for the mask to work\n");
        fails++;
    }

    printf(fails ? "[FAIL] %d composition(s) wrong\n"
                 : "[PASS] cursor and playhead compose\n", fails);
    return fails != 0;
}
