/*
 * USB MIDI, as a CDC + MIDI composite, and the state machine that makes
 * turning it on safe.
 *
 * WHY A COMPOSITE. The ESP32-S3 has one USB PHY. Becoming a MIDI device means
 * taking it from USB-Serial-JTAG, which is where the console and the serial
 * keyboard live - the entire troubleshooting path. The endpoint budget does
 * not force that trade: CDC-ACM costs 2 IN + 1 OUT and MIDI 1 IN + 1 OUT
 * against five IN TX FIFOs, so both fit and the console moves to CDC.
 *
 * WHY THE STATE MACHINE. If the composite fails to enumerate, the owner loses
 * the console AND the serial keyboard on the same stroke; if the Bluetooth
 * keyboard also happens to be asleep, there is no way left to type '>flash'.
 * So the mode is ARMED for one boot at a time and confirms itself:
 *
 *   OFF     want=0            the shipped state, console on USB-Serial-JTAG
 *   ARMED   want=1, armed=1   written by '>usb on', then reboot
 *   TRIAL   this boot came up with want=1; a timer is watching
 *   ACTIVE  want=1, armed=0   a host mounted us, so it demonstrably works
 *
 * Confirmation is tud_mount_cb() - objective, instant, and requiring nothing
 * of the owner. There is deliberately no '>usb keep': a confirmation someone
 * has to type is a second thing that can be forgotten, on exactly the input
 * path this design assumes is unreliable.
 *
 * The attempt counter lives in RTC_NOINIT rather than NVS because a bring-up
 * that panics never reaches code that could write NVS, and a counter that
 * cannot be incremented during the failure it counts is not a counter. It
 * survives esp_restart AND panic_restart, and a power cycle clears it - so
 * "unplug it" is always a real way out.
 */
#include "usbdev.h"

#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "tinyusb.h"
#include "tinyusb_cdc_acm.h"
#include "tusb.h"

#include "kbd.h"
#include "serialkbd_map.h"
#include "usbmux.h"

static const char *TAG = "usbdev";

#define NVS_NS       "deck"
#define KEY_WANT     "usb_want"
#define KEY_ARMED    "usb_armed"
#define MAX_TRIES    3
/* How long to wait for a host to mount us before concluding it will not. Long
 * enough for a laptop to enumerate a composite device unhurried, short enough
 * that nobody watching a blank console decides the deck is dead. A judgement,
 * not a measurement. */
#define TRIAL_MS     8000

/* ---------------------------------------------------------------- descriptors
 *
 * Borrowed from ESP-IDF's tusb_midi example (the MIDI half) and TinyUSB's own
 * cdc_msc example (the composite half); there is no published CDC+MIDI
 * example, so this is their union. The device class MUST be MISC/COMMON/IAD
 * for a composite, or Windows in particular binds only the first interface.
 */
enum { ITF_CDC = 0, ITF_CDC_DATA, ITF_MIDI, ITF_MIDI_STREAM, ITF_COUNT };

#define EP_CDC_NOTIF 0x81
#define EP_CDC_OUT   0x02
#define EP_CDC_IN    0x82
#define EP_MIDI_OUT  0x03
#define EP_MIDI_IN   0x83

static const tusb_desc_device_t s_dev_desc = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0x303A,            /* Espressif, per their VID/PID
                                              * allocation for TinyUSB devices */
    .idProduct          = 0x4001,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01,
};

#define CFG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_MIDI_DESC_LEN)

static const uint8_t s_cfg_desc[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_COUNT, 0, CFG_TOTAL_LEN, 0x80, 250),
    TUD_CDC_DESCRIPTOR(ITF_CDC, 4, EP_CDC_NOTIF, 8, EP_CDC_OUT, EP_CDC_IN, 64),
    TUD_MIDI_DESCRIPTOR(ITF_MIDI, 5, EP_MIDI_OUT, EP_MIDI_IN, 64),
};
_Static_assert(sizeof(s_cfg_desc) == CFG_TOTAL_LEN, "config descriptor length");

static const char *s_strings[] = {
    (const char[]){ 0x09, 0x04 },   /* 0: English (US) */
    "cyberdeck",                    /* 1: manufacturer */
    "cyberdeck",                    /* 2: product      */
    "deck-0001",                    /* 3: serial       */
    "cyberdeck console",            /* 4: CDC          */
    "cyberdeck MIDI",               /* 5: MIDI         */
};

/* ------------------------------------------------------------------- state */

#define TRY_MAGIC 0x55534201u
static RTC_NOINIT_ATTR uint32_t s_try_magic;
static RTC_NOINIT_ATTR uint32_t s_tries;

static bool     s_active;          /* this boot is running USB MIDI */
static bool     s_confirmed;
static bool     s_host_seen_usj;
static esp_timer_handle_t s_trial;

static uint32_t s_msgs, s_packets;

unsigned usbdev_tries(void) { return (unsigned)s_tries; }
bool     usbdev_mounted(void) { return s_active && tud_midi_mounted(); }

void usbdev_packing(uint32_t *m, uint32_t *p)
{
    if (m != NULL) { *m = s_msgs; }
    if (p != NULL) { *p = s_packets; }
}

static esp_err_t nvs_put_u8(const char *key, uint8_t v)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_set_u8(h, key, v);
    if (err == ESP_OK) {
        err = nvs_commit(h);
    }
    nvs_close(h);
    return err;
}

static uint8_t nvs_get_u8_or(const char *key, uint8_t dflt)
{
    nvs_handle_t h;
    uint8_t v = dflt;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) == ESP_OK) {
        if (nvs_get_u8(h, key, &v) != ESP_OK) {
            v = dflt;
        }
        nvs_close(h);
    }
    return v;
}

bool usbdev_wanted(void) { return nvs_get_u8_or(KEY_WANT, 0) != 0; }

esp_err_t usbdev_want(bool on)
{
    if (on && s_tries >= MAX_TRIES) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = nvs_put_u8(KEY_WANT, on ? 1 : 0);
    if (err == ESP_OK) {
        err = nvs_put_u8(KEY_ARMED, on ? 1 : 0);
    }
    return err;
}

/* --------------------------------------------------------------- MIDI out
 *
 * The USB MIDI TX FIFO is 64 bytes and CFG_TUD_MIDI_TX_BUFSIZE is a hard
 * define in the component, not a Kconfig - so a short write is a SILENTLY
 * DROPPED NOTE. It is counted here and reported by the sequencer's own
 * throttled reporter, never logged from this path: this runs on the MIDI
 * task, and logging that the transport is behind is a good way to put it
 * further behind. */
static uint32_t s_dropped;

void usbdev_midi_send(uint8_t status, uint8_t d1, uint8_t d2)
{
    if (!usbdev_mounted()) {
        return;
    }
    const uint8_t msg[3] = { status, d1, d2 };
    /* System real-time messages are a single byte. Sending three would inject
     * two zero bytes, which a receiver reads as a note-off on channel 1. */
    const uint32_t n = (status >= 0xF8) ? 1 : ((status == 0xF2) ? 3 : 3);
    if (tud_midi_stream_write(0, msg, n) != n) {
        s_dropped++;
    } else {
        s_msgs++;
    }
}

void usbdev_midi_flush(void)
{
    /* USB MIDI has no packet the way BLE does - the stack coalesces into the
     * 64-byte endpoint buffer itself and ships it on the next 1 ms frame. The
     * hook exists so the destination table has one shape, and so the packet
     * count means the same thing in '>jitter' for both transports. */
    if (s_msgs != 0) {
        s_packets++;
    }
}

/* ------------------------------------------------------------- the console
 *
 * Bytes arriving on CDC go through the SAME mapper as USB-Serial-JTAG, so the
 * transport change cannot alter a single key. tools/test_serialkbd_map.c pins
 * that mapping; this is the code path it protects. */
static void cdc_rx(int itf, cdcacm_event_t *event)
{
    (void)event;
    static skb_state_t st;
    uint8_t buf[64];
    size_t got = 0;
    if (tinyusb_cdcacm_read(itf, buf, sizeof buf, &got) != ESP_OK) {
        return;
    }
    for (size_t i = 0; i < got; i++) {
        const skb_ev_t e = skb_feed(&st, buf[i]);
        if (e.emit) {
            const kbd_event_t ev = { .type = (kbd_ev_type_t)e.type,
                                     .ch = e.ch, .mods = e.mods,
                                     .repeat = false };
            kbd_inject(&ev);
        }
    }
}

/* ------------------------------------------------------------ the trial */

static void revert(const char *why)
{
    ESP_LOGE(TAG, "%s - reverting to the console", why);
    nvs_put_u8(KEY_WANT, 0);
    nvs_put_u8(KEY_ARMED, 0);
    usbmux_release_to_usj();
    esp_restart();
}

static void trial_expired(void *arg)
{
    (void)arg;
    if (s_confirmed || tud_mounted()) {
        return;
    }
    if (!s_host_seen_usj) {
        /* There was no host when we started, so nothing has been proven
         * broken and no console was lost - there was nobody to lose it to.
         * Stay up and wait; the timer is re-armed when a bus reset says a
         * host has arrived. This closes the otherwise fatal case of arming on
         * battery and only discovering the failure on stage. */
        ESP_LOGW(TAG, "no host yet; staying armed");
        return;
    }
    revert("USB did not enumerate");
}

void tud_mount_cb(void)
{
    s_confirmed = true;
    if (s_trial != NULL) {
        esp_timer_stop(s_trial);
    }
    s_tries = 0;
    nvs_put_u8(KEY_ARMED, 0);
    ESP_LOGW(TAG, "USB MIDI live - a host mounted us");
}

bool usbdev_boot(void)
{
    if (s_try_magic != TRY_MAGIC) {
        s_try_magic = TRY_MAGIC;
        s_tries = 0;
    }
    if (!usbdev_wanted()) {
        return false;                       /* OFF: the shipped path */
    }

    s_tries++;
    if (s_tries > MAX_TRIES) {
        /* The layer that catches a bring-up which panics or hangs, where no
         * timer ever gets to run - and the only layer that survives
         * panic_restart(), which does not run shutdown handlers. */
        ESP_LOGE(TAG, "USB failed %u times; giving up", (unsigned)s_tries);
        nvs_put_u8(KEY_WANT, 0);
        nvs_put_u8(KEY_ARMED, 0);
        return false;
    }

    /* Latch whether a host was there BEFORE the peripheral is torn down.
     * This is SOF-based, so it tells a laptop from a phone charger. */
    s_host_seen_usj = usb_serial_jtag_is_connected();

    const tinyusb_config_t cfg = {
        .port = TINYUSB_PORT_FULL_SPEED_0,
        .phy = { .skip_setup = false, .self_powered = false, .vbus_monitor_io = -1 },
        .task = { .size = 4096, .priority = 5, .xCoreID = 1 },
        .descriptor = {
            .device            = &s_dev_desc,
            .string            = s_strings,
            .string_count      = sizeof s_strings / sizeof s_strings[0],
            .full_speed_config = s_cfg_desc,
        },
    };
    if (tinyusb_driver_install(&cfg) != ESP_OK) {
        ESP_LOGE(TAG, "tinyusb_driver_install failed");
        nvs_put_u8(KEY_WANT, 0);
        nvs_put_u8(KEY_ARMED, 0);
        usbmux_release_to_usj();
        return false;                       /* fall back, no reboot needed */
    }

    const tinyusb_config_cdcacm_t acm = {
        .usb_dev = TINYUSB_USBDEV_0,
        .cdc_port = TINYUSB_CDC_ACM_0,
        .callback_rx = cdc_rx,
    };
    if (tinyusb_cdcacm_init(&acm) != ESP_OK) {
        ESP_LOGE(TAG, "CDC init failed");
    }

    const esp_timer_create_args_t targs = {
        .callback = trial_expired, .name = "usbtrial",
        .dispatch_method = ESP_TIMER_TASK,
    };
    if (esp_timer_create(&targs, &s_trial) == ESP_OK) {
        esp_timer_start_once(s_trial, (uint64_t)TRIAL_MS * 1000);
    }

    s_active = true;
    ESP_LOGW(TAG, "USB MIDI mode, attempt %u of %u",
             (unsigned)s_tries, MAX_TRIES);
    return true;
}
