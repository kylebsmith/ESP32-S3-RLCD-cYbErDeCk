/*
 * Bytes from a serial line to key events. Pure, transport-free, host-testable.
 *
 * WHY THIS IS SPLIT OUT. The serial keyboard currently reads from
 * USB-Serial-JTAG. USB MIDI needs the USB peripheral, which means the console
 * and this keyboard both move onto a TinyUSB CDC interface - a different
 * transport with the same bytes. Separating the mapping from the read means
 * that migration cannot quietly change what a key does, and a host check can
 * assert the mapping without a board.
 *
 * The one mapping nobody would guess, and the one most likely to be lost in a
 * transport change: A TERMINAL SENDS CR FOR ENTER, SO LF IS FREE, and LF is
 * the only way to reach Ctrl+Enter - the Run verb - down a cable. Lose that
 * and the deck becomes unable to run a command over serial, which is how
 * every test in this project is driven.
 */
#ifndef SERIALKBD_MAP_H
#define SERIALKBD_MAP_H

#include <stdbool.h>
#include <stdint.h>

/* Kept in step with kbd.h by the host check, which includes both. */
typedef struct {
    int     type;        /* kbd_ev_type_t                      */
    char    ch;
    uint8_t mods;
    bool    emit;        /* false: byte consumed, nothing yet  */
} skb_ev_t;

/* Escape-sequence state, owned by the caller so the mapper stays pure. */
typedef struct { int esc; } skb_state_t;

/* Values mirror kbd.h. The host check asserts they agree, so a renumbering
 * there cannot silently change what this produces. */
#define SKB_CHAR       0
#define SKB_ENTER      1
#define SKB_BACKSPACE  2
#define SKB_TAB        3
#define SKB_ESC        4
#define SKB_LEFT       5
#define SKB_RIGHT      6
#define SKB_UP         7
#define SKB_DOWN       8
#define SKB_MOD_LCTRL  0x01

static inline skb_ev_t skb_feed(skb_state_t *st, uint8_t b)
{
    skb_ev_t e = { 0, 0, 0, false };

    /* Arrow keys arrive as ESC [ A..D from any terminal. */
    if (st->esc == 1) {
        st->esc = (b == '[') ? 2 : 0;
        if (st->esc == 0 && b == 0x1B) {
            e.type = SKB_ESC; e.emit = true;
        }
        return e;
    }
    if (st->esc == 2) {
        st->esc = 0;
        switch (b) {
        case 'A': e.type = SKB_UP;    e.emit = true; break;
        case 'B': e.type = SKB_DOWN;  e.emit = true; break;
        case 'C': e.type = SKB_RIGHT; e.emit = true; break;
        case 'D': e.type = SKB_LEFT;  e.emit = true; break;
        default: break;
        }
        return e;
    }

    switch (b) {
    case 0x1B: st->esc = 1;                         break;
    case '\r': e.type = SKB_ENTER;     e.emit = true; break;
    case '\n':
        /* CR is Enter, so LF is free - and it is the only way to reach
         * Ctrl+Enter, the Run verb, down a cable. */
        e.type = SKB_ENTER; e.mods = SKB_MOD_LCTRL; e.emit = true;
        break;
    case 0x08:
    case 0x7F: e.type = SKB_BACKSPACE; e.emit = true; break;
    case '\t': e.type = SKB_TAB;       e.emit = true; break;
    default:
        if (b >= 0x20 && b < 0x7F) {
            e.type = SKB_CHAR; e.ch = (char)b; e.emit = true;
        } else if (b >= 1 && b <= 26) {
            /* A terminal sends Ctrl-A..Ctrl-Z as bytes 1..26. Turning them
             * back into a character plus a modifier means the serial keyboard
             * reaches the same chords as the BLE one, through the same event
             * taxonomy. */
            e.type = SKB_CHAR;
            e.ch   = (char)('a' + b - 1);
            e.mods = SKB_MOD_LCTRL;
            e.emit = true;
        }
        break;
    }
    return e;
}

#endif /* SERIALKBD_MAP_H */
