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
#define ENS_MAGIC 0x4B4C5232u          /* "KLR2" - a deck clock, version 2 */

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
typedef enum {
    ENS_BEACON = 1,     /* leader -> everyone: I am here, and my tempo     */
    ENS_PROBE,          /* follower -> leader: what time is it, and echo t1 */
    ENS_REPLY,          /* leader -> follower: here, minus my turnaround   */
} ens_kind_t;

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint8_t  kind;
    uint32_t tick;      /* the sender's absolute pulse since play */
    int32_t  ahead_us;  /* sent_us - due_us: how late the send was */
    int64_t  echo_t1;   /* PROBE/REPLY: the follower's send time, echoed */
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

/* A window of round trips. Sixteen at ten probes a second is a correction about
 * every second and a half - faster than any tempo change a person makes, and slow
 * enough that one bad exchange cannot steer the clock. */
#define WINDOW 16
#define PROBE_EVERY_US 100000          /* ten probes a second, following */

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

static int64_t s_best_rtt;             /* smallest round trip this window    */
static int64_t s_best_offset;          /* THAT exchange's offset, our clock  */
static int     s_nsample;
static int64_t s_last_probe;
static uint8_t s_leader[6];
static bool    s_have_leader;

/* A probe the leader has received and not yet answered. One is enough: a follower
 * that probed again before being answered would only be measuring its own
 * impatience. Answered from the main loop, with the holding time reported, so a
 * slow reply costs accuracy nothing - PTP calls it residence time. */
static struct {
    bool    waiting;
    uint8_t mac[6];
    int64_t echo_t1;
    int64_t got_us;
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
            s_reply.echo_t1 = p.echo_t1;
            s_reply.got_us  = now;
            s_reply.waiting = true;
        }
        break;

    case ENS_REPLY: {
        if (s_role != ENSEMBLE_FOLLOW || !p.running) {
            break;
        }
        /* THE ROUND TRIP, ENTIRELY IN OUR OWN CLOCK.
         *
         *   t1            we sent the probe
         *   now           we got the reply
         *   resid_us      how long the leader held it
         *
         * so rtt is the time on the wire, both ways, with the leader's own delay
         * taken out. Half of it is the one-way flight - the assumption this whole
         * method rests on, and the reason the SMALLEST rtt is the sample to keep:
         * the fastest exchange is the one where that assumption is closest to
         * true. */
        const int64_t rtt = (now - p.echo_t1) - (int64_t)p.resid_us;
        if (rtt < 0 || rtt > 200000) {
            break;                      /* nonsense, or a very bad moment */
        }
        /* The leader sent this reply at our time (now - rtt/2). It reported that
         * its pulse `tick` was due `ahead_us` before that send. */
        const int64_t reply_sent_here = now - rtt / 2;
        const int64_t due_here = reply_sent_here - (int64_t)p.ahead_us;

        /* Where WE have that pulse. */
        uint32_t ourtick = 0; int64_t ourdue = 0; int ourbpm = 0;
        seq_timebase(&ourtick, &ourdue, &ourbpm);
        if (ourbpm <= 0) { break; }
        const int64_t per = 60000000LL / ourbpm / 96;
        int64_t off = due_here -
                      (ourdue + ((int64_t)p.tick - (int64_t)ourtick) * per);
        if (per > 0) {
            while (off >  per / 2) { off -= per; }
            while (off < -per / 2) { off += per; }
        }

        if (s_floor_rtt == 0 || rtt < s_floor_rtt) {
            s_floor_rtt = rtt;
        }
        if (s_nsample == 0 || rtt < s_best_rtt) {
            s_best_rtt    = rtt;
            s_best_offset = off;
        }
        s_nsample++;
        if (s_nsample >= WINDOW) {
            s_nsample = 0;
            /* TRUST IN PROPORTION TO HOW CLEAN THE WINDOW WAS, rather than
             * believing it or discarding it.
             *
             * A hard gate at twice the floor was tried and it is fragile in a way
             * that took a measurement to see: the floor is the fastest exchange
             * EVER, so one lucky probe sets an impossible standard, almost every
             * window is then discarded, and the clock coasts and drifts between
             * the rare accepted ones. Measured, that turned a 184 us worst case
             * into 2400 us - worse than no gate at all.
             *
             * So a clean window moves the clock half way, a middling one an
             * eighth, and only a hopeless one is dropped. Corrections never
             * starve, and a bad window can no longer do much harm because its
             * gain is small. */
            int32_t gain_div;
            if (s_best_rtt <= s_floor_rtt * 2 + 500)       { gain_div = 2; }
            else if (s_best_rtt <= s_floor_rtt * 4 + 1000) { gain_div = 8; }
            else { s_skipped++; break; }

            /* AND LET THE FLOOR RELAX. It can only ever fall, so a single fast
             * exchange would pin it for the rest of the performance and every
             * later window would be judged against a moment that is not coming
             * back. A few per cent a window lets it follow the room. */
            s_floor_rtt += s_floor_rtt / 64 + 1;

            s_err_us = (int32_t)s_best_offset;
            seq_nudge_by((int32_t)(s_best_offset / gain_div), (int)p.bpm);
        }
        break;
    }
    default:
        break;
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
        s_up = true;
    }
    s_role = role;
    s_heard = 0;
    s_err_us = 0;
    ESP_LOGI(TAG, "%s", role == ENSEMBLE_LEAD ? "leading" : "following");
    return ESP_OK;
}

ensemble_role_t ensemble_role(void) { return s_role; }

int64_t ensemble_floor_rtt(void) { return s_floor_rtt; }
uint32_t ensemble_skipped(void)   { return s_skipped; }

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
        r.echo_t1  = s_reply.echo_t1;
        r.resid_us = (int32_t)(esp_timer_get_time() - s_reply.got_us);
        s_reply.waiting = false;
        esp_now_peer_info_t peer = { 0 };
        memcpy(peer.peer_addr, s_reply.mac, 6);
        peer.channel = 0;
        peer.encrypt = false;
        if (!esp_now_is_peer_exist(s_reply.mac)) {
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
    /* t1 goes in last and is read back out of the reply: the follower never has
     * to remember which probe it is being answered about. */
    p.echo_t1 = esp_timer_get_time();
    (void)esp_now_send(s_leader, (const uint8_t *)&p, sizeof p);
}
