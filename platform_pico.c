/*
 * platform_pico.c - platform.h on the Raspberry Pi Pico SDK (PicoCalc
 * firmware, RP2040 and RP2350).
 *
 * Author: Thomas Dzubin
 */
#include <stdbool.h>

#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/watchdog.h"

#include "platform.h"

/* The vendored audio driver (audio.c) tests this flag in its blocking song
 * player, so it must exist for the link to succeed. The keyboard and serial
 * drivers set it; nothing here reads it. */
volatile bool user_interrupt = false;

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
    reset_usb_boot(0, 0);
    for (;;)
        tight_loop_contents();
}

_Noreturn void plat_reboot(void)
{
    watchdog_reboot(0, 0, 0);       /* pc = 0: normal boot from flash */
    for (;;)
        tight_loop_contents();
}
