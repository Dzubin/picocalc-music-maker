# PicoCalc Music Maker - desktop (SDL2) build for Windows and Linux, by Thomas Dzubin.
#
# Builds the same music_maker.c as the firmware. Instead of the PicoCalc drivers it
# links the small SDL2 shim in this folder, which re-implements the
# picocalc-text-starter driver API (lcd / audio / south-bridge keyboard, by
# Blair Leduc) - see shim.h. The vendored starter's headers and fonts are used
# as-is and nothing in picocalc-text-starter-main/ is modified.
#
# Build on Windows (MSYS2 UCRT64 shell with SDL2 installed):
#     cmake -S desktop -B build-windows -G Ninja
#     ninja -C build-windows
# Output: picocalc-music-maker-Windows.exe, copied to the top-level project folder. SDL2 is
# linked statically, so it is one self-contained file.
#
# Build on Linux (needs the SDL2 development package, e.g. libsdl2-dev):
#     cmake -S desktop -B build-linux
#     cmake --build build-linux
# Output: picocalc-music-maker-Linux, copied to the top-level project folder together with
# libSDL2-2.0.so.0 (SDL2 is NOT linked in statically on Linux - keep the two
# files side by side; the executable finds the library through an $ORIGIN
# run-path).

cmake_minimum_required(VERSION 3.13)

project(picocalc-music-maker-desktop C)

set(CMAKE_C_STANDARD 11)

# The platform is part of the executable's file name (picocalc-music-maker-Windows.exe /
# picocalc-music-maker-Linux), so a downloaded file always says which system it is for.
if(WIN32)
    set(PLATFORM_NAME Windows)
else()
    set(PLATFORM_NAME Linux)
endif()
set(APP_NAME picocalc-music-maker-${PLATFORM_NAME})

get_filename_component(TOP ${CMAKE_CURRENT_LIST_DIR}/.. ABSOLUTE)
set(STARTER ${TOP}/picocalc-text-starter-main)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

add_executable(${APP_NAME}
    ${TOP}/music_maker.c
    platform_desktop.c
    ${STARTER}/songs.c
    ${STARTER}/drivers/font-8x10.c
    ${STARTER}/drivers/font-5x10.c
    shim_core.c
    shim_lcd.c
    shim_input.c
    shim_audio.c
)

# The shim folder comes first so "pico/stdlib.h" finds the stand-in header.
target_include_directories(${APP_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${TOP}
    ${STARTER}
    ${STARTER}/drivers
    ${SDL2_INCLUDE_DIRS}
)

# SDL_MAIN_HANDLED: keep the program's own main() (no SDL_main rewriting).
target_compile_definitions(${APP_NAME} PRIVATE
    SDL_MAIN_HANDLED
    "SHIM_WINDOW_TITLE=\"PicoCalc Music Maker\""
)
target_compile_options(${APP_NAME} PRIVATE -Wall)

if(WIN32)
    # Windows: link SDL2 (and the C runtime) statically so the .exe is one
    # self-contained file with no SDL2.dll beside it. SDL2main is dropped
    # because of SDL_MAIN_HANDLED.
    list(REMOVE_ITEM SDL2_STATIC_LIBRARIES SDL2main)
    target_link_directories(${APP_NAME} PRIVATE ${SDL2_STATIC_LIBRARY_DIRS})
    target_link_libraries(${APP_NAME} PRIVATE ${SDL2_STATIC_LIBRARIES})
    target_link_options(${APP_NAME} PRIVATE -static ${SDL2_STATIC_LDFLAGS_OTHER})
else()
    # Linux: link SDL2 dynamically and ship the library beside the executable.
    # "$ORIGIN" in the run-path makes the executable look for libSDL2 in its
    # own folder first, so the two files travel together.
    target_link_directories(${APP_NAME} PRIVATE ${SDL2_LIBRARY_DIRS})
    target_link_libraries(${APP_NAME} PRIVATE ${SDL2_LIBRARIES} m)
    target_link_options(${APP_NAME} PRIVATE "LINKER:-rpath,$ORIGIN")

    # Find the real shared library (following the .so symlinks) and copy it,
    # under its soname, next to the executable in the build folder and in the
    # top-level project folder.
    find_library(SDL2_SHARED_LIB NAMES libSDL2-2.0.so.0 SDL2-2.0 SDL2 HINTS ${SDL2_LIBRARY_DIRS})
    if(NOT SDL2_SHARED_LIB)
        message(FATAL_ERROR "libSDL2 shared library not found - install the SDL2 development package (e.g. libsdl2-dev)")
    endif()
    get_filename_component(SDL2_SHARED_REAL "${SDL2_SHARED_LIB}" REALPATH)
    add_custom_command(TARGET ${APP_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${SDL2_SHARED_REAL}" $<TARGET_FILE_DIR:${APP_NAME}>/libSDL2-2.0.so.0
        COMMAND ${CMAKE_COMMAND} -E copy "${SDL2_SHARED_REAL}" ${TOP}/libSDL2-2.0.so.0
    )
endif()

# Copy the finished executable to the top-level project folder.
add_custom_command(TARGET ${APP_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${APP_NAME}> ${TOP}/
)
