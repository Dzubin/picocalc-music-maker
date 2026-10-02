/*
 * shim_config.h - tunables for the Windows (SDL2) PicoCalc shim.
 *
 * Author: Thomas Dzubin
 *
 * Every constant the shim uses lives here so the .c files hold only code.
 */
#ifndef SHIM_CONFIG_H
#define SHIM_CONFIG_H

/* Window title; the CMake file overrides this per project. */
#ifndef SHIM_WINDOW_TITLE
#define SHIM_WINDOW_TITLE       "PicoCalc"
#endif

/* The window opens at (LCD size * this); it is resizable, and SDL keeps
 * the picture at the LCD's aspect ratio. */
#define SHIM_WINDOW_SCALE       2

/* Longest gap between screen refreshes while the program is busy. */
#define SHIM_PRESENT_INTERVAL_US 16000

/* Keyboard: size of the key-event FIFO and of the keyboard_get_key() buffer. */
#define SHIM_KEY_QUEUE_SIZE     64

/* Audio: stereo signed 16-bit square waves, like the PicoCalc's PWM output. */
#define SHIM_AUDIO_RATE         44100
#define SHIM_AUDIO_SAMPLES      512
#define SHIM_AUDIO_AMPLITUDE    5000

#endif /* SHIM_CONFIG_H */
