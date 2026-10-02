/*
 * pico/stdlib.h - stand-in for the Raspberry Pi Pico SDK header of the same
 * name, so PicoCalc code (and the vendored picocalc-text-starter headers by
 * Blair Leduc) compile unchanged for Windows. Only the small slice of the SDK
 * that these programs use is provided; the implementations are in
 * shim_core.c.
 *
 * Author: Thomas Dzubin
 */
#ifndef SHIM_PICO_STDLIB_H
#define SHIM_PICO_STDLIB_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The SDK's shorthand for unsigned int (glibc on Linux already provides it). */
#if !(defined(__GLIBC__) && defined(__USE_MISC))
typedef unsigned int uint;
#endif

/* The real SDK uses a struct; a plain microsecond count behaves the same for
 * every way these programs use it. */
typedef uint64_t absolute_time_t;

void     stdio_init_all(void);

void     sleep_ms(uint32_t ms);
void     sleep_us(uint64_t us);
void     busy_wait_ms(uint32_t ms);
void     busy_wait_us(uint64_t us);
void     tight_loop_contents(void);

uint32_t time_us_32(void);
uint64_t time_us_64(void);
absolute_time_t get_absolute_time(void);
uint32_t to_ms_since_boot(absolute_time_t t);
uint64_t to_us_since_boot(absolute_time_t t);
int64_t  absolute_time_diff_us(absolute_time_t from, absolute_time_t to);
absolute_time_t delayed_by_ms(absolute_time_t t, uint32_t ms);
absolute_time_t make_timeout_time_ms(uint32_t ms);
bool     time_reached(absolute_time_t t);

/* Memory-placement attributes mean nothing on a PC. */
#define __not_in_flash_func(f)   f
#define __time_critical_func(f)  f
#define __no_inline_not_in_flash_func(f) f

#include "hardware/gpio.h"   /* the real pico/stdlib.h pulls this in too */

#endif /* SHIM_PICO_STDLIB_H */
