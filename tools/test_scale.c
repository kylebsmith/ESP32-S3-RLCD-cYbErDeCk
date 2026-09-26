/*
 * The key, checked on the host against the shipping header.
 *
 * seq_scale.h is the whole pitch system: a table of modes, a parser for
 * '>scale', and a degree turned into a note. The parser had one wrong turn -
 * a 'b' after the root was always a flat, so "cblues" was C-flat plus "lues"
 * and was refused, and blues after a natural root could not be typed at all.
 * The checks marked (old) fail on that parser.
 */
#include <stdio.h>
#include <string.h>

#include "seq_scale.h"

static int fails;

static void parses(const char *spec, int root, const char *mode)
{
    int r = -1;
    const seq_mode_t *m = NULL;
    if (seq_scale_parse(spec, &r, &m) != 0 || r != root || m == NULL ||
        strcmp(m->name, mode) != 0) {
        printf("[FAIL] '%s' should be root %d %s, got %d %s\n", spec, root, mode,
               r, m ? m->name : "(refused)");
        fails++;
    } else {
        printf("[ ok ] '%s' is root %d, %s\n", spec, root, mode);
    }
}

static void refused(const char *spec)
{
    int r = -1;
    const seq_mode_t *m = NULL;
    if (seq_scale_parse(spec, &r, &m) == 0) {
        printf("[FAIL] '%s' should be refused, read as %d %s\n", spec, r, m->name);
        fails++;
    } else {
        printf("[ ok ] '%s' is refused\n", spec);
    }
}

static void note_is(const char *spec, int deg, int oct, int want)
{
    int r = 0;
    const seq_mode_t *m = NULL;
    seq_scale_parse(spec, &r, &m);
    const int got = m ? seq_degree_note(r, m, (uint8_t)deg, oct) : -1;
    if (got != want) {
        printf("[FAIL] %s degree %d octave %d: got %d want %d\n", spec, deg, oct,
               got, want);
        fails++;
    }
}

int main(void)
{
    /* The table: every mode starts on its root and climbs inside one octave. */
    for (int i = 0; i < SEQ_MODE_COUNT; i++) {
        const seq_mode_t *m = &seq_modes[i];
        int ok = m->n >= 5 && m->n <= 12 && m->iv[0] == 0;
        for (int k = 1; k < m->n && ok; k++) {
            ok = m->iv[k] > m->iv[k - 1] && m->iv[k] < 12;
        }
        if (!ok) {
            printf("[FAIL] mode %s is not a scale\n", m->name);
            fails++;
        }
    }
    if (strcmp(seq_modes[SEQ_MODE_MIN].name, "min") != 0) {
        printf("[FAIL] a bare root must mean minor\n");
        fails++;
    }

    /* What the verbs page and the guide teach. */
    parses("dmin", 2, "min");
    parses("c", 0, "min");
    parses("f#mix", 6, "mix");
    parses("apent", 9, "pent");
    parses("ebblues", 3, "blues");
    parses("fmin", 5, "min");
    parses("Dmin", 2, "min");
    parses("bb", 10, "min");
    parses("bbmaj", 10, "maj");
    parses("ebmaj", 3, "maj");
    parses("gbmaj5", 6, "maj5");
    parses("cmaj5", 0, "maj5");
    parses("cmaj", 0, "maj");
    parses("ddorian", 2, "dor");

    /* (old) Blues after a natural root. */
    parses("cblues", 0, "blues");
    parses("eblues", 4, "blues");
    parses("gblues", 7, "blues");
    parses("bblues", 11, "blues");
    /* ...and a flat before it still works. */
    parses("abblues", 8, "blues");
    parses("cbblues", 11, "blues");

    refused("");
    refused("h");
    refused("dfoo");
    refused("c#x");
    refused("#c");
    refused("d min");

    /* A degree is a note: D minor from octave 2, the bass's register. */
    note_is("dmin", 0, 2, 38);
    note_is("dmin", 2, 2, 41);
    note_is("dmin", 7, 2, 50);          /* past the top: the next octave */
    note_is("dmin", 9, 2, 53);
    note_is("cpent", 5, 3, 60);         /* five notes, so 5 is the octave */
    note_is("ddor", 5, 3, 59);          /* the raised sixth: B, not B-flat */
    note_is("dphr", 1, 3, 51);          /* the flat second: E-flat */
    note_is("dlyd", 3, 3, 56);          /* the raised fourth: G-sharp */
    note_is("gmaj", 9, 8, 127);         /* clamped, never past MIDI */

    if (fails) {
        printf("[FAIL] %d check(s) failed\n", fails);
    } else {
        printf("[PASS] the key: table, parser and degrees\n");
    }
    return fails != 0;
}
