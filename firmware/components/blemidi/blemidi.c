#include "blemidi.h"

#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "host/ble_gap.h"
#include "host/ble_hs.h"
#include "os/os_mbuf.h"
#include "services/gatt/ble_svc_gatt.h"

static const char *TAG = "blemidi";

/* BLE-MIDI 1.0 UUIDs, little-endian byte order as NimBLE wants them. */
static const ble_uuid128_t MIDI_SVC_UUID = BLE_UUID128_INIT(
    0x00, 0xC7, 0xC4, 0x4E, 0xE3, 0x6C, 0x51, 0xA7,
    0x33, 0x4B, 0xE8, 0xED, 0x5A, 0x0E, 0xB8, 0x03);

static const ble_uuid128_t MIDI_CHR_UUID = BLE_UUID128_INIT(
    0xF3, 0x6B, 0x10, 0x9D, 0x66, 0xF2, 0xA9, 0xA1,
    0x12, 0x41, 0x68, 0x38, 0xDB, 0xE5, 0x72, 0x77);

static uint16_t s_chr_handle;
static uint16_t s_conn = BLE_HS_CONN_HANDLE_NONE;
static bool     s_subscribed;
static uint8_t  s_own_addr_type;

bool blemidi_connected(void)
{
    return s_conn != BLE_HS_CONN_HANDLE_NONE && s_subscribed;
}

static int chr_access(uint16_t conn, uint16_t attr, struct ble_gatt_access_ctxt *ctxt,
                      void *arg)
{
    (void)conn; (void)attr; (void)arg;
    /* A host may write to us (MIDI in). Nothing consumes it yet, but the
     * characteristic must be writable or some hosts refuse the service. */
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        return 0;
    }
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        return 0;                    /* an empty read is legal BLE-MIDI */
    }
    return BLE_ATT_ERR_UNLIKELY;
}

static const struct ble_gatt_svc_def s_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &MIDI_SVC_UUID.u,
        .characteristics = (struct ble_gatt_chr_def[]) { {
            .uuid = &MIDI_CHR_UUID.u,
            .access_cb = chr_access,
            .val_handle = &s_chr_handle,
            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE_NO_RSP |
                     BLE_GATT_CHR_F_NOTIFY,
        }, { 0 } },
    },
    { 0 },
};

static int adv_event(struct ble_gap_event *ev, void *arg);

static void advertise(void)
{
    /* The 128-bit service UUID and the name do not both fit. An advertising
     * payload is 31 bytes: flags costs 3, a 128-bit UUID costs 18, and
     * "cyberdeck" costs 11 - which is 32, and the controller rejects it with
     * BLE_HS_EMSGSIZE.
     *
     * macOS will not list a device in Audio MIDI Setup unless the MIDI
     * service UUID is in the ADVERTISEMENT, so the UUID stays and the name
     * moves to the scan response, which is a second 31 bytes the host asks
     * for separately. This is the ordinary way round for BLE and the reason
     * scan responses exist. */
    struct ble_hs_adv_fields f;
    memset(&f, 0, sizeof f);
    f.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    f.uuids128 = (ble_uuid128_t *)&MIDI_SVC_UUID;
    f.num_uuids128 = 1;
    f.uuids128_is_complete = 1;

    int rc = ble_gap_adv_set_fields(&f);
    if (rc != 0) {
        ESP_LOGE(TAG, "adv fields rejected: %d", rc);
        return;
    }

    struct ble_hs_adv_fields rsp;
    memset(&rsp, 0, sizeof rsp);
    rsp.name = (uint8_t *)"cyberdeck";
    rsp.name_len = 9;
    rsp.name_is_complete = 1;
    rc = ble_gap_adv_rsp_set_fields(&rsp);
    if (rc != 0) {
        ESP_LOGW(TAG, "scan response rejected: %d", rc);
    }

    struct ble_gap_adv_params p;
    memset(&p, 0, sizeof p);
    p.conn_mode = BLE_GAP_CONN_MODE_UND;
    p.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(s_own_addr_type, NULL, BLE_HS_FOREVER, &p,
                           adv_event, NULL);
    if (rc == BLE_HS_EALREADY) {
        return;
    }
    if (rc != 0) {
        ESP_LOGE(TAG, "advertising failed to start: %d", rc);
    } else {
        ESP_LOGI(TAG, "advertising as 'cyberdeck' - pair from Audio MIDI Setup");
    }
}

/* THE CONNECTION INTERVAL IS THE JITTER FLOOR, AND WE WERE NOT ASKING FOR ONE.
 *
 * A BLE peripheral can only transmit during a connection event, so the
 * interval the central grants is a hard quantisation on every note: notes are
 * delivered in buckets that wide, however precise the sequencer's own clock
 * is. Measured on this device, the clock holds to well under a millisecond -
 * so if the player hears jitter, this is where it is coming from.
 *
 * The BLE-MIDI specification asks for the shortest interval the central will
 * grant, and THIS FIRMWARE NEVER REQUESTED ONE - it accepted whatever macOS
 * chose. That is a spec violation and, on the evidence, the largest single
 * term this device can control.
 *
 * 6 units x 1.25 ms = 7.5 ms, the minimum BLE permits. It is a REQUEST: the
 * central may refuse or counter, which is why the granted value is logged
 * rather than assumed. Logged on connect and again on every update, because
 * a central may change it later - macOS is known to relax intervals to save
 * power once a link looks idle. */
static void report_and_request_interval(uint16_t conn)
{
    struct ble_gap_conn_desc d;
    if (ble_gap_conn_find(conn, &d) == 0) {
        ESP_LOGW(TAG, "link: interval %u us, latency %u, timeout %u ms",
                 (unsigned)(d.conn_itvl * 1250), (unsigned)d.conn_latency,
                 (unsigned)(d.supervision_timeout * 10));
    }
    const struct ble_gap_upd_params p = {
        .itvl_min = 6, .itvl_max = 6,       /* 7.5 ms, the BLE minimum */
        .latency = 0,
        .supervision_timeout = 200,         /* 2 s */
        .min_ce_len = 0, .max_ce_len = 0,
    };
    const int rc = ble_gap_update_params(conn, &p);
    if (rc != 0) {
        ESP_LOGW(TAG, "interval request refused locally: %d", rc);
    }
}

static int adv_event(struct ble_gap_event *ev, void *arg)
{
    (void)arg;
    switch (ev->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (ev->connect.status == 0) {
            s_conn = ev->connect.conn_handle;
            ESP_LOGI(TAG, "a host connected for MIDI");
            report_and_request_interval(s_conn);
        } else {
            advertise();
        }
        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGW(TAG, "MIDI host disconnected (reason %d)",
                 ev->disconnect.reason);
        s_conn = BLE_HS_CONN_HANDLE_NONE;
        s_subscribed = false;
        advertise();
        return 0;

    case BLE_GAP_EVENT_SUBSCRIBE:
        if (ev->subscribe.attr_handle == s_chr_handle) {
            s_subscribed = ev->subscribe.cur_notify != 0;
            ESP_LOGI(TAG, "host %s notifications",
                     s_subscribed ? "wants" : "dropped");
        }
        return 0;

    case BLE_GAP_EVENT_CONN_UPDATE: {
        /* What the central actually granted. This is the number that belongs
         * in a paper, not the one that was asked for. */
        struct ble_gap_conn_desc d;
        if (ble_gap_conn_find(ev->conn_update.conn_handle, &d) == 0) {
            ESP_LOGW(TAG, "link updated: interval %u us, latency %u",
                     (unsigned)(d.conn_itvl * 1250), (unsigned)d.conn_latency);
        }
        return 0;
    }

    case BLE_GAP_EVENT_MTU:
        return 0;

    default:
        return 0;
    }
}

void blemidi_register(void)
{
    ble_svc_gatt_init();
    int rc = ble_gatts_count_cfg(s_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "count_cfg failed: %d", rc);
        return;
    }
    rc = ble_gatts_add_svcs(s_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "add_svcs failed: %d", rc);
        return;
    }
    ESP_LOGI(TAG, "MIDI service registered");
}

void blemidi_start(void)
{
    ble_hs_id_infer_auto(0, &s_own_addr_type);
    advertise();
}

/* THE PACKET BUFFER.
 *
 * Bounded by the negotiated ATT MTU minus the three bytes of notification
 * header. The default MTU is 23, so 20 bytes - a header, then up to five
 * timestamped three-byte messages, which is more than any single step of a
 * pattern can produce on eight lanes. If a step ever did overflow it, the
 * buffer is flushed and a new packet started rather than anything being
 * dropped: a note late by one connection interval is bad, a note gone is
 * worse. */
#define MIDI_PKT_MAX 64
static uint8_t  s_pkt[MIDI_PKT_MAX];
static int      s_pkt_n;
static uint8_t  s_pkt_hdr;
static uint32_t s_packed;      /* messages sent */
static uint32_t s_packets;     /* notifications used to send them */

void blemidi_packing(uint32_t *msgs, uint32_t *packets)
{
    if (msgs    != NULL) { *msgs    = s_packed;  }
    if (packets != NULL) { *packets = s_packets; }
}

static int pkt_limit(void)
{
    const int mtu = (s_conn == BLE_HS_CONN_HANDLE_NONE)
                    ? 23 : (int)ble_att_mtu(s_conn);
    int lim = mtu - 3;
    if (lim < 5)            { lim = 5; }
    if (lim > MIDI_PKT_MAX) { lim = MIDI_PKT_MAX; }
    return lim;
}

void blemidi_flush(void)
{
    if (s_pkt_n == 0 || !blemidi_connected()) {
        s_pkt_n = 0;
        return;
    }
    struct os_mbuf *om = ble_hs_mbuf_from_flat(s_pkt, (uint16_t)s_pkt_n);
    s_pkt_n = 0;
    if (om == NULL) {
        return;                      /* out of mbufs; drop rather than block */
    }
    s_packets++;
    ble_gatts_notify_custom(s_conn, s_chr_handle, om);
}

void blemidi_send(uint8_t status, uint8_t d1, uint8_t d2)
{
    if (!blemidi_connected()) {
        return;
    }
    /* BLE-MIDI framing: a header byte carrying the top six bits of a 13-bit
     * millisecond timestamp, then a timestamp byte before EVERY message, then
     * the message. Both have the high bit set, which is what distinguishes
     * them from data. */
    const uint32_t ts = (uint32_t)(esp_timer_get_time() / 1000) & 0x1FFF;
    const uint8_t hdr = (uint8_t)(0x80 | ((ts >> 7) & 0x3F));

    /* How many data bytes follow is a property of the status byte, and
     * getting it wrong does not fail loudly - it shifts every later byte and
     * the far end reads garbage as notes. MIDI clock in particular is a
     * SINGLE byte; sending it as three would inject two zero bytes into the
     * stream, which a receiver reads as a note-off on channel 1.
     *
     * System messages (0xF0 and up) are not channel messages and their
     * lengths do not follow the 0xF0 mask, so they are decided first. */
    int data;
    if (status >= 0xF0) {
        switch (status) {
        case 0xF1: case 0xF3: data = 1; break;   /* MTC, song select       */
        case 0xF2:            data = 2; break;   /* song position pointer  */
        default:              data = 0; break;   /* clock, start, stop ... */
        }
    } else {
        const uint8_t type = status & 0xF0;
        data = (type == 0xC0 || type == 0xD0) ? 1 : 2;
    }

    const int need = 1 + 1 + data;               /* ts byte + status + data */
    /* A packet carries ONE header, so every message in it must share the top
     * six timestamp bits. When they stop agreeing - once every 128 ms - the
     * packet is closed rather than mis-stamped. */
    if (s_pkt_n > 0 && (s_pkt_hdr != hdr || s_pkt_n + need > pkt_limit())) {
        blemidi_flush();
    }
    if (s_pkt_n == 0) {
        s_pkt_hdr = hdr;
        s_pkt[s_pkt_n++] = hdr;
    }
    s_pkt[s_pkt_n++] = (uint8_t)(0x80 | (ts & 0x7F));
    s_pkt[s_pkt_n++] = status;
    if (data >= 1) { s_pkt[s_pkt_n++] = d1; }
    if (data >= 2) { s_pkt[s_pkt_n++] = d2; }
    s_packed++;
}
