/*
 * What the deck was doing just before it stopped.
 *
 * WHY THIS EXISTS. docs/OS.md records a hang in USB MIDI mode, reproduced three
 * times and not root-caused, and the reason it is not root-caused is that the
 * console is one of the things that dies. Three hypotheses have been tested and
 * eliminated from the outside; the fourth needs evidence from the inside.
 *
 * RTC MEMORY IS NO USE HERE, which is the whole difficulty. It survives
 * esp_restart() and a panic, but the only thing that recovers this hang is a
 * power cycle, and a power cycle is exactly what clears it. The evidence has to
 * reach flash before the deck dies, or it does not reach anywhere.
 *
 * SO IT WRITES TO NVS, AND ONLY WHILE HUNTING. Once a minute, and only while USB
 * MIDI mode is active - which is the only place the bug has ever been seen. A
 * deck in serial mode writes nothing at all, so the flash cost is bounded to the
 * sessions where it might buy something: sixty writes an hour, during a hunt.
 *
 * Granularity is therefore one minute. That is enough to answer the question
 * that matters - did the loop stop, or did the whole scheduler go - because the
 * loop counter either advanced between records or it did not.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t uptime_s;      /* when this record was written              */
    uint32_t loops;         /* main loop iterations since boot           */
    uint32_t ticks;         /* sequencer ticks since play                */
    uint32_t heap_free;     /* internal, in bytes                        */
    uint8_t  usb_mode;      /* was USB MIDI active                       */
    uint8_t  running;       /* was the clock running                     */
    uint8_t  reset_reason;  /* what started the run that wrote this      */
    uint8_t  valid;         /* 0 when nothing has ever been written      */
    uint8_t  deliberate;    /* the run said goodbye before restarting    */
    uint8_t  pad[3];
} vitals_t;

/* Read the record the PREVIOUS run left, then start a new one. Call once, early,
 * after nvs_flash_init(). */
void vitals_begin(void);

/* Mark that this run began, once the USB mode is known - vitals_verdict.h
 * explains why a run that writes nothing else still has to. */
void vitals_started(bool usb_mode);

/* The previous run's last words, or a record with valid == 0. */
const vitals_t *vitals_previous(void);

/* Called from the main loop. Counts an iteration and, once a minute while USB
 * MIDI is active, writes the record. Cheap on every other call: an increment and
 * one comparison. */
void vitals_loop(bool usb_mode, bool running, uint32_t ticks);

/* SAY GOODBYE. Called immediately before a deliberate esp_restart(), so the next
 * boot can tell a restart the deck CHOSE from a run that simply stopped.
 *
 * This is the discriminator, and the first attempt got it wrong by looking at the
 * current boot's reset reason instead: after a flash that reason is ESP_RST_USB,
 * after a power cycle it is POWERON, and neither says anything about whether the
 * PREVIOUS run ended on purpose. A record that was never closed is the evidence;
 * a reason code for the boot that reads it is not. */
void vitals_goodbye(bool usb_mode, bool running, uint32_t ticks);

/* The previous run's last words, as up to VITALS_LINES lines of at most thirty
 * characters - because that is the width of the panel and of '>jitter'.
 *
 * It was one long string, and the one thing worth reading was at the end: "died
 * silently". A diagnostic truncated before its verdict is worse than none,
 * because it looks like it answered. Returns the number of lines. */
#define VITALS_LINES 4
int vitals_report(const char *out[VITALS_LINES]);
