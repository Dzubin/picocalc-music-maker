# shim_desktop.cmake - shared build logic for the desktop (SDL2) PicoCalc
# build on Windows and Linux. By Thomas Dzubin.
#
# The project's desktop/CMakeLists.txt includes this file and calls
# picocalc_desktop_app(), which adds the executable, the shim sources, SDL2
# (static on Windows, dynamic on Linux) and the copy of the finished program
# into the top-level project folder (the parent of desktop/).
#
#     include(${CMAKE_CURRENT_LIST_DIR}/shim_desktop.cmake)
#     picocalc_desktop_app(
#         NAME     my-program                  # file becomes my-program-Windows.exe
#         TITLE    "My Program"                # window title
#         SOURCES  ${TOP}/my_program.c ...     # the program and any vendored .c
#         INCLUDES ${TOP} ${STARTER} ${STARTER}/drivers
#     )
#
# INCLUDES must reach the vendored lcd.h, which shim.h includes. The shim
# folder is always searched first so "pico/stdlib.h" finds the stand-in.
#
# Windows: cmake -S desktop -B build-windows -G Ninja ; ninja -C build-windows
# Linux:   cmake -S desktop -B build-linux ; cmake --build build-linux
#
# This file is copied in by sync-kit.sh. Change the kit's copy, not this one.

set(PICOCALC_SHIM_DIR ${CMAKE_CURRENT_LIST_DIR})

function(picocalc_desktop_app)
    cmake_parse_arguments(APP "" "NAME;TITLE" "SOURCES;INCLUDES" ${ARGN})
    if(NOT APP_NAME OR NOT APP_SOURCES)
        message(FATAL_ERROR "picocalc_desktop_app: NAME and SOURCES are required")
    endif()
    if(NOT APP_TITLE)
        set(APP_TITLE "PicoCalc")
    endif()

    # The platform is part of the executable's file name (NAME-Windows.exe /
    # NAME-Linux), so a downloaded file always says which system it is for.
    if(WIN32)
        set(PLATFORM_NAME Windows)
    else()
        set(PLATFORM_NAME Linux)
    endif()
    set(TARGET_NAME ${APP_NAME}-${PLATFORM_NAME})

    # The finished program is copied to the parent of the desktop/ folder.
    get_filename_component(OUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/.. ABSOLUTE)

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)

    add_executable(${TARGET_NAME}
        ${APP_SOURCES}
        ${PICOCALC_SHIM_DIR}/shim_core.c
        ${PICOCALC_SHIM_DIR}/shim_lcd.c
        ${PICOCALC_SHIM_DIR}/shim_input.c
        ${PICOCALC_SHIM_DIR}/shim_audio.c
    )

    target_include_directories(${TARGET_NAME} PRIVATE
        ${PICOCALC_SHIM_DIR}
        ${APP_INCLUDES}
        ${SDL2_INCLUDE_DIRS}
    )

    # SDL_MAIN_HANDLED: keep the program's own main() (no SDL_main rewriting).
    target_compile_definitions(${TARGET_NAME} PRIVATE
        SDL_MAIN_HANDLED
        "SHIM_WINDOW_TITLE=\"${APP_TITLE}\""
    )
    target_compile_options(${TARGET_NAME} PRIVATE -Wall)

    if(WIN32)
        # Windows: link SDL2 (and the C runtime) statically so the .exe is one
        # self-contained file with no SDL2.dll beside it. SDL2main is dropped
        # because of SDL_MAIN_HANDLED.
        list(REMOVE_ITEM SDL2_STATIC_LIBRARIES SDL2main)
        target_link_directories(${TARGET_NAME} PRIVATE ${SDL2_STATIC_LIBRARY_DIRS})
        target_link_libraries(${TARGET_NAME} PRIVATE ${SDL2_STATIC_LIBRARIES})
        target_link_options(${TARGET_NAME} PRIVATE -static ${SDL2_STATIC_LDFLAGS_OTHER})
    else()
        # Linux: link SDL2 dynamically and ship the library beside the
        # executable. "$ORIGIN" in the run-path makes the executable look for
        # libSDL2 in its own folder first, so the two files travel together.
        target_link_directories(${TARGET_NAME} PRIVATE ${SDL2_LIBRARY_DIRS})
        target_link_libraries(${TARGET_NAME} PRIVATE ${SDL2_LIBRARIES} m)
        target_link_options(${TARGET_NAME} PRIVATE "LINKER:-rpath,$ORIGIN")

        # Find the real shared library (following the .so symlinks) and copy
        # it, under its soname, next to the executable in the build folder and
        # in the top-level project folder.
        find_library(SDL2_SHARED_LIB NAMES libSDL2-2.0.so.0 SDL2-2.0 SDL2 HINTS ${SDL2_LIBRARY_DIRS})
        if(NOT SDL2_SHARED_LIB)
            message(FATAL_ERROR "libSDL2 shared library not found - install the SDL2 development package (e.g. libsdl2-dev)")
        endif()
        get_filename_component(SDL2_SHARED_REAL "${SDL2_SHARED_LIB}" REALPATH)
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy "${SDL2_SHARED_REAL}" $<TARGET_FILE_DIR:${TARGET_NAME}>/libSDL2-2.0.so.0
            COMMAND ${CMAKE_COMMAND} -E copy "${SDL2_SHARED_REAL}" ${OUT_DIR}/libSDL2-2.0.so.0
        )
    endif()

    # Copy the finished executable to the top-level project folder.
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${TARGET_NAME}> ${OUT_DIR}/
    )
endfunction()
