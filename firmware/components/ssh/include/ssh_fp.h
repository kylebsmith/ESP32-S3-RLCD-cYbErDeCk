/*
 * A host key's fingerprint, and where the deck keeps it.
 *
 * THE FIRST KEY A HOST SHOWS IS KEPT, AND A DIFFERENT ONE IS REFUSED before any
 * password is sent - docs/NETWORK.md has the decision. The fingerprint is
 * printed exactly as `ssh-keygen -lf` prints it, so the owner can hold the
 * deck's line up against the host's own and see that they match. Pure, so
 * tools/test_ask.c runs this exact code.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* "SHA256:" and the unpadded base64 of the key's SHA-256 - 50 characters. */
#define SSH_FP_TEXT 51

static inline void ssh_fp_text(const uint8_t h[32], char *out, size_t max)
{
    static const char B[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char t[SSH_FP_TEXT] = "SHA256:";
    size_t k = 7;
    for (int i = 0; i < 32; i += 3) {
        const uint32_t v = ((uint32_t)h[i] << 16) |
                           ((i + 1 < 32 ? (uint32_t)h[i + 1] : 0u) << 8) |
                           (i + 2 < 32 ? (uint32_t)h[i + 2] : 0u);
        t[k++] = B[(v >> 18) & 63];
        t[k++] = B[(v >> 12) & 63];
        if (i + 1 < 32) { t[k++] = B[(v >> 6) & 63]; }
        if (i + 2 < 32) { t[k++] = B[v & 63]; }
    }
    t[k] = '\0';
    snprintf(out, max, "%s", t);
}

/* The NVS key a host's fingerprint is kept under: 'k' and eight hex digits of
 * FNV-1a over "host:port", because an NVS key is at most 15 characters. Two
 * hosts that collide share a slot, and the second is refused as a changed key -
 * the safe way to be wrong. */
static inline void ssh_fp_slot(const char *host, int port, char key[16])
{
    char hp[80];
    snprintf(hp, sizeof hp, "%s:%d", host, port);
    uint32_t f = 2166136261u;
    for (const char *p = hp; *p != '\0'; p++) {
        f = (f ^ (uint8_t)*p) * 16777619u;
    }
    snprintf(key, 16, "k%08x", (unsigned)f);
}
