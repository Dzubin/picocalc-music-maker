/*
 * platform_pico_const.h - constants for platform_pico.c: leaving the program
 * for the PicoCalc UF2 Loader (pelrun/uf2loader).
 *
 * The loader has no call for an app to use, but its own menu hands commands to
 * its start-up code through the chip's watchdog scratch registers, which
 * survive a watchdog reboot: scratch 0 holds a magic number, 1 the boot mode,
 * 2 an argument. Asking for boot mode "SD" and then rebooting makes the loader
 * show its menu again. If the program was flashed straight to the chip with no
 * loader, nothing reads the request and the program simply restarts.
 *
 * Author: Thomas Dzubin
 */
#ifndef PLATFORM_PICO_CONST_H
#define PLATFORM_PICO_CONST_H

#define LOADER_COMMAND_MAGIC      0xE98CC638u /* PICOCALC_BL_MAGIC in the loader's proginfo.h */
#define LOADER_BOOT_MODE_SD       1           /* BOOT_SD: load the menu from the SD card */
#define LOADER_SCRATCH_MAGIC      0
#define LOADER_SCRATCH_MODE       1
#define LOADER_SCRATCH_ARGUMENT   2

/* The watchdog reboot happens this many ms after it is requested. */
#define LOADER_REBOOT_DELAY_MS    10

#endif /* PLATFORM_PICO_CONST_H */
