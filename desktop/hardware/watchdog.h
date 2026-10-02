/*
 * hardware/watchdog.h - Windows stand-in for the Pico SDK header. A watchdog
 * reboot has no PC equivalent, so it closes the program; the feed/enable calls
 * do nothing.
 *
 * Author: Thomas Dzubin
 */
#ifndef SHIM_HARDWARE_WATCHDOG_H
#define SHIM_HARDWARE_WATCHDOG_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

static inline void watchdog_reboot(uint32_t pc, uint32_t sp, uint32_t delay_ms)
{
    (void)pc;
    (void)sp;
    (void)delay_ms;
    exit(0);
}

static inline void watchdog_enable(uint32_t delay_ms, bool pause_on_debug)
{
    (void)delay_ms;
    (void)pause_on_debug;
}

static inline void watchdog_update(void) { }

#endif /* SHIM_HARDWARE_WATCHDOG_H */
