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

static int adv_event(struct ble_gap_event *ev, void *arg)
{
    (void)arg;
    switch (ev->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (ev->connect.status == 0) {
            s_conn = ev->connect.conn_handle;
            ESP_LOGI(TAG, "a host connected for MIDI");
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

void blemidi_send(uint8_t status, uint8_t d1, uint8_t d2)
{
    if (!blemidi_connected()) {
        return;
    }
    /* BLE-MIDI framing: a header byte carrying the top six bits of a 13-bit
     * millisecond timestamp, then a timestamp byte, then the message. Both
     * have the high bit set, which is what distinguishes them from data. */
    const uint32_t ts = (uint32_t)(esp_timer_get_time() / 1000) & 0x1FFF;
    uint8_t pkt[5];
    int n = 0;
    pkt[n++] = (uint8_t)(0x80 | ((ts >> 7) & 0x3F));
    pkt[n++] = (uint8_t)(0x80 | (ts & 0x7F));
    pkt[n++] = status;
    pkt[n++] = d1;
    /* Program change and channel pressure are two bytes; everything the
     * sequencer emits is three. */
    const uint8_t type = status & 0xF0;
    if (type != 0xC0 && type != 0xD0) {
        pkt[n++] = d2;
    }

    struct os_mbuf *om = ble_hs_mbuf_from_flat(pkt, (uint16_t)n);
    if (om == NULL) {
        return;                      /* out of mbufs; drop rather than block */
    }
    ble_gatts_notify_custom(s_conn, s_chr_handle, om);
}
