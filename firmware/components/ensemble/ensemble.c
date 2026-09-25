#include "ensemble.h"

#include <string.h>

#include "esp_log.h"
#include "esp_now.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "seq.h"

static const char *TAG = "ens";

/* THE PACKET. Twenty bytes, broadcast eight times a second: 160 bytes/s on the
 * air for an entire ensemble, which is nothing.
 *
 * `sent_us` is the sender's own clock at the moment of the send, and `due_us` is
 * when the pulse it is reporting was due on the sender's ideal grid. The
 * difference between them is how far into the pulse the send happened, and
 * subtracting it is what lets a follower correct for flight time without either
 * deck knowing the other's absolute time. */
#define ENS_MAGIC 0x4B4C5233u          /* "KLR3" - a deck clock, version 3 */

/* THREE KINDS OF PACKET, AND THE ROUND TRIP IS THE WHOLE POINT.
 *
 * Version 1 was a one-way broadcast: the leader said where it was and every
 * follower believed it. That cannot work better than the transport's variance,
 * because a packet delayed behind other work reports its pulse as later than it
 * was and there is no way to know by how much. Measured: mean error -248 us, which
 * says the estimate was nearly UNBIASED, with a spread of 3340 us - so the problem
 * was never offset, it was variance.
 *
 * The fix is the one NTP, PTP and Ableton Link all use. The follower PROBES, the
 * leader REPLIES, and the follower knows the round trip in its own clock - no
 * shared absolute time needed anywhere. Collect a handful of round trips and keep
 * the one with the SMALLEST RTT, because the fastest exchange is the one that
 * queued least in both directions, and use that sample's offset.
 *
 * The distinction that matters, and which cost a wrong turn earlier: the minimum
 * is over ROUND-TRIP TIME, not over the offset. Taking the minimum offset is
 * biased - with symmetric noise it systematically undershoots, which is exactly
 * what was measured when it was tried.
 *
 * THE LEADER REPLIES FROM THE MAIN LOOP, not from the radio callback, and reports
 * how long it took. That is safe - no radio send from a callback - and costs
 * nothing in accuracy, because the turnaround is subtracted out. PTP calls this
 * the residence time, and reporting it is why a slow reply is harmless. */
/* THE BEACON IS BROADCAST; PROBES AND REPLIES ARE UNICAST. Tried the other way, and
 * the measurement was unambiguous.
 *
 * The argument for broadcasting everything is good: a broadcast is never acknowledged
 * and therefore never retried, so it goes out once and the send callback describes the
 * transmit that actually happened rather than the last of several attempts. That
 * matters, because an unacknowledged unicast is retried and its callback fires after
 * the retries - a timestamp several milliseconds later than the frame that landed.
 *
 * It is also, on this hardware at this range, a 90 per cent packet loss. Measured:
 * unicast probes drew about seventeen replies a second out of twenty; broadcast probes
 * drew about one and a half. The retries were not overhead, they were the delivery. A
 * clock that measures beautifully on one exchange in thirteen is worse than one that
 * measures adequately on seventeen in twenty, because the windows in between are spent
 * coasting - the same failure the hard RTT gate produced, reached by a third route.
 *
 * So: unicast, and the retry problem is handled where it belongs. An unacknowledged
 * send is marked and its exchange is dropped, because its departure time is not
 * trustworthy; that costs the fraction of exchanges the air was going to lose anyway.
 * The beacon stays broadcast because it is an announcement, not a measurement.
 *
 * The addressee field stays too. It costs two bytes, it is what lets a reply be
 * recognised without relying on the sender's address, and with several followers in a
 * room it is what stops one deck's tag matching another's. */
typedef enum {
    ENS_BEACON = 1,     /* leader -> everyone: I am here, and my tempo     */
    ENS_PROBE,          /* follower -> leader: what time is it? here is a tag */
    ENS_REPLY,          /* leader -> follower: here, minus my turnaround   */
} ens_kind_t;

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint8_t  kind;
    uint32_t tick;      /* the sender's absolute pulse since play */
    int32_t  ahead_us;  /* sent_us - due_us: how late the send was */
    /* A TAG, NOT A TIMESTAMP, and the change is the whole accuracy fix.
     *
     * Version 2 put the follower's send time in here and read it back out of the
     * reply, so the follower never had to remember which probe was being answered.
     * Neat, and wrong: that timestamp was taken before esp_now_send, which only
     * QUEUES the frame. What follows - the WiFi task waking, CSMA backoff waiting
     * for a quiet channel, the transmit itself - landed inside the measured round
     * trip, entirely on the outbound leg, while the half-RTT assumption spread it
     * over both. Measured, that was worth milliseconds on a busy channel and it is
     * where every one of the 2 ms outliers came from.
     *
     * So the packet carries an opaque tag, and the follower learns when the probe
     * actually left from the ESP-NOW send callback, which fires when the frame has
     * been transmitted and acknowledged. The tag is what matches the two up. */
    uint32_t tag;       /* PROBE/REPLY: which probe this is about */
    /* WHO THE REPLY IS FOR. Every packet in this protocol is broadcast - see below -
     * so a reply has to say whose probe it answers. Two bytes of the asking deck's
     * MAC, which is enough: a mistaken match needs two decks to collide on both the
     * low two bytes of their address and a 32-bit tag in the same two milliseconds. */
    uint8_t  whom[2];
    int32_t  resid_us;  /* REPLY: how long the leader held the probe */
    uint16_t bpm;
    uint8_t  running;
    /* WHO IS SPEAKING. A follower broadcasts too, slowly, so the LEADER can see
     * that the room is with it - '>sync' on the leading deck said "0 other decks"
     * while a deck was following it, which is true of an empty room and of a full
     * one and is the distinction a player needs on stage.
     *
     * Followers ignore each other. Two followers steering one another would be a
     * feedback loop with no reference, and the first one to drift would take the
     * rest with it. */
    uint8_t  role;      /* ENSEMBLE_LEAD or ENSEMBLE_FOLLOW */
} ens_pkt_t;

static const uint8_t BROADCAST[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

#define SEND_EVERY_US  125000          /* eight times a second, leading  */
/* FIVE TIMES A SECOND FOLLOWING, and the number comes from a measurement.
 *
 * A follower's beat exists only so the leader can count the room. At twice a
 * second the leader heard about one every two and a half seconds, because it is
 * transmitting eight times a second itself and misses receives while it does -
 * measured at 21 per cent delivery follower-to-leader against 86 the other way.
 * The count flickered between none and one. Five a second gives the five-second
 * window ten chances at the measured rate. */
#define BEAT_EVERY_US  200000
/* FIVE SECONDS, NOT TWO, and the reason is measured rather than guessed.
 *
 * A follower beats twice a second, so two seconds looked like four chances to be
 * heard. Delivery is not symmetric though: leader-to-follower measured 86 per cent
 * of expected packets, follower-to-leader about 21 - the leading deck is
 * transmitting eight times a second and misses receives while it does. So the
 * leader saw gaps longer than two seconds and reported "0 other decks" while a
 * deck was plainly following it.
 *
 * Five seconds is ten chances at the measured rate. It is also the right
 * resolution for the question: "is anyone else here" does not need answering in
 * under a second, and a number that flickers between 0 and 1 is worse than one
 * that lags. */
#define PEER_STALE_US 5000000
#define PEERS_MAX 8

static ensemble_role_t s_role;
static bool     s_up;
static int64_t  s_last_send;
static uint32_t s_heard;
static int32_t  s_err_us;

/* A WINDOW OF ROUND TRIPS, AND WHY THE BEST ONE IS NOT ENOUGH.
 *
 * Version 2 kept the single fastest exchange in the window and corrected from its
 * offset. That is what NTP does and it is right in principle - the fastest exchange
 * queued least, so the half-RTT assumption is closest to true for it. But it is ONE
 * DRAW. Its offset still carries the full jitter of the one receive timestamp it
 * was built from, and there is no averaging anywhere to reduce it: a window of
 * twenty-four probes was being thrown away to keep a single number.
 *
 * So keep the best SIX by round trip and correct toward their MEDIAN. Six samples
 * that all queued lightly are six near-independent measurements of the same offset;
 * their median has roughly a third of the spread of any one of them, and unlike a
 * mean it cannot be dragged by one outlier that slipped past the RTT filter.
 *
 * Their DISAGREEMENT is the other prize, and it is better than the RTT floor ever
 * was at judging a window. The floor is a memory of the luckiest packet ever seen,
 * which is why gating on it starved - it holds a deck to a moment that is not coming
 * back. Six probes agreeing within 300 us is self-evidently a clean window, needs no
 * history to interpret, and says so about THIS moment. */
/* TWENTY-FOUR PROBES TO A WINDOW, KEEPING SIX, and forty was tried and reverted.
 *
 * A deck that is also DRAWING is a deck whose WiFi task shares a core with a panel
 * render and six full-frame passes a step. The local clock does not care - measured
 * 4 us standard deviation bare and 5 us with six visual lanes firing, zero late ticks
 * either way, which is the two-core split doing its job. But the RECEIVE TIMESTAMP is
 * taken on that busy core, so the probes disagree about twice as much: median spread
 * 1204 us bare against 2187 us loaded.
 *
 * More samples looked like the honest answer to more noise, and eight of forty made it
 * WORSE, for a reason worth writing down: a window is not forty probes, it is forty
 * USABLE exchanges, and the usable rate is well under the probe rate. Widening the
 * window stretched the interval between corrections until the deck was coasting
 * between them, which is the same failure the hard RTT gate produced by a different
 * route. Six of twenty-four is measured good - 22 of 22 samples inside 500 us, worst
 * 264 us - and the way to improve on it is to raise the usable rate, not the window.
 *
 * Which is what removing the acknowledgement requirement in on_sent did: it roughly
 * doubled the usable exchanges at a stroke. Fixing the supply beat enlarging the
 * bucket. */
#define WINDOW 24
#define KEEP    6
#define PROBE_EVERY_US 50000           /* twenty probes a second, following */

/* THE FLOOR: the fastest round trip ever seen on this pair of decks. It is the
 * closest thing available to the true flight time, so it is the yardstick for
 * whether a window is worth believing.
 *
 * Six windows in eight were landing inside 500 us with the round trip alone, and
 * two were near 2 ms - windows where every one of the sixteen probes was delayed,
 * so even the fastest was slow and the half-RTT assumption was wrong for all of
 * them. Discarding those is what NTP and PTP do, and it is safe here because the
 * local timer is excellent on its own: a deck that skips a correction coasts at
 * 3 us standard deviation, which is better than any correction it could have
 * computed from bad data. */
static int64_t s_floor_rtt;
static uint32_t s_skipped;

/* A TEMPO CHANGE IS THE ONE MOMENT HALF GAIN IS WRONG.
 *
 * Measured: in steady state every window landed within 73 us, and the only two
 * readings over 300 us in a whole run were the first one after the leader changed
 * tempo - once at -570 us. The cause is not the radio. seq_bpm re-anchors the grid
 * so the CURRENT pulse keeps its place in absolute time, which is right, but the two
 * decks re-anchor at the two different moments they each heard about the change, and
 * that difference modulo the new pulse is the error. It is a step, not noise.
 *
 * Converging on a step in halves takes several windows. So: notice the tempo moved,
 * throw away the window in progress because half of it was measured against the old
 * grid, and take the next corroborated window whole. A step deserves a step. */
static uint16_t s_seen_bpm;
static bool     s_snap;

/* WHERE THE EXCHANGES GO, so a stalled estimator can be read rather than guessed
 * at. The frozen-numbers failure this was written for looked identical from outside
 * to "the radio is quiet", and the two want opposite fixes. */
static uint32_t s_replies;      /* replies that arrived at all            */
static uint32_t s_stale;        /* ... for a probe the table has forgotten */
static uint32_t s_dup;          /* ... a retransmission of one already seen */
static uint32_t s_lost;         /* probes the leader never acknowledged    */
static uint32_t s_windows;      /* windows that produced a correction     */

/* The KEEP lowest-round-trip exchanges of the window, and what each said the
 * offset was. Unsorted - six elements is small enough that finding the worst by
 * walking them is cheaper than keeping them in order. */
static struct { int64_t rtt; int32_t off; } s_keep[KEEP];
static int     s_nkeep;
static int     s_nsample;
static int32_t s_spread;               /* how far the kept six disagreed    */
static int64_t s_last_probe;
static uint8_t s_leader[6];
static bool    s_have_leader;
static uint8_t s_mac[6];               /* ours, to pick our replies out of the air */

/* ONE EXCHANGE, HALF-FILLED BY EITHER CALLBACK, AND WHY IT CANNOT BE SIMPLER.
 *
 * A round trip needs two facts from two different callbacks: when the probe actually
 * left, which only the SEND callback knows, and when the reply arrived, which only the
 * RECEIVE callback knows. The first version did the arithmetic in the receive callback
 * and required the departure to be there already - and that is a RACE between two
 * callbacks on the same task, with no ordering guarantee whatsoever.
 *
 * It lost the race 87 per cent of the time. Measured: 1452 of 1675 replies discarded
 * for having no recorded departure, three corrections in a minute, and a round-trip
 * floor of 26 ms that was really the dispatch delay of a send callback. Worse, it
 * measured beautifully first - fifteen per cent discarded, 22 of 22 samples inside
 * 500 us - so the design was confirmed by luck and the luck did not hold. A
 * correctness argument that rests on which of two callbacks runs first is not an
 * argument.
 *
 * So neither callback owns the calculation. Each fills in its half and whichever
 * completes the pair does the arithmetic. Both run on the WiFi task, so they are
 * serialised against each other and the table needs no lock; and the send callback is
 * always delivered, so a reply that arrives first is never orphaned.
 *
 * Four slots: at twenty probes a second a slot comes round again after 200 ms, and a
 * round trip is one or two. */
#define AIRRING 4
typedef struct {
    uint32_t tag;                  /* which probe. 0 = the slot is idle        */
    int64_t  air;                  /* it left. 0 = the send callback is owed   */
    int64_t  rx;                   /* the reply came. 0 = the reply is owed    */
    bool     lost;                 /* the send was not acknowledged            */
    int32_t  resid_us;             /* what the leader reported, with the reply */
    int32_t  ahead_us;
    uint16_t bpm;
} exch_t;
static volatile exch_t s_exch[AIRRING];
static uint32_t s_tag;                 /* next tag to hand out              */

/* WHICH PROBE THE NEXT SEND CALLBACK IS ABOUT, AS A QUEUE RATHER THAN A GUESS.
 *
 * This read `s_tag` - the most recently handed-out tag - on the reasoning that only one
 * probe is ever outstanding. That reasoning is wrong, and wrong in a way that inflated
 * every measurement: the send callback can be dispatched after the NEXT probe has
 * already been queued, and it then stamped that probe's slot with the previous probe's
 * departure. The error is one probe interval, fifty milliseconds, and it showed up as a
 * round-trip floor of 24 ms - a number with no physical meaning, since a broadcast
 * frame of twenty-six bytes is on the air in well under one.
 *
 * Send callbacks are delivered one per send and in order, so a queue matches them
 * exactly. Four deep, the same as the exchange table. */
static volatile uint32_t s_pend[AIRRING];
static volatile uint8_t  s_pend_head, s_pend_tail;

static volatile exch_t *slot_for(uint32_t tag)
{
    volatile exch_t *e = &s_exch[tag % AIRRING];
    return (e->tag == tag) ? e : NULL;
}

static void keep_sample(int64_t rtt, int32_t off)
{
    int slot;
    if (s_nkeep < KEEP) {
        slot = s_nkeep++;
    } else {
        int worst = 0;
        for (int i = 1; i < KEEP; i++) {
            if (s_keep[i].rtt > s_keep[worst].rtt) { worst = i; }
        }
        if (rtt >= s_keep[worst].rtt) { return; }   /* not good enough to keep */
        slot = worst;
    }
    s_keep[slot].rtt = rtt;
    s_keep[slot].off = off;
}

/* The median of what the kept exchanges said, and how much they disagreed. */
static int32_t keep_verdict(int32_t *spread_out)
{
    int32_t v[KEEP];
    const int n = s_nkeep;
    for (int i = 0; i < n; i++) { v[i] = s_keep[i].off; }
    for (int i = 1; i < n; i++) {               /* insertion sort, six elements */
        const int32_t k = v[i];
        int j = i - 1;
        while (j >= 0 && v[j] > k) { v[j + 1] = v[j]; j--; }
        v[j + 1] = k;
    }
    *spread_out = v[n - 1] - v[0];
    if (n & 1) { return v[n / 2]; }
    return (int32_t)(((int64_t)v[n / 2 - 1] + (int64_t)v[n / 2]) / 2);
}

/* A probe the leader has received and not yet answered. One is enough: a follower
 * that probed again before being answered would only be measuring its own
 * impatience. Answered from the main loop, with the holding time reported, so a
 * slow reply costs accuracy nothing - PTP calls it residence time. */
static struct {
    bool     waiting;
    uint8_t  mac[6];
    uint32_t tag;
    int64_t  got_us;
} s_reply;

/* Who has been heard from, and when. Only the count is reported; the addresses
 * exist so two packets from one deck are not counted as two decks. */
static struct { uint8_t mac[6]; int64_t seen_us; } s_peer[PEERS_MAX];

static void note_peer(const uint8_t *mac)
{
    const int64_t now = esp_timer_get_time();
    int free_slot = -1;
    for (int i = 0; i < PEERS_MAX; i++) {
        if (memcmp(s_peer[i].mac, mac, 6) == 0) {
            s_peer[i].seen_us = now;
            return;
        }
        if (s_peer[i].seen_us == 0 && free_slot < 0) { free_slot = i; }
    }
    if (free_slot >= 0) {
        memcpy(s_peer[free_slot].mac, mac, 6);
        s_peer[free_slot].seen_us = now;
    }
}

static int count_peers(void)
{
    const int64_t now = esp_timer_get_time();
    int n = 0;
    for (int i = 0; i < PEERS_MAX; i++) {
        if (s_peer[i].seen_us == 0) { continue; }
        if (now - s_peer[i].seen_us > PEER_STALE_US) {
            s_peer[i].seen_us = 0;                  /* gone quiet */
            memset(s_peer[i].mac, 0, 6);
            continue;
        }
        n++;
    }
    return n;
}

/* BOTH HALVES ARE IN: do the arithmetic. Called from whichever of the two callbacks
 * completed the pair, and from nowhere else. */
static void settle(volatile exch_t *e)
{
    if (e->lost || e->air == 0 || e->rx == 0) {
        return;                         /* still owed a half, or the send failed */
    }
    const int64_t now = e->rx;
    (void)now;
        /* THE ROUND TRIP, ENTIRELY IN OUR OWN CLOCK.
         *
         *   air           the probe's own departure, from the send callback
         *   now           we got the reply
         *   resid_us      how long the leader held it
         *
         * so rtt is the time on the wire, both ways, with the leader's own delay
         * taken out and - new in version 3 - our own transmit queueing taken out
         * too, because `air` is when the frame left rather than when we asked for
         * it. Half of it is the one-way flight, which is the assumption this whole
         * method rests on, and it is far more nearly true now that the one clearly
         * one-sided delay has been removed from the sum.
         *
         * The send callback fires on transmit-and-acknowledge, so `air` is later
         * than the true egress by one short acknowledgement - tens of microseconds,
         * the same every time, and therefore half of that as a fixed bias in the
         * result. It is under a tenth of the error budget and buying it back would
         * cost a second packet. */
        const int64_t rtt = (e->rx - e->air) - (int64_t)e->resid_us;
        if (rtt < 0 || rtt > 200000) {
            return;                     /* nonsense, or a very bad moment */
        }
        /* The leader sent this reply at our time (now - rtt/2). It reported that
         * its pulse `tick` was due `ahead_us` before that send. */
        const int64_t reply_sent_here = e->rx - rtt / 2;
        const int64_t due_here = reply_sent_here - (int64_t)e->ahead_us;

        /* Where WE have that pulse. */
        uint32_t ourtick = 0; int64_t ourdue = 0; int ourbpm = 0;
        seq_timebase(&ourtick, &ourdue, &ourbpm);
        if (ourbpm <= 0) { return; }
        const int64_t per = 60000000LL / ourbpm / 96;
        if (per <= 0) { return; }
        /* PHASE IS THE SUB-PULSE PART, AND ONE MODULO IS THE WHOLE OF IT.
         *
         * A whole-pulse disagreement is a different bar, not a phase error, so the
         * difference folds into plus or minus half a pulse. Version 2 folded it with
         * a `while` loop after subtracting (their tick - ours) * per, and both
         * halves of that were a mistake. The tick term is an exact multiple of the
         * pulse, so it vanishes under the fold and never affected the answer - but
         * two decks that started playing minutes apart differ by a hundred thousand
         * ticks, so the loop it fed ran a hundred thousand times, inside a radio
         * callback. A modulo is the same answer in constant time. */
        int64_t off = (due_here - ourdue) % per;
        if (off >  per / 2) { off -= per; }
        if (off < -per / 2) { off += per; }
        (void)ourtick;

        if (s_floor_rtt == 0 || rtt < s_floor_rtt) {
            s_floor_rtt = rtt;
        }
        keep_sample(rtt, (int32_t)off);
        s_nsample++;
        if (s_nsample >= WINDOW) {
            s_nsample = 0;
            if (s_nkeep < 3) { s_nkeep = 0; s_skipped++; return; }
            int32_t spread = 0;
            const int32_t med = keep_verdict(&spread);
            s_nkeep  = 0;
            s_spread = spread;
            /* TRUST IN PROPORTION TO HOW MUCH THE KEPT PROBES AGREED.
             *
             * Two gates were tried before this one. A hard gate at twice the RTT
             * floor starved, because the floor is the fastest exchange EVER seen and
             * one lucky probe sets a standard that is not coming back: measured, it
             * turned a 184 us worst case into 2400 us, worse than no gate at all.
             * Graduating the gain off the floor instead of gating on it fixed the
             * starving but kept the floor's real flaw, which is that it judges this
             * window by a memory.
             *
             * Agreement has no memory. Six lightly-queued probes that all put the
             * offset within 300 us of each other are six measurements corroborating
             * one another right now, and that is the definition of a window worth
             * believing. It also degrades in the right direction: a window whose
             * probes disagree gets a small gain rather than a veto, so corrections
             * cannot starve, and the local timer coasting at 3 us standard deviation
             * means a skipped window costs almost nothing anyway. */
            int32_t gain_div;
            if (s_snap && spread <= 300) { gain_div = 1; s_snap = false; }
            else if (spread <= 300)  { gain_div = 2; }
            else if (spread <= 1200) { gain_div = 4; }
            else if (spread <= 4000) { gain_div = 8; }
            else { s_skipped++; return; }

            /* The floor no longer gates anything - it is reported by '>sync' as
             * the best flight time this pair of decks has managed, which is the
             * number that says whether the radio or the estimator is the limit. It
             * still relaxes, because a figure that can only ever fall stops
             * describing the room after the first lucky packet. */
            s_floor_rtt += s_floor_rtt / 64 + 1;

            s_windows++;
            s_err_us = med;
            seq_nudge_by(med / gain_div, (int)e->bpm);
        }
}

/* ESP-NOW hands this to us on the WiFi task. It must not block and must not do
 * anything a radio callback should not: all it does is arithmetic and one call
 * into the sequencer, which itself only slides a 64-bit integer. */
static void on_recv(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    const int64_t now = esp_timer_get_time();
    if (len != (int)sizeof(ens_pkt_t) || info == NULL) {
        return;
    }
    ens_pkt_t p;
    memcpy(&p, data, sizeof p);
    if (p.magic != ENS_MAGIC) {
        return;
    }
    note_peer(info->src_addr);
    s_heard++;

    if (s_role == ENSEMBLE_FOLLOW && p.running && p.bpm > 0) {
        if (s_seen_bpm != 0 && p.bpm != s_seen_bpm) {
            s_snap    = true;
            s_nkeep   = 0;      /* measured against a grid that no longer exists */
            s_nsample = 0;
        }
        s_seen_bpm = p.bpm;
    }

    switch (p.kind) {
    case ENS_BEACON:
        /* Remember who is leading, so a follower knows where to send its probes.
         * The beacon is the only broadcast in the protocol and this is all it is
         * for - the tempo it carries is a convenience for a deck that has not
         * completed a round trip yet. */
        if (s_role == ENSEMBLE_FOLLOW) {
            memcpy(s_leader, info->src_addr, 6);
            s_have_leader = true;
            /* TEMPO IS FOLLOWED AT ONCE, PHASE IS NOT.
             *
             * A tempo is a decision somebody made; a phase is a measurement, and
             * the measurement is gated on the round trip being clean. Most windows
             * are rejected by that gate - measured, thirty skipped in fifty
             * seconds - which is fine for phase because the local timer coasts at
             * 3 us, and would be badly wrong for tempo: a deck that took several
             * seconds to notice the room had sped up is a deck nobody plays with.
             *
             * seq_nudge_by does nothing unless the tempo actually differs, so this
             * is free on the other seven beacons a second. */
            if (p.running && p.bpm > 0) {
                seq_nudge_by(0, (int)p.bpm);
            }
        }
        break;

    case ENS_PROBE:
        /* Hold it for the main loop to answer. Overwriting an unanswered probe is
         * correct: the newest one is the only one whose reply can still be
         * useful. */
        if (s_role == ENSEMBLE_LEAD) {
            memcpy(s_reply.mac, info->src_addr, 6);
            s_reply.tag     = p.tag;
            s_reply.got_us  = now;
            s_reply.waiting = true;
        }
        break;

    case ENS_REPLY: {
        if (s_role != ENSEMBLE_FOLLOW || !p.running) {
            break;
        }
        if (p.whom[0] != s_mac[4] || p.whom[1] != s_mac[5]) {
            break;                      /* somebody else's answer */
        }
        s_replies++;
        volatile exch_t *e = slot_for(p.tag);
        if (e == NULL) {
            s_stale++;                  /* answered a probe we have forgotten */
            break;
        }
        if (e->rx != 0) {
            /* A DUPLICATE, AND ENTIRELY NORMAL. If our acknowledgement of the reply
             * goes missing the leader's MAC retransmits it, so the same reply arrives
             * twice - measured at roughly four in ten while the deck was drawing
             * hard. The first one is the one that was on the air first, so the second
             * is dropped rather than allowed to overwrite a good measurement. It is
             * counted apart from a forgotten probe because the two mean opposite
             * things about the link. */
            s_dup++;
            break;
        }
        e->rx       = now;
        e->resid_us = p.resid_us;
        e->ahead_us = p.ahead_us;
        e->bpm      = p.bpm;
        settle(e);
        break;
    }
    default:
        break;
    }
}

/* THE ONLY THING THIS EXISTS FOR: the moment the probe left.
 *
 * esp_now_send returns as soon as the frame is queued. Between that and the air
 * there is the WiFi task waking up, then CSMA backoff waiting for a quiet channel -
 * unbounded in principle, milliseconds in practice on a busy band. All of it used to
 * land inside the measured round trip, on the outbound leg only, and halving a sum
 * with a one-sided delay in it puts half that delay straight into the answer.
 *
 * Runs on the WiFi task, same as the receive callback, so the two are serialised and
 * the ring needs no lock. */
static void on_sent(const esp_now_send_info_t *tx_info, esp_now_send_status_t status)
{
    (void)tx_info;
    if (s_role != ENSEMBLE_FOLLOW) {
        return;                 /* beacons and replies are not timed */
    }
    const int64_t at = esp_timer_get_time();
    if (s_pend_head == s_pend_tail) {
        return;                         /* a send we did not queue: not ours */
    }
    const uint32_t tag = s_pend[s_pend_head % AIRRING];
    s_pend_head++;
    volatile exch_t *e = slot_for(tag);
    if (e == NULL || e->air != 0) { return; }
    /* AN UNACKNOWLEDGED SEND IS NOT A USABLE DEPARTURE. A unicast that draws no
     * acknowledgement has been retried, so this callback is timing the last attempt
     * rather than whichever one landed - and a reply may still arrive, which is how
     * that first looked like a bug worth "fixing" by accepting it. Accepting it puts
     * milliseconds into the answer. Dropping the exchange costs the fraction the air
     * was losing anyway, and '>sync' reports the count so a bad room says so. */
    if (status == ESP_NOW_SEND_SUCCESS) {
        e->air = at;
        settle(e);              /* does nothing if the reply is still owed */
    } else {
        e->lost = true;
        s_lost++;
    }
}

esp_err_t ensemble_set(ensemble_role_t role)
{
    if (role == s_role) {
        return ESP_OK;
    }
    if (role == ENSEMBLE_OFF) {
        if (s_up) {
            esp_now_unregister_recv_cb();
            esp_now_unregister_send_cb();
            esp_now_deinit();
            s_up = false;
        }
        s_role = ENSEMBLE_OFF;
        memset(s_peer, 0, sizeof s_peer);
        return ESP_OK;
    }

    if (!s_up) {
        /* ESP-NOW NEEDS THE RADIO ON BUT NOT CONNECTED. Station mode with no
         * association is enough, and is what makes an ensemble need no router and
         * no password. If the deck is already on a network this leaves it alone -
         * ESP-NOW and a normal connection share the radio happily as long as the
         * channel is the one the connection is using, which it is because we never
         * set one. */
        esp_err_t err = esp_netif_init();
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) { return err; }
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        err = esp_wifi_init(&cfg);
        if (err != ESP_OK && err != ESP_ERR_WIFI_INIT_STATE) {
            /* Already initialised by the network code is fine. */
            if (err != ESP_ERR_INVALID_STATE) { return err; }
        }
        (void)esp_wifi_set_mode(WIFI_MODE_STA);
        (void)esp_wifi_start();
        /* POWER SAVE OFF, AND THIS IS THE LARGEST SINGLE TIMING FIX IN THE FILE.
         *
         * A station defaults to WIFI_PS_MIN_MODEM: the radio sleeps between beacon
         * intervals and wakes to check for traffic. For a browser that is free battery
         * life. For a clock it means a transmit waits for the next wake and a receive
         * is noticed at one, so both ends of every measurement are quantised to
         * something on the order of tens of milliseconds.
         *
         * Measured, with everything else in this file already correct: a round-trip
         * floor of 22 ms and one reply a second out of twenty probes. Every earlier
         * explanation for the variance - transmit queueing, acknowledgement retries,
         * callback ordering - was real and was worth fixing, and all of them together
         * were smaller than this. The numbers moved by orders of magnitude when a
         * measurement finally pointed here rather than at the arithmetic.
         *
         * A deck is mains- or battery-powered and drawing a screen; the radio's sleep
         * was never the interesting power saving, and a clock is the thing this
         * instrument rests on. */
        (void)esp_wifi_set_ps(WIFI_PS_NONE);

        err = esp_now_init();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_now_init: %s", esp_err_to_name(err));
            return err;
        }
        esp_now_peer_info_t peer = { 0 };
        memcpy(peer.peer_addr, BROADCAST, 6);
        peer.channel = 0;              /* whatever channel we are already on */
        peer.encrypt = false;
        (void)esp_now_add_peer(&peer);
        if (esp_now_register_recv_cb(on_recv) != ESP_OK) {
            esp_now_deinit();
            return ESP_FAIL;
        }
        if (esp_now_register_send_cb(on_sent) != ESP_OK) {
            esp_now_unregister_recv_cb();
            esp_now_deinit();
            return ESP_FAIL;
        }
        (void)esp_wifi_get_mac(WIFI_IF_STA, s_mac);
        s_up = true;
    }
    s_role   = role;
    s_heard  = 0;
    s_err_us = 0;
    s_nkeep  = 0;
    s_nsample = 0;
    s_spread = 0;
    s_seen_bpm = 0;
    s_snap   = false;
    s_replies = 0; s_stale = 0; s_lost = 0; s_dup = 0; s_windows = 0;
    memset((void *)s_exch, 0, sizeof s_exch);
    s_pend_head = 0; s_pend_tail = 0;
    ESP_LOGI(TAG, "%s", role == ENSEMBLE_LEAD ? "leading" : "following");
    return ESP_OK;
}

ensemble_role_t ensemble_role(void) { return s_role; }

int64_t ensemble_floor_rtt(void) { return s_floor_rtt; }
uint32_t ensemble_skipped(void)   { return s_skipped; }
int32_t ensemble_spread(void)     { return s_spread; }
void ensemble_counts(uint32_t *replies, uint32_t *stale, uint32_t *lost,
                     uint32_t *dup, uint32_t *windows)
{
    if (replies != NULL) { *replies = s_replies; }
    if (stale   != NULL) { *stale   = s_stale;   }
    if (lost    != NULL) { *lost    = s_lost;    }
    if (dup     != NULL) { *dup     = s_dup;     }
    if (windows != NULL) { *windows = s_windows; }
}

bool ensemble_state(int *peers, int32_t *err_us, uint32_t *heard)
{
    if (s_role == ENSEMBLE_OFF) {
        return false;
    }
    if (peers != NULL)  { *peers = count_peers(); }
    if (err_us != NULL) { *err_us = s_err_us; }
    if (heard != NULL)  { *heard = s_heard; s_heard = 0; }
    return true;
}

/* Fill in the parts of a packet that describe this deck's clock. */
static void stamp(ens_pkt_t *p, uint8_t kind)
{
    uint32_t tick = 0;
    int64_t due = 0;
    int bpm = 0;
    seq_timebase(&tick, &due, &bpm);
    memset(p, 0, sizeof *p);
    p->magic    = ENS_MAGIC;
    p->kind     = kind;
    p->tick     = tick;
    p->bpm      = (uint16_t)bpm;
    p->running  = seq_running() ? 1u : 0u;
    p->role     = (uint8_t)s_role;
    /* Last, so it is as close to the send as it can be. The receiver subtracts
     * it, so what matters is that it is measured against the same clock as the
     * pulse it describes - not that it is small. */
    p->ahead_us = (int32_t)(esp_timer_get_time() - due);
}

void ensemble_service(void)
{
    if (s_role == ENSEMBLE_OFF || !s_up) {
        return;
    }
    const int64_t now = esp_timer_get_time();

    /* ANSWER A PROBE FIRST, and report how long it waited. Doing this from the
     * main loop rather than the radio callback is what keeps a radio send out of
     * a callback, and reporting the residence time is what makes that free. */
    if (s_role == ENSEMBLE_LEAD && s_reply.waiting) {
        ens_pkt_t r;
        stamp(&r, ENS_REPLY);
        r.tag      = s_reply.tag;
        r.whom[0]  = s_reply.mac[4];
        r.whom[1]  = s_reply.mac[5];
        r.resid_us = (int32_t)(esp_timer_get_time() - s_reply.got_us);
        s_reply.waiting = false;
        if (!esp_now_is_peer_exist(s_reply.mac)) {
            esp_now_peer_info_t peer = { 0 };
            memcpy(peer.peer_addr, s_reply.mac, 6);
            peer.channel = 0;
            peer.encrypt = false;
            (void)esp_now_add_peer(&peer);
        }
        (void)esp_now_send(s_reply.mac, (const uint8_t *)&r, sizeof r);
    }

    if (s_role == ENSEMBLE_LEAD) {
        if (now - s_last_send < SEND_EVERY_US) {
            return;
        }
        s_last_send = now;
        ens_pkt_t p;
        stamp(&p, ENS_BEACON);
        /* Unchecked on purpose: a dropped beacon costs one announcement out of
         * eight a second, and a deck that logged every radio hiccup would spend a
         * performance logging. */
        (void)esp_now_send(BROADCAST, (const uint8_t *)&p, sizeof p);
        return;
    }

    /* FOLLOWING: probe the leader, ten times a second. The probe is also the
     * hello that lets the leader count the room, so there is no separate beat. */
    if (!s_have_leader || now - s_last_probe < PROBE_EVERY_US) {
        return;
    }
    s_last_probe = now;
    if (!esp_now_is_peer_exist(s_leader)) {
        esp_now_peer_info_t peer = { 0 };
        memcpy(peer.peer_addr, s_leader, 6);
        peer.channel = 0;
        peer.encrypt = false;
        (void)esp_now_add_peer(&peer);
    }
    ens_pkt_t p;
    stamp(&p, ENS_PROBE);
    /* The tag is set, and s_tag_flight with it, BEFORE the send - the callback can
     * fire the moment esp_now_send is called. Zero is not handed out so the callback
     * has a value meaning "nothing outstanding". */
    if (++s_tag == 0) { s_tag = 1; }
    p.tag = s_tag;
    /* CLAIM THE SLOT BEFORE THE SEND, because the send callback can fire the moment
     * esp_now_send is called and it looks the slot up by tag. */
    volatile exch_t *e = &s_exch[s_tag % AIRRING];
    e->tag = 0;                        /* nobody may settle a half-built slot */
    e->air = 0; e->rx = 0; e->lost = false;
    e->resid_us = 0; e->ahead_us = 0; e->bpm = 0;
    e->tag = s_tag;
    /* Queued before the send, because the callback can fire inside esp_now_send. */
    s_pend[s_pend_tail % AIRRING] = s_tag;
    s_pend_tail++;
    (void)esp_now_send(s_leader, (const uint8_t *)&p, sizeof p);
}
