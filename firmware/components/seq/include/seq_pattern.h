/*
 * Pattern compilation: the one walk over a pattern string.
 *
 * seq_lane() compiles a pattern into events for the clock. The editor must walk
 * the SAME string to put the playhead on the step that is sounding. Two walks
 * that must agree about which characters are steps is two chances to disagree,
 * and the disagreement would be invisible: the mark would sit one column off
 * and look deliberate. So there is ONE compiler, here, in a header with no
 * dependencies, built into both the firmware and the host checks - the same
 * arrangement as st7305_addr.h and mirror_path.h.
 *
 * ===================================================================== *
 * A STEP IS ONE CHARACTER PLUS OPTIONAL MODIFIERS  (docs/MANIFESTO.md §3.6)
 * ===================================================================== *
 *
 * The rule used to be one character per step, "so the playhead can sit on the
 * character that is sounding". It was already broken - 'x%15' is four
 * characters for one step - and it was the rule used to refuse chords, note
 * length and per-step velocity. The owner decided it on 2026-09-25: keep the
 * GOAL, drop the MECHANISM. A step is a head character and what is attached to
 * it, and the playhead lights the whole span.
 *
 *     x        a hit at the lane's own level
 *     0-9      a hit with an amount: velocity on a drum, degree on a voice,
 *              value on a controller, how much on a picture
 *     .        rest - the only rest
 *     _        tie - the note before it keeps sounding
 *     u d l r  a hit that says which way (docs/MANIFESTO.md §3.10 will move
 *              direction to a parameter lane)
 *     [ab]     a group: subdivides the step it occupies, any depth
 *     [0,4,7]  a stack: every member sounds at once - a chord
 *     <ab>     alternation: one member per cycle
 *     %NN      on any step or group: NN per cent odds
 *
 * and at the end of the line, after a space:  /2 *2  (rate)  !4  (play four
 * cycles, then stop - docs/NEXT.md §4).
 *
 * ANYTHING ELSE IS AN ERROR, WITH THE CHARACTER'S POSITION (§3.2). This used to
 * compile any non-rest as a hit, so '>hat x...x...x;..' played the semicolon and
 * the deck said nothing, and an unclosed '[' became a nonsense septuplet. A
 * pattern that is refused with a reason is a pattern a performer can fix at
 * 2 a.m.; one that plays wrong is not.
 *
 * ===================================================================== *
 * EVENTS, NOT FLATTENED SLOTS
 * ===================================================================== *
 *
 * The clock reads a list of events per lane, each at a SLOT of the lane's cycle,
 * each carrying a CYCLE CLASS: it plays when (cycle % per == ph). That one pair
 * is what alternation compiles to, at any depth - '<a <b c>>' is a on cycles
 * 0 mod 2, b on 1 mod 4, c on 3 mod 4 - so '<>' costs no slots at all.
 * Alternation used to be flattened by laying the pattern down once per cycle,
 * which cost lcm(...) slots and hit an invisible cliff at 64 (§3.9). The cycle
 * number comes from the global tick, so it costs no runtime state either: the
 * "one byte of cycle counter" the manifesto asked for turned out to be zero.
 *
 * Nesting is still resolved here: every step gets `div` slots, the least common
 * multiple of what its contents need, so every leaf lands on a slot boundary. A
 * pattern whose subdivision does not fit is REFUSED with the number it needed -
 * a silently shortened bar is a bug that sounds like a composition choice.
 */
#ifndef SEQ_PATTERN_H
#define SEQ_PATTERN_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SEQ_PATTERN_MAX_SLOTS   64   /* slots in one cycle                    */
#define SEQ_PATTERN_MAX_DEPTH    4   /* brackets inside brackets              */
#define SEQ_PATTERN_MAX_LEAVES 160   /* steps, rests and ties, every member   */
#define SEQ_PATTERN_MAX_PER    240   /* bars before an alternation repeats    */
#define SEQ_PATTERN_MAX_TAIL    16   /* notes one tie can hold at once        */
#define SEQ_PATTERN_MAX_RATE    32
#define SEQ_PATTERN_MAX_COUNT  255
/* 96 PPQN, so a sixteenth is 24 ticks. Duplicated from seq.h on purpose: this
 * header has no dependencies, and test_seq_pattern.c asserts the two agree. */
#define SEQ_PATTERN_TICKS_PER_STEP 24

#define SEQ_VAL_X        0xFF   /* 'x': the lane's own level          */
#define SEQ_PROB_ALWAYS  255

enum { SEQ_LEAF_HIT = 0, SEQ_LEAF_REST, SEQ_LEAF_TIE };

/* Why a pattern was refused. Each has a sentence in seq_pattern_error_text(). */
enum {
    SEQ_PAT_OK = 0,
    SEQ_PAT_EMPTY,        /* nothing but spacing - not an error to the caller */
    SEQ_PAT_BAD_CHAR,     /* not a step                                        */
    SEQ_PAT_OPEN,         /* a '[' or '<' that never closes                    */
    SEQ_PAT_CLOSE,        /* a ']' or '>' with nothing open                    */
    SEQ_PAT_MISMATCH,     /* '[' closed by '>', or '<' by ']'                  */
    SEQ_PAT_EMPTY_GROUP,  /* '[]', '<>', or a stack member with nothing in it  */
    SEQ_PAT_COMMA,        /* ',' outside brackets                              */
    SEQ_PAT_PERCENT,      /* '%' without a number 0-100                        */
    SEQ_PAT_TIE_START,    /* '_' with nothing before it                        */
    SEQ_PAT_TIE_ALT,      /* '_' that holds a note on some of its bars only    */
    SEQ_PAT_DEEP,         /* brackets deeper than SEQ_PATTERN_MAX_DEPTH        */
    SEQ_PAT_SLOTS,        /* subdivision needs more than MAX_SLOTS             */
    SEQ_PAT_LEAVES,       /* more steps than MAX_LEAVES                        */
    SEQ_PAT_PER,          /* alternation repeats after more than MAX_PER bars  */
    SEQ_PAT_TRAILER,      /* a rate or a count that is not a number in range   */
    SEQ_PAT_TWICE,        /* two rates, or two counts                          */
    SEQ_PAT_FINE,         /* a slot shorter than one tick of the clock         */
};

/* One step as the compiler found it. Rests and ties are leaves too, because the
 * playhead marks the STEP, not the hit - it sweeps rests, and it sits on a tie
 * while the note it holds is still sounding. */
typedef struct {
    uint8_t slot;    /* where it starts, within one cycle                     */
    uint8_t width;   /* its own slots - what the playhead covers              */
    uint8_t len;     /* slots it SOUNDS: width, plus any ties after it        */
    uint8_t val;     /* 0-9, or SEQ_VAL_X                                     */
    uint8_t prob;    /* 0-100, or SEQ_PROB_ALWAYS                             */
    uint8_t per;     /* plays when cycle % per == ph                          */
    uint8_t ph;
    uint8_t kind;    /* SEQ_LEAF_*                                            */
    char    dir;     /* 'u' 'd' 'l' 'r', or 0                                 */
    uint8_t at;      /* its first character                                   */
    uint8_t end;     /* one past its last, modifiers included                 */
} seq_leaf_t;

typedef struct {
    int  slots;      /* per cycle                                             */
    int  div;        /* slots per top-level step                              */
    int  steps;      /* top-level steps: the lane's length in sixteenths      */
    int  per;        /* bars before the whole pattern repeats exactly         */
    int  rnum, rden; /* '*n' and '/n'                                         */
    int  count;      /* '!n', 0 = for ever                                    */
    char dir;        /* a direction written in front: '>ramp u 4'            */
    int  plen;       /* characters before the trailing tokens                 */
    int  err;        /* SEQ_PAT_*                                             */
    int  err_at;     /* the character it is about                             */
    int  err_num;    /* a number that makes the message exact, or 0           */
    int  n;
    seq_leaf_t leaf[SEQ_PATTERN_MAX_LEAVES];
} seq_comp_t;

/* ---------------------------------------------------------------- lexing */

static inline int seq_pattern_is_spacing(char c)
{
    /* A tab is spacing too. It used to compile as a HIT, because "anything that
     * is not a rest is a hit" - which is exactly the rule §3.2 retires. The
     * keyboard cannot type one (Tab inserts two spaces), so this only matters
     * for text that arrived some other way, and there it should not play. */
    return c == ' ' || c == '\t';
}

static inline int seq_pattern_is_head(char c)
{
    return c == 'x' || (c >= '0' && c <= '9') || c == '.' || c == '_' ||
           c == 'u' || c == 'd' || c == 'l' || c == 'r';
}

/* '%' and one to three digits, 0-100. Returns the length, 0 if there is no
 * '%' here, or -1 if there is one and it is malformed. */
static inline int seq_pattern_mod_len(const char *p, const char *end, int *val)
{
    if (p >= end || *p != '%') {
        return 0;
    }
    int n = 1, v = 0;
    while (p + n < end && p[n] >= '0' && p[n] <= '9' && n < 4) {
        v = v * 10 + (p[n] - '0');
        n++;
    }
    if (n == 1 || v > 100 || (p + n < end && p[n] >= '0' && p[n] <= '9')) {
        return -1;
    }
    if (val != NULL) { *val = v; }
    return n;
}

static inline int seq_pattern_gcd(int a, int b)
{
    if (a < 0) { a = -a; }
    if (b < 0) { b = -b; }
    while (b != 0) { const int t = a % b; a = b; b = t; }
    return a < 1 ? 1 : a;
}

static inline int seq_pattern_lcm(int a, int b)
{
    if (a < 1) { a = 1; }
    if (b < 1) { b = 1; }
    const long l = (long)(a / seq_pattern_gcd(a, b)) * b;
    return l > 100000 ? 100000 : (int)l;   /* saturates; callers compare */
}

/* ------------------------------------------------------------ trailers */

/* THE TOKENS AT THE END OF THE LINE: '/2' or '*2' sets this lane's rate, '!4'
 * plays four cycles and stops. Whitespace-separated, after the pattern, in any
 * order. They live HERE and not in the command layer for the reason everything
 * does: the editor and the compiler must agree about which characters are
 * steps, and '/2' would otherwise be two more of them.
 *
 * A token that starts with one of the three and is not a number in range is an
 * ERROR, not picture - '/', '*' and '!' are not steps, so there is nothing else
 * it could be. Returns the pattern's length before the trailers, or -1. */
static inline int seq_pattern_trailer(const char *pat, seq_comp_t *c)
{
    int end = (int)strlen(pat);
    int seen_rate = 0, seen_count = 0;
    for (;;) {
        while (end > 0 && seq_pattern_is_spacing(pat[end - 1])) { end--; }
        int start = end;
        while (start > 0 && !seq_pattern_is_spacing(pat[start - 1])) { start--; }
        if (start == end || start == 0) {
            break;                        /* no token, or the pattern itself */
        }
        const char op = pat[start];
        if (op != '/' && op != '*' && op != '!') {
            break;
        }
        int v = 0, ok = (end - start >= 2 && end - start <= 4);
        for (int i = start + 1; i < end && ok; i++) {
            if (pat[i] < '0' || pat[i] > '9') { ok = 0; break; }
            v = v * 10 + (pat[i] - '0');
        }
        const int max = (op == '!') ? SEQ_PATTERN_MAX_COUNT : SEQ_PATTERN_MAX_RATE;
        if (!ok || v < 1 || v > max) {
            c->err = SEQ_PAT_TRAILER;
            c->err_at = start;
            return -1;
        }
        if (op == '!') {
            if (seen_count++) { c->err = SEQ_PAT_TWICE; c->err_at = start; return -1; }
            c->count = v;
        } else {
            if (seen_rate++) { c->err = SEQ_PAT_TWICE; c->err_at = start; return -1; }
            if (op == '/') { c->rden = v; } else { c->rnum = v; }
        }
        end = start;
    }
    while (end > 0 && seq_pattern_is_spacing(pat[end - 1])) { end--; }
    return end;
}

/* ------------------------------------------------------------ the walk */

/* The compiler is two passes over the same recursive shape: MEASURE finds how
 * many slots every item needs and refuses anything malformed; PLACE lays the
 * leaves down. Both find item boundaries with seq_pattern_item_end(), so they
 * cannot disagree about where one ends.
 *
 * Nothing here keeps an array per nesting level: the walk runs on a device task
 * stack, and four levels of member tables was four kilobytes. Members and
 * alternatives are walked in place instead, which costs a second scan of a
 * string that is never longer than a line. */

/* Where the item starting at `p` ends, modifiers included, and where its
 * bracket closes (`*close`, one past the ']' or '>'; for a step, p + 1). `p`
 * must be at a non-spacing character. Returns NULL and sets the error if the
 * item is malformed. */
static inline const char *seq_pattern_item_end(const char *base, const char *p,
                                               const char *end, seq_comp_t *c,
                                               const char **close)
{
    const char *q = p + 1;
    if (*p == '[' || *p == '<') {
        char open[SEQ_PATTERN_MAX_DEPTH];
        int depth = 0;
        q = p;
        while (q < end) {
            if (*q == '[' || *q == '<') {
                if (depth >= SEQ_PATTERN_MAX_DEPTH) {
                    c->err = SEQ_PAT_DEEP; c->err_at = (int)(q - base);
                    return NULL;
                }
                open[depth++] = *q;
            } else if (*q == ']' || *q == '>') {
                if (open[depth - 1] != ((*q == ']') ? '[' : '<')) {
                    c->err = SEQ_PAT_MISMATCH; c->err_at = (int)(q - base);
                    return NULL;
                }
                if (--depth == 0) { q++; break; }
            }
            q++;
        }
        if (depth != 0) {
            c->err = SEQ_PAT_OPEN; c->err_at = (int)(p - base);
            return NULL;
        }
    } else if (*p == ']' || *p == '>') {
        c->err = SEQ_PAT_CLOSE; c->err_at = (int)(p - base);
        return NULL;
    } else if (*p == ',') {
        c->err = SEQ_PAT_COMMA; c->err_at = (int)(p - base);
        return NULL;
    } else if (!seq_pattern_is_head(*p)) {
        c->err = SEQ_PAT_BAD_CHAR; c->err_at = (int)(p - base);
        return NULL;
    }
    if (close != NULL) { *close = q; }
    const int m = seq_pattern_mod_len(q, end, NULL);
    if (m < 0) {
        c->err = SEQ_PAT_PERCENT; c->err_at = (int)(q - base);
        return NULL;
    }
    return q + m;
}

/* The next ','-separated member of a group's contents, starting at *q. Sets
 * [*ms, *me) and moves *q past the separator. Returns 1 for a member, 0 when
 * there are no more, -1 on error. An empty member - '[]' or '[x,]' - is an
 * error: it is a typo, not a silence. */
static inline int seq_pattern_member(const char *base, const char **q,
                                     const char *end, const char **ms,
                                     const char **me, seq_comp_t *c)
{
    if (*q > end) {
        return 0;
    }
    const char *s = *q;
    const char *p = *q;
    int items = 0;
    while (p < end && *p != ',') {
        if (seq_pattern_is_spacing(*p)) { p++; continue; }
        p = seq_pattern_item_end(base, p, end, c, NULL);
        if (p == NULL) { return -1; }
        items++;
    }
    if (items == 0) {
        c->err = SEQ_PAT_EMPTY_GROUP; c->err_at = (int)(p - base);
        return -1;
    }
    *ms = s;
    *me = p;
    *q = p + 1;               /* past the ',' - or one past the end, which stops us */
    return 1;
}

static inline int seq_pattern_measure_seq(const char *base, const char *p,
                                          const char *end, int depth,
                                          seq_comp_t *c, int *items, int *per);

/* Slots one item needs, and how many cycles it takes to come round (*per). A
 * step needs one slot. A '[]' needs what its widest member needs. A '<>' needs
 * what its widest ALTERNATIVE needs, because only one plays per cycle but the
 * step has to hold whichever one it is. */
static inline int seq_pattern_measure_item(const char *base, const char *p,
                                           const char *e, const char *close,
                                           int depth, seq_comp_t *c, int *per)
{
    *per = 1;
    if (*p != '[' && *p != '<') {
        return 1;
    }
    const char *q = p + 1, *inner_end = close - 1, *ms, *me;
    int need = 1, cyc = 1, r;
    (void)e;
    while ((r = seq_pattern_member(base, &q, inner_end, &ms, &me, c)) == 1) {
        if (*p == '[') {
            int items = 0, mp = 1;
            const int w = seq_pattern_measure_seq(base, ms, me, depth - 1, c,
                                                  &items, &mp);
            if (w < 0) { return -1; }
            need = seq_pattern_lcm(need, w);
            cyc  = seq_pattern_lcm(cyc, mp);
        } else {
            /* ALTERNATION: each item of this member is one alternative. A
             * member of k items comes round every k cycles, times whatever its
             * items themselves alternate by. */
            int k = 0, inner = 1;
            for (const char *a = ms; a < me; ) {
                if (seq_pattern_is_spacing(*a)) { a++; continue; }
                const char *ac = NULL;
                const char *ae = seq_pattern_item_end(base, a, me, c, &ac);
                if (ae == NULL) { return -1; }
                int ip = 1;
                const int w = seq_pattern_measure_item(base, a, ae, ac, depth - 1,
                                                       c, &ip);
                if (w < 0) { return -1; }
                need  = seq_pattern_lcm(need, w);
                inner = seq_pattern_lcm(inner, ip);
                k++;
                a = ae;
            }
            cyc = seq_pattern_lcm(cyc, k * inner);
        }
    }
    if (r < 0) { return -1; }
    *per = cyc;
    return need;
}

/* Slots a sequence needs: its item count times the lcm of what its items need,
 * so every item gets an equal share on a slot boundary. */
static inline int seq_pattern_measure_seq(const char *base, const char *p,
                                          const char *end, int depth,
                                          seq_comp_t *c, int *items, int *per)
{
    if (depth < 0) {
        c->err = SEQ_PAT_DEEP; c->err_at = (int)(p - base);
        return -1;
    }
    int k = 0, l = 1, cyc = 1;
    while (p < end) {
        if (seq_pattern_is_spacing(*p)) { p++; continue; }
        const char *close = NULL;
        const char *e = seq_pattern_item_end(base, p, end, c, &close);
        if (e == NULL) { return -1; }
        int ip = 1;
        const int s = seq_pattern_measure_item(base, p, e, close, depth, c, &ip);
        if (s < 0) { return -1; }
        l = seq_pattern_lcm(l, s);
        cyc = seq_pattern_lcm(cyc, ip);
        k++;
        p = e;
    }
    *items = k;
    *per = cyc;
    return k * l;
}

/* A tie extends whatever was sounding at the end of the item before it. The
 * placement passes that set down as a TAIL: the notes that end exactly where
 * the next item begins. */
typedef struct {
    int n;
    int16_t idx[SEQ_PATTERN_MAX_TAIL];
} seq_tail_t;

static inline int seq_pattern_new_leaf(seq_comp_t *c, int at_char)
{
    if (c->n >= SEQ_PATTERN_MAX_LEAVES) {
        c->err = SEQ_PAT_LEAVES; c->err_at = at_char; c->err_num = c->n + 1;
        return -1;
    }
    memset(&c->leaf[c->n], 0, sizeof c->leaf[0]);
    return c->n++;
}

/* Is every cycle of class (p1,k1) also in (p2,k2)? */
static inline int seq_pattern_class_in(int p1, int k1, int p2, int k2)
{
    return p2 > 0 && p1 % p2 == 0 && k1 % p2 == k2;
}

/* Do the two classes share any cycle at all? By the Chinese remainder theorem,
 * exactly when the phases agree modulo the gcd of the periods. */
static inline int seq_pattern_class_meet(int p1, int k1, int p2, int k2)
{
    return (k1 - k2) % seq_pattern_gcd(p1, p2) == 0;
}

/* Odds multiply: a step at 50 % inside a group at 50 % plays a quarter of the
 * time, which is what the two marks say read together. */
static inline int seq_pattern_prob(int parent, int own)
{
    if (parent == SEQ_PROB_ALWAYS && own == SEQ_PROB_ALWAYS) {
        return SEQ_PROB_ALWAYS;
    }
    const int a = (parent == SEQ_PROB_ALWAYS) ? 100 : parent;
    const int b = (own == SEQ_PROB_ALWAYS) ? 100 : own;
    return (a * b + 50) / 100;
}

static inline int seq_pattern_place_seq(const char *base, const char *p,
                                        const char *end, int start, int width,
                                        int per, int ph, int prob, int depth,
                                        seq_comp_t *c, seq_tail_t *tail);

static inline void seq_pattern_tail_add(seq_tail_t *to, const seq_tail_t *from)
{
    for (int j = 0; j < from->n && to->n < SEQ_PATTERN_MAX_TAIL; j++) {
        to->idx[to->n++] = from->idx[j];
    }
}

/* Place one item across [start, start + width). `tail` holds, on entry, the
 * notes sounding up to `start`; on exit, the notes sounding at its end. */
static inline int seq_pattern_place_item(const char *base, const char *p,
                                         const char *e, const char *close,
                                         int start, int width, int per, int ph,
                                         int prob, int depth, seq_comp_t *c,
                                         seq_tail_t *tail)
{
    int own = SEQ_PROB_ALWAYS;
    if (seq_pattern_mod_len(close, e, &own) <= 0) { own = SEQ_PROB_ALWAYS; }
    const int pr = seq_pattern_prob(prob, own);

    if (*p == '[' || *p == '<') {
        const char *q = p + 1, *inner_end = close - 1, *ms, *me;
        seq_tail_t out = { 0, { 0 } };
        int r;
        while ((r = seq_pattern_member(base, &q, inner_end, &ms, &me, c)) == 1) {
            if (*p == '[') {
                /* every member of a stack follows the same note */
                seq_tail_t t = *tail;
                if (seq_pattern_place_seq(base, ms, me, start, width, per, ph, pr,
                                          depth - 1, c, &t) < 0) {
                    return -1;
                }
                seq_pattern_tail_add(&out, &t);
                continue;
            }
            /* ALTERNATIVE j of k plays on cycles where (cycle / per) % k == j,
             * which is ONE residue class: cycle % (per*k) == ph + per*j. That is
             * why nesting composes: '<a <b c>>' puts b on 1 mod 4 and c on
             * 3 mod 4 - the inner group advances only when it is chosen, which
             * is what Strudel does, and tools/corpus checks it against Strudel. */
            int k = 0;
            for (const char *a = ms; a < me; ) {
                if (seq_pattern_is_spacing(*a)) { a++; continue; }
                a = seq_pattern_item_end(base, a, me, c, NULL);
                if (a == NULL) { return -1; }
                k++;
            }
            if ((long)per * k > SEQ_PATTERN_MAX_PER) {
                c->err = SEQ_PAT_PER; c->err_at = (int)(p - base);
                c->err_num = per * k;
                return -1;
            }
            int j = 0;
            for (const char *a = ms; a < me; ) {
                if (seq_pattern_is_spacing(*a)) { a++; continue; }
                const char *ac = NULL;
                const char *ae = seq_pattern_item_end(base, a, me, c, &ac);
                seq_tail_t t = *tail;
                if (seq_pattern_place_item(base, a, ae, ac, start, width, per * k,
                                           ph + per * j, pr, depth - 1, c, &t) < 0) {
                    return -1;
                }
                seq_pattern_tail_add(&out, &t);
                j++;
                a = ae;
            }
        }
        if (r < 0) { return -1; }
        *tail = out;
        return 0;
    }

    /* A LEAF. */
    if (*p == '_' && c->n == 0) {
        c->err = SEQ_PAT_TIE_START; c->err_at = (int)(p - base);
        return -1;
    }
    const int li = seq_pattern_new_leaf(c, (int)(p - base));
    if (li < 0) { return -1; }
    seq_leaf_t *L = &c->leaf[li];
    L->slot  = (uint8_t)start;
    L->width = (uint8_t)width;
    L->len   = (uint8_t)width;
    L->per   = (uint8_t)per;
    L->ph    = (uint8_t)ph;
    L->prob  = (uint8_t)pr;
    L->at    = (uint8_t)(p - base);
    L->end   = (uint8_t)(e - base);
    L->val   = SEQ_VAL_X;
    if (*p == '.') {
        L->kind = SEQ_LEAF_REST;
        tail->n = 0;
        return 0;
    }
    if (*p == '_') {
        L->kind = SEQ_LEAF_TIE;
        /* A TIE: every note sounding up to here keeps sounding across this step.
         * A note that plays on bars the tie is NOT there for - '0<_ .>' - would
         * need two lengths, and one note has one; that is refused rather than
         * guessed. A note that never meets this tie is simply not held by it. */
        seq_tail_t held = { 0, { 0 } };
        for (int j = 0; j < tail->n; j++) {
            seq_leaf_t *N = &c->leaf[tail->idx[j]];
            if (N->slot + N->len != start ||
                !seq_pattern_class_meet(N->per, N->ph, per, ph)) {
                continue;
            }
            if (!seq_pattern_class_in(N->per, N->ph, per, ph)) {
                c->err = SEQ_PAT_TIE_ALT; c->err_at = (int)(p - base);
                return -1;
            }
            N->len = (uint8_t)(N->len + width);
            held.idx[held.n++] = tail->idx[j];
        }
        *tail = held;
        return 0;
    }
    L->kind = SEQ_LEAF_HIT;
    if (*p >= '0' && *p <= '9') {
        L->val = (uint8_t)(*p - '0');
    } else if (*p == 'u' || *p == 'd' || *p == 'l' || *p == 'r') {
        L->dir = *p;
    }
    tail->n = 1;
    tail->idx[0] = (int16_t)li;
    return 0;
}

static inline int seq_pattern_place_seq(const char *base, const char *p,
                                        const char *end, int start, int width,
                                        int per, int ph, int prob, int depth,
                                        seq_comp_t *c, seq_tail_t *tail)
{
    int k = 0;
    for (const char *q = p; q < end; ) {
        if (seq_pattern_is_spacing(*q)) { q++; continue; }
        q = seq_pattern_item_end(base, q, end, c, NULL);
        if (q == NULL) { return -1; }
        k++;
    }
    if (k == 0) { return 0; }
    const int share = width / k;
    int idx = 0;
    while (p < end) {
        if (seq_pattern_is_spacing(*p)) { p++; continue; }
        const char *close = NULL;
        const char *e = seq_pattern_item_end(base, p, end, c, &close);
        if (seq_pattern_place_item(base, p, e, close, start + idx * share, share,
                                   per, ph, prob, depth, c, tail) < 0) {
            return -1;
        }
        idx++;
        p = e;
    }
    return 0;
}

/* Compile. Returns c->err: SEQ_PAT_OK, SEQ_PAT_EMPTY for a pattern with no
 * steps, or the reason it was refused, with c->err_at the character. */
static inline int seq_pattern_compile(const char *pat, seq_comp_t *c)
{
    memset(c, 0, offsetof(seq_comp_t, leaf));
    c->rnum = 1;
    c->rden = 1;
    c->per = 1;
    c->err_at = -1;
    if (pat == NULL) {
        return c->err = SEQ_PAT_EMPTY;
    }
    const int plen = seq_pattern_trailer(pat, c);
    if (plen < 0) {
        return c->err;
    }
    c->plen = plen;
    const char *base = pat;
    const char *p = pat, *end = pat + plen;
    while (p < end && seq_pattern_is_spacing(*p)) { p++; }

    /* A DIRECTION IN FRONT: '>ramp u 4' says which way and how far. One
     * character per step meant a step could hold a value or a direction, never
     * both, and the bindings that point somewhere need both. Harmless on a note
     * lane, which never reads it. */
    if (p + 1 < end && (*p == 'u' || *p == 'd' || *p == 'l' || *p == 'r') &&
        seq_pattern_is_spacing(p[1])) {
        c->dir = *p;
        p += 2;
    }

    int items = 0, per = 1;
    const int w = seq_pattern_measure_seq(base, p, end, SEQ_PATTERN_MAX_DEPTH,
                                          c, &items, &per);
    if (w < 0) {
        return c->err;
    }
    if (items == 0) {
        return c->err = SEQ_PAT_EMPTY;
    }
    if (w > SEQ_PATTERN_MAX_SLOTS) {
        c->err = SEQ_PAT_SLOTS; c->err_at = (int)(p - base); c->err_num = w;
        return c->err;
    }
    if (per > SEQ_PATTERN_MAX_PER) {
        c->err = SEQ_PAT_PER; c->err_at = (int)(p - base); c->err_num = per;
        return c->err;
    }
    c->per   = per;
    c->steps = items;
    c->div   = w / items;
    c->slots = w;
    /* A SLOT SHORTER THAN A TICK CANNOT BE PLAYED ON TIME - two slots would
     * land on one tick and one of them would vanish. The clock has 24 ticks
     * to a sixteenth, so this is '*4' on a seven-way split and nothing anyone
     * reaches for; it is refused so that it can never be heard. */
    if (c->div * c->rnum > SEQ_PATTERN_TICKS_PER_STEP * c->rden) {
        c->err = SEQ_PAT_FINE; c->err_at = (int)(p - base);
        c->err_num = c->div * c->rnum;
        return c->err;
    }
    seq_tail_t tail = { 0, { 0 } };
    if (seq_pattern_place_seq(base, p, end, 0, w, 1, 0, SEQ_PROB_ALWAYS,
                              SEQ_PATTERN_MAX_DEPTH, c, &tail) < 0) {
        return c->err;
    }
    return c->err = SEQ_PAT_OK;
}

/* ------------------------------------------------------------ time */

/* WHICH SLOT OF A LANE STARTS ON THIS TICK, AND IN WHICH CYCLE.
 *
 * Slot g (counted from play) starts on the tick NEAREST g * Q / D, with
 * Q = 24 * rden and D = div * rnum: floor((2gQ + D) / 2D). It used to be
 * 24 / div ticks per slot in integer arithmetic, so a five-way split got 4
 * ticks where it needed 4.8, and a lane of '[xxxxx]...' looped in 80 ticks
 * instead of 96 - sixteen ticks, 81 ms at 124 bpm, early every bar against
 * everything else. Now its bar is exactly a bar, and each note is
 * within half a tick (2.5 ms at 124 bpm) of where it belongs. Rounding DOWN,
 * the first version of this fix, put the second note a whole tick early.
 *
 * SWING BENDS TIME, AND EVERY LANE LIVES IN THE SAME TIME. An eighth is two
 * sixteenths; swing makes the first longer and the second shorter, so the
 * second sixteenth of every eighth starts `swing_ticks` late. Whatever is
 * inside a sixteenth keeps its place WITHIN it - stretched in the first,
 * squeezed in the second - so a roll moves with the sixteenth it is in, and
 * the start of every eighth never moves at all.
 *
 * It delayed a lane's odd SLOTS instead, which is the same thing only while a
 * slot is a sixteenth. One '[xx]' made every slot of its lane a thirty-second,
 * and the whole line went straight: at swing 67 'xxxxxxxxxxxxxxx[xx]' put its
 * offbeats on ticks 24, 72, 120, 168 where 'xxxxxxxxxxxxxxxx' put them on 32,
 * 80, 128, 176 - a 42 ms flam at 120 bpm between two lanes playing the same
 * sixteenths (host, 2026-09-26). And a '/2' lane's slots are eighths, so
 * 'xxxxxxxx /2' swung its offbeat eighths twice as hard as anything else
 * could, while 'x.x.x.x.x.x.x.x.' - the same notes - did not swing at all.
 * A plain sixteenth lane lands exactly where it always did.
 *
 * Returns 1 and sets *slot and *cycle when a slot starts on `tick`. */

/* Where slot g starts once swing has bent it: its unswung start is g*Q/D ticks,
 * kept as a fraction over D so nothing is rounded twice. With no swing this is
 * the nearest tick to g*Q/D, exactly as it was. */
static inline uint64_t seq_pattern_swung(uint64_t g, uint64_t Q, uint64_t D,
                                         int swing_ticks)
{
    const uint64_t T = SEQ_PATTERN_TICKS_PER_STEP;          /* a sixteenth */
    const uint64_t s = (uint64_t)(swing_ticks < 0 ? 0
                     : swing_ticks >= (int)T ? (int)T - 1 : swing_ticks);
    const uint64_t at = g * Q;
    const uint64_t e = at / (2 * T * D);                    /* which eighth */
    const uint64_t r = at - e * 2 * T * D;                  /* into it, over D */
    const uint64_t num = (r < T * D) ? r * (T + s)
                       : (T + s) * T * D + (r - T * D) * (T - s);
    const uint64_t den = T * D;
    return e * 2 * T + (2 * num + den) / (2 * den);         /* the nearest tick */
}

static inline int seq_pattern_slot_at(uint32_t tick, int slots, int div,
                                      int rnum, int rden, int swing_ticks,
                                      int *slot, uint32_t *cycle)
{
    if (slots <= 0 || div <= 0 || rnum <= 0 || rden <= 0) { return 0; }
    const uint64_t Q = (uint64_t)SEQ_PATTERN_TICKS_PER_STEP * (uint64_t)rden;
    const uint64_t D = (uint64_t)div * (uint64_t)rnum;
    /* The last slot whose unswung start is at or before this tick. Swing only
     * ever delays, so no later slot can start here; walk back through the ones
     * it may have delayed onto it - a sixteenth's worth at most. */
    uint64_t g = (D * (2 * (uint64_t)tick + 1) - 1) / (2 * Q);
    for (;;) {
        const uint64_t at = seq_pattern_swung(g, Q, D, swing_ticks);
        if (at == (uint64_t)tick) {
            if (slot != NULL)  { *slot = (int)(g % (uint64_t)slots); }
            if (cycle != NULL) { *cycle = (uint32_t)(g / (uint64_t)slots); }
            return 1;
        }
        if (at < (uint64_t)tick || g == 0) {
            return 0;
        }
        g--;
    }
}

/* The slot that is sounding at `tick` - the last one to have started - for the
 * playhead. Swing is ignored: the mark moves on the grid, not on the groove. */
static inline void seq_pattern_slot_now(uint32_t tick, int slots, int div,
                                        int rnum, int rden, int *slot,
                                        uint32_t *cycle)
{
    *slot = 0;
    *cycle = 0;
    if (slots <= 0 || div <= 0 || rnum <= 0 || rden <= 0) { return; }
    const uint64_t Q = (uint64_t)SEQ_PATTERN_TICKS_PER_STEP * (uint64_t)rden;
    const uint64_t D = (uint64_t)div * (uint64_t)rnum;
    const uint64_t g = (D * (2 * (uint64_t)tick + 1) - 1) / (2 * Q);
    *slot  = (int)(g % (uint64_t)slots);
    *cycle = (uint32_t)(g / (uint64_t)slots);
}

/* ------------------------------------------------------------ playhead */

/* THE SPAN TO LIGHT for the step sounding at (slot, cycle): from the first
 * character of the first leaf covering it to the end of the last. One leaf is
 * a step and its modifiers - 'x%15' lights all four. Several are a chord, and
 * the span runs from its first note to its last. Returns 1 if there is one. */
static inline int seq_pattern_mark(const seq_comp_t *c, int slot, uint32_t cycle,
                                   int *from, int *to)
{
    int a = -1, b = -1;
    for (int i = 0; i < c->n; i++) {
        const seq_leaf_t *L = &c->leaf[i];
        if (slot < L->slot || slot >= L->slot + L->width) { continue; }
        if (L->per > 1 && (int)(cycle % L->per) != L->ph) { continue; }
        if (a < 0 || L->at < a) { a = L->at; }
        if (L->end > b) { b = L->end; }
    }
    if (a < 0) { return 0; }
    *from = a;
    *to = b;
    return 1;
}

/* ------------------------------------------------------------ errors */

/* The reason, in at most 30 characters - the width of the status bar at its
 * narrowest, checked by tools/test_seq_pattern.c. The character itself is
 * named, because at 2 a.m. "not a step" is useless without WHICH, and the
 * characters people bring from other languages are told where they went. */
static inline void seq_pattern_error_text(const seq_comp_t *c, const char *pat,
                                          char *out, size_t n)
{
    const char ch = (pat != NULL && c->err_at >= 0 &&
                     c->err_at < (int)strlen(pat)) ? pat[c->err_at] : '?';
    switch (c->err) {
    case SEQ_PAT_OK:
    case SEQ_PAT_EMPTY:       snprintf(out, n, "ok"); break;
    case SEQ_PAT_BAD_CHAR:
        switch (ch) {
        case 'X': snprintf(out, n, "X is gone: 9 is loud"); break;
        case '?': snprintf(out, n, "? is gone: x%%50 is maybe"); break;
        case '-': snprintf(out, n, "- is not a rest here: ."); break;
        case '~': snprintf(out, n, "~ is not a rest here: ."); break;
        case '*': case '/':
                  snprintf(out, n, "%c goes last: x.x. %c2", ch, ch); break;
        case '!': snprintf(out, n, "! goes last: x.x. !4"); break;
        case '@': snprintf(out, n, "@ is not here: hold with _"); break;
        default:
            if (ch > ' ' && ch < 0x7F) {
                snprintf(out, n, "'%c' is not a step - x hits", ch);
            } else {
                snprintf(out, n, "not a step - x hits");
            }
            break;
        }
        break;
    case SEQ_PAT_OPEN:        snprintf(out, n, "'%c' is never closed", ch); break;
    case SEQ_PAT_CLOSE:       snprintf(out, n, "'%c' closes nothing", ch); break;
    case SEQ_PAT_MISMATCH:    snprintf(out, n, "'%c' closes the wrong one", ch); break;
    case SEQ_PAT_EMPTY_GROUP: snprintf(out, n, "empty brackets"); break;
    case SEQ_PAT_COMMA:       snprintf(out, n, "a chord goes in []: [0,4,7]"); break;
    case SEQ_PAT_PERCENT:     snprintf(out, n, "%% takes 0-100: x%%25"); break;
    case SEQ_PAT_TIE_START:   snprintf(out, n, "_ needs a note before it"); break;
    case SEQ_PAT_TIE_ALT:     snprintf(out, n, "_ holds it only some bars"); break;
    case SEQ_PAT_DEEP:        snprintf(out, n, "brackets go %d deep at most",
                                       SEQ_PATTERN_MAX_DEPTH); break;
    case SEQ_PAT_SLOTS:       snprintf(out, n, "needs %d slots, %d fit",
                                       c->err_num, SEQ_PATTERN_MAX_SLOTS); break;
    case SEQ_PAT_LEAVES:      snprintf(out, n, "over %d steps",
                                       SEQ_PATTERN_MAX_LEAVES); break;
    case SEQ_PAT_PER:         snprintf(out, n, "repeats in %d bars: %d max",
                                       c->err_num, SEQ_PATTERN_MAX_PER); break;
    case SEQ_PAT_TRAILER:
        if (ch == '!') {
            snprintf(out, n, "!n is 1-%d times", SEQ_PATTERN_MAX_COUNT);
        } else {
            snprintf(out, n, "a rate is /1-/%d or *1-*%d",
                     SEQ_PATTERN_MAX_RATE, SEQ_PATTERN_MAX_RATE);
        }
        break;
    case SEQ_PAT_TWICE:       snprintf(out, n, "one rate and one count only"); break;
    case SEQ_PAT_FINE:        snprintf(out, n, "too fine for the clock"); break;
    default:                  snprintf(out, n, "not a pattern"); break;
    }
}

/* ------------------------------------------------------------ small uses */

/* Slots a pattern compiles to per cycle, 0 if it is empty, -1 if refused. */
static inline int seq_pattern_steps(const char *pat, int max_steps)
{
    static seq_comp_t c;
    const int e = seq_pattern_compile(pat, &c);
    if (e == SEQ_PAT_EMPTY) { return 0; }
    if (e != SEQ_PAT_OK)    { return -1; }
    return (c.slots > max_steps) ? max_steps : c.slots;
}

/* A hash of the pattern AS TYPED, used to tell "I re-ran this line untouched"
 * from "I edited it and want it updated".
 *
 * It hashes the raw characters, spacing included - deliberately. A hash that
 * ignored spacing, or a comparison of the compiled events, would treat
 * "x...x...x...x..." and "x... x... x... x..." as the same line and silence
 * the lane the moment the player re-spaced it for readability. That is
 * exactly the surprise the toggle rule exists to avoid, so the rule fails
 * toward compiling and never toward silence.
 *
 * FNV-1a, 32-bit. Not cryptographic and does not need to be: the cost of a
 * collision is one press that silences instead of recompiling, and the next
 * press undoes it. */
static inline uint32_t seq_pattern_hash(const char *pat)
{
    uint32_t h = 2166136261u;
    if (pat == NULL) {
        return h;
    }
    for (const char *p = pat; *p != '\0'; p++) {
        h ^= (uint32_t)(unsigned char)*p;
        h *= 16777619u;
    }
    return h;
}

#endif /* SEQ_PATTERN_H */
