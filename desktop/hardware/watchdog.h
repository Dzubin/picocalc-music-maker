/*
 * hardware/watchdog.h - desktop stand-in for the Pico SDK header. A watchdog
 * reboot has no PC equivalent, so it closes the program; the feed/enable calls
 * do nothing. Of the chip's watchdog registers only the scratch registers are
 * modelled (see watchdog_hw below).
 *
 * Author: Thomas Dzubin
 */
#ifndef SHIM_HARDWARE_WATCHDOG_H
#define SHIM_HARDWARE_WATCHDOG_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

/* The firmware writes a command for the PicoCalc UF2 Loader into the
 * watchdog scratch registers just before it reboots. A PC has no loader, so
 * this only gives that code somewhere to write; the watchdog_reboot() that
 * follows closes the program.                                              */
typedef struct {
    volatile uint32_t scratch[8];
} shim_watchdog_regs_t;

static inline shim_watchdog_regs_t *shim_watchdog_hw(void)
{
    static shim_watchdog_regs_t regs;
    return &regs;
}

#define watchdog_hw shim_watchdog_hw()

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
