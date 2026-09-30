/*
 * A note-off ends its own note, never a later one - checked with the rule the
 * firmware runs, firmware/components/seq/include/seq_offs.h.
 *
 * The wire is modelled the way a synth hears it: a note sounds from its on to
 * the first off of its pitch on its channel. Two cases from the EP as it stood
 * on 2026-09-29: ORBITALS' rolling bass, sixteenths on a voice gated 1800 ms,
 * and GRID's pad, chords a bar apart held a bar and a half, sharing tones.
 * THE OLD QUEUE, offs scheduled and never paired, is run too, and must cut
 * notes - so the check can tell the two apart - then the new one must cut none.
 */
#include <stdio.h>
#include <string.h>

#include "seq_offs.h"

static int fails;

#define CHECK(cond, ...) do { if (!(cond)) { printf("[FAIL] " __VA_ARGS__); \
    printf("\n"); fails++; } else { printf("[ ok ] " __VA_ARGS__); printf("\n"); } } while (0)

#define N 64
typedef struct { int64_t t; int on; uint8_t note; } wire_t;
static wire_t s_wire[4096];
static int s_n;

static void send(int64_t t, int on, uint8_t note)
{
    if (s_n < (int)(sizeof s_wire / sizeof s_wire[0])) {
        s_wire[s_n++] = (wire_t){ t, on, note };
    }
}

/* A note: its start and gate. Played through the queue, paired or not. */
typedef struct { int64_t t, gate; uint8_t note; } note_t;

static void play(const note_t *notes, int k, int paired)
{
    seq_off_t offs[N];
    memset(offs, 0, sizeof offs);
    s_n = 0;
    for (int j = 0; j <= k; j++) {
        const int64_t now = (j < k) ? notes[j].t : INT64_MAX / 2;
        /* service_offs: what fell due before this note, in time order */
        for (;;) {
            int first = -1;
            for (int i = 0; i < N; i++) {
                if (offs[i].armed && offs[i].due_us <= now &&
                    (first < 0 || offs[i].due_us < offs[first].due_us)) { first = i; }
            }
            if (first < 0) { break; }
            offs[first].armed = false;
            send(offs[first].due_us, 0, offs[first].d1);
        }
        if (j == k) { break; }
        if (paired) {
            const int i = seq_off_take(offs, N, 0, notes[j].note);
            if (i >= 0) { send(now, 0, offs[i].d1); }
        }
        send(now, 1, notes[j].note);
        if (seq_off_arm(offs, N, now + notes[j].gate, "t", 0, notes[j].note) < 0) {
            send(now, 0, notes[j].note);
        }
    }
}

/* Notes that ended before their gate, other than by the next note of their
 * pitch starting: the synth's view of the wire. */
static int cut_short(const note_t *notes, int k)
{
    int cut = 0;
    for (int j = 0; j < k; j++) {
        int64_t next = INT64_MAX;
        for (int m = j + 1; m < k; m++) {
            if (notes[m].note == notes[j].note && notes[m].t > notes[j].t) { next = notes[m].t; break; }
        }
        int64_t want = notes[j].t + notes[j].gate;
        if (next < want) { want = next; }
        /* where this note really ended: the first off of its pitch after its on */
        int seen = 0;
        int64_t end = INT64_MAX;
        for (int w = 0; w < s_n; w++) {
            if (s_wire[w].note != notes[j].note) { continue; }
            if (s_wire[w].on && s_wire[w].t == notes[j].t) { seen = 1; continue; }
            if (seen && !s_wire[w].on) { end = s_wire[w].t; break; }
            if (seen && s_wire[w].on) { break; }
        }
        if (end < want) { cut++; }
    }
    return cut;
}

int main(void)
{
    const int64_t step = 60000000 / 124 / 4;           /* a sixteenth at 124 */
    /* ORBITALS' roll: .000.000.000.000 on D2, gate 1800 ms, two bars */
    static note_t roll[32];
    int k = 0;
    for (int s = 0; s < 32; s++) {
        if (s % 4 != 0) { roll[k++] = (note_t){ s * step, 1800000, 38 }; }
    }
    play(roll, k, 0);
    const int old_roll = cut_short(roll, k);
    CHECK(old_roll > 0, "the old queue cuts the roll: %d of %d notes end early", old_roll, k);
    play(roll, k, 1);
    CHECK(cut_short(roll, k) == 0, "paired, no note of the roll ends before its time");

    /* GRID's pad: a chord a bar, each held a bar and a half, the fifth shared */
    const int64_t bar = 16 * step, hold = bar + bar / 2;
    const note_t pad[] = {
        { 0, hold, 53 }, { 0, hold, 56 }, { 0, hold, 63 },
        { bar, hold, 56 }, { bar, hold, 60 }, { bar, hold, 63 },
        { 2 * bar, hold, 60 }, { 2 * bar, hold, 61 }, { 2 * bar, hold, 65 },
    };
    const int kp = (int)(sizeof pad / sizeof pad[0]);
    play(pad, kp, 0);
    const int old_pad = cut_short(pad, kp);
    CHECK(old_pad > 0, "the old queue cuts the pad's common tones: %d", old_pad);
    play(pad, kp, 1);
    CHECK(cut_short(pad, kp) == 0, "paired, every common tone rings to its time");

    /* and every on still gets an off: nothing left hanging */
    int ons = 0, offs = 0;
    for (int w = 0; w < s_n; w++) { if (s_wire[w].on) { ons++; } else { offs++; } }
    CHECK(ons == offs, "every note-on has its off (%d, %d)", ons, offs);

    if (fails) { printf("[FAIL] %d\n", fails); return 1; }
    printf("[PASS] a note-off ends its own note\n");
    return 0;
}
