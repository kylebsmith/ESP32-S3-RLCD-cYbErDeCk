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
    KBD_EV_HOME,
    KBD_EV_END,
    KBD_EV_CONNECTED,
    KBD_EV_DISCONNECTED,
} kbd_ev_type_t;

/* HID modifier bits, as they arrive in byte 0 of a report. */
#define KBD_MOD_LCTRL  0x01
#define KBD_MOD_LSHIFT 0x02
#define KBD_MOD_LALT   0x04
#define KBD_MOD_LGUI   0x08
#define KBD_MOD_RCTRL  0x10
#define KBD_MOD_RSHIFT 0x20
#define KBD_MOD_RALT   0x40
#define KBD_MOD_RGUI   0x80

#define KBD_CTRL  (KBD_MOD_LCTRL  | KBD_MOD_RCTRL)
#define KBD_SHIFT (KBD_MOD_LSHIFT | KBD_MOD_RSHIFT)
#define KBD_ALT   (KBD_MOD_LALT   | KBD_MOD_RALT)

/* Modifiers that mean "this is a command", as opposed to modifiers that mean
 * "this is a different character".
 *
 * RIGHT ALT IS NOT A COMMAND MODIFIER. On most compact keyboards - including
 * the Rii - the Fn key is delivered to the host as AltGr, and AltGr is how
 * half the characters on a small layout are reached at all. Treating it as a
 * command modifier meant that typing '>' arrived carrying RALT, was routed
 * into the chord handler, matched no binding, and was DISCARDED IN SILENCE.
 * The keyboard appeared to stop working the moment the owner typed the one
 * character the command syntax requires. */
#define KBD_COMMAND_MODS (KBD_CTRL | KBD_MOD_LALT)

typedef struct {
    kbd_ev_type_t type;
    char          ch;
    uint8_t       mods;      /* modifiers held when the key went down */
    bool          repeat;    /* synthesised by us, not sent by the keyboard */
} kbd_event_t;

/* NimBLE is initialised here because the keyboard needs it first. Anything
 * else wanting a GATT service registers through these: `gatt` runs after
 * nimble_port_init and before the host task starts, which is the only window
 * in which services may be added; `synced` runs once the controller is up and
 * is where advertising may begin. */
void kbd_set_ble_hooks(void (*gatt)(void), void (*synced)(void));

esp_err_t kbd_init(void);

/* Block up to timeout_ms for an event. */
bool kbd_poll(kbd_event_t *ev, uint32_t timeout_ms);

bool kbd_connected(void);

/* Whether Wi-Fi or ESP-NOW is using the radio. While it is, looking for a
 * keyboard takes a 30 ms window every 160 ms instead of all of the radio's
 * time - which left an access point receiving 10 datagrams in 280. */
void kbd_share_radio(bool shared);

/* Modifiers held right now, for a visible indicator. docs/OS.md: sticky or
 * invisible modifier state is a bug generator. */
uint8_t kbd_mods(void);

/* The pairing-recovery gesture: drop every bond and rescan.
 *
 * REACHABLE BY TYPING, NOT ONLY BY HOLDING A BUTTON. This was bound solely to a
 * two-second hold on KEY - undiscoverable, silent, all-or-nothing, and on a
 * board whose switch identities are still an OPEN ITEM in docs/ASSEMBLY.md it
 * was a gesture the owner could not reliably perform. '>kbd forget' is the same
 * action with a name. The hold stays as the way in when there is no keyboard to
 * type it with, which is the one case that matters. */
void kbd_forget_all(void);

/* How many keyboards are bonded. Zero means nothing has ever paired, which is
 * a different problem from a keyboard that is bonded and out of range - and
 * telling those two apart without this is guesswork. */
int kbd_bond_count(void);

/* Push an event from another source. The USB serial console uses this, which
 * makes the deck usable over the cable when no keyboard is paired - and makes
 * the whole editor path testable without a radio. */
void kbd_inject(const kbd_event_t *ev);

/* Called when a passkey must be shown so it can be typed on the keyboard.
 * Implemented by the UI; a weak default does nothing. */
void kbd_on_passkey(uint32_t passkey);

/* Printable name of what we are doing, for the status line. */
const char *kbd_state_name(void);
