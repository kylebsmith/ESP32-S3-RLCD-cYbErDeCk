#include "net.h"

#include <string.h>

#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "net";

#define NVS_NS "deck"

static bool           s_started;      /* the Wi-Fi driver is up            */
/* ...and the radio is on. Not the same thing: '>wifi off' stops the radio and
 * keeps the driver, and the status said "join <ssid> -" after it - on a deck
 * that had been HOSTING, too - because it asked only whether the driver was up. */
static bool           s_on;
static bool           s_joined;       /* a station link has an address     */
static bool           s_hosting;
static char           s_ssid[33];
static char           s_ip[16] = "-";
static esp_netif_t   *s_sta;
static esp_netif_t   *s_ap;

bool net_up(void) { return s_joined || s_hosting; }

bool net_radio_on(void) { return s_on; }

void net_status(char *out, size_t max)
{
    if (!s_started || !s_on) {
        snprintf(out, max, "wifi off");
        return;
    }
    snprintf(out, max, "%s %.12s %s", s_hosting ? "host" : "join",
             s_ssid, s_ip);
}

/* Events, not polling. A station link can drop at any moment - a venue's
 * network is not a reliable thing - and the deck must keep playing when it
 * does rather than blocking on a send to an address it no longer has. */
static void on_wifi(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        s_joined = false;
        snprintf(s_ip, sizeof s_ip, "-");
        ESP_LOGW(TAG, "link lost; retrying");
        esp_wifi_connect();
        return;
    }
    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        const ip_event_got_ip_t *e = (const ip_event_got_ip_t *)data;
        snprintf(s_ip, sizeof s_ip, IPSTR, IP2STR(&e->ip_info.ip));
        s_joined = true;
        ESP_LOGW(TAG, "joined %s as %s", s_ssid, s_ip);
    }
}

static esp_err_t wifi_once(void)
{
    if (s_started) {
        return ESP_OK;
    }
    ESP_ERROR_CHECK(esp_netif_init());
    /* The default event loop may already exist - the BLE stack does not create
     * one, but being tolerant here costs nothing and a hard failure would take
     * the whole deck down for a feature that is optional. */
    const esp_err_t le = esp_event_loop_create_default();
    if (le != ESP_OK && le != ESP_ERR_INVALID_STATE) {
        return le;
    }
    s_sta = esp_netif_create_default_wifi_sta();
    s_ap  = esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, "wifi_init");
    /* Credentials live in NVS, so the driver has no reason to also keep them
     * in its own flash area - one source of truth. */
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, on_wifi, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, on_wifi, NULL, NULL));
    s_started = true;
    return ESP_OK;
}

void net_remembered(char *out, size_t max)
{
    out[0] = '\0';
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) == ESP_OK) {
        size_t n = max;
        if (nvs_get_str(h, "ssid", out, &n) != ESP_OK) {
            out[0] = '\0';
        }
        nvs_close(h);
    }
}

void net_forget(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_erase_key(h, "ssid");
        nvs_erase_key(h, "pass");
        nvs_commit(h);
        nvs_close(h);
    }
    ESP_LOGW(TAG, "forgot the network");
}

esp_err_t net_rejoin(void)
{
    /* The credentials are remembered the moment they are TYPED, not once a
     * join succeeds - so a wrong password, a network that is out of range, or
     * simply walking away from the command still leaves the deck knowing what
     * it was told. The owner reported typing an SSID and password, backing
     * out, and finding it gone; that is why. */
    char ssid[33] = "", pass[65] = "";
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) {
        return ESP_ERR_NOT_FOUND;
    }
    size_t sn = sizeof ssid, pn = sizeof pass;
    const bool have = (nvs_get_str(h, "ssid", ssid, &sn) == ESP_OK) &&
                      ssid[0] != '\0';
    if (nvs_get_str(h, "pass", pass, &pn) != ESP_OK) {
        pass[0] = '\0';
    }
    nvs_close(h);
    if (!have) {
        return ESP_ERR_NOT_FOUND;
    }
    /* A name remembered with the help's placeholder brackets round it - see
     * unbracket() in secret_line.h - is joined, and remembered, without them. */
    const size_t sl = strlen(ssid);
    if (sl >= 3 && ssid[0] == '<' && ssid[sl - 1] == '>') {
        memmove(ssid, ssid + 1, sl - 2);
        ssid[sl - 2] = '\0';
        ESP_LOGW(TAG, "the remembered name had <> round it: using %s", ssid);
    }
    ESP_LOGW(TAG, "rejoining %s", ssid);
    return net_join(ssid, pass);
}

static void remember(const char *ssid, const char *pass)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) {
        ESP_LOGW(TAG, "could not remember the network");
        return;
    }
    nvs_set_str(h, "ssid", ssid);
    nvs_set_str(h, "pass", pass ? pass : "");
    nvs_commit(h);
    nvs_close(h);
}

esp_err_t net_join(const char *ssid, const char *pass)
{
    if (ssid == NULL || ssid[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }
    ESP_RETURN_ON_ERROR(wifi_once(), TAG, "start");

    wifi_config_t wc = { 0 };
    snprintf((char *)wc.sta.ssid, sizeof wc.sta.ssid, "%.31s", ssid);
    if (pass != NULL) {
        snprintf((char *)wc.sta.password, sizeof wc.sta.password, "%.63s", pass);
    }
    /* WPA2 minimum. An open network is still joinable, but the deck will not
     * silently accept a downgrade on a network that claims to be secured. */
    wc.sta.threshold.authmode = (pass && pass[0]) ? WIFI_AUTH_WPA2_PSK
                                                  : WIFI_AUTH_OPEN;
    snprintf(s_ssid, sizeof s_ssid, "%.32s", ssid);
    s_hosting = false;
    /* Remembered FIRST. A join can fail for a dozen reasons that have nothing
     * to do with the credentials being wrong, and losing them on every one of
     * those is how the owner ended up retyping a password repeatedly. */
    remember(ssid, pass);
    /* ONE JOIN AT A TIME. A station still trying the last network refuses a new
     * configuration ("still connecting"), and it retries a missing network for
     * ever - so '>wifi' typed again to correct a name failed, measured
     * 2026-09-25. Stop the radio first; the retry that the stop provokes
     * finds it stopped and does nothing. */
    if (s_on) {
        esp_wifi_stop();
        s_on = s_joined = s_hosting = false;
    }
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "mode");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wc), TAG, "cfg");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "start");
    s_on = true;
    return esp_wifi_connect();
}

esp_err_t net_host(const char *ssid, const char *pass)
{
    if (ssid == NULL || ssid[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }
    ESP_RETURN_ON_ERROR(wifi_once(), TAG, "start");

    wifi_config_t wc = { 0 };
    snprintf((char *)wc.ap.ssid, sizeof wc.ap.ssid, "%.31s", ssid);
    wc.ap.ssid_len = (uint8_t)strlen(ssid);
    wc.ap.channel = 6;
    wc.ap.max_connection = 4;
    /* WPA2 needs eight characters. Anything shorter would be silently
     * downgraded to an open network by the driver, and an access point that
     * says it has a password and does not is worse than one that admits it. */
    if (pass != NULL && strlen(pass) >= 8) {
        snprintf((char *)wc.ap.password, sizeof wc.ap.password, "%.63s", pass);
        wc.ap.authmode = WIFI_AUTH_WPA2_PSK;
    } else {
        wc.ap.authmode = WIFI_AUTH_OPEN;
        if (pass != NULL && pass[0] != '\0') {
            ESP_LOGW(TAG, "password under 8 chars - hosting OPEN instead");
        }
    }
    snprintf(s_ssid, sizeof s_ssid, "%.32s", ssid);
    if (s_on) {
        esp_wifi_stop();               /* one mode at a time - see net_join */
        s_on = s_joined = s_hosting = false;
    }
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_AP), TAG, "mode");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_AP, &wc), TAG, "cfg");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "start");
    s_on = true;
    s_hosting = true;
    s_joined  = false;
    snprintf(s_ip, sizeof s_ip, "192.168.4.1");
    ESP_LOGW(TAG, "hosting %s (%s) at %s", ssid,
             wc.ap.authmode == WIFI_AUTH_OPEN ? "open" : "wpa2", s_ip);
    return ESP_OK;
}

void net_stop(void)
{
    if (!s_started) {
        return;
    }
    esp_wifi_stop();
    s_on = s_joined = s_hosting = false;
    snprintf(s_ip, sizeof s_ip, "-");
    ESP_LOGW(TAG, "radio off - the airtime is the keyboard's again");
}
