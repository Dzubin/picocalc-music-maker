/*
 * hardware/gpio.h - desktop stand-in: pin muxing and direction calls do
 * nothing on a PC.
 *
 * Author: Thomas Dzubin
 */
#ifndef SHIM_HARDWARE_GPIO_H
#define SHIM_HARDWARE_GPIO_H

#include "pico/stdlib.h"

enum gpio_function { GPIO_FUNC_XIP = 0, GPIO_FUNC_SPI = 1, GPIO_FUNC_UART = 2,
                     GPIO_FUNC_I2C = 3, GPIO_FUNC_PWM = 4, GPIO_FUNC_SIO = 5,
                     GPIO_FUNC_PIO0 = 6, GPIO_FUNC_PIO1 = 7, GPIO_FUNC_NULL = 0x1f };

#define GPIO_OUT true
#define GPIO_IN  false

static inline void gpio_set_function(uint gpio, enum gpio_function f) { (void)gpio; (void)f; }
static inline void gpio_set_dir(uint gpio, bool out)                  { (void)gpio; (void)out; }
static inline void gpio_put(uint gpio, bool value)                    { (void)gpio; (void)value; }

#endif /* SHIM_HARDWARE_GPIO_H */
