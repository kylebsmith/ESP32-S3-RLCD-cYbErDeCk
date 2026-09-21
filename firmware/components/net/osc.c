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
#include "lwip/inet.h"

static const char *TAG = "osc";

static int                s_sock = -1;
static struct sockaddr_in s_to;
static uint32_t           s_msgs, s_packets;

/* One datagram per drained step, built here and sent by net_osc_flush(). 512
 * bytes is comfortably inside the 1500-byte MTU and more than eight lanes can
 * produce in one step. */
static uint8_t s_buf[512];
static int     s_len;

void net_osc_counts(uint32_t *m, uint32_t *p)
{
    if (m != NULL) { *m = s_msgs; }
    if (p != NULL) { *p = s_packets; }
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
    /* Non-blocking, because this is called from the MIDI task and a socket
     * that can block is a clock that can stall. */
    int fl = fcntl(s_sock, F_GETFL, 0);
    fcntl(s_sock, F_SETFL, fl | O_NONBLOCK);
    ESP_LOGW(TAG, "osc -> %s:%d", ip, port);
    return ESP_OK;
}

/* OSC strings are null-terminated and padded with nulls to a multiple of four.
 * Returns false when the buffer is full rather than truncating - a malformed
 * packet is worse than a dropped one, because a receiver may reject the whole
 * datagram and lose the messages that were fine. */
static bool put_str(const char *s)
{
    const int n = (int)strlen(s) + 1;
    const int pad = (4 - (n % 4)) % 4;
    if (s_len + n + pad > (int)sizeof s_buf) {
        return false;
    }
    memcpy(&s_buf[s_len], s, (size_t)n);
    s_len += n;
    memset(&s_buf[s_len], 0, (size_t)pad);
    s_len += pad;
    return true;
}

static bool put_i32(int32_t v)
{
    if (s_len + 4 > (int)sizeof s_buf) {
        return false;
    }
    s_buf[s_len++] = (uint8_t)(v >> 24);
    s_buf[s_len++] = (uint8_t)(v >> 16);
    s_buf[s_len++] = (uint8_t)(v >> 8);
    s_buf[s_len++] = (uint8_t)v;
    return true;
}

void net_osc_send(const char *lane, uint8_t status, uint8_t d1, uint8_t d2,
                  uint32_t when_us)
{
    (void)when_us;
    if (s_sock < 0) {
        return;
    }
    /* Note-offs and the transport bytes are not sent. A visual patch wants to
     * know that a thing HAPPENED; a note-off is a MIDI housekeeping detail and
     * sending it would double the traffic to say nothing. */
    const uint8_t type = status & 0xF0;
    char addr[40];
    if (type == 0x90 && d2 > 0) {
        snprintf(addr, sizeof addr, "/deck/%s", (lane && *lane) ? lane : "x");
    } else if (type == 0xB0) {
        snprintf(addr, sizeof addr, "/deck/%s", (lane && *lane) ? lane : "cc");
    } else {
        return;
    }

    const int mark = s_len;
    if (!put_str(addr) || !put_str(",ii") ||
        !put_i32((int32_t)d1) || !put_i32((int32_t)d2)) {
        s_len = mark;            /* leave the datagram valid */
        return;
    }
    s_msgs++;
}

void net_osc_flush(void)
{
    if (s_sock < 0 || s_len == 0) {
        s_len = 0;
        return;
    }
    const int n = s_len;
    s_len = 0;
    /* One datagram per step. Fire and forget: a failed send is a dropped
     * frame, and a dropped frame is better than a stalled clock. */
    if (sendto(s_sock, s_buf, (size_t)n, 0,
               (struct sockaddr *)&s_to, sizeof s_to) > 0) {
        s_packets++;
    }
}
