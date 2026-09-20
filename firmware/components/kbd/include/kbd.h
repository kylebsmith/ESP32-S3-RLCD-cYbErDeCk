/*
 * BLE HID keyboard host (HOGP over NimBLE).
 *
 * The ESP32-S3 has no Bluetooth Classic radio at all, so this is the only
 * transport a wireless keyboard can use. docs/OS.md records that an earlier
 * analysis claiming the Rii 518BT is Classic-only was refuted by direct
 * observation - it advertises the HID service over LE.
 *
 * Bonds live in NVS and reconnection is automatic. kbd_forget_all() is wired
 * to a long hold on the KEY button, because with no screen affordance that is
 * the only way out of a broken pairing.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

typedef enum {
    KBD_EV_CHAR = 0,     /* ch holds a printable character */
    KBD_EV_ENTER,
    KBD_EV_BACKSPACE,
    KBD_EV_TAB,
    KBD_EV_ESC,
    KBD_EV_LEFT,
    KBD_EV_RIGHT,
    KBD_EV_UP,
    KBD_EV_DOWN,
    KBD_EV_CONNECTED,
    KBD_EV_DISCONNECTED,
} kbd_ev_type_t;

typedef struct {
    kbd_ev_type_t type;
    char          ch;
    bool          repeat;    /* synthesised by us, not sent by the keyboard */
} kbd_event_t;

esp_err_t kbd_init(void);

/* Block up to timeout_ms for an event. */
bool kbd_poll(kbd_event_t *ev, uint32_t timeout_ms);

bool kbd_connected(void);

/* The pairing-recovery gesture: drop every bond and rescan. */
void kbd_forget_all(void);

/* Push an event from another source. The USB serial console uses this, which
 * makes the deck usable over the cable when no keyboard is paired - and makes
 * the whole editor path testable without a radio. */
void kbd_inject(const kbd_event_t *ev);

/* Printable name of what we are doing, for the status line. */
const char *kbd_state_name(void);
