/*
 * pico/bootrom.h - desktop stand-in. "Reboot to BOOTSEL" has no meaning on a
 * PC, so reset_usb_boot() simply closes the program.
 *
 * Author: Thomas Dzubin
 */
#ifndef SHIM_PICO_BOOTROM_H
#define SHIM_PICO_BOOTROM_H

#include <stdint.h>

void reset_usb_boot(uint32_t usb_activity_gpio_pin_mask, uint32_t disable_interface_mask);

/* The RP2350 SDK spells it this way as well. */
#define rom_reset_usb_boot(pin_mask, disable_mask) reset_usb_boot((pin_mask), (disable_mask))

#endif /* SHIM_PICO_BOOTROM_H */
