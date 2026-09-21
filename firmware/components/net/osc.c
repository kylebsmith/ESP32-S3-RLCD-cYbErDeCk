/*
 * OSC over UDP. The visual half of the destination model.
 *
 * WHY OSC AND NOT MIDI OVER THE WIRE. MIDI discards which lane fired: a kick
 * is note 36 on channel 10, and the receiver has to already share that
 * convention. A visual patch cannot work from that - it wants to bind to
 * '/deck/kick' and '/deck/cut' by name. Carrying the lane name is the whole
 * reason this destination exists, and it is why seq_sink_t gained a `lane`
 * argument rather than OSC being bolted on beside MIDI.
 *
 * WHY UDP AND FIRE-AND-FORGET. A late visual frame is worse than a missing
 * one, and a retransmit is a late frame by definition. sendto() on a
 * non-blocking socket either goes or does not, and either way the clock does
 * not wait - which is the same rule the MIDI queue already follows.
 *
 * BORROWED: the packet layout is OSC 1.0 (opensoundcontrol.org/spec-1_0),
 * which is four-byte aligned, big-endian, and simple enough that a dependency
 * would cost more than it saves. Bundles are deliberately NOT used: a bundle
 * buys atomic timing that a 1 ms-jitter visual pipeline cannot perceive, and
 * costs a 16-byte header plus a size word per message.
 */
#include "net.h"

#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "lwip/sockets.h"
#include "osc_pack.h"
#include "lwip/inet.h"

static const char *TAG = "osc";

static int                s_sock = -1;
static struct sockaddr_in s_to;
static uint32_t           s_msgs, s_packets;

/* One datagram per drained step. 512 bytes is inside any MTU and more than
 * eight lanes can produce in a step. */
static uint8_t s_raw[512];
static osc_t   s_pk;
static bool    s_open;

void net_osc_counts(uint32_t *m, uint32_t *p)
{
    if (m != NULL) { *m = s_msgs; }
    if (p != NULL) { *p = s_packets; }
}

static void pk_open(void)
{
    if (!s_open) {
        osc_init(&s_pk, s_raw, (int)sizeof s_raw);
        s_open = true;
    }
}

esp_err_t net_osc_target(const char *ip, int port)
{
    if (s_sock >= 0) {
        close(s_sock);
        s_sock = -1;
    }
    if (port <= 0 || port > 65535 || ip == NULL) {
        ESP_LOGW(TAG, "osc off");
        return ESP_OK;
    }
    memset(&s_to, 0, sizeof s_to);
    s_to.sin_family = AF_INET;
    s_to.sin_port   = htons((uint16_t)port);
    if (inet_pton(AF_INET, ip, &s_to.sin_addr) != 1) {
        ESP_LOGE(TAG, "'%s' is not an address", ip);
        return ESP_ERR_INVALID_ARG;
    }
    s_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s_sock < 0) {
        return ESP_FAIL;
    }
    /* Non-blocking: this is called from the MIDI task, and a socket that can
     * block is a clock that can stall. */
    const int fl = fcntl(s_sock, F_GETFL, 0);
    fcntl(s_sock, F_SETFL, fl | O_NONBLOCK);
    ESP_LOGW(TAG, "osc -> %s:%d", ip, port);
    return ESP_OK;
}

void net_osc_send(const char *lane, uint8_t status, uint8_t d1, uint8_t d2,
                  uint32_t when_us)
{
    (void)when_us;
    if (s_sock < 0) {
        return;
    }
    pk_open();

    /* Note-offs and transport bytes are not sent: a visual wants to know a
     * thing HAPPENED, and a note-off would double the traffic to say nothing.
     * The transport clock goes out as /deck/step instead, once per step, which
     * is a receiver's whole timebase in one message. */
    /* The step marker the sequencer emits for exactly this purpose. */
    if (status == 0xF9) {
        if (osc_msg_i(&s_pk, "/deck/step", (int32_t)d1)) {
            s_msgs++;
        }
        return;
    }
    const uint8_t type = status & 0xF0;
    if (!((type == 0x90 && d2 > 0) || type == 0xB0)) {
        return;
    }
    char addr[40];
    snprintf(addr, sizeof addr, "/deck/%s",
             (lane && *lane) ? lane : (type == 0xB0 ? "cc" : "x"));
    if (osc_msg_ii(&s_pk, addr, (int32_t)d1, (int32_t)d2)) {
        s_msgs++;
    }
}

void net_osc_step(int step)
{
    if (s_sock < 0) {
        return;
    }
    pk_open();
    if (osc_msg_i(&s_pk, "/deck/step", (int32_t)step)) {
        s_msgs++;
    }
}

esp_err_t net_osc_frame(const char *text)
{
    if (s_sock < 0 || text == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    /* A frame goes in its OWN datagram rather than riding with a step: it is
     * up to a few hundred bytes of ASCII and would push the step packet past
     * anything worth calling small. */
    static uint8_t raw[1100];
    osc_t o;
    osc_init(&o, raw, (int)sizeof raw);
    if (!osc_msg_s(&o, "/deck/frame", text)) {
        return ESP_ERR_INVALID_SIZE;
    }
    if (sendto(s_sock, raw, (size_t)o.len, 0,
               (struct sockaddr *)&s_to, sizeof s_to) <= 0) {
        return ESP_FAIL;
    }
    s_msgs++;
    s_packets++;
    return ESP_OK;
}

void net_osc_flush(void)
{
    if (s_sock < 0 || !s_open || s_pk.len == 0) {
        s_open = false;
        return;
    }
    const int n = s_pk.len;
    s_open = false;
    /* Fire and forget. A failed send is a dropped frame, and a dropped frame
     * is better than a stalled clock. */
    if (sendto(s_sock, s_raw, (size_t)n, 0,
               (struct sockaddr *)&s_to, sizeof s_to) > 0) {
        s_packets++;
    }
}
