/*
 * platform_desktop.c - platform.h for the Windows / Linux (SDL2) build. The
 * clock and sleep come from the shim's core; there is no BOOTSEL or UF2 Loader
 * on a PC, so both just close the program.
 *
 * Author: Thomas Dzubin
 */
#include <stdlib.h>

#include "pico/stdlib.h"            /* the shim's stand-in header */

#include "platform.h"

void plat_init(void)
{
    stdio_init_all();
}

uint32_t plat_now_ms(void)
{
    return to_ms_since_boot(get_absolute_time());
}

void plat_sleep_ms(uint32_t ms)
{
    sleep_ms(ms);
}

_Noreturn void plat_bootsel(void)
{
    exit(0);
}

_Noreturn void plat_exit_to_loader(void)
{
    exit(0);
}
