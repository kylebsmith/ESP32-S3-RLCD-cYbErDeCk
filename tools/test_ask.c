/*
 * No secret on a line, and none on the screen.
 *
 * Ground rule 6: nothing secret ever enters a document. '>wifi', '>host' and
 * '>ssh' took their password as a word of the line, and a line is a document
 * line - journalled, mirrored to the SD card, copied to the owner's DGX. Now a
 * command asks, and firmware/main/ask.h takes the answer. This runs that exact
 * code, and the two pure halves beside it:
 *
 *   - the prompt shows a star for each character and never a character, hands
 *     over exactly what was typed, and wipes it on Enter and on Esc;
 *   - secret_line.h finds where a second word - the old password - starts, so
 *     it can be refused and cut from the line;
 *   - ssh_fp.h prints a host key's fingerprint exactly as `ssh-keygen -lf`
 *     does, so the owner can hold the deck's line against the host's.
 *
 * The check that fails on the old state is in tools/test_ui_text.c: the guide
 * taught '>wifi <ssid> <pass>' and '>host deck 12345678', and it fails on both.
 */
#include <stdio.h>
#include <string.h>

#include "ask.h"
#include "secret_line.h"
#include "ssh_fp.h"

static int fails;

#define CHECK(cond, ...) do { if (!(cond)) { printf("[FAIL] " __VA_ARGS__); \
    printf("\n"); fails++; } else { printf("[ ok ] " __VA_ARGS__); printf("\n"); } } while (0)

static int key(ask_t *a, char c, unsigned mods)
{
    const kbd_event_t ev = { .type = KBD_EV_CHAR, .ch = c, .mods = (uint8_t)mods };
    return ask_feed(a, &ev);
}

static int press(ask_t *a, kbd_ev_type_t t)
{
    const kbd_event_t ev = { .type = t };
    return ask_feed(a, &ev);
}

static int all_zero(const ask_t *a)
{
    for (size_t i = 0; i < sizeof a->buf; i++) {
        if (a->buf[i] != '\0') {
            return 0;
        }
    }
    return a->len == 0;
}

int main(void)
{
    /* ---- the prompt ------------------------------------------------------ */
    static ask_t a;
    const char *secret = "Tr0ub4dor&3 x";
    ask_start(&a, "home password");
    int r = ASK_KEEP;
    for (const char *p = secret; *p; p++) {
        r |= key(&a, *p, 0);
    }
    char shown[96];
    ask_render(&a, shown, sizeof shown);
    CHECK(r == ASK_KEEP && strcmp(a.buf, secret) == 0, "typing keeps every character");
    CHECK(strcmp(shown, "home password: *************") == 0,
          "the status line shows '%s'", shown);
    int leaked = 0;
    for (const char *p = shown + strlen("home password: "); *p; p++) {
        leaked |= (*p != '*');
    }
    CHECK(!leaked, "and not one character of what was typed");

    /* A chord is not a character, and Ctrl-O must not switch documents under
     * a half-typed password - it is simply ignored. AltGr is a character. */
    CHECK(key(&a, 'o', KBD_MOD_LCTRL) == ASK_KEEP && a.len == 13,
          "Ctrl-O is ignored, not typed");
    CHECK(key(&a, '@', KBD_MOD_RALT) == ASK_KEEP && a.buf[13] == '@',
          "AltGr is a character, as it is everywhere else");
    CHECK(press(&a, KBD_EV_BACKSPACE) == ASK_KEEP && a.len == 13 &&
          a.buf[13] == '\0', "backspace takes one back");
    CHECK(press(&a, KBD_EV_ENTER) == ASK_SUBMIT && strcmp(a.buf, secret) == 0,
          "Enter hands over exactly what was typed");
    ask_end(&a);
    CHECK(all_zero(&a) && !a.active, "and then every byte of it is zero");

    ask_start(&a, "q");
    key(&a, 'x', 0);
    CHECK(press(&a, KBD_EV_ESC) == ASK_CANCEL, "Esc takes it back");
    ask_end(&a);
    CHECK(all_zero(&a), "and wipes it too");
    ask_start(&a, "q");
    CHECK(key(&a, 'c', KBD_MOD_LCTRL) == ASK_CANCEL &&
          key(&a, 'G', KBD_MOD_RCTRL) == ASK_CANCEL, "so do Ctrl-C and Ctrl-G");

    /* Bounded: the 64th character is not stored and nothing is overrun. */
    ask_start(&a, "q");
    for (int i = 0; i < 80; i++) {
        key(&a, 'k', 0);
    }
    CHECK(a.len == ASK_MAX && a.buf[ASK_MAX] == '\0', "it stops at %d", ASK_MAX);
    key(&a, '\t', 0);
    ask_start(&a, "q");
    key(&a, '\x01', 0);
    CHECK(a.len == 0, "a control byte is not a character");
    /* A new question never starts with the last answer in it. */
    key(&a, 's', 0);
    ask_start(&a, "again");
    CHECK(all_zero(&a), "a new question starts empty");

    /* ---- the old password, found --------------------------------------- */
    char w[33];
    size_t n = 0;
    CHECK(first_word_rest("home", w, sizeof w, &n) == -1 && strcmp(w, "home") == 0,
          "'home' is one word");
    CHECK(first_word_rest("home hunter2", w, sizeof w, &n) == 4,
          "'home hunter2': a second word, from column 4");
    CHECK(first_word_rest("  home   hunter2  ", w, sizeof w, &n) == 6 && n == 4,
          "spaces around it change nothing but the column");
    CHECK(first_word_rest("home   ", w, sizeof w, &n) == -1,
          "trailing spaces are not a word");
    CHECK(first_word_rest("", w, sizeof w, &n) == -1 && n == 0 && w[0] == '\0',
          "an empty line is no word at all");
    const char *longname = "an-ssid-that-is-far-longer-than-thirty-two";
    CHECK(first_word_rest(longname, w, sizeof w, &n) == -1 &&
          n == strlen(longname) && strlen(w) == 32,
          "a name too long is cut to fit, and its real length reported");

    /* The help's placeholder, typed with its brackets - measured: the deck
     * looked for a network called "<HomeNet>" and retried for ever. */
    char nm[40] = "<HomeNet>";
    CHECK(unbracket(nm) && strcmp(nm, "HomeNet") == 0, "'<HomeNet>' is HomeNet");
    char plain[40] = "HomeNet";
    CHECK(!unbracket(plain) && strcmp(plain, "HomeNet") == 0,
          "a name without them is left alone");
    char half[40] = "<HomeNet";
    char tiny[4] = "<>";
    CHECK(!unbracket(half) && !unbracket(tiny) && strcmp(tiny, "<>") == 0,
          "half a bracket, or brackets round nothing, are not a placeholder");

    /* ---- the fingerprint, as ssh-keygen prints it ------------------------ */
    uint8_t h[32];
    char fp[SSH_FP_TEXT];
    for (int i = 0; i < 32; i++) { h[i] = (uint8_t)i; }
    ssh_fp_text(h, fp, sizeof fp);
    CHECK(strcmp(fp, "SHA256:AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8") == 0,
          "bytes 0..31 are %s", fp);
    memset(h, 0xFF, sizeof h);
    ssh_fp_text(h, fp, sizeof fp);
    CHECK(strcmp(fp, "SHA256://////////////////////////////////////////8") == 0,
          "all ones are %s", fp);
    /* A throwaway ECDSA key made for this check; `ssh-keygen -lf` printed
     * SHA256:J9HgKB0gRFeSg9sCZKwzrW0yojhztQAZozq/qGJAxJc for it. */
    static const uint8_t real[32] = {
        0x27, 0xd1, 0xe0, 0x28, 0x1d, 0x20, 0x44, 0x57, 0x92, 0x83, 0xdb, 0x02,
        0x64, 0xac, 0x33, 0xad, 0x6d, 0x32, 0xa2, 0x38, 0x73, 0xb5, 0x00, 0x19,
        0xa3, 0x3a, 0xbf, 0xa8, 0x62, 0x40, 0xc4, 0x97,
    };
    ssh_fp_text(real, fp, sizeof fp);
    CHECK(strcmp(fp, "SHA256:J9HgKB0gRFeSg9sCZKwzrW0yojhztQAZozq/qGJAxJc") == 0,
          "a real key reads as ssh-keygen read it");

    char k1[16], k2[16], k3[16];
    ssh_fp_slot("laptop.local", 22, k1);
    ssh_fp_slot("laptop.local", 2222, k2);
    ssh_fp_slot("pi.local", 22, k3);
    CHECK(strlen(k1) == 9 && k1[0] == 'k', "a host's slot is an NVS key: %s", k1);
    CHECK(strcmp(k1, k2) != 0 && strcmp(k1, k3) != 0,
          "another port or another host is another slot");

    if (fails) {
        printf("[FAIL] %d check(s) failed\n", fails);
    } else {
        printf("[PASS] secrets are asked for, starred, handed over and wiped\n");
    }
    return fails != 0;
}
