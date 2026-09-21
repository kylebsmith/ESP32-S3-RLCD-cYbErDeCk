#include "battery.h"

#include <stdio.h>
#include <string.h>

#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

static const char *TAG = "battery";

/* ADC1 channels 0..9 are GPIO1..GPIO10 on the ESP32-S3. These are the ones not
 * already claimed: the display holds 5, 11, 12, 40, 41; the SD card holds 21,
 * 38, 39; KEY holds 18. So channels 0-3 and 5-9 are candidates, and channel 4
 * (GPIO5) is excluded because the panel owns it. */
static const int s_cands[] = { 0, 1, 2, 3, 5, 6, 7, 8, 9 };

static adc_oneshot_unit_handle_t s_adc;

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
    return true;
}

int battery_percent(void)
{
    /* Unknown, and it says so. See battery.h: no pin is documented, and a
     * fabricated percentage on a performance instrument is worse than a blank
     * where the number would be. */
    return -1;
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
        const adc_oneshot_chan_cfg_t cc = {
            .atten = ADC_ATTEN_DB_12,      /* up to ~2.5 V at the pin */
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        if (adc_oneshot_config_channel(s_adc, ch, &cc) != ESP_OK) {
            continue;
        }
        int raw = 0;
        if (adc_oneshot_read(s_adc, ch, &raw) != ESP_OK) {
            continue;
        }
        /* Printed raw AND as an approximate voltage. A cell at 3.7 V behind a
         * 2:1 divider reads about 1.85 V, so a channel sitting near there and
         * MOVING when USB is unplugged is the one. */
        const int mv = raw * 2500 / 4095;
        char one[24];
        snprintf(one, sizeof one, "g%d:%dmV ", s_cands[i] + 1, mv);
        if (used + strlen(one) >= max) {
            break;
        }
        strncat(out, one, max - strlen(out) - 1);
        used = strlen(out);
    }
}
