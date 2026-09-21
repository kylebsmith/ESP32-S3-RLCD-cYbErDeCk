/*
 * An SSH client. See ssh.h for why this runs a command rather than a terminal.
 */
#include "ssh.h"

#include <stdio.h>
#include <string.h>

#include "docstore.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "libssh2.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"

static const char *TAG = "ssh";
static char s_last[40] = "ssh idle";

void ssh_status(char *out, size_t max) { snprintf(out, max, "%s", s_last); }

/* THE SESSION LIVES IN PSRAM.
 *
 * libssh2 allocates on the order of 80 KB for a session, and the internal SRAM
 * on this part is already carrying the framebuffer, the document buffers,
 * NimBLE and the sequencer. Handing libssh2 the external RAM keeps an optional
 * feature from competing with the things that must never stall. The crypto
 * itself is mbedTLS, which IDF has already placed. */
static void *ss_alloc(size_t n, void **abs)  { (void)abs;
    void *p = heap_caps_malloc(n, MALLOC_CAP_SPIRAM);
    return p ? p : malloc(n); }
static void  ss_free(void *p, void **abs)    { (void)abs; free(p); }
static void *ss_realloc(void *p, size_t n, void **abs) { (void)abs;
    void *q = heap_caps_realloc(p, n, MALLOC_CAP_SPIRAM);
    return q ? q : realloc(p, n); }

static void out_line(int buf, const char *s) { doc_buf_append(buf, s); }

esp_err_t ssh_run(const char *user, const char *host, int port,
                  const char *pass, const char *cmd)
{
    if (!user || !host || !cmd) {
        return ESP_ERR_INVALID_ARG;
    }
    const int out = doc_buf_ensure("+ssh");
    if (out < 0) {
        return ESP_ERR_NO_MEM;
    }

    char line[96];
    snprintf(line, sizeof line, "$ %.90s", cmd);
    out_line(out, line);

    /* Resolve and connect. Everything below reports through the SAME buffer
     * the output goes to, so a failure is visible where the owner is already
     * looking rather than only in a log they may not have. */
    struct addrinfo hints = { .ai_family = AF_INET, .ai_socktype = SOCK_STREAM };
    struct addrinfo *res = NULL;
    char portstr[8];
    snprintf(portstr, sizeof portstr, "%u",
             (unsigned)((port > 0 && port < 65536) ? port : 22));
    if (getaddrinfo(host, portstr, &hints, &res) != 0 || res == NULL) {
        out_line(out, "cannot resolve that host");
        snprintf(s_last, sizeof s_last, "ssh: no such host");
        return ESP_FAIL;
    }
    const int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        freeaddrinfo(res);
        return ESP_FAIL;
    }
    if (connect(sock, res->ai_addr, res->ai_addrlen) != 0) {
        freeaddrinfo(res);
        close(sock);
        out_line(out, "connection refused");
        snprintf(s_last, sizeof s_last, "ssh: refused");
        return ESP_FAIL;
    }
    freeaddrinfo(res);

    esp_err_t rv = ESP_FAIL;
    LIBSSH2_SESSION *ses = NULL;
    LIBSSH2_CHANNEL *ch = NULL;

    if (libssh2_init(0) != 0) {
        out_line(out, "ssh library would not start");
        goto done;
    }
    ses = libssh2_session_init_ex(ss_alloc, ss_free, ss_realloc, NULL);
    if (ses == NULL) {
        out_line(out, "no memory for a session");
        goto done;
    }
    libssh2_session_set_blocking(ses, 1);
    /* A bounded timeout, because this runs on the editor task: an SSH server
     * that never answers must not become a deck that never redraws. */
    libssh2_session_set_timeout(ses, 15000);

    if (libssh2_session_handshake(ses, sock) != 0) {
        out_line(out, "handshake failed");
        goto done;
    }
    /* THE HOST KEY IS REPORTED, NOT VERIFIED, AND THAT IS SAID OUT LOUD.
     *
     * A known-hosts file would need somewhere to live and a way to be edited,
     * which is a real design question and not one to answer silently. Until it
     * is answered the fingerprint is printed into the output buffer so the
     * owner can see it change. This is weaker than ssh(1) and the buffer says
     * so rather than implying otherwise. */
    const char *fp = libssh2_hostkey_hash(ses, LIBSSH2_HOSTKEY_HASH_SHA1);
    if (fp != NULL) {
        char h[64] = "key ";
        size_t k = strlen(h);
        for (int i = 0; i < 8 && k + 3 < sizeof h; i++) {
            k += (size_t)snprintf(h + k, sizeof h - k, "%02x",
                                  (unsigned char)fp[i]);
        }
        out_line(out, h);
        out_line(out, "(not checked against a known list)");
    }

    if (libssh2_userauth_password(ses, user, pass ? pass : "") != 0) {
        out_line(out, "password refused");
        snprintf(s_last, sizeof s_last, "ssh: auth failed");
        goto done;
    }
    ch = libssh2_channel_open_session(ses);
    if (ch == NULL) {
        out_line(out, "server refused a channel");
        goto done;
    }
    if (libssh2_channel_exec(ch, cmd) != 0) {
        out_line(out, "could not run it");
        goto done;
    }

    /* Read the whole reply, splitting on newlines so it lands as lines in a
     * document rather than one long wrap. Bounded: a command that prints
     * forever must not fill the buffer and take the deck with it. */
    char buf[256];
    char acc[128];
    size_t alen = 0;
    int lines = 0;
    for (;;) {
        const ssize_t n = libssh2_channel_read(ch, buf, sizeof buf);
        if (n <= 0) {
            break;
        }
        for (ssize_t i = 0; i < n; i++) {
            const char c = buf[i];
            if (c == '\n' || alen == sizeof acc - 1) {
                acc[alen] = '\0';
                out_line(out, acc);
                alen = 0;
                if (++lines >= 200) {
                    out_line(out, "... truncated at 200 lines");
                    goto reply_done;
                }
            } else if (c != '\r') {
                acc[alen++] = c;
            }
        }
    }
reply_done:
    if (alen > 0) {
        acc[alen] = '\0';
        out_line(out, acc);
        lines++;
    }
    snprintf(s_last, sizeof s_last, "ssh: %d line%s", lines,
             lines == 1 ? "" : "s");
    rv = ESP_OK;

done:
    if (ch != NULL)  { libssh2_channel_free(ch); }
    if (ses != NULL) { libssh2_session_disconnect(ses, "bye");
                       libssh2_session_free(ses); }
    libssh2_exit();
    close(sock);
    ESP_LOGW(TAG, "%s", s_last);
    return rv;
}
