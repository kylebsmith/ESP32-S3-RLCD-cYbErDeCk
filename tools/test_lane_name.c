/*
 * A lane's address and a name's definition, checked on the host against the
 * shipping header (firmware/components/cmd/include/lane_name.h).
 *
 * Two jobs. The address must have ONE canonical spelling - 'disc:1' is 'disc' -
 * because the lane table, the playhead and '>route' all look a lane up by it,
 * and two spellings of one lane that reached the table as two keys would be two
 * lanes playing one part. And a definition must refuse anything it cannot mean,
 * with a reason that fits the status bar, because a definition that half-worked
 * would bind a name to the wrong note in every document that uses it.
 */
#include <stdio.h>
#include <string.h>

#include "lane_name.h"

static int fails;

static void addr(const char *w, int err, const char *canon)
{
    lane_name_t o;
    const int e = lane_name_parse(w, strlen(w), &o);
    if (e != err || (err == LN_OK && strcmp(o.canon, canon) != 0)) {
        printf("[FAIL] %-16s got %d '%s', want %d '%s'\n", w, e, o.canon, err,
               canon);
        fails++;
    } else {
        printf("[ ok ] %-16s %s\n", w, err == LN_OK ? o.canon : "refused");
    }
}

static void def(const char *arg, int kind, int num, int chan, int gate)
{
    lane_def_t d;
    lane_def_parse(arg, &d);
    if (d.kind != kind || (kind != LD_ERROR && kind != LD_REMOVE &&
                           kind != LD_DRAW && kind != LD_KNOB && kind != LD_PAD &&
                           (d.num != num || d.chan != chan || d.gate != gate))) {
        printf("[FAIL] = %-22s got kind %d %d ch%d gate%d (%s)\n", arg, d.kind,
               d.num, d.chan, d.gate, d.why);
        fails++;
    } else if (kind == LD_ERROR) {
        printf("[ ok ] = %-22s refused: %s\n", arg, d.why);
        if (strlen(d.why) > 30) {
            printf("[FAIL] that reason is %zu wide\n", strlen(d.why));
            fails++;
        }
    } else {
        printf("[ ok ] = %-22s kind %d\n", arg, d.kind);
    }
}

int main(void)
{
    printf("-- the address --\n");
    addr("disc",          LN_OK, "disc");
    addr("disc:2",        LN_OK, "disc:2");
    addr("disc:x",        LN_OK, "disc:x");
    addr("disc:2:x",      LN_OK, "disc:2:x");
    /* ONE SPELLING PER LANE: the first instance is the plain name. */
    addr("disc:1",        LN_OK, "disc");
    addr("disc:1:x",      LN_OK, "disc:x");
    addr("tom2",          LN_OK, "tom2");     /* a name may end in a digit */
    addr("kick:99",       LN_OK, "kick:99");

    addr("disc:x:2",      LN_ORDER, "");      /* instance before part       */
    addr("disc:2:3",      LN_ORDER, "");
    addr("disc:x:y",      LN_ORDER, "");
    addr("disc:",         LN_ORDER, "");
    addr("disc::x",       LN_ORDER, "");
    addr("disc:0",        LN_INST,  "");
    addr("disc:100",      LN_INST,  "");
    addr("disc:xylophone", LN_PART, "");
    addr("disc:2x",       LN_PART,  "");
    addr("disc[x]",       LN_BRACKET, "");    /* the old spelling, told so  */
    addr("2disc",         LN_BASE,  "");
    addr("Disc",          LN_BASE,  "");
    addr("abcdefghi",     LN_LONG,  "");

    /* The longest address there can be still fits a lane's name. */
    {
        lane_name_t o;
        lane_name_parse("abcdefgh:99:abcde", 17, &o);
        if (strlen(o.canon) + 1 > LANE_NAME_MAX) {
            printf("[FAIL] the longest address does not fit LANE_NAME_MAX\n");
            fails++;
        }
    }

    printf("\n-- the definition --\n");
    /* The defaults are the ones the built-in names always had. */
    def(" note 36",               LD_NOTE, 36, 10, 40);
    def(" note 36 ch 1 gate 90",  LD_NOTE, 36, 1, 90);
    def(" voice 2 ch 1 gate 180", LD_VOICE, 2, 1, 180);
    def(" voice 4",               LD_VOICE, 4, 1, 150);
    def(" cc 74",                 LD_CC,   74, 1, 150);
    def(" cc 74 ch 2",            LD_CC,   74, 2, 150);
    def(" disc",                  LD_DRAW,  0, 0, 0);
    /* Inputs (docs/NEXT.md §5, §8): a name fed from outside. */
    def(" knob",                  LD_KNOB,  0, 0, 0);
    def(" pad",                   LD_PAD,   0, 0, 0);
    def(" knob 3",                LD_ERROR, 0, 0, 0);
    def(" pad ch 2",              LD_ERROR, 0, 0, 0);
    def("",                       LD_REMOVE, 0, 0, 0);
    def("   ",                    LD_REMOVE, 0, 0, 0);

    def(" note",                  LD_ERROR, 0, 0, 0);
    def(" note 128",              LD_ERROR, 0, 0, 0);
    def(" note x",                LD_ERROR, 0, 0, 0);
    def(" voice 9",               LD_ERROR, 0, 0, 0);
    def(" note 36 ch 17",         LD_ERROR, 0, 0, 0);
    def(" note 36 ch 0",          LD_ERROR, 0, 0, 0);
    def(" note 36 gate 0",        LD_ERROR, 0, 0, 0);
    def(" cc 74 gate 10",         LD_ERROR, 0, 0, 0);   /* a cc has no gate */
    def(" note 36 vel 90",        LD_ERROR, 0, 0, 0);
    def(" disc x",                LD_ERROR, 0, 0, 0);
    def(" Disc",                  LD_ERROR, 0, 0, 0);

    /* A sound moves with only what changes: '>bass = ch 5' keeps the octave
     * and gate the name had. Nothing moves that is not a sound. */
    {
        const lane_def_t bass = { .kind = LD_VOICE, .num = 2, .chan = 1, .gate = 180 };
        const lane_def_t cut = { .kind = LD_CC, .num = 74, .chan = 1 };
        const lane_def_t none = { .kind = LD_REMOVE };
        const lane_def_t disc = { .kind = LD_DRAW };
        struct { const char *arg; const lane_def_t *was; int ret; int chan, gate; } t[] = {
            { " ch 5",           &bass, 1, 5, 180 },
            { "ch 6 gate 40",    &bass, 1, 6, 40 },
            { " gate 900",       &bass, 1, 1, 900 },
            { " ch 3",           &cut,  1, 3, 150 },
            { " ch 5",           &none, -1, 0, 0 },
            { " ch 5",           &disc, -1, 0, 0 },
            { " voice 1 ch 5",   &bass, 0, 0, 0 },
            { " chord",          &bass, 0, 0, 0 },
        };
        for (size_t i = 0; i < sizeof t / sizeof t[0]; i++) {
            char full[80];
            lane_def_t d = { 0 };
            const int r = lane_def_shift(t[i].arg, t[i].was, full, sizeof full);
            if (r == 1) {
                lane_def_parse(full, &d);
            }
            if (r != t[i].ret || (r == 1 && (d.kind != t[i].was->kind ||
                d.num != t[i].was->num || d.chan != t[i].chan || d.gate != t[i].gate))) {
                printf("[FAIL] shift '%s' -> %d, ch %d gate %d\n", t[i].arg, r, d.chan, d.gate);
                fails++;
            }
        }
        lane_def_t d;
        char full[80];
        if (lane_def_shift(" ch 17", &bass, full, sizeof full) != 1 ||
            lane_def_parse(full, &d) != LD_ERROR) {
            printf("[FAIL] ch 17 moved the bass\n");
            fails++;
        }
        lane_def_parse(" voice 2 ch 5 gate 180", &d);
        lane_def_text(&d, full, sizeof full);
        if (strcmp(full, "voice 2 ch 5 gate 180") != 0) {
            printf("[FAIL] the reply reads '%s'\n", full);
            fails++;
        }
    }

    /* Every reason a name is refused fits the status bar too. */
    for (int e = LN_EMPTY; e <= LN_BRACKET; e++) {
        char why[64];
        lane_name_error_text(e, why, sizeof why);
        if (strlen(why) > 30) {
            printf("[FAIL] name error %d is %zu wide: %s\n", e, strlen(why), why);
            fails++;
        }
    }

    printf(fails ? "[FAIL] %d check(s) failed\n" : "[PASS] lane names\n", fails);
    return fails != 0;
}
