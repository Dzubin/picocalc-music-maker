/*
 * shim_core.c - window, timing and event pump for the Windows (SDL2) PicoCalc
 * shim. Everything else in the shim hangs off shim_pump(): any call that
 * waits (sleep_ms, sb_read_keyboard, ...) or draws (lcd_blit) runs it, so the
 * window stays responsive without the program having to know about SDL.
 *
 * Author: Thomas Dzubin
 */
#include "shim.h"

uint16_t shim_framebuffer[WIDTH * HEIGHT];

static SDL_Window   *window;
static SDL_Renderer *renderer;
static SDL_Texture  *texture;
static bool          dirty;
static uint64_t      last_present_us;
static uint64_t      start_counter;

/* ---- timing ---------------------------------------------------------- */

uint64_t time_us_64(void)
{
    uint64_t freq = SDL_GetPerformanceFrequency();
    uint64_t now  = SDL_GetPerformanceCounter() - start_counter;
    return (now / freq) * 1000000u + ((now % freq) * 1000000u) / freq;
}

uint32_t time_us_32(void)                 { return (uint32_t)time_us_64(); }
absolute_time_t get_absolute_time(void)   { return time_us_64(); }
uint32_t to_ms_since_boot(absolute_time_t t) { return (uint32_t)(t / 1000u); }
uint64_t to_us_since_boot(absolute_time_t t) { return t; }
int64_t  absolute_time_diff_us(absolute_time_t from, absolute_time_t to)
{
    return (int64_t)(to - from);
}
absolute_time_t delayed_by_ms(absolute_time_t t, uint32_t ms) { return t + ms * 1000ull; }
absolute_time_t make_timeout_time_ms(uint32_t ms) { return time_us_64() + ms * 1000ull; }
bool time_reached(absolute_time_t t)      { return time_us_64() >= t; }

void sleep_us(uint64_t us)
{
    uint64_t end = time_us_64() + us;
    for (;;) {
        shim_pump();
        uint64_t now = time_us_64();
        if (now >= end)
            break;
        SDL_Delay((end - now) >= 1000u ? 1u : 0u);
    }
}

void sleep_ms(uint32_t ms)       { sleep_us(ms * 1000ull); }
void busy_wait_ms(uint32_t ms)   { sleep_us(ms * 1000ull); }
void busy_wait_us(uint64_t us)   { sleep_us(us); }
void tight_loop_contents(void)   { shim_pump(); }
void stdio_init_all(void)        { }

void reset_usb_boot(uint32_t pin_mask, uint32_t disable_mask)
{
    (void)pin_mask;
    (void)disable_mask;
    exit(0);
}

/* ---- window ---------------------------------------------------------- */

static void shim_shutdown(void)
{
    if (texture)  SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window)   SDL_DestroyWindow(window);
    SDL_Quit();
}

void shim_init(void)
{
    if (window)
        return;

    SDL_SetMainReady();
    start_counter = SDL_GetPerformanceCounter();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        exit(1);
    }
    atexit(shim_shutdown);

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");    /* crisp pixels */
    window = SDL_CreateWindow(SHIM_WINDOW_TITLE,
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              WIDTH * SHIM_WINDOW_SCALE, HEIGHT * SHIM_WINDOW_SCALE,
                              SDL_WINDOW_RESIZABLE);
    if (window)
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (window && !renderer)
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (renderer) {
        SDL_RenderSetLogicalSize(renderer, WIDTH, HEIGHT);
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565,
                                    SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    }
    if (!texture) {
        fprintf(stderr, "SDL window setup failed: %s\n", SDL_GetError());
        exit(1);
    }
    dirty = true;
    last_present_us = 0;
}

static void present(void)
{
    SDL_UpdateTexture(texture, NULL, shim_framebuffer, WIDTH * (int)sizeof(uint16_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    dirty = false;
    last_present_us = time_us_64();
}

void shim_mark_dirty(void)
{
    dirty = true;
    shim_pump();
}

void shim_pump(void)
{
    SDL_Event e;

    if (!window)
        return;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_QUIT:
            exit(0);
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            shim_input_event(&e.key);
            break;
        case SDL_WINDOWEVENT:
            if (e.window.event == SDL_WINDOWEVENT_EXPOSED ||
                e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                dirty = true;
            break;
        default:
            break;
        }
    }
    if (dirty && time_us_64() - last_present_us >= SHIM_PRESENT_INTERVAL_US)
        present();
}
