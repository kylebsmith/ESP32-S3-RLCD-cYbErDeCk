/*
 * Who owns the single internal USB FSLS PHY.
 *
 * THE ESP32-S3 HAS ONE USB PHY AND TWO PERIPHERALS THAT WANT IT: the
 * USB-Serial-JTAG block, which is how this deck is flashed and how its console
 * and serial keyboard work, and USB-OTG, which is what a USB MIDI device needs.
 * A two-bit mux in RTC_CNTL decides.
 *
 * WHY THIS FILE EXISTS, AND IT IS NOT A TIDINESS ARGUMENT.
 *
 * '>flash' was written specifically so that USB MIDI could never leave this
 * deck needing a BOOT button. It sets RTC_CNTL_FORCE_DOWNLOAD_BOOT and
 * restarts, and that was verified on hardware. It is NOT SUFFICIENT, for a
 * reason that is only visible if you go and look:
 *
 *  - usb_new_phy(OTG) sets RTC_CNTL_SW_HW_USB_PHY_SEL (bit 20) and
 *    RTC_CNTL_SW_USB_PHY_SEL (bit 19)  [hal/esp32s3/usb_wrap_ll.h:56-67].
 *  - Nothing in ESP-IDF puts them back. usb_del_phy() drops the pull
 *    overrides and gates the bus clock; it does not touch the mux.
 *  - esp_restart() does not reset RTC_CNTL, so the bits SURVIVE A REBOOT -
 *    exactly as RTC_CNTL_OPTION1_REG does, which this project already knows
 *    because it was bitten by that once.
 *  - And the ROM does not restore them either. The whole ESP32-S3 ROM
 *    references 0x60008120 three times and never writes bit 19 or 20:
 *    usb_otg_get_phy@0x400539e0 only reads, chip_usb_dw_init@0x40053c00
 *    clears only bits 18 and 17, chip_usb_dw_prepare_persist@0x40053cf4 the
 *    same two.
 *
 * So a deck that switched to OTG and then ran '>flash' would reboot into a
 * download mode reachable only over a PHY that USB-Serial-JTAG no longer
 * owns. The familiar /dev node would simply not appear. That is the exact
 * brick '>flash' exists to prevent, and it would have been discovered by
 * hitting it.
 *
 * Clearing both bits returns the mux to its hardware default, which is
 * USB-Serial-JTAG on any part whose USB_PHY_SEL eFuse is unburned.
 */
#pragma once

/* Hand the PHY back to USB-Serial-JTAG. Safe to call when it already owns it,
 * and safe to call before anything has ever taken it away - which is why it
 * runs unconditionally at the top of app_main. */
void usbmux_release_to_usj(void);

/* Point the PHY at USB-OTG. This is what TinyUSB's bring-up does internally;
 * it exists here so the restore can be TESTED without pulling in TinyUSB,
 * before anything is allowed to depend on it. */
void usbmux_take_for_otg(void);

/* How many consecutive boots have tried to bring USB up without a host ever
 * confirming. Lives in RTC_NOINIT: it survives esp_restart AND a panic reset,
 * and is cleared by power loss - which is exactly the lifetime wanted, because
 * "unplug it" must always be a real way out. */
unsigned usbmux_try_count(void);
void     usbmux_try_bump(void);
void     usbmux_try_clear(void);
