/*
 * The serial keymap, checked against the SHIPPING header.
 *
 * This exists because USB MIDI will move the console and this keyboard from
 * USB-Serial-JTAG onto a TinyUSB CDC interface. That is a transport change
 * that must not alter a single key, and the mapping most likely to be lost is
 * the one nobody would guess: a terminal sends CR for Enter, so LF is free,
 * and LF is the ONLY way to reach Ctrl+Enter - the Run verb - down a cable.
 * Every automated test in this project drives the deck that way.
 */
#include <stdio.h>
#include <string.h>

#include "serialkbd_map.h"

static int fails;

static void expect(const char *what, skb_ev_t e, bool emit, int type,
                   char ch, uint8_t mods)
{
    if (e.emit != emit || (emit && (e.type != type || e.ch != ch || e.mods != mods))) {
        printf("[FAIL] %s: emit=%d type=%d ch=%d mods=%02x "
               "(wanted emit=%d type=%d ch=%d mods=%02x)\n",
               what, e.emit, e.type, e.ch, e.mods, emit, type, ch, mods);
        fails++;
    }
}

int main(void)
{
    skb_state_t st = { 0 };

    /* THE ONE THAT MATTERS. CR inserts; LF runs. */
    expect("CR is plain Enter",  skb_feed(&st, '\r'), true, SKB_ENTER, 0, 0);
    expect("LF is Ctrl+Enter",   skb_feed(&st, '\n'), true, SKB_ENTER, 0,
           SKB_MOD_LCTRL);

    expect("printable",          skb_feed(&st, 'x'),  true, SKB_CHAR, 'x', 0);
    expect("the command sigil",  skb_feed(&st, '>'),  true, SKB_CHAR, '>', 0);
    expect("backspace 0x08",     skb_feed(&st, 0x08), true, SKB_BACKSPACE, 0, 0);
    expect("backspace 0x7F",     skb_feed(&st, 0x7F), true, SKB_BACKSPACE, 0, 0);
    expect("tab",                skb_feed(&st, '\t'), true, SKB_TAB, 0, 0);

    /* Ctrl-A..Ctrl-Z arrive as 1..26 and must become a letter plus Ctrl, so
     * the cable reaches the same chords as the BLE keyboard. */
    expect("Ctrl-O toggles out",  skb_feed(&st, 0x0F), true, SKB_CHAR, 'o',
           SKB_MOD_LCTRL);
    expect("Ctrl-A",              skb_feed(&st, 0x01), true, SKB_CHAR, 'a',
           SKB_MOD_LCTRL);
    expect("Ctrl-Z",              skb_feed(&st, 0x1A), true, SKB_CHAR, 'z',
           SKB_MOD_LCTRL);

    /* Arrows: ESC [ A..D, and the two lead bytes emit nothing. */
    expect("ESC consumed",  skb_feed(&st, 0x1B), false, 0, 0, 0);
    expect("[ consumed",    skb_feed(&st, '['),  false, 0, 0, 0);
    expect("up",            skb_feed(&st, 'A'),  true, SKB_UP, 0, 0);
    skb_feed(&st, 0x1B); skb_feed(&st, '[');
    expect("down",          skb_feed(&st, 'B'),  true, SKB_DOWN, 0, 0);
    skb_feed(&st, 0x1B); skb_feed(&st, '[');
    expect("right",         skb_feed(&st, 'C'),  true, SKB_RIGHT, 0, 0);
    skb_feed(&st, 0x1B); skb_feed(&st, '[');
    expect("left",          skb_feed(&st, 'D'),  true, SKB_LEFT, 0, 0);

    /* A lone ESC ESC is the Escape key; the state machine must not wedge. */
    skb_feed(&st, 0x1B);
    expect("ESC ESC is Escape", skb_feed(&st, 0x1B), true, SKB_ESC, 0, 0);
    expect("and it recovered",  skb_feed(&st, 'q'),  true, SKB_CHAR, 'q', 0);

    /* An unknown byte after ESC [ is swallowed, not turned into a character -
     * otherwise a terminal's Home/End would type stray letters into the
     * document. */
    skb_feed(&st, 0x1B); skb_feed(&st, '[');
    expect("unknown CSI swallowed", skb_feed(&st, 'H'), false, 0, 0, 0);
    expect("and it recovered",      skb_feed(&st, 'z'), true, SKB_CHAR, 'z', 0);

    /* Bytes with the high bit set are not characters on this device. */
    expect("high byte ignored", skb_feed(&st, 0xC3), false, 0, 0, 0);

    printf(fails ? "[FAIL] %d mapping(s) wrong\n" : "[PASS] serial keymap\n",
           fails);
    return fails != 0;
}
