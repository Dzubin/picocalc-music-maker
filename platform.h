/*
 * platform.h - the few things music_maker.c needs from the machine and the
 * operating system besides the LCD, audio and keyboard drivers: starting the
 * runtime, a millisecond clock, sleeping, and the two ways of leaving the
 * program. Each platform supplies its own implementation:
 *
 *     PicoCalc firmware   platform_pico.c
 *     Windows / Linux     desktop/platform_desktop.c
 *
 * Author: Thomas Dzubin
 */
#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>

void     plat_init(void);               /* start the runtime, once at startup  */
uint32_t plat_now_ms(void);             /* milliseconds since start (wraps)    */
void     plat_sleep_ms(uint32_t ms);    /* wait; the platform keeps running    */

/* Leave the program. Neither one returns. */
_Noreturn void plat_bootsel(void);      /* PicoCalc: USB drive (BOOTSEL) mode  */
_Noreturn void plat_reboot(void);       /* PicoCalc: restart; PC: just exit    */

#endif /* PLATFORM_H */
