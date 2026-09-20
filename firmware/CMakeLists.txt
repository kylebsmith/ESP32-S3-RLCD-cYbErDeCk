# cYbErDeCk OS - firmware
#
# Steps 1-4 of the build order in docs/OS.md: display, text grid, BLE HID
# keyboard, and a text buffer that survives power loss.

cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(cyberdeck)
