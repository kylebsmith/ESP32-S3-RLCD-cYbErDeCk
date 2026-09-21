#include "battery.h"

#include <stdio.h>
#include <string.h>

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "nvs.h"
#include "esp_log.h"

static const char *TAG = "battery";

/* ADC1 channels 0..9 are GPIO1..GPIO10 on the ESP32-S3. These are the ones not
 * already claimed: the display holds 5, 11, 12, 40, 41; the SD card holds 21,
 * 38, 39; KEY holds 18. So channels 0-3 and 5-9 are candidates, and channel 4
 * (GPIO5) is excluded because the panel owns it. */
static const int s_cands[] = { 0, 1, 2, 3, 5, 6, 7, 8, 9 };

static adc_oneshot_unit_handle_t s_adc;
static adc_cali_handle_t         s_cali;
static int  s_chan = -1;         /* ADC1 channel carrying the cell, or -1   */
static int  s_divx10 = 20;       /* divider x10; 20 means a 2:1 network     */

/* AN 18650 LITHIUM CELL, which is what this deck carries: 3.0 V empty, 4.2 V
 * full, and nominally 3.7 V. The curve is not linear and pretending it is
 * makes a gauge that reads 50 % for most of the discharge - so this maps the
 * flat middle honestly rather than prettily. Points are voltage/percent pairs
 * from the standard discharge shape; between them it interpolates. */
static const struct { int mv, pct; } s_curve[] = {
    { 4150, 100 }, { 4050, 90 }, { 3950, 80 }, { 3870, 70 }, { 3800, 60 },
    { 3750, 50 },  { 3700, 40 }, { 3650, 30 }, { 3580, 20 }, { 3450, 10 },
    { 3200,  3 },  { 3000,  0 },
};

static int curve_pct(int mv)
{
    const size_t n = sizeof s_curve / sizeof s_curve[0];
    if (mv >= s_curve[0].mv)     { return 100; }
    if (mv <= s_curve[n - 1].mv) { return 0; }
    for (size_t i = 1; i < n; i++) {
        if (mv >= s_curve[i].mv) {
            const int dv = s_curve[i - 1].mv - s_curve[i].mv;
            const int dp = s_curve[i - 1].pct - s_curve[i].pct;
            return s_curve[i].pct + (dv ? (mv - s_curve[i].mv) * dp / dv : 0);
        }
    }
    return 0;
}

static bool adc_once(void)
{
    if (s_adc != NULL) {
        return true;
    }
    const adc_oneshot_unit_init_cfg_t u = { .unit_id = ADC_UNIT_1 };
    if (adc_oneshot_new_unit(&u, &s_adc) != ESP_OK) {
        ESP_LOGW(TAG, "no ADC unit");
        return false;
    }
    /* Curve-fitted calibration from the chip's own eFuse. Without it a raw
     * count has to be scaled by a guessed full-scale voltage, and the guess is
     * wrong by enough to matter on a lithium curve where 100 mV is twenty per
     * cent of the charge. */
    const adc_cali_curve_fitting_config_t cc = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_curve_fitting(&cc, &s_cali) != ESP_OK) {
        s_cali = NULL;
        ESP_LOGW(TAG, "no ADC calibration; readings are approximate");
    }
    /* Which channel, if the owner has already identified it. */
    nvs_handle_t h;
    if (nvs_open("deck", NVS_READONLY, &h) == ESP_OK) {
        int32_t g = 0, d = 0;
        if (nvs_get_i32(h, "bat_gpio", &g) == ESP_OK && g >= 1 && g <= 10) {
            s_chan = g - 1;
        }
        if (nvs_get_i32(h, "bat_div", &d) == ESP_OK && d >= 10 && d <= 100) {
            s_divx10 = d;
        }
        nvs_close(h);
    }
    return true;
}

static int read_mv(adc_channel_t ch)
{
    const adc_oneshot_chan_cfg_t cc = {
        .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_oneshot_config_channel(s_adc, ch, &cc) != ESP_OK) {
        return -1;
    }
    /* Eight samples averaged. A single ADC read on this part is noisy enough
     * to move a percentage by a couple of points between glances, and a gauge
     * that flickers is a gauge nobody trusts. */
    int acc = 0, got = 0;
    for (int i = 0; i < 8; i++) {
        int raw = 0;
        if (adc_oneshot_read(s_adc, ch, &raw) == ESP_OK) { acc += raw; got++; }
    }
    if (got == 0) {
        return -1;
    }
    const int raw = acc / got;
    int mv = 0;
    if (s_cali != NULL &&
        adc_cali_raw_to_voltage(s_cali, raw, &mv) == ESP_OK) {
        return mv;
    }
    return raw * 3100 / 4095;        /* uncalibrated fallback, and it says so */
}

esp_err_t battery_use(int gpio, int divider_x10)
{
    nvs_handle_t h;
    if (nvs_open("deck", NVS_READWRITE, &h) != ESP_OK) {
        return ESP_FAIL;
    }
    nvs_set_i32(h, "bat_gpio", gpio);
    nvs_set_i32(h, "bat_div", divider_x10);
    nvs_commit(h);
    nvs_close(h);
    s_chan = (gpio >= 1 && gpio <= 10) ? gpio - 1 : -1;
    s_divx10 = (divider_x10 >= 10 && divider_x10 <= 100) ? divider_x10 : 20;
    ESP_LOGW(TAG, "battery on GPIO%d, divider %d.%d:1",
             gpio, s_divx10 / 10, s_divx10 % 10);
    return ESP_OK;
}

int battery_mv(void)
{
    if (!adc_once() || s_chan < 0) {
        return -1;
    }
    const int mv = read_mv((adc_channel_t)s_chan);
    return (mv < 0) ? -1 : mv * s_divx10 / 10;
}

int battery_percent(void)
{
    /* Still -1 until the owner has identified the pin - see battery.h. A
     * fabricated percentage on a performance instrument is worse than a blank
     * where the number would be. */
    const int mv = battery_mv();
    return (mv < 0) ? -1 : curve_pct(mv);
}

void battery_scan(char *out, size_t max)
{
    out[0] = '\0';
    if (!adc_once()) {
        snprintf(out, max, "no ADC");
        return;
    }
    size_t used = 0;
    for (size_t i = 0; i < sizeof s_cands / sizeof s_cands[0]; i++) {
        const adc_channel_t ch = (adc_channel_t)s_cands[i];


        const int mv = read_mv(ch);
        if (mv < 0) {
            continue;
        }
        char one[32];
        snprintf(one, sizeof one, "g%d:%dmV ", s_cands[i] + 1, mv < 9999 ? mv : 9999);
        if (used + strlen(one) >= max) {
            break;
        }
        strncat(out, one, max - strlen(out) - 1);
        used = strlen(out);
    }
}
