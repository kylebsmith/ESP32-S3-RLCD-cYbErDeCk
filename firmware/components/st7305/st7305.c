/*
 * ST7305 reflective-LCD driver - see include/st7305.h for the geometry.
 *
 * Derived from SolarOS src/drivers/rlcd_st7305.c, Copyright 2026 nilseuropa,
 * Apache License 2.0. The power-on register values, the command order, the
 * CASET mirroring rule and the 4x2 packing are that work's; the framebuffer
 * layout and damage model are ours. See firmware/NOTICE.
 */
#include "st7305.h"

#include <string.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

static const char *TAG = "st7305";

/* docs/HARDWARE.md pin map, corroborated by solar_term.toml. */
#define PIN_SCK   11
#define PIN_MOSI  12
#define PIN_CS    40
#define PIN_DC     5
#define PIN_RST   41

#define SPI_HOST_USED  SPI2_HOST
#define SPI_WRITE_HZ   24000000   /* known-good on this panel (H9)          */
#define SPI_READ_HZ     6000000   /* reads need 150ns tSCYC; 5x slower      */
#define CHUNK_BYTES        4092   /* the reference driver's transfer cap    */

#define IDLE_LPM_MS        1000

/* FRCTRL (0xB2): HFRA bit selects the high half of the HPM rate pairs, the
 * low three bits select the LPM rate. 0x12 = HPM 32 Hz + LPM 1 Hz. */
#define FRCTRL_HFRA  0x10
#define FRCTRL_LFRA  0x07

static spi_device_handle_t s_spi_write;
static spi_device_handle_t s_spi_read;
static uint8_t *s_fb;       /* 15,000 B, controller order, internal DMA SRAM */
static uint8_t *s_stage;    /* gather buffer for windows narrower than a row */
/* A damage LIST, not a single rectangle.
 *
 * docs/OS.md specifies a damage list and the reason is visible the moment one
 * is not used: a character changing at the top of the screen and a status bar
 * changing at the bottom union into a rectangle covering the whole panel, and
 * a 36-byte update becomes a 13,800-byte one. Measured, not theorised.
 *
 * Four rectangles is enough for text + cursor + rule + status; beyond that the
 * two cheapest to combine are merged, so the list degrades into the old
 * behaviour rather than dropping damage. */
#define DMG_MAX 4
static st7305_damage_t s_dmg[DMG_MAX];
static int s_dmg_count;
static st7305_power_policy_t s_policy = ST7305_POWER_AUTO;
static bool s_in_hpm;
static esp_timer_handle_t s_idle_timer;
static st7305_orient_t s_orient = ST7305_ORIENT_1;

/* The bus is reached from three tasks: the main task drawing, the esp_timer
 * task dropping the panel to LPM after an idle timeout, and the NimBLE host
 * task. st_cmd_data holds CS low across a command and its payload, so an
 * interleaved command from another task lands INSIDE that window and the
 * panel receives a spliced transaction. Recursive because a flush takes the
 * lock and then calls st_cmd_data, which takes it again. */
static SemaphoreHandle_t s_bus;

static inline void bus_lock(void)
{
    if (s_bus != NULL) {
        xSemaphoreTakeRecursive(s_bus, portMAX_DELAY);
    }
}

static inline void bus_unlock(void)
{
    if (s_bus != NULL) {
        xSemaphoreGiveRecursive(s_bus);
    }
}

/* --------------------------------------------------------------------------
 * Low-level SPI. CS is driven by hand because a RAM write has to hold CS low
 * across the command byte and the whole payload.
 * -------------------------------------------------------------------------- */

static esp_err_t spi_raw(const uint8_t *data, size_t len)
{
    while (len > 0) {
        const size_t n = len > CHUNK_BYTES ? CHUNK_BYTES : len;
        spi_transaction_t t = { .length = n * 8, .tx_buffer = data };
        const esp_err_t err = spi_device_polling_transmit(s_spi_write, &t);
        if (err != ESP_OK) {
            return err;
        }
        data += n;
        len  -= n;
    }
    return ESP_OK;
}

static esp_err_t st_cmd_data(uint8_t cmd, const uint8_t *data, size_t len)
{
    bus_lock();
    gpio_set_level(PIN_DC, 0);
    gpio_set_level(PIN_CS, 0);
    esp_err_t err = spi_raw(&cmd, 1);
    if (err == ESP_OK && len > 0) {
        gpio_set_level(PIN_DC, 1);
        err = spi_raw(data, len);
    }
    gpio_set_level(PIN_CS, 1);
    bus_unlock();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "command 0x%02x failed: %s", cmd, esp_err_to_name(err));
    }
    return err;
}

static esp_err_t st_cmd(uint8_t cmd)
{
    return st_cmd_data(cmd, NULL, 0);
}

#define ST_TRY(expr) do { \
    const esp_err_t _e = (expr); \
    if (_e != ESP_OK) { return _e; } \
} while (0)

/* --------------------------------------------------------------------------
 * Power modes
 * -------------------------------------------------------------------------- */

static esp_err_t set_mode(bool hpm)
{
    if (s_in_hpm == hpm) {
        return ESP_OK;
    }
    ST_TRY(st_cmd(hpm ? 0x38 : 0x39));
    s_in_hpm = hpm;
    return ESP_OK;
}

esp_err_t st7305_set_inverted(bool inverted)
{
    return st_cmd(inverted ? 0x21 : 0x20);
}

esp_err_t st7305_set_hpm(void) { return set_mode(true); }
esp_err_t st7305_set_lpm(void) { return set_mode(false); }

static void idle_timer_cb(void *arg)
{
    (void)arg;
    if (s_policy == ST7305_POWER_AUTO) {
        /* docs/OS.md power policy: drop back to 1 Hz once typing stops. */
        (void)set_mode(false);
    }
}

esp_err_t st7305_set_power_policy(st7305_power_policy_t policy)
{
    s_policy = policy;
    if (policy == ST7305_POWER_HPM) {
        return set_mode(true);
    }
    if (policy == ST7305_POWER_LPM) {
        return set_mode(false);
    }
    return ESP_OK;
}

/* --------------------------------------------------------------------------
 * Init
 * -------------------------------------------------------------------------- */

static esp_err_t panel_reset(void)
{
    ST_TRY(gpio_set_level(PIN_RST, 1));
    vTaskDelay(pdMS_TO_TICKS(50));
    ST_TRY(gpio_set_level(PIN_RST, 0));
    vTaskDelay(pdMS_TO_TICKS(20));
    ST_TRY(gpio_set_level(PIN_RST, 1));
    vTaskDelay(pdMS_TO_TICKS(50));
    return ESP_OK;
}

static esp_err_t panel_init_registers(void)
{
    /* Values verbatim from SolarOS rlcd_waveshare_settings. Do not tune these
     * without the panel in front of you; several are gate/source timings. */
    static const uint8_t d6[]   = {0x17, 0x02};
    static const uint8_t d1[]   = {0x01};
    static const uint8_t c0[]   = {0x11, 0x04};
    static const uint8_t c1[]   = {0x69, 0x69, 0x69, 0x69};
    static const uint8_t c2[]   = {0x19, 0x19, 0x19, 0x19};
    static const uint8_t c4[]   = {0x4B, 0x4B, 0x4B, 0x4B};
    static const uint8_t c5[]   = {0x19, 0x19, 0x19, 0x19};
    static const uint8_t b3[]   = {0xE5, 0xF6, 0x05, 0x46, 0x77,
                                   0x77, 0x77, 0x77, 0x76, 0x45};
    static const uint8_t b4[]   = {0x05, 0x46, 0x77, 0x77, 0x77,
                                   0x77, 0x76, 0x45};
    static const uint8_t gate[] = {0x32, 0x03, 0x1F};
    static const uint8_t b7[]   = {0x13};
    static const uint8_t b0[]   = {0x64};
    static const uint8_t c9[]   = {0x00};
    static const uint8_t m36[]  = {0x48};   /* MADCTL - column pointer
                                             * decrements from the CASET end */
    static const uint8_t m3a[]  = {0x11};
    static const uint8_t b9[]   = {0x20};
    static const uint8_t b8[]   = {0x29};
    static const uint8_t m35[]  = {0x00};
    static const uint8_t d0[]   = {0xFF};

    /* OSCSET 0xA6 + HFRA set = HPM 32 Hz; LFRA 2 = LPM 1 Hz. */
    static const uint8_t d8[]   = {0xA6, 0xE9};
    static const uint8_t b2[]   = {(0x02 & (uint8_t)~FRCTRL_LFRA) | FRCTRL_HFRA | 0x02};

    static const uint8_t win_a[] = {ST7305_ADDR_START, ST7305_ADDR_END};
    static const uint8_t win_b[] = {0x00, ST7305_ROW_ADDRS - 1};

    ST_TRY(st_cmd_data(0xD6, d6,   sizeof d6));
    ST_TRY(st_cmd_data(0xD1, d1,   sizeof d1));
    ST_TRY(st_cmd_data(0xC0, c0,   sizeof c0));
    ST_TRY(st_cmd_data(0xC1, c1,   sizeof c1));
    ST_TRY(st_cmd_data(0xC2, c2,   sizeof c2));
    ST_TRY(st_cmd_data(0xC4, c4,   sizeof c4));
    ST_TRY(st_cmd_data(0xC5, c5,   sizeof c5));
    ST_TRY(st_cmd_data(0xD8, d8,   sizeof d8));
    ST_TRY(st_cmd_data(0xB2, b2,   sizeof b2));
    ST_TRY(st_cmd_data(0xB3, b3,   sizeof b3));
    ST_TRY(st_cmd_data(0xB4, b4,   sizeof b4));
    ST_TRY(st_cmd_data(0x62, gate, sizeof gate));
    ST_TRY(st_cmd_data(0xB7, b7,   sizeof b7));
    ST_TRY(st_cmd_data(0xB0, b0,   sizeof b0));
    ST_TRY(st_cmd(0x11));                       /* sleep out */
    vTaskDelay(pdMS_TO_TICKS(120));

    ST_TRY(st_cmd_data(0xC9, c9,   sizeof c9));
    ST_TRY(st_cmd_data(0x36, m36,  sizeof m36));
    ST_TRY(st_cmd_data(0x3A, m3a,  sizeof m3a));
    ST_TRY(st_cmd_data(0xB9, b9,   sizeof b9));
    ST_TRY(st_cmd_data(0xB8, b8,   sizeof b8));
    ST_TRY(st_cmd(0x20));                       /* inversion off */
    ST_TRY(st_cmd_data(0x2A, win_a, sizeof win_a));
    ST_TRY(st_cmd_data(0x2B, win_b, sizeof win_b));
    ST_TRY(st_cmd_data(0x35, m35,  sizeof m35));
    ST_TRY(st_cmd_data(0xD0, d0,   sizeof d0));
    ST_TRY(st_cmd(0x38));                       /* HPM */
    s_in_hpm = true;
    ST_TRY(st_cmd(0x29));                       /* display on */
    return ESP_OK;
}

esp_err_t st7305_init(void)
{
    const gpio_config_t io = {
        .pin_bit_mask = (1ULL << PIN_DC) | (1ULL << PIN_CS) | (1ULL << PIN_RST),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&io), TAG, "gpio config");
    gpio_set_level(PIN_CS, 1);
    gpio_set_level(PIN_DC, 1);
    gpio_set_level(PIN_RST, 1);

    const spi_bus_config_t bus = {
        .sclk_io_num     = PIN_SCK,
        .mosi_io_num     = PIN_MOSI,
        .miso_io_num     = -1,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = CHUNK_BYTES,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(SPI_HOST_USED, &bus, SPI_DMA_CH_AUTO),
                        TAG, "spi bus init");

    const spi_device_interface_config_t wr = {
        .clock_speed_hz = SPI_WRITE_HZ,
        .mode           = 0,
        .spics_io_num   = -1,       /* CS by hand - see st_cmd_data */
        .queue_size     = 1,
    };
    ESP_RETURN_ON_ERROR(spi_bus_add_device(SPI_HOST_USED, &wr, &s_spi_write),
                        TAG, "spi write device");

    const spi_device_interface_config_t rd = {
        .clock_speed_hz = SPI_READ_HZ,
        .mode           = 0,
        .spics_io_num   = -1,
        .queue_size     = 1,
        .flags          = SPI_DEVICE_HALFDUPLEX | SPI_DEVICE_3WIRE,
    };
    ESP_RETURN_ON_ERROR(spi_bus_add_device(SPI_HOST_USED, &rd, &s_spi_read),
                        TAG, "spi read device");

    /* The framebuffer must be internal DMA SRAM: SPI DMA cannot reach PSRAM
     * without cache-coherence work, and 15 KB of 512 KB is affordable. */
    s_bus = xSemaphoreCreateRecursiveMutex();
    if (s_bus == NULL) {
        return ESP_ERR_NO_MEM;
    }

    s_fb = heap_caps_calloc(1, ST7305_FB_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    s_stage = heap_caps_malloc(ST7305_FB_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (s_fb == NULL || s_stage == NULL) {
        ESP_LOGE(TAG, "framebuffer allocation failed");
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(TAG, "framebuffer %d B at %p (internal=%d dma=%d)",
             ST7305_FB_SIZE, s_fb,
             esp_ptr_internal(s_fb) ? 1 : 0,
             esp_ptr_dma_capable(s_fb) ? 1 : 0);

    ST_TRY(panel_reset());
    ST_TRY(panel_init_registers());

    const esp_timer_create_args_t targs = {
        .callback = idle_timer_cb,
        .name     = "st7305_idle",
    };
    ESP_RETURN_ON_ERROR(esp_timer_create(&targs, &s_idle_timer), TAG, "idle timer");

    ESP_LOGI(TAG, "init ok: %dx%d logical, %d B framebuffer, %d MHz",
             ST7305_WIDTH, ST7305_HEIGHT, ST7305_FB_SIZE, SPI_WRITE_HZ / 1000000);
    return ESP_OK;
}

uint8_t *st7305_framebuffer(void) { return s_fb; }

/* --------------------------------------------------------------------------
 * Pixels. Logical (x,y) -> native (nx = y, ny = x).
 *   byte = (ny/2) * ROW_BYTES + nx/4  =  (x/2)*75 + y/4
 *   bit  = 7 - (nx%4)*2 - (ny%2)      =  7 - (y%4)*2 - (x%2)
 * The top (smaller native y) pixel is the higher bit.
 * -------------------------------------------------------------------------- */

void st7305_set_orientation(st7305_orient_t o) { s_orient = o; }
st7305_orient_t st7305_orientation(void) { return s_orient; }

/* Thin wrappers so call sites stay readable; the arithmetic is in
 * st7305_addr.h and is shared with the host-side check. */
static inline void to_native(int x, int y, int *nx, int *ny)
{
    st7305_to_native((int)s_orient, x, y, nx, ny);
}
#define fb_index_n st7305_fb_index_n
#define fb_mask_n  st7305_fb_mask_n

static inline long rect_area(const st7305_damage_t *r)
{
    return (long)(r->x1 - r->x0 + 1) * (long)(r->y1 - r->y0 + 1);
}

static inline void rect_union(st7305_damage_t *a, const st7305_damage_t *b)
{
    if (b->x0 < a->x0) { a->x0 = b->x0; }
    if (b->y0 < a->y0) { a->y0 = b->y0; }
    if (b->x1 > a->x1) { a->x1 = b->x1; }
    if (b->y1 > a->y1) { a->y1 = b->y1; }
}

/* Would merging these two cost less than keeping them apart? Rectangles that
 * already overlap or sit within a controller quantum of each other are always
 * worth merging, because the window would quantise them together anyway. */
static long merge_cost(const st7305_damage_t *a, const st7305_damage_t *b)
{
    st7305_damage_t u = *a;
    rect_union(&u, b);
    return rect_area(&u) - rect_area(a) - rect_area(b);
}

void st7305_damage(int x, int y, int w, int h)
{
    if (w <= 0 || h <= 0) {
        return;
    }
    int x1 = x + w - 1;
    int y1 = y + h - 1;
    if (x < 0) { x = 0; }
    if (y < 0) { y = 0; }
    if (x1 > ST7305_WIDTH  - 1) { x1 = ST7305_WIDTH  - 1; }
    if (y1 > ST7305_HEIGHT - 1) { y1 = ST7305_HEIGHT - 1; }
    if (x > x1 || y > y1) {
        return;
    }

    st7305_damage_t r = { (int16_t)x, (int16_t)y, (int16_t)x1, (int16_t)y1 };

    /* Fold into an existing rectangle when they touch; a quantum of slack
     * because the window is snapped out to 12 px and 2 lines regardless. */
    for (int i = 0; i < s_dmg_count; i++) {
        if (r.x0 <= s_dmg[i].x1 + 2 && s_dmg[i].x0 <= r.x1 + 2 &&
            r.y0 <= s_dmg[i].y1 + 12 && s_dmg[i].y0 <= r.y1 + 12) {
            rect_union(&s_dmg[i], &r);
            return;
        }
    }

    if (s_dmg_count < DMG_MAX) {
        s_dmg[s_dmg_count++] = r;
        return;
    }

    /* Full: merge whichever pair costs least, then take the freed slot. */
    int bi = 0, bj = 1;
    long best = merge_cost(&s_dmg[0], &s_dmg[1]);
    for (int i = 0; i < s_dmg_count; i++) {
        for (int j = i + 1; j < s_dmg_count; j++) {
            const long cst = merge_cost(&s_dmg[i], &s_dmg[j]);
            if (cst < best) { best = cst; bi = i; bj = j; }
        }
    }
    rect_union(&s_dmg[bi], &s_dmg[bj]);
    s_dmg[bj] = s_dmg[s_dmg_count - 1];
    s_dmg[s_dmg_count - 1] = r;
}

void st7305_pixel_raw(int x, int y, bool on)
{
    if ((unsigned)x >= ST7305_WIDTH || (unsigned)y >= ST7305_HEIGHT) {
        return;
    }
    int nx, ny;
    to_native(x, y, &nx, &ny);
    const uint8_t m = fb_mask_n(nx, ny);
    uint8_t *p = &s_fb[fb_index_n(nx, ny)];
    if (on) { *p |= m; } else { *p = (uint8_t)(*p & ~m); }
}

void st7305_fill_raw(int x, int y, int w, int h, bool on)
{
    for (int yy = y; yy < y + h; yy++) {
        for (int xx = x; xx < x + w; xx++) {
            st7305_pixel_raw(xx, yy, on);
        }
    }
}

void st7305_pixel(int x, int y, bool on)
{
    if ((unsigned)x >= ST7305_WIDTH || (unsigned)y >= ST7305_HEIGHT) {
        return;
    }
    int nx, ny;
    to_native(x, y, &nx, &ny);
    const uint8_t m = fb_mask_n(nx, ny);
    uint8_t *p = &s_fb[fb_index_n(nx, ny)];
    if (on) { *p |= m; } else { *p = (uint8_t)(*p & ~m); }
    st7305_damage(x, y, 1, 1);
}

void st7305_fill(int x, int y, int w, int h, bool on)
{
    for (int yy = y; yy < y + h; yy++) {
        for (int xx = x; xx < x + w; xx++) {
            if ((unsigned)xx < ST7305_WIDTH && (unsigned)yy < ST7305_HEIGHT) {
                int nx, ny;
                to_native(xx, yy, &nx, &ny);
                const uint8_t m = fb_mask_n(nx, ny);
                uint8_t *p = &s_fb[fb_index_n(nx, ny)];
                if (on) { *p |= m; } else { *p = (uint8_t)(*p & ~m); }
            }
        }
    }
    st7305_damage(x, y, w, h);
}

void st7305_clear(bool on)
{
    memset(s_fb, on ? 0xFF : 0x00, ST7305_FB_SIZE);
    s_dmg_count = 1;
    s_dmg[0].x0 = 0; s_dmg[0].y0 = 0;
    s_dmg[0].x1 = ST7305_WIDTH - 1; s_dmg[0].y1 = ST7305_HEIGHT - 1;
}

/* --------------------------------------------------------------------------
 * Flush
 *
 * THE RULE THE DATASHEET DOES NOT GIVE YOU (docs/HARDWARE.md):
 *
 *     CASET = { 0x3C - addr_end , 0x3C - addr_start }    reversed and mirrored
 *     RASET = { first_y/2 , last_y/2 }
 *
 * At full width {0x3C-0x2A, 0x3C-0x12} is {0x12, 0x2A} - an identity - which
 * is why every full-frame driver is silently correct and never finds this.
 * It only bites once a window narrows.
 * -------------------------------------------------------------------------- */

static esp_err_t push_window(int nx0, int nx1, int ny0, int ny1, size_t *sent)
{
    bus_lock();
    /* Quantise out: native x to 12 px, native y to 2 lines. */
    st7305_window_t w;
    st7305_window(nx0, nx1, ny0, ny1, &w);
    const int send_start = w.send_start;
    const int send_count = w.send_count;
    const int rows       = w.rows;
    const int row_first  = w.raset[0];

    if (s_policy == ST7305_POWER_AUTO) {
        const esp_err_t e = set_mode(true);
        if (e != ESP_OK) { bus_unlock(); return e; }
        esp_timer_stop(s_idle_timer);
        esp_timer_start_once(s_idle_timer, (uint64_t)IDLE_LPM_MS * 1000);
    }

    {
        const esp_err_t e1 = st_cmd_data(0x2A, w.caset, sizeof w.caset);
        if (e1 != ESP_OK) { bus_unlock(); return e1; }
        const esp_err_t e2 = st_cmd_data(0x2B, w.raset, sizeof w.raset);
        if (e2 != ESP_OK) { bus_unlock(); return e2; }
    }

    const uint8_t *payload;
    if (send_count == ST7305_ROW_BYTES) {
        /* Full-width: already contiguous in the framebuffer, no gather. */
        payload = &s_fb[(size_t)row_first * ST7305_ROW_BYTES];
    } else {
        uint8_t *dst = s_stage;
        for (int r = row_first; r < row_first + rows; r++) {
            memcpy(dst, &s_fb[(size_t)r * ST7305_ROW_BYTES + send_start],
                   (size_t)send_count);
            dst += send_count;
        }
        payload = s_stage;
    }

    const size_t len = (size_t)send_count * (size_t)rows;
    const esp_err_t err = st_cmd_data(0x2C, payload, len);
    bus_unlock();
    if (err != ESP_OK) {
        return err;
    }
    if (sent != NULL) {
        *sent = len;
    }
    return ESP_OK;
}

esp_err_t st7305_flush(size_t *bytes_sent)
{
    if (bytes_sent != NULL) {
        *bytes_sent = 0;
    }
    if (s_dmg_count == 0) {
        return ESP_OK;                      /* nothing changed */
    }

    const int count = s_dmg_count;
    s_dmg_count = 0;                        /* cleared up front: a failure
                                             * should not replay for ever */
    for (int i = 0; i < count; i++) {
        /* Map the rectangle's corners through the orientation transform and
         * take the extremes; a mirror swaps which corner is which. */
        int ax, ay, bx, by;
        to_native(s_dmg[i].x0, s_dmg[i].y0, &ax, &ay);
        to_native(s_dmg[i].x1, s_dmg[i].y1, &bx, &by);
        const int nx0 = ax < bx ? ax : bx, nx1 = ax < bx ? bx : ax;
        const int ny0 = ay < by ? ay : by, ny1 = ay < by ? by : ay;

        size_t sent = 0;
        const esp_err_t err = push_window(nx0, nx1, ny0, ny1, &sent);
        if (err != ESP_OK) {
            return err;
        }
        if (bytes_sent != NULL) {
            *bytes_sent += sent;
        }
    }
    return ESP_OK;
}

esp_err_t st7305_flush_full(void)
{
    s_dmg_count = 1;
    s_dmg[0].x0 = 0; s_dmg[0].y0 = 0;
    s_dmg[0].x1 = ST7305_WIDTH - 1; s_dmg[0].y1 = ST7305_HEIGHT - 1;
    return st7305_flush(NULL);
}

/* --------------------------------------------------------------------------
 * Read-back. The only evidence that the panel is physically answering rather
 * than the writes falling into an unconnected bus. Runs at 6 MHz because the
 * read cycle time is 150 ns against 30 ns for a write.
 * -------------------------------------------------------------------------- */

esp_err_t st7305_read_id(uint8_t out[3])
{
    /* The FPC carries ONE bidirectional data line (docs/HARDWARE.md: the
     * 23-pin flex has GND, VCC3V3, SCL, SDA, CS, RS, TE, RESET and nothing
     * else), so this is 3-wire SIO: MOSI is turned around for the read. That
     * also means command and data cannot share one transaction - the driver
     * rejects a half-duplex transfer with both phases - so it is a write
     * followed by a read, with CS held low across both.
     *
     * The read clock is 6 MHz, not 24: tSCYC is 150 ns for a read against
     * 30 ns for a write, and a read path driven at the write clock fails
     * intermittently rather than cleanly.
     */
    const uint8_t cmd = 0x04;          /* RDDID */
    uint8_t rx[4] = {0};

    gpio_set_level(PIN_DC, 0);
    gpio_set_level(PIN_CS, 0);

    spi_transaction_t tx = {
        .length    = 8,
        .tx_buffer = &cmd,
        .rxlength  = 0,
        .rx_buffer = NULL,
    };
    esp_err_t err = spi_device_polling_transmit(s_spi_read, &tx);

    if (err == ESP_OK) {
        gpio_set_level(PIN_DC, 1);
        spi_transaction_t rd = {
            .length    = 0,
            .tx_buffer = NULL,
            .rxlength  = 32,           /* one dummy byte then three ID bytes */
            .rx_buffer = rx,
        };
        err = spi_device_polling_transmit(s_spi_read, &rd);
    }

    gpio_set_level(PIN_CS, 1);
    gpio_set_level(PIN_DC, 1);

    if (err == ESP_OK) {
        out[0] = rx[1];
        out[1] = rx[2];
        out[2] = rx[3];
    }
    return err;
}
