/*
 * HID-over-GATT keyboard host.
 *
 * Flow: scan -> connect to anything advertising the HID service or a keyboard
 * appearance -> bond -> discover the HID service -> force Boot Protocol ->
 * subscribe to the Boot Keyboard Input Report -> decode 8-byte reports.
 *
 * Boot Protocol is chosen deliberately. It fixes the report at 8 bytes
 * (modifiers, reserved, six keycodes) and removes any need to parse a HID
 * report descriptor, which is where this kind of code usually goes wrong.
 * If the keyboard has no boot report we fall back to subscribing to every
 * notifiable Report characteristic and decoding anything 8 bytes long.
 *
 * Key repeat is synthesised HERE, not by the keyboard: docs/OS.md makes repeat
 * the OS's job because BLE HID repeat behaviour is device-dependent, and BLE
 * connection intervals of 7.5-15 ms jitter press timestamps.
 */
#include "kbd.h"
#include "keymap.h"

#include <string.h>

#include "esp_log.h"
#include "esp_random.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_bt.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

static const char *TAG = "kbd";

#define UUID_HID_SVC        0x1812
#define UUID_REPORT         0x2A4D
#define UUID_BOOT_KBD_IN    0x2A22
#define UUID_PROTOCOL_MODE  0x2A4E
#define UUID_HID_CTRL_POINT 0x2A4C
#define UUID_REPORT_REF     0x2908
#define UUID_DEVICE_NAME    0x2A00
#define UUID_CCCD           0x2902

#define APPEARANCE_KEYBOARD 0x03C1

#define REPEAT_DELAY_MS   400
#define REPEAT_PERIOD_MS   45

static QueueHandle_t s_q;
static uint16_t      s_conn = BLE_HS_CONN_HANDLE_NONE;
static uint8_t       s_own_addr_type;
static bool          s_connected;
static bool          s_subscribed;
static const char   *s_state = "starting";
static int           s_adv_logged;
static int           s_adv_seen;
static int           s_reports_logged;
static bool          s_clear_bonds_on_sync;

/* Bumped when the pairing scheme changes. */
#define KBD_PAIR_SCHEME 2

/* Held-key tracking for synthesised repeat. */
static uint8_t  s_held_usage;
static uint8_t  s_held_mods;
static int64_t  s_held_since_ms;
static int64_t  s_next_repeat_ms;

static void scan_start(void);

bool kbd_connected(void)      { return s_connected; }
const char *kbd_state_name(void) { return s_state; }

static void emit(kbd_ev_type_t t, char ch, bool repeat)
{
    const kbd_event_t ev = { .type = t, .ch = ch, .repeat = repeat };
    if (s_q != NULL) {
        xQueueSend(s_q, &ev, 0);
    }
}

void kbd_inject(const kbd_event_t *ev)
{
    if (s_q != NULL && ev != NULL) {
        xQueueSend(s_q, ev, 0);
    }
}

bool kbd_poll(kbd_event_t *ev, uint32_t timeout_ms)
{
    if (s_q == NULL) {
        return false;
    }
    return xQueueReceive(s_q, ev, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

/* ------------------------------------------------------------------ report */

static void dispatch_usage(uint8_t usage, uint8_t mods, bool repeat)
{
    const bool shift = (mods & 0x22) != 0;    /* either shift */

    switch (usage) {
    case 0x28: emit(KBD_EV_ENTER,     0, repeat); return;
    case 0x29: emit(KBD_EV_ESC,       0, repeat); return;
    case 0x2A: emit(KBD_EV_BACKSPACE, 0, repeat); return;
    case 0x2B: emit(KBD_EV_TAB,       0, repeat); return;
    case 0x4F: emit(KBD_EV_RIGHT,     0, repeat); return;
    case 0x50: emit(KBD_EV_LEFT,      0, repeat); return;
    case 0x51: emit(KBD_EV_DOWN,      0, repeat); return;
    case 0x52: emit(KBD_EV_UP,        0, repeat); return;
    default: break;
    }
    const char c = keymap_char(usage, shift);
    if (c != 0) {
        emit(KBD_EV_CHAR, c, repeat);
    }
}

/* An 8-byte boot keyboard report. Only newly-pressed keys fire; a key that
 * was already down in the previous report is a hold, which the repeat timer
 * owns rather than the report path. */
static void handle_report(const uint8_t *r, int len)
{
    /* The boot layout is {modifiers, reserved, six keycodes}. Report-protocol
     * keyboards usually send the same shape, sometimes with a leading report
     * ID and sometimes with fewer than six keycode slots, so the count is
     * taken from the length rather than assumed. Anything under three bytes
     * is not a keyboard report at all - a one-byte notification is a battery
     * level or a consumer-control key - and is ignored. */
    if (len < 3) {
        return;
    }
    static uint8_t prev[6];
    const uint8_t mods = r[0];
    const uint8_t *keys = &r[2];
    int nkeys = len - 2;
    if (nkeys > 6) {
        nkeys = 6;
    }

    for (int i = 0; i < nkeys; i++) {
        const uint8_t u = keys[i];
        if (u == 0 || u == 0x01) {
            continue;                 /* empty or rollover-error */
        }
        bool was_down = false;
        for (int j = 0; j < 6; j++) {
            if (prev[j] == u) { was_down = true; break; }
        }
        if (!was_down) {
            dispatch_usage(u, mods, false);
            s_held_usage     = u;
            s_held_mods      = mods;
            s_held_since_ms  = esp_timer_get_time() / 1000;
            s_next_repeat_ms = s_held_since_ms + REPEAT_DELAY_MS;
        }
    }

    /* Is the key we were repeating still down? */
    bool still = false;
    for (int i = 0; i < nkeys; i++) {
        if (keys[i] == s_held_usage && s_held_usage != 0) { still = true; break; }
    }
    if (!still) {
        s_held_usage = 0;
    }
    memset(prev, 0, sizeof prev);
    memcpy(prev, keys, (size_t)nkeys);
}

static void repeat_task(void *arg)
{
    (void)arg;
    int64_t last_beat = 0;
    while (1) {
        const int64_t nowb = esp_timer_get_time() / 1000;
        if (nowb - last_beat >= 10000) {
            last_beat = nowb;
            if (!s_connected) {
                ESP_LOGW(TAG, "%s - %d adverts heard, no keyboard yet "
                              "(put the keyboard into PAIRING mode)",
                         s_state, s_adv_seen);
            }
        }
        if (s_held_usage != 0 && s_connected) {
            const int64_t now = esp_timer_get_time() / 1000;
            if (now >= s_next_repeat_ms) {
                dispatch_usage(s_held_usage, s_held_mods, true);
                s_next_repeat_ms = now + REPEAT_PERIOD_MS;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(15));
    }
}

/* --------------------------------------------------------------- discovery
 *
 * NimBLE allows exactly ONE GATT procedure in flight per connection. Starting
 * descriptor discovery from inside the characteristic-discovery callback -
 * the obvious way to write this - fails with BLE_HS_EBUSY, the CCCD is never
 * written, and the keyboard connects perfectly and then types nothing. So
 * discovery is a sequential state machine: each step is kicked off only by
 * the completion callback of the one before it.
 */

#define MAX_REPORT_CHRS 8

struct chr_rec {
    uint16_t def_handle;
    uint16_t val_handle;
    uint16_t uuid;
};

static uint16_t       s_svc_start, s_svc_end;
static struct chr_rec s_chrs[MAX_REPORT_CHRS];
static int            s_chr_count;
/* Every characteristic declaration in the service, report or not. A report's
 * descriptor range ends just before the NEXT declaration of any kind; bound it
 * with only the report characteristics and the range runs straight through the
 * ones in between, so the same CCCD gets written several times and reports get
 * double-subscribed. */
static uint16_t       s_all_defs[16];
static int            s_all_def_count;
static uint16_t       s_protocol_mode_handle;
static uint16_t       s_ctrl_point_handle;
static int            s_sub_index;        /* which report we are subscribing */
static int            s_sub_done;
/* Descriptor discovery reports a CCCD and THEN reports completion. If both
 * paths advance the index, every characteristic that has a CCCD advances
 * twice and the walk runs off the end of the list - which is how a keyboard
 * ends up "subscribed to report 7 of 5" while some real reports are skipped.
 * Exactly one path advances: the CCCD write when there is one, completion
 * when there is not. */
static bool           s_cccd_started;

static void subscribe_next(uint16_t conn);
static void after_subscribe(uint16_t conn);

/* The end of a characteristic's descriptor range is just before the next
 * characteristic's declaration, or the end of the service for the last one. */
static uint16_t dsc_end_for(int i)
{
    uint16_t best = s_svc_end;
    for (int j = 0; j < s_all_def_count; j++) {
        if (s_all_defs[j] > s_chrs[i].val_handle && s_all_defs[j] - 1 < best) {
            best = (uint16_t)(s_all_defs[j] - 1);
        }
    }
    return best;
}

static int on_cccd_write(uint16_t conn, const struct ble_gatt_error *err,
                         struct ble_gatt_attr *attr, void *arg)
{
    (void)attr; (void)arg;
    if (s_sub_index >= s_chr_count) {
        return 0;                      /* a late callback from a finished walk */
    }
    if (err->status == 0) {
        s_sub_done++;
        ESP_LOGI(TAG, "subscribed to report %d of %d",
                 s_sub_index + 1, s_chr_count);
    } else {
        ESP_LOGW(TAG, "CCCD write for report %d failed, status %d",
                 s_sub_index + 1, err->status);
    }
    s_sub_index++;
    subscribe_next(conn);
    return 0;
}

static int on_dsc(uint16_t conn, const struct ble_gatt_error *err,
                  uint16_t chr_val_handle, const struct ble_gatt_dsc *dsc,
                  void *arg)
{
    (void)chr_val_handle; (void)arg;

    if (err->status == 0 && dsc != NULL) {
        ESP_LOGI(TAG, "  descriptor 0x%04X at handle %u",
                 ble_uuid_u16(&dsc->uuid.u), dsc->handle);
    }
    if (err->status == 0 && dsc != NULL &&
        ble_uuid_u16(&dsc->uuid.u) == UUID_CCCD) {
        static const uint8_t on[2] = { 0x01, 0x00 };
        const int rc = ble_gattc_write_flat(conn, dsc->handle,
                                            on, sizeof on, on_cccd_write, NULL);
        if (rc == 0) {
            s_cccd_started = true;     /* on_cccd_write owns the next step */
        } else {
            ESP_LOGW(TAG, "CCCD write could not start: %d", rc);
            s_sub_index++;
            subscribe_next(conn);
        }
        return 0;
    }

    if (err->status == BLE_HS_EDONE && !s_cccd_started) {
        ESP_LOGW(TAG, "report %d has no CCCD", s_sub_index + 1);
        s_sub_index++;
        subscribe_next(conn);
    }
    return 0;
}

static int on_name_read(uint16_t conn, const struct ble_gatt_error *err,
                        struct ble_gatt_attr *attr, void *arg)
{
    (void)conn; (void)arg;
    if (err->status == 0 && attr != NULL && attr->om != NULL) {
        char name[32] = {0};
        uint16_t n = OS_MBUF_PKTLEN(attr->om);
        if (n > sizeof name - 1) {
            n = sizeof name - 1;
        }
        ble_hs_mbuf_to_flat(attr->om, name, n, NULL);
        ESP_LOGW(TAG, "PEER DEVICE NAME: '%s'", name);
    } else {
        ESP_LOGW(TAG, "could not read the peer device name (status %d)",
                 err->status);
    }
    return 0;
}

static int on_ctrl_point_write(uint16_t conn, const struct ble_gatt_error *err,
                               struct ble_gatt_attr *attr, void *arg)
{
    (void)attr; (void)arg;
    ESP_LOGI(TAG, "HID exit-suspend write status %d", err->status);
    /* Whatever happened, now ask who we are actually talking to. */
    const ble_uuid16_t name = BLE_UUID16_INIT(UUID_DEVICE_NAME);
    ble_gattc_read_by_uuid(conn, 1, 0xFFFF, &name.u, on_name_read, NULL);
    return 0;
}

/* Once the reports are subscribed, two things are still worth doing.
 *
 * The HID Control Point gets 0x01, Exit Suspend. A HOGP keyboard that has
 * parked itself in suspend stays subscribed and simply stops notifying, which
 * looks exactly like a working connection that types nothing - and SolarOS
 * added a periodic version of this write for the same reason.
 *
 * Then read the peer's device name, because "found a keyboard (unnamed)" is
 * not proof that the thing we connected to is the keyboard in the owner's
 * hands rather than some other HID peripheral in range. */
static void after_subscribe(uint16_t conn)
{
    if (s_ctrl_point_handle != 0) {
        static const uint8_t exit_suspend = 0x01;
        ESP_LOGI(TAG, "writing HID exit-suspend to handle %u",
                 s_ctrl_point_handle);
        if (ble_gattc_write_no_rsp_flat(conn, s_ctrl_point_handle,
                                        &exit_suspend, 1) == 0) {
            /* write-no-response has no callback; go straight on. */
            const ble_uuid16_t name = BLE_UUID16_INIT(UUID_DEVICE_NAME);
            ble_gattc_read_by_uuid(conn, 1, 0xFFFF, &name.u, on_name_read, NULL);
            return;
        }
        ESP_LOGW(TAG, "exit-suspend write could not start");
    } else {
        ESP_LOGW(TAG, "peer has no HID Control Point");
    }
    const ble_uuid16_t name = BLE_UUID16_INIT(UUID_DEVICE_NAME);
    ble_gattc_read_by_uuid(conn, 1, 0xFFFF, &name.u, on_name_read, NULL);
}

static void subscribe_next(uint16_t conn)
{
    if (s_sub_index >= s_chr_count) {
        if (s_sub_done > 0) {
            if (!s_subscribed) {
                s_subscribed = true;
                s_state = "connected";
                ESP_LOGI(TAG, "keyboard ready - %d report(s) subscribed",
                         s_sub_done);
                emit(KBD_EV_CONNECTED, 0, false);
                after_subscribe(conn);
            }
        } else {
            ESP_LOGE(TAG, "connected but NOTHING subscribed - the keyboard "
                          "exposed no notifiable report characteristic");
            s_state = "no reports";
        }
        return;
    }
    const int i = s_sub_index;
    s_cccd_started = false;
    ESP_LOGI(TAG, "discovering descriptors of report %d (val %u, range %u..%u)",
             i + 1, s_chrs[i].val_handle,
             (unsigned)(s_chrs[i].val_handle + 1), dsc_end_for(i));
    const int rc = ble_gattc_disc_all_dscs(conn,
                                           s_chrs[i].val_handle,
                                           dsc_end_for(i), on_dsc, NULL);
    if (rc != 0) {
        ESP_LOGW(TAG, "descriptor discovery could not start: %d", rc);
        s_sub_index++;
        subscribe_next(conn);
    }
}

static int on_chr(uint16_t conn, const struct ble_gatt_error *err,
                  const struct ble_gatt_chr *chr, void *arg)
{
    (void)arg;

    if (err->status == BLE_HS_EDONE) {
        ESP_LOGI(TAG, "characteristic discovery done: %d report(s), "
                      "protocol mode %s",
                 s_chr_count, s_protocol_mode_handle ? "present" : "absent");

        /* Subscribe to ALL of them. Collapsing to the boot keyboard report
         * would be neater, but it is only correct if the keyboard actually
         * enters boot protocol - and this one answers the protocol-mode write
         * with ATT Write Not Permitted, so it stays in report protocol where
         * the boot report never notifies. Subscribing to everything costs a
         * few CCCD writes and does not care which mode won. */
        s_sub_index = 0;
        s_sub_done  = 0;

        /* Protocol Mode is Write Without Response only; a write REQUEST to it
         * is answered with ATT Write Not Permitted. Report Protocol is the
         * default on connection and is what we decode, so this only needs to
         * be nudged, not confirmed. */
        if (s_protocol_mode_handle != 0) {
            static const uint8_t report_mode = 0x01;
            const int rc = ble_gattc_write_no_rsp_flat(conn,
                                                       s_protocol_mode_handle,
                                                       &report_mode, 1);
            ESP_LOGI(TAG, "report protocol nudged (rc %d)", rc);
        }
        subscribe_next(conn);
        return 0;
    }

    if (err->status != 0 || chr == NULL) {
        return 0;
    }

    if (s_all_def_count < (int)(sizeof s_all_defs / sizeof s_all_defs[0])) {
        s_all_defs[s_all_def_count++] = chr->def_handle;
    }

    const uint16_t u = ble_uuid_u16(&chr->uuid.u);
    if (u == UUID_PROTOCOL_MODE) {
        s_protocol_mode_handle = chr->val_handle;
        return 0;
    }
    if (u == UUID_HID_CTRL_POINT) {
        s_ctrl_point_handle = chr->val_handle;
        return 0;
    }
    const bool notifiable = (chr->properties & BLE_GATT_CHR_PROP_NOTIFY) != 0;
    if (notifiable && (u == UUID_BOOT_KBD_IN || u == UUID_REPORT) &&
        s_chr_count < MAX_REPORT_CHRS) {
        s_chrs[s_chr_count].def_handle = chr->def_handle;
        s_chrs[s_chr_count].val_handle = chr->val_handle;
        s_chrs[s_chr_count].uuid       = u;
        s_chr_count++;
        ESP_LOGI(TAG, "found %s at val %u",
                 u == UUID_BOOT_KBD_IN ? "boot keyboard report" : "report",
                 chr->val_handle);
    }
    return 0;
}

static int on_svc(uint16_t conn, const struct ble_gatt_error *err,
                  const struct ble_gatt_svc *svc, void *arg)
{
    (void)arg;

    if (err->status == BLE_HS_EDONE) {
        if (s_svc_start == 0) {
            ESP_LOGE(TAG, "peer has no HID service - disconnecting");
            ble_gap_terminate(conn, BLE_ERR_REM_USER_CONN_TERM);
            return 0;
        }
        s_chr_count = 0;
        s_all_def_count = 0;
        s_protocol_mode_handle = 0;
        s_ctrl_point_handle = 0;
        const int rc = ble_gattc_disc_all_chrs(conn, s_svc_start, s_svc_end,
                                               on_chr, NULL);
        if (rc != 0) {
            ESP_LOGE(TAG, "characteristic discovery could not start: %d", rc);
        }
        return 0;
    }
    if (err->status != 0 || svc == NULL) {
        return 0;
    }
    s_svc_start = svc->start_handle;
    s_svc_end   = svc->end_handle;
    ESP_LOGI(TAG, "HID service at %u..%u", svc->start_handle, svc->end_handle);
    return 0;
}

static void discover_hid(uint16_t conn)
{
    s_svc_start = 0;
    s_svc_end   = 0;
    s_state = "discovering";
    const ble_uuid16_t hid = BLE_UUID16_INIT(UUID_HID_SVC);
    const int rc = ble_gattc_disc_svc_by_uuid(conn, &hid.u, on_svc, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "service discovery could not start: %d", rc);
    }
}

/* --------------------------------------------------------------------- GAP */

static bool adv_is_keyboard(const struct ble_hs_adv_fields *f)
{
    if (f->appearance_is_present && f->appearance == APPEARANCE_KEYBOARD) {
        return true;
    }
    for (int i = 0; i < f->num_uuids16; i++) {
        if (ble_uuid_u16(&f->uuids16[i].u) == UUID_HID_SVC) {
            return true;
        }
    }
    return false;
}

/* A keyboard we have already bonded with may advertise without the HID UUID,
 * so a known peer is worth connecting to on its address alone. */
static bool addr_is_bonded(const ble_addr_t *addr)
{
    ble_addr_t peers[CONFIG_BT_NIMBLE_MAX_BONDS];
    int count = 0;
    if (ble_store_util_bonded_peers(peers, &count,
                                    CONFIG_BT_NIMBLE_MAX_BONDS) != 0) {
        return false;
    }
    for (int i = 0; i < count; i++) {
        if (ble_addr_cmp(&peers[i], addr) == 0) {
            return true;
        }
    }
    return false;
}

static int gap_event(struct ble_gap_event *ev, void *arg)
{
    (void)arg;
    struct ble_hs_adv_fields f;

    switch (ev->type) {
    case BLE_GAP_EVENT_DISC:
        if (ble_hs_adv_parse_fields(&f, ev->disc.data, ev->disc.length_data) != 0) {
            return 0;
        }
        /* A keyboard match is ALWAYS logged; everything else only for the
         * first few. Capping both together meant the log went blind to the
         * one device that matters as soon as a busy room filled the quota. */
        {
            const bool match = adv_is_keyboard(&f);
            if (match || s_adv_logged < 16) {
                char nm[32] = "";
                if (f.name != NULL && f.name_len > 0) {
                    const int n = f.name_len < (int)sizeof nm - 1
                                  ? f.name_len : (int)sizeof nm - 1;
                    memcpy(nm, f.name, n);
                    nm[n] = '\0';
                }
                ESP_LOGI(TAG, "adv %02x:%02x:%02x:%02x:%02x:%02x rssi %d "
                              "name '%s' appearance %04x uuid16s %d%s",
                         ev->disc.addr.val[5], ev->disc.addr.val[4],
                         ev->disc.addr.val[3], ev->disc.addr.val[2],
                         ev->disc.addr.val[1], ev->disc.addr.val[0],
                         ev->disc.rssi, nm,
                         f.appearance_is_present ? f.appearance : 0,
                         f.num_uuids16,
                         match ? "   <-- KEYBOARD" : "");
                if (!match) {
                    s_adv_logged++;
                }
            }
            s_adv_seen++;
        }
        if (!adv_is_keyboard(&f) && !addr_is_bonded(&ev->disc.addr)) {
            return 0;
        }
        ESP_LOGI(TAG, "found a keyboard (%s), connecting",
                 f.name_len > 0 ? "named" : "unnamed");
        ble_gap_disc_cancel();
        s_state = "connecting";
        if (ble_gap_connect(s_own_addr_type, &ev->disc.addr, 10000,
                            NULL, gap_event, NULL) != 0) {
            scan_start();
        }
        return 0;

    case BLE_GAP_EVENT_CONNECT:
        if (ev->connect.status == 0) {
            s_conn = ev->connect.conn_handle;
            struct ble_gap_conn_desc d;
            if (ble_gap_conn_find(s_conn, &d) == 0) {
                ESP_LOGW(TAG, "CONNECTED TO %02x:%02x:%02x:%02x:%02x:%02x "
                              "(addr type %d)",
                         d.peer_id_addr.val[5], d.peer_id_addr.val[4],
                         d.peer_id_addr.val[3], d.peer_id_addr.val[2],
                         d.peer_id_addr.val[1], d.peer_id_addr.val[0],
                         d.peer_id_addr.type);
            }
            ESP_LOGI(TAG, "starting encryption");
            s_state = "pairing";
            /* If already bonded this resolves immediately from NVS. */
            if (ble_gap_security_initiate(s_conn) != 0) {
                discover_hid(s_conn);
            }
        } else {
            ESP_LOGW(TAG, "connect failed (status %d)", ev->connect.status);
            scan_start();
        }
        return 0;

    case BLE_GAP_EVENT_ENC_CHANGE:
        ESP_LOGI(TAG, "encryption change, status %d", ev->enc_change.status);
        discover_hid(ev->enc_change.conn_handle);
        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGW(TAG, "keyboard disconnected (reason %d) - rescanning",
                 ev->disconnect.reason);
        s_connected  = false;
        s_subscribed = false;
        s_held_usage = 0;
        s_conn = BLE_HS_CONN_HANDLE_NONE;
        emit(KBD_EV_DISCONNECTED, 0, false);
        scan_start();
        return 0;

    case BLE_GAP_EVENT_NOTIFY_RX: {
        const int len = OS_MBUF_PKTLEN(ev->notify_rx.om);
        uint8_t buf[16];
        if (len >= 1 && len <= (int)sizeof buf &&
            ble_hs_mbuf_to_flat(ev->notify_rx.om, buf, sizeof buf, NULL) == 0) {
            if (!s_connected) {
                s_connected = true;
                ESP_LOGI(TAG, "first report received - the keyboard is live");
            }
            if (s_reports_logged < 40) {
                ESP_LOGI(TAG, "report handle %u len %d: %02x %02x %02x %02x %02x %02x %02x %02x",
                         ev->notify_rx.attr_handle,
                         len, buf[0], buf[1], buf[2], buf[3],
                         len > 4 ? buf[4] : 0, len > 5 ? buf[5] : 0,
                         len > 6 ? buf[6] : 0, len > 7 ? buf[7] : 0);
                s_reports_logged++;
            }
            handle_report(buf, len);
        } else if (len > 0) {
            ESP_LOGW(TAG, "ignoring a %d-byte report", len);
        }
        return 0;
    }

    case BLE_GAP_EVENT_PASSKEY_ACTION: {
        if (ev->passkey.params.action == BLE_SM_IOACT_DISP) {
            struct ble_sm_io io = {
                .action  = BLE_SM_IOACT_DISP,
                .passkey = esp_random() % 1000000u,
            };
            ESP_LOGW(TAG, "=====================================");
            ESP_LOGW(TAG, "  TYPE THIS ON THE KEYBOARD: %06u",
                     (unsigned)io.passkey);
            ESP_LOGW(TAG, "  then press Enter");
            ESP_LOGW(TAG, "=====================================");
            s_state = "pairing";
            kbd_on_passkey((uint32_t)io.passkey);
            ble_sm_inject_io(ev->passkey.conn_handle, &io);
        } else if (ev->passkey.params.action == BLE_SM_IOACT_NUMCMP) {
            struct ble_sm_io io = { .action = BLE_SM_IOACT_NUMCMP,
                                    .numcmp_accept = 1 };
            ble_sm_inject_io(ev->passkey.conn_handle, &io);
        } else {
            ESP_LOGW(TAG, "unsupported passkey action %d",
                     ev->passkey.params.action);
        }
        return 0;
    }

    case BLE_GAP_EVENT_REPEAT_PAIRING: {
        /* The peer re-pairs: drop the stale bond and accept the new one.
         * Without this a keyboard that was reset pairs once and then fails
         * for ever, which is the "requires reboot after pairing" defect
         * docs/OS.md warns about in the prior art. */
        struct ble_gap_conn_desc desc;
        if (ble_gap_conn_find(ev->repeat_pairing.conn_handle, &desc) == 0) {
            ble_store_util_delete_peer(&desc.peer_id_addr);
        }
        return BLE_GAP_REPEAT_PAIRING_RETRY;
    }

    case BLE_GAP_EVENT_DISC_COMPLETE:
        if (!s_connected && s_conn == BLE_HS_CONN_HANDLE_NONE) {
            scan_start();
        }
        return 0;

    default:
        return 0;
    }
}

static void scan_start(void)
{
    struct ble_gap_disc_params p = {
        .itvl              = 0,
        .window            = 0,
        .filter_policy     = 0,
        .limited           = 0,
        .passive           = 0,
        .filter_duplicates = 1,
    };
    s_state = "scanning";
    const int rc = ble_gap_disc(s_own_addr_type, BLE_HS_FOREVER, &p, gap_event, NULL);
    if (rc != 0 && rc != BLE_HS_EALREADY) {
        ESP_LOGW(TAG, "scan failed to start: %d", rc);
    }
}

static void on_sync(void)
{
    if (s_clear_bonds_on_sync) {
        ble_store_clear();
        s_clear_bonds_on_sync = false;
    }
    ble_hs_util_ensure_addr(0);
    ble_hs_id_infer_auto(0, &s_own_addr_type);
    ESP_LOGI(TAG, "BLE ready, scanning for a keyboard");
    scan_start();
}

static void on_reset(int reason)
{
    ESP_LOGW(TAG, "BLE host reset, reason %d", reason);
    s_connected = false;
}

static void host_task(void *arg)
{
    (void)arg;
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void kbd_forget_all(void)
{
    ESP_LOGW(TAG, "forgetting all keyboard bonds");
    if (s_conn != BLE_HS_CONN_HANDLE_NONE) {
        ble_gap_terminate(s_conn, BLE_ERR_REM_USER_CONN_TERM);
    }
    ble_store_clear();
    s_connected  = false;
    s_subscribed = false;
    scan_start();
}

esp_err_t kbd_init(void)
{
    s_q = xQueueCreate(64, sizeof(kbd_event_t));
    if (s_q == NULL) {
        return ESP_ERR_NO_MEM;
    }

    const esp_err_t err = nimble_port_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nimble_port_init failed: %s", esp_err_to_name(err));
        return err;
    }

    ble_hs_cfg.sync_cb  = on_sync;
    ble_hs_cfg.reset_cb = on_reset;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    /* A bond created under a different pairing scheme is worse than none: the
     * peer reuses it and never re-pairs, so the new scheme never takes effect.
     * Cleared once per scheme change, not on every boot. */
    {
        nvs_handle_t nh;
        uint8_t ver = 0;
        if (nvs_open("deck", NVS_READWRITE, &nh) == ESP_OK) {
            nvs_get_u8(nh, "pair_ver", &ver);
            if (ver != KBD_PAIR_SCHEME) {
                ESP_LOGW(TAG, "pairing scheme changed - clearing old bonds");
                s_clear_bonds_on_sync = true;
                nvs_set_u8(nh, "pair_ver", KBD_PAIR_SCHEME);
                nvs_commit(nh);
            }
            nvs_close(nh);
        }
    }

    /* The deck has a screen and the keyboard has keys, so the correct
     * association model is Passkey Entry with US displaying: we show six
     * digits, they are typed on the keyboard, and the link comes up
     * MITM-authenticated.
     *
     * Declaring NoInputNoOutput instead degrades this to Just Works, which
     * pairs, bonds and encrypts perfectly happily - and then a HID keyboard
     * that requires an authenticated link simply never sends an input report.
     * The link looks healthy, the battery service notifies, and not one
     * keystroke arrives. That is exactly the symptom this hardware showed. */
    ble_hs_cfg.sm_io_cap    = BLE_HS_IO_DISPLAY_ONLY;
    ble_hs_cfg.sm_bonding   = 1;
    ble_hs_cfg.sm_mitm      = 1;
    ble_hs_cfg.sm_sc        = 1;
    ble_hs_cfg.sm_our_key_dist  = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    ble_hs_cfg.sm_their_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;

    /* No GAP service is registered: we are a central only, we never
     * advertise, and the peripheral role is compiled out entirely. */

    nimble_port_freertos_init(host_task);
    xTaskCreate(repeat_task, "kbd_repeat", 2560, NULL, 5, NULL);
    return ESP_OK;
}
