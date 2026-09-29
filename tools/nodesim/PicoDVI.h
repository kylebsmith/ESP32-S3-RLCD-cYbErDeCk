/* The view node's sketch on this computer: just enough of PicoDVI and Arduino
 * for view/deckview/deckview.ino to draw into memory, so tools/node_sim.cpp can
 * look at exactly what the node puts on the screen. */
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#define LED_BUILTIN 25
#define OUTPUT 1
static inline void pinMode(int, int) {}
static inline void digitalWrite(int, int) {}
extern uint32_t g_millis;
static inline uint32_t millis(void) { return g_millis; }

struct NodeSerial {
    const uint8_t *in = nullptr;
    int n = 0, at = 0;
    void begin(int) {}
    int available() { return n - at; }
    int readBytes(uint8_t *b, int k) {
        if (k > n - at) k = n - at;
        memcpy(b, in + at, (size_t)k);
        at += k;
        return k;
    }
    int printf(const char *, ...) { return 0; }
};
extern NodeSerial Serial;

enum { DVI_RES_320x240p60 };
extern int adafruit_feather_dvi_cfg;

struct DVIGFX8 {
    uint8_t buf[2][320 * 240];
    uint16_t pal[2][256];
    int back = 0;
    DVIGFX8(int, bool, int) {}
    bool begin() { return true; }
    uint8_t *getBuffer() { return buf[back]; }
    uint16_t *getPalette() { return pal[back]; }
    void swap(bool, bool) { back ^= 1; }
    const uint8_t *front() const { return buf[back ^ 1]; }
    const uint16_t *front_pal() const { return pal[back ^ 1]; }
};
