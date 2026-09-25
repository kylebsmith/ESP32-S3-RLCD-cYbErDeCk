#include "dinmidi.h"

#include <string.h>

#include "driver/uart.h"
#include "midi_len.h"
#include "esp_log.h"

static const char *TAG = "din";

/* UART1. UART0 is the ROM console's and must stay untouched even though this
 * board talks to the host over USB-Serial-JTAG instead: leaving it alone costs
 * nothing and keeps one recovery path that does not depend on this file. */
#define DIN_UART      UART_NUM_1
#define DIN_BAUD      31250          /* the MIDI rate since 1983 */
#define DIN_TX_BUF    512

static int      s_pin = -1;
static bool     s_up;
static uint32_t s_bytes;

int  dinmidi_pin(void)     { return s_pin; }
bool dinmidi_running(void) { return s_up; }

uint32_t dinmidi_bytes(void)
{
    const uint32_t n = s_bytes;
    s_bytes = 0;
    return n;
}

esp_err_t dinmidi_start(int gpio)
{
    if (gpio < 0) {
        dinmidi_stop();
        return ESP_OK;
    }
    if (s_up && gpio == s_pin) {
        return ESP_OK;
    }
    if (s_up) {
        dinmidi_stop();
    }

    const uart_config_t cfg = {
        .baud_rate = DIN_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    /* THE RX BUFFER IS NOT OPTIONAL even though nothing reads it. IDF rejects a
     * zero rx_buffer_size outright - it must exceed the hardware FIFO - so
     * asking for a TX-only driver by passing 0 fails the whole install with
     * ESP_ERR_INVALID_ARG, which looked exactly like "GPIO17 is bad". The
     * smallest legal buffer costs 256 bytes and is never touched. */
    esp_err_t err = uart_driver_install(DIN_UART, 256, DIN_TX_BUF, 0, NULL, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install: %s", esp_err_to_name(err));
        return err;
    }
    err = uart_param_config(DIN_UART, &cfg);
    if (err == ESP_OK) {
        /* TX only. A MIDI input needs an optocoupler, so claiming an RX pin
         * here would promise something the hardware cannot do. */
        err = uart_set_pin(DIN_UART, gpio, UART_PIN_NO_CHANGE,
                           UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart config/pin on GPIO%d: %s", gpio,
                 esp_err_to_name(err));
        uart_driver_delete(DIN_UART);
        return err;
    }
    s_pin = gpio;
    s_up = true;
    s_bytes = 0;
    ESP_LOGI(TAG, "MIDI out on GPIO%d at %d baud", gpio, DIN_BAUD);
    return ESP_OK;
}

void dinmidi_stop(void)
{
    if (!s_up) {
        return;
    }
    uart_driver_delete(DIN_UART);
    s_up = false;
    s_pin = -1;
}

void dinmidi_send(const char *lane, uint8_t status, uint8_t d1, uint8_t d2,
                  uint32_t when_us)
{
    (void)lane;
    (void)when_us;
    if (!s_up) {
        return;
    }
    /* 0xF9 IS NOT MIDI. It is the deck's own step marker, undefined in the
     * spec, and it is how a destination that cares about the bar learns where
     * the bar is. Putting it on a wire that real hardware listens to would send
     * an undefined realtime byte to a parser with no reason to expect one - the
     * USB and BLE sinks both drop it, and this one has to agree. */
    if (midi_is_internal(status)) {
        return;
    }
    uint8_t b[3];
    int n = 0;
    b[n++] = status;
    const int extra = midi_datalen(status);
    if (extra >= 1) { b[n++] = (uint8_t)(d1 & 0x7F); }
    if (extra >= 2) { b[n++] = (uint8_t)(d2 & 0x7F); }

    /* NEVER BLOCK. This is called from the transport task, which the clock
     * depends on staying responsive; a full buffer must lose a byte rather than
     * stall a tick. At 31250 baud a three-byte message is under a millisecond
     * and the 512-byte buffer is a sixth of a second of slack, so the only way
     * to fill it is to have no cable attached and no receiver draining it -
     * in which case there is nothing to be late for. */
    const int wrote = uart_write_bytes(DIN_UART, (const char *)b, (size_t)n);
    if (wrote > 0) {
        s_bytes += (uint32_t)wrote;
    }
}
