#include "usbmux.h"

#include "esp_attr.h"
#include "esp_log.h"
#include "hal/usb_serial_jtag_ll.h"
#include "soc/rtc_cntl_reg.h"

static const char *TAG = "usbmux";

/* RTC_NOINIT, not NVS. A boot loop caused by a USB bring-up that panics never
 * reaches code that could write NVS, and a counter that cannot be incremented
 * during the failure it is counting is not a counter. This survives
 * esp_restart and panic_restart, and a power cycle clears it. */
#define TRY_MAGIC 0x5542AA01u
static RTC_NOINIT_ATTR uint32_t s_try_magic;
static RTC_NOINIT_ATTR uint32_t s_try;

static void try_init(void)
{
    if (s_try_magic != TRY_MAGIC) {
        s_try_magic = TRY_MAGIC;
        s_try = 0;
    }
}

unsigned usbmux_try_count(void) { try_init(); return (unsigned)s_try; }
void     usbmux_try_bump(void)  { try_init(); s_try++; }
void     usbmux_try_clear(void) { try_init(); s_try = 0; }

void usbmux_release_to_usj(void)
{
    /* Order matters: bring the USJ pad up BEFORE handing it the PHY, so there
     * is no window in which the PHY is routed to a block that is not driving
     * it. This is a different register from RTC_CNTL_USB_PAD_ENABLE, which is
     * gated behind an override bit nothing in ESP-IDF ever sets - leave that
     * one alone. Fewer bits written is fewer bits to be wrong about. */
    usb_serial_jtag_ll_phy_enable_pad(true);
    CLEAR_PERI_REG_MASK(RTC_CNTL_USB_CONF_REG,
                        RTC_CNTL_SW_USB_PHY_SEL | RTC_CNTL_SW_HW_USB_PHY_SEL);
}

void usbmux_take_for_otg(void)
{
    /* sw_hw_usb_phy_sel = 1 takes software control of the mux; sw_usb_phy_sel
     * = 1 then points the internal PHY at the USB Wrap (OTG) and leaves the
     * USJ looking at an external PHY that does not exist. */
    SET_PERI_REG_MASK(RTC_CNTL_USB_CONF_REG,
                      RTC_CNTL_SW_HW_USB_PHY_SEL | RTC_CNTL_SW_USB_PHY_SEL);
    ESP_LOGW(TAG, "PHY handed to USB-OTG; the console is gone until restored");
}
