/*
 * shim.h - internal interface between the pieces of the Windows (SDL2)
 * PicoCalc shim: the window/timing core, the LCD, the keyboard and the audio.
 *
 * Author: Thomas Dzubin
 */
#ifndef SHIM_H
#define SHIM_H

#include <SDL.h>

#include "pico/stdlib.h"
#include "lcd.h"            /* WIDTH, HEIGHT (vendored header, unmodified) */
#include "shim_config.h"

/* The 320x320 RGB565 picture that lcd.c draws into and the window shows. */
extern uint16_t shim_framebuffer[WIDTH * HEIGHT];

void     shim_init(void);           /* open the window (safe to call twice) */
void     shim_mark_dirty(void);     /* the picture changed; show it soon    */
void     shim_pump(void);           /* handle window/keyboard events, redraw */

void     shim_input_event(const SDL_KeyboardEvent *key);
uint16_t shim_key_pop(void);        /* (state << 8) | code, or 0 if none    */

#endif /* SHIM_H */
