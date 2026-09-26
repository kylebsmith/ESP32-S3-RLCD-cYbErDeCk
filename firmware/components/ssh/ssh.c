/*
 * An SSH client. See ssh.h for why this runs a command rather than a terminal.
 */
#include "ssh.h"

#include <stdio.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>

#include "docstore.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/message_buffer.h"
#include "freertos/task.h"
#include "libssh2.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "nvs.h"
#include "ssh_fp.h"

static const char *TAG = "ssh";
static char s_last[40] = "ssh idle";

void ssh_status(char *out, size_t max) { snprintf(out, max, "%s", s_last); }

/* ---- a session is its own task ------------------------------------------
 *
 * IT RAN ON THE EDITOR'S TASK, which is under a 10 s watchdog that panics. A
 * host that never answers holds connect() through twelve SYN retries, far past
 * 10 s, so '>ssh' to a wrong address would reboot the deck mid-set; a slow
 * handshake could too. Now the session has a task of its own, the editor never
 * waits for it, and everything the session says comes back through a message
 * buffer that the editor's loop empties into '+out' - ssh_service(). Docstore
 * belongs to the editor's task; only ssh_service touches it.
 *
 * One session at a time, and its stack is PSRAM. With the WiFi station up the
 * deck has about 16 KB of internal RAM left (measured, 2026-09-25), and mbedTLS
 * is configured to allocate only from internal RAM, so the stack does not get
 * to spend it. A task with its stack in PSRAM must never touch flash, so the
 * session never does: the host's kept key is read before it starts and written
 * after it ends, both from the editor's task. */
#define SSH_STACK 16384

typedef struct {
    char user[33], host[64], pass[64], cmd[128];
    int  port;
    bool kept;                 /* a key was kept for this host...            */
    uint8_t key[32];           /* ...and this is it; or the new one to keep  */
    bool keep_new;             /* the session met this host for the first time */
} ssh_job_t;

static ssh_job_t s_job;
static MessageBufferHandle_t s_said;
static volatile int s_state;          /* SSH_IDLE, SSH_RUNNING, SSH_DONE */
enum { SSH_IDLE = 0, SSH_RUNNING, SSH_DONE };
static size_t s_reply_from;
static bool s_reply_started;

static void wipe(char *p, size_t n)
{
    volatile char *v = p;
    for (size_t i = 0; i < n; i++) {
        v[i] = '\0';
    }
}

/* One line from the session to the editor. If the editor is not emptying the
 * buffer, the line is dropped after a second rather than stalling the session
 * forever. */
static void say(const char *s)
{
    if (s_said != NULL) {
        xMessageBufferSend(s_said, s, strlen(s), pdMS_TO_TICKS(1000));
    }
}

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

/* ---- the host's key ---------------------------------------------------------
 *
 * KEPT THE FIRST TIME, AND A CHANGED ONE IS REFUSED, BEFORE ANY PASSWORD IS SENT.
 *
 * The key used to be shown and never checked. With a password that is not a
 * cosmetic gap: anyone on the same network who answers in the host's place is
 * handed the password - encrypted, but to them. So a host's
 * first key is kept in NVS - not a document; nothing here reaches the card - and
 * from then on a different key stops the session before authentication. The
 * first connection is still taken on trust, as ssh(1) takes it; the fingerprint
 * is printed as `ssh-keygen -lf` prints it so that trust can be checked by eye.
 * docs/NETWORK.md has the decision. */
#define KNOWN_NS "sshkeys"

/* From the editor's task only: see SSH_STACK. */
static bool read_kept(const char *host, int port, uint8_t out[32])
{
    char slot[16];
    ssh_fp_slot(host, port, slot);
    nvs_handle_t h;
    if (nvs_open(KNOWN_NS, NVS_READONLY, &h) != ESP_OK) {
        return false;
    }
    size_t n = 32;
    const bool ok = nvs_get_blob(h, slot, out, &n) == ESP_OK && n == 32;
    nvs_close(h);
    return ok;
}

static esp_err_t keep(const char *host, int port, const uint8_t fp[32])
{
    char slot[16];
    ssh_fp_slot(host, port, slot);
    nvs_handle_t h;
    esp_err_t e = nvs_open(KNOWN_NS, NVS_READWRITE, &h);
    if (e != ESP_OK) {
        return e;
    }
    e = nvs_set_blob(h, slot, fp, 32);
    if (e == ESP_OK) {
        e = nvs_commit(h);
    }
    nvs_close(h);
    return e;
}

esp_err_t ssh_forget(const char *host, int port)
{
    char slot[16];
    ssh_fp_slot(host, port > 0 ? port : 22, slot);
    nvs_handle_t h;
    esp_err_t e = nvs_open(KNOWN_NS, NVS_READWRITE, &h);
    if (e != ESP_OK) {
        return e;
    }
    e = nvs_erase_key(h, slot);
    if (e == ESP_OK) {
        e = nvs_commit(h);
    }
    nvs_close(h);
    return e;
}

static const char *key_type(int t)
{
    switch (t) {
    case LIBSSH2_HOSTKEY_TYPE_RSA:       return "key ssh-rsa";
    case LIBSSH2_HOSTKEY_TYPE_ECDSA_256: return "key ecdsa-sha2-nistp256";
    case LIBSSH2_HOSTKEY_TYPE_ECDSA_384: return "key ecdsa-sha2-nistp384";
    case LIBSSH2_HOSTKEY_TYPE_ECDSA_521: return "key ecdsa-sha2-nistp521";
    default:                             return "key of another type";
    }
}

/* How long a host gets to answer the knock. Without a bound, connect() waits
 * out twelve SYN retries. */
#define CONNECT_MS 5000

static int connect_bounded(int sock, const struct sockaddr *a, socklen_t n)
{
    const int fl = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, fl | O_NONBLOCK);
    int r = connect(sock, a, n);
    if (r != 0 && errno == EINPROGRESS) {
        fd_set w;
        FD_ZERO(&w);
        FD_SET(sock, &w);
        struct timeval tv = { .tv_sec = CONNECT_MS / 1000,
                              .tv_usec = (CONNECT_MS % 1000) * 1000 };
        r = -1;
        if (select(sock + 1, NULL, &w, NULL, &tv) == 1) {
            int err = 0;
            socklen_t el = sizeof err;
            getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &el);
            r = (err == 0) ? 0 : -1;
            errno = err;
        } else {
            errno = ETIMEDOUT;
        }
    }
    fcntl(sock, F_SETFL, fl);
    return r;
}

static void session(ssh_job_t *j)
{
    const char *user = j->user, *host = j->host, *cmd = j->cmd;
    const int port = j->port;

    char line[96];
    snprintf(line, sizeof line, "$ %.90s", cmd);
    say(line);

    /* Resolve and connect. Everything below reports through the SAME buffer
     * the output goes to, so a failure is visible where the owner is already
     * looking rather than only in a log they may not have. */
    struct addrinfo hints = { .ai_family = AF_INET, .ai_socktype = SOCK_STREAM };
    struct addrinfo *res = NULL;
    char portstr[8];
    snprintf(portstr, sizeof portstr, "%u",
             (unsigned)((port > 0 && port < 65536) ? port : 22));
    if (getaddrinfo(host, portstr, &hints, &res) != 0 || res == NULL) {
        say("cannot resolve that host");
        snprintf(s_last, sizeof s_last, "ssh: no such host");
        return;
    }
    const int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        freeaddrinfo(res);
        snprintf(s_last, sizeof s_last, "ssh: no socket");
        return;
    }
    if (connect_bounded(sock, res->ai_addr, res->ai_addrlen) != 0) {
        const bool slow = (errno == ETIMEDOUT);
        freeaddrinfo(res);
        close(sock);
        say(slow ? "no answer in 5 seconds" : "connection refused");
        snprintf(s_last, sizeof s_last, slow ? "ssh: no answer" : "ssh: refused");
        return;
    }
    freeaddrinfo(res);

    LIBSSH2_SESSION *ses = NULL;
    LIBSSH2_CHANNEL *ch = NULL;

    if (libssh2_init(0) != 0) {
        say("ssh library would not start");
        goto done;
    }
    ses = libssh2_session_init_ex(ss_alloc, ss_free, ss_realloc, NULL);
    if (ses == NULL) {
        say("no memory for a session");
        goto done;
    }
    libssh2_session_set_blocking(ses, 1);
    /* Bounded, so a server that stops answering ends the session rather than
     * holding the one session slot forever. */
    libssh2_session_set_timeout(ses, 15000);

    if (libssh2_session_handshake(ses, sock) != 0) {
        say("handshake failed");
        goto done;
    }
    /* libssh2 makes the socket non-blocking during the handshake, and relies on
     * it: its timeouts only run when a read says it would block. When the build
     * lost HAVE_O_NONBLOCK a session hung after the reply (CMakeLists.txt), so
     * this says so rather than waiting to find out. */
    if ((fcntl(sock, F_GETFL, 0) & O_NONBLOCK) == 0) {
        ESP_LOGE(TAG, "the ssh socket is still blocking - libssh2 was built "
                      "without HAVE_O_NONBLOCK, and a read can wait for ever");
    }
    /* The host's key, before anything is sent to it - see read_kept(). */
    const char *fp = libssh2_hostkey_hash(ses, LIBSSH2_HOSTKEY_HASH_SHA256);
    if (fp == NULL) {
        say("the host showed no key - refused");
        snprintf(s_last, sizeof s_last, "ssh: no host key");
        goto done;
    }
    {
        size_t klen = 0;
        int ktype = 0;
        (void)libssh2_session_hostkey(ses, &klen, &ktype);
        char fpt[SSH_FP_TEXT];
        ssh_fp_text((const uint8_t *)fp, fpt, sizeof fpt);
        say(key_type(ktype));
        say(fpt);
        /* The key and the verdict also go to the console - public facts about
         * the host, and the only way to check the deck saw what ssh-keygen
         * sees. What the command prints stays in '+out' alone. */
        ESP_LOGI(TAG, "%s %s%s", key_type(ktype), fpt,
                 !j->kept ? " - first time, keeping it"
                 : memcmp(j->key, fp, 32) == 0 ? " - the key it had last time"
                                               : " - CHANGED, refused");
        if (j->kept && memcmp(j->key, fp, 32) != 0) {
            say("THE HOST KEY HAS CHANGED.");
            say("refused - no password sent.");
            say("if you changed it yourself:");
            snprintf(line, sizeof line, ">ssh forget %.60s", host);
            say(line);
            snprintf(s_last, sizeof s_last, "ssh: host key changed");
            goto done;
        }
        if (j->kept) {
            say("the key it had last time");
        } else {
            /* Kept by ssh_service, from the editor's task, when this ends. */
            memcpy(j->key, fp, 32);
            j->keep_new = true;
            say("first time here: keeping it");
        }
    }

    const int auth = libssh2_userauth_password(ses, user, j->pass);
    wipe(j->pass, sizeof j->pass);            /* used once, then gone */
    if (auth != 0) {
        say("password refused");
        snprintf(s_last, sizeof s_last, "ssh: auth failed");
        goto done;
    }
    ESP_LOGD(TAG, "step: authenticated");
    ch = libssh2_channel_open_session(ses);
    ESP_LOGD(TAG, "step: channel %s", ch ? "open" : "refused");
    if (ch == NULL) {
        say("server refused a channel");
        goto done;
    }
    {
        const int ex = libssh2_channel_exec(ch, cmd);
        ESP_LOGD(TAG, "step: exec %d", ex);
        if (ex != 0) {
            say("could not run it");
            goto done;
        }
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
        ESP_LOGD(TAG, "step: read %d, eof %d", (int)n, libssh2_channel_eof(ch));
        if (n < 0) {
            char *why = NULL;
            libssh2_session_last_error(ses, &why, NULL, 0);
            ESP_LOGW(TAG, "step: read error %d: %s", (int)n, why ? why : "?");
        }
        if (n <= 0) {
            break;
        }
        for (ssize_t i = 0; i < n; i++) {
            const char c = buf[i];
            if (c == '\n' || alen == sizeof acc - 1) {
                acc[alen] = '\0';
                say(acc);
                alen = 0;
                if (++lines >= 200) {
                    say("... truncated at 200 lines");
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
        say(acc);
        lines++;
    }
    snprintf(s_last, sizeof s_last, "ssh: %d line%s", lines,
             lines == 1 ? "" : "s");
    ESP_LOGD(TAG, "step: reply read, %d line%s", lines, lines == 1 ? "" : "s");

done:
    if (ch != NULL)  { ESP_LOGD(TAG, "step: freeing the channel");
                       libssh2_channel_free(ch);
                       ESP_LOGD(TAG, "step: channel freed"); }
    if (ses != NULL) { libssh2_session_disconnect(ses, "bye");
                       libssh2_session_free(ses);
                       ESP_LOGD(TAG, "step: session closed"); }
    libssh2_exit();
    close(sock);
}

static void ssh_task(void *arg)
{
    (void)arg;
    session(&s_job);
    wipe(s_job.pass, sizeof s_job.pass);
    ESP_LOGW(TAG, "%s", s_last);
    s_state = SSH_DONE;
    vTaskDeleteWithCaps(NULL);
}

esp_err_t ssh_start(const char *user, const char *host, int port,
                    const char *pass, const char *cmd)
{
    if (!user || !host || !cmd) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_state != SSH_IDLE) {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_said == NULL) {
        s_said = xMessageBufferCreateWithCaps(8192, MALLOC_CAP_SPIRAM);
        if (s_said == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }
    snprintf(s_job.user, sizeof s_job.user, "%s", user);
    snprintf(s_job.host, sizeof s_job.host, "%s", host);
    snprintf(s_job.pass, sizeof s_job.pass, "%s", pass ? pass : "");
    snprintf(s_job.cmd,  sizeof s_job.cmd,  "%s", cmd);
    s_job.port = (port > 0 && port < 65536) ? port : 22;
    s_job.kept = read_kept(s_job.host, s_job.port, s_job.key);
    s_job.keep_new = false;
    snprintf(s_last, sizeof s_last, "ssh: connecting");
    s_reply_started = false;
    s_state = SSH_RUNNING;
    /* CPU0, the editor's core, at the editor's priority: the crypto shares
     * time with the editor rather than with the clock on CPU1. */
    if (xTaskCreatePinnedToCoreWithCaps(ssh_task, "ssh", SSH_STACK, NULL, 1,
                                        NULL, 0, MALLOC_CAP_SPIRAM) != pdPASS) {
        wipe(s_job.pass, sizeof s_job.pass);
        s_state = SSH_IDLE;
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

int ssh_service(size_t *reply_from)
{
    if (s_said == NULL) {
        return SSH_QUIET;
    }
    int r = SSH_QUIET;
    char line[161];
    size_t n;
    while ((n = xMessageBufferReceive(s_said, line, sizeof line - 1, 0)) > 0) {
        line[n] = '\0';
        const int out = doc_buf_ensure("+out");
        if (out >= 0) {
            if (!s_reply_started) {
                s_reply_from = doc_buf_len(out);
                s_reply_started = true;
            }
            doc_buf_append(out, line);
        }
        r = SSH_SAID;
    }
    if (s_state == SSH_DONE && xMessageBufferIsEmpty(s_said)) {
        if (s_job.keep_new) {
            s_job.keep_new = false;
            if (keep(s_job.host, s_job.port, s_job.key) != ESP_OK) {
                const int out = doc_buf_ensure("+out");
                if (out >= 0) {
                    doc_buf_append(out, "the key could NOT be kept");
                }
            }
        }
        s_state = SSH_IDLE;
        r = SSH_FINISHED;
    }
    if (reply_from != NULL) {
        *reply_from = s_reply_from;
    }
    return r;
}

bool ssh_busy(void) { return s_state != SSH_IDLE; }
