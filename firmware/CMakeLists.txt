# cYbErDeCk OS - firmware
#
# Steps 1-4 of the build order in docs/OS.md: display, text grid, BLE HID
# keyboard, and a text buffer that survives power loss.

cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(cyberdeck)

# WHY USB MIDI SENT NOTHING, AND THE ONE-LINE FIX.
#
# TinyUSB buffers MIDI into a FIFO and only starts a USB transfer when the
# FIFO holds a full packet:
#
#   tusb.c tu_edpt_stream_write():
#     if ((tu_fifo_count(&s->ff) >= s->mps) || (tu_fifo_depth(&s->ff) < s->mps))
#         tu_edpt_stream_write_xfer(s);
#
# The TX stream's mps is CFG_TUD_MIDI_TX_EPSIZE, which esp_tinyusb leaves at
# 64. A note-on is ONE four-byte USB-MIDI event packet, and a sequencer then
# goes quiet for ~121 ms - so the FIFO never reached 64 bytes and the transfer
# was never started. The deck reported 242 messages written and zero dropped,
# every write succeeded, the device and the MIDI interface were both mounted,
# and not one byte reached the host. Sixteen notes had to pile up before
# anything moved.
#
# Four bytes is exactly one USB-MIDI event packet, so every note flushes as it
# is written. A short bulk packet against a 64-byte endpoint descriptor is
# ordinary USB and terminates the transfer, which is what low latency looks
# like here. The RX direction is untouched and stays 64.
#
# Set on the component target rather than by forking it, because the value is
# #ifndef-guarded precisely so that it can be overridden.
if(TARGET __idf_espressif__tinyusb)
    target_compile_definitions(__idf_espressif__tinyusb
                               PRIVATE CFG_TUD_MIDI_TX_EPSIZE=4)
endif()
