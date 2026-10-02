# PicoCalc Music Maker - desktop (SDL2) build for Windows and Linux, by Thomas Dzubin.
#
# Builds the same music_maker.c as the firmware. Instead of the PicoCalc drivers it
# links the small SDL2 shim in this folder, which re-implements the
# picocalc-text-starter driver API (lcd / audio / south-bridge keyboard, by
# Blair Leduc) - see shim.h. The vendored starter's headers and fonts are used
# as-is and nothing in picocalc-text-starter-main/ is modified. The shared build
# logic (SDL2, output names, copying the result) is in shim_desktop.cmake.
#
# Build on Windows (MSYS2 UCRT64 shell with SDL2 installed):
#     cmake -S desktop -B build-windows -G Ninja
#     ninja -C build-windows
# Output: music-maker-Windows.exe, copied to the top-level project folder. SDL2 is
# linked statically, so it is one self-contained file.
#
# Build on Linux (needs the SDL2 development package, e.g. libsdl2-dev):
#     cmake -S desktop -B build-linux
#     cmake --build build-linux
# Output: music-maker-Linux, copied to the top-level project folder together with
# libSDL2-2.0.so.0 (SDL2 is NOT linked in statically on Linux - keep the two
# files side by side; the executable finds the library through an $ORIGIN
# run-path).

cmake_minimum_required(VERSION 3.13)

project(picocalc-music-maker-desktop C)

set(CMAKE_C_STANDARD 11)

get_filename_component(TOP ${CMAKE_CURRENT_LIST_DIR}/.. ABSOLUTE)
set(STARTER ${TOP}/picocalc-text-starter-main)

include(${CMAKE_CURRENT_LIST_DIR}/shim_desktop.cmake)

picocalc_desktop_app(
    NAME     music-maker
    TITLE    "PicoCalc Music Maker"
    SOURCES
        ${TOP}/music_maker.c
        ${CMAKE_CURRENT_LIST_DIR}/platform_desktop.c
        ${STARTER}/songs.c
        ${STARTER}/drivers/font-8x10.c
        ${STARTER}/drivers/font-5x10.c
    INCLUDES
        ${TOP}
        ${STARTER}
        ${STARTER}/drivers
)
