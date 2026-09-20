/*
 * A keyboard over the USB cable.
 *
 * Two reasons this exists, and the second is the one that matters tonight:
 *
 *  1. It is a real fallback. If no keyboard is paired - or the BLE keyboard is
 *     flat, or lost - the deck is still usable from any terminal over the same
 *     cable that flashes it.
 *  2. It makes the editor testable with no radio at all. Without it, the gap
 *     buffer, the wrap, the cursor, the autosave and the redraw path have no
 *     key source and cannot be exercised on the bench.
 *
 * Input goes through the same queue as BLE HID, so there is exactly one input
 * path and one event taxonomy, per docs/OS.md.
 */
#include "serialkbd.h"

#include "driver/usb_serial_jtag.h"
#include "driver/usb_serial_jtag_vfs.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "kbd.h"

static const char *TAG = "serialkbd";

static void push(kbd_ev_type_t t, char ch)
{
    const kbd_event_t ev = { .type = t, .ch = ch, .repeat = false };
    kbd_inject(&ev);
}

/* A terminal sends Ctrl-A..Ctrl-Z as bytes 1..26. Turning them back into a
 * character plus a modifier means the serial keyboard reaches the same chords
 * as the BLE one, through the same event taxonomy - so undo is testable over
 * the cable and usable when no keyboard is paired. */
static void push_ctrl(char ch)
{
    const kbd_event_t ev = { .type = KBD_EV_CHAR, .ch = ch,
                             .mods = KBD_MOD_LCTRL, .repeat = false };
    kbd_inject(&ev);
}

static void serial_task(void *arg)
{
    (void)arg;
    uint8_t b;
    int esc = 0;               /* 0 none, 1 saw ESC, 2 saw ESC [ */

    while (1) {
        const int n = usb_serial_jtag_read_bytes(&b, 1, portMAX_DELAY);
        if (n <= 0) {
            continue;
        }

        /* Arrow keys arrive as ESC [ A..D from any terminal. */
        if (esc == 1) {
            esc = (b == '[') ? 2 : 0;
            if (esc == 0 && b == 0x1B) {
                push(KBD_EV_ESC, 0);
            }
            continue;
        }
        if (esc == 2) {
            esc = 0;
            switch (b) {
            case 'A': push(KBD_EV_UP, 0);    break;
            case 'B': push(KBD_EV_DOWN, 0);  break;
            case 'C': push(KBD_EV_RIGHT, 0); break;
            case 'D': push(KBD_EV_LEFT, 0);  break;
            default: break;
            }
            continue;
        }

        switch (b) {
        case 0x1B: esc = 1;                      break;
        case '\r': push(KBD_EV_ENTER, 0);        break;
        case '\n': {
            /* A terminal sends CR for Enter, so LF is free - and it is the
             * only way to reach Ctrl+Enter (the Run verb) down a cable. */
            const kbd_event_t ev = { .type = KBD_EV_ENTER,
                                     .mods = KBD_MOD_LCTRL };
            kbd_inject(&ev);
            break;
        }
        case 0x08:
        case 0x7F: push(KBD_EV_BACKSPACE, 0);    break;
        case '\t': push(KBD_EV_TAB, 0);          break;
        default:
            if (b >= 0x20 && b < 0x7F) {
                push(KBD_EV_CHAR, (char)b);
            } else if (b >= 1 && b <= 26) {
                push_ctrl((char)('a' + b - 1));
            }
            break;
        }
    }
}

esp_err_t serialkbd_init(void)
{
    usb_serial_jtag_driver_config_t cfg = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    cfg.rx_buffer_size = 256;
    cfg.tx_buffer_size = 1024;

    const esp_err_t err = usb_serial_jtag_driver_install(&cfg);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "driver install failed: %s", esp_err_to_name(err));
        return err;
    }
    /* Route the console through the driver too, so logging keeps working now
     * that the peripheral has an owner. */
    usb_serial_jtag_vfs_use_driver();

    xTaskCreate(serial_task, "serialkbd", 3072, NULL, 5, NULL);
    ESP_LOGI(TAG, "typing over USB is live - this terminal is a keyboard");
    return ESP_OK;
}
