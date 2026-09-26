/*
 * An ensemble of decks, sharing one clock.
 *
 * WHY ESP-NOW AND NOT ESP-MESH. Mesh builds a routing tree and forwards packets
 * hop by hop, so latency grows with the depth of the tree and every deck's timing
 * depends on where it happens to sit in it. ESP-NOW is connectionless: a
 * broadcast goes out on the air once and every deck in range hears it directly,
 * in about a millisecond, with no root, no association and no router. For a room
 * full of people playing together that is the right shape - and it means an
 * ensemble needs no network at all, which is one less thing to fail in a venue.
 *
 * It also needs no credentials, which matters: a deck that had to be told a
 * password to play with the deck next to it would be a deck nobody plays with.
 *
 * A SHARED CLOCK IS NOT A SHARED TICK, and this is the part worth understanding.
 * Sending every pulse would inherit the radio's jitter directly - several
 * milliseconds, which is audible on a hi-hat. So the leader broadcasts where it
 * IS a few times a second, and each follower runs its own hardware timer and
 * trims its own grid toward what it heard. The radio carries intent; the timer
 * keeps time. Each deck's clock stays as accurate as it was alone - measured at
 * 3 us standard deviation - and the ensemble agrees to within a fraction of a
 * pulse.
 *
 * That is also Ableton Link's model, deliberately. If Link is ever licensed and
 * ported it replaces the transport underneath this and nothing above changes.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

typedef enum {
    ENSEMBLE_OFF = 0,
    ENSEMBLE_LEAD,       /* broadcast this deck's clock */
    ENSEMBLE_FOLLOW,     /* trim this deck's clock to what it hears */
} ensemble_role_t;

/* Start or stop. Safe to call repeatedly with the same role. */
esp_err_t ensemble_set(ensemble_role_t role);
ensemble_role_t ensemble_role(void);

/* What a player needs to see to know whether the room is together:
 *   peers  how many other decks have been heard from lately
 *   err_us the last phase error, before correction - 0 when leading
 *   heard  packets received since this was last asked
 * Returns false when the ensemble is off. */
bool ensemble_state(int *peers, int32_t *err_us, uint32_t *heard);

/* Called from the main loop. Broadcasts when leading; does nothing otherwise.
 * Never called from the clock callback - a radio send is exactly the kind of
 * blocking docs/OS.md keeps out of there. */
void ensemble_service(void);

/* THE DIAGNOSTIC THAT MAKES THIS DEBUGGABLE. The floor is the fastest round trip
 * ever measured with the leader, which is the closest thing to the true flight
 * time - so it says what this link is capable of. `skipped` counts windows thrown
 * away for being too congested to trust; a rising count means the air is busy, not
 * that the clock is wrong. */
int64_t  ensemble_floor_rtt(void);
uint32_t ensemble_skipped(void);

/* How many pulses the leader's count is ahead of this deck's, from the last
 * exchange - the part of being together that the phase cannot see. A step is
 * 24 pulses and a 4/4 bar 384. False until a reply has been measured. */
bool ensemble_count_off(int32_t *pulses);

/* HOW FAR THE LAST WINDOW'S BEST PROBES DISAGREED, in microseconds, and the number
 * to watch if the phase is wandering. It is the estimator's own opinion of itself:
 * six lightly-queued exchanges that put the offset within 300 us of one another
 * corroborate each other, and the correction is trusted at half gain. A wide spread
 * with a low floor means the radio is fine and the receive timestamps are being
 * delayed; a wide spread with a high floor means the air is busy. */
int32_t  ensemble_spread(void);

/* WHERE THE EXCHANGES WENT, and the numbers exist because a stalled estimator looked
 * exactly like a quiet radio from outside - frozen error, no skips - and the two want
 * opposite fixes. `lost` counts probes the leader never acknowledged, which is the air
 * being busy. `stale` counts replies for a probe the table has already forgotten,
 * which would mean the round trip has grown past four probe intervals. `windows`
 * counts corrections actually applied: if it stops climbing while `replies` does not,
 * the estimator is the problem and not the room. */
void     ensemble_counts(uint32_t *replies, uint32_t *stale, uint32_t *lost,
                         uint32_t *dup, uint32_t *windows);
