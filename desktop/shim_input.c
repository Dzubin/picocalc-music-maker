/*
 * shim_input.c - the PicoCalc south-bridge keyboard (southbridge.h / keyboard.h
 * from picocalc-text-starter by Blair Leduc) re-implemented on SDL key events
 * for the Windows build.
 *
 * The hardware keyboard FIFO returns one 16-bit event per read: the state
 * (pressed / hold / released) in the high byte and the key code in the low
 * byte. SDL key events are translated into exactly that, so programs that
 * poll sb_read_keyboard() directly and programs that use keyboard_get_key()
 * both work unchanged. A US keyboard layout is assumed for shifted symbols.
 *
 * Author: Thomas Dzubin
 */
#include "shim.h"
#include "keyboard.h"
#include "southbridge.h"

static uint16_t queue[SHIM_KEY_QUEUE_SIZE];
static unsigned q_head, q_tail;
static uint8_t  down_code[SDL_NUM_SCANCODES];   /* code sent on press, for the release */

static char kbuf[KBD_BUFFER_SIZE];
static unsigned k_head, k_tail;
static keyboard_key_available_callback_t key_callback;

static uint8_t lcd_backlight = 100;
static uint8_t kbd_backlight = 0;

static void push(uint8_t state, uint8_t code)
{
    unsigned next = (q_head + 1) % SHIM_KEY_QUEUE_SIZE;

    if (next == q_tail)
        return;                                 /* FIFO full: drop, like the hardware */
    queue[q_head] = (uint16_t)((state << 8) | code);
    q_head = next;
}

uint16_t shim_key_pop(void)
{
    uint16_t e;

    if (q_tail == q_head)
        return 0;
    e = queue[q_tail];
    q_tail = (q_tail + 1) % SHIM_KEY_QUEUE_SIZE;
    return e;
}

/* US-layout shifted character for an unshifted one. */
static uint8_t shifted(uint8_t c)
{
    static const char from[] = "`1234567890-=[]\\;',./";
    static const char to[]   = "~!@#$%^&*()_+{}|:\"<>?";
    const char *p;

    if (c >= 'a' && c <= 'z')
        return (uint8_t)(c - 'a' + 'A');
    p = strchr(from, c);
    return (p && c) ? (uint8_t)to[p - from] : c;
}

static uint8_t map_key(const SDL_KeyboardEvent *k)
{
    SDL_Keycode sym = k->keysym.sym;

    switch (sym) {
    case SDLK_UP:        return KEY_UP;
    case SDLK_DOWN:      return KEY_DOWN;
    case SDLK_LEFT:      return KEY_LEFT;
    case SDLK_RIGHT:     return KEY_RIGHT;
    case SDLK_ESCAPE:    return KEY_ESC;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:  return KEY_ENTER;
    case SDLK_BACKSPACE: return KEY_BACKSPACE;
    case SDLK_TAB:       return KEY_TAB;
    case SDLK_DELETE:    return KEY_DEL;
    case SDLK_INSERT:    return KEY_INSERT;
    case SDLK_HOME:      return KEY_HOME;
    case SDLK_END:       return KEY_END;
    case SDLK_PAGEUP:    return KEY_PAGE_UP;
    case SDLK_PAGEDOWN:  return KEY_PAGE_DOWN;
    case SDLK_CAPSLOCK:  return KEY_CAPS_LOCK;
    case SDLK_PAUSE:     return KEY_BREAK;
    case SDLK_LSHIFT:    return KEY_MOD_SHL;
    case SDLK_RSHIFT:    return KEY_MOD_SHR;
    case SDLK_LCTRL:
    case SDLK_RCTRL:     return KEY_MOD_CTRL;
    case SDLK_LALT:
    case SDLK_RALT:      return KEY_MOD_ALT;
    case SDLK_LGUI:
    case SDLK_RGUI:      return KEY_MOD_SYM;
    /* Numeric keypad: the same characters as the main keyboard row. */
    case SDLK_KP_0:      return '0';
    case SDLK_KP_1:      return '1';
    case SDLK_KP_2:      return '2';
    case SDLK_KP_3:      return '3';
    case SDLK_KP_4:      return '4';
    case SDLK_KP_5:      return '5';
    case SDLK_KP_6:      return '6';
    case SDLK_KP_7:      return '7';
    case SDLK_KP_8:      return '8';
    case SDLK_KP_9:      return '9';
    case SDLK_KP_PERIOD: return '.';
    case SDLK_KP_PLUS:   return '+';
    case SDLK_KP_MINUS:  return '-';
    case SDLK_KP_MULTIPLY: return '*';
    case SDLK_KP_DIVIDE: return '/';
    case SDLK_KP_EQUALS: return '=';
    default:             break;
    }
    if (sym >= SDLK_F1 && sym <= SDLK_F9)
        return (uint8_t)(KEY_F1 + (sym - SDLK_F1));
    if (sym == SDLK_F10)
        return KEY_F10;
    if (sym >= 32 && sym <= 126) {
        uint8_t c = (uint8_t)sym;

        return (k->keysym.mod & KMOD_SHIFT) ? shifted(c) : c;
    }
    return 0;
}

void shim_input_event(const SDL_KeyboardEvent *k)
{
    SDL_Scancode sc = k->keysym.scancode;

    if (sc >= SDL_NUM_SCANCODES)
        return;
    if (k->type == SDL_KEYDOWN) {
        uint8_t code = map_key(k);

        if (!code)
            return;
        if (k->repeat) {
            push(KEY_STATE_HOLD, down_code[sc] ? down_code[sc] : code);
        } else {
            down_code[sc] = code;
            push(KEY_STATE_PRESSED, code);
        }
    } else if (down_code[sc]) {
        push(KEY_STATE_RELEASED, down_code[sc]);
        down_code[sc] = 0;
    }
}

/* ---- south bridge ---------------------------------------------------- */

void     sb_init(void)                       { shim_init(); }
bool     sb_available(void)                  { return true; }

uint16_t sb_read_keyboard(void)
{
    shim_pump();
    return shim_key_pop();
}

uint16_t sb_read_keyboard_state(void)        { return 0; }
uint8_t  sb_read_battery(void)               { return 100; }
uint8_t  sb_read_lcd_backlight(void)         { return lcd_backlight; }
uint8_t  sb_write_lcd_backlight(uint8_t b)   { lcd_backlight = b; return b; }
uint8_t  sb_read_keyboard_backlight(void)    { return kbd_backlight; }
uint8_t  sb_write_keyboard_backlight(uint8_t b) { kbd_backlight = b; return b; }
bool     sb_is_power_off_supported(void)     { return true; }

bool sb_write_power_off_delay(uint8_t delay_seconds)
{
    (void)delay_seconds;
    exit(0);
}

bool sb_reset(uint8_t delay_seconds)
{
    (void)delay_seconds;
    exit(0);
}

/* ---- keyboard.h: buffered character interface -------------------------- */

void keyboard_init(void)                     { shim_init(); }
void keyboard_set_key_available_callback(keyboard_key_available_callback_t cb) { key_callback = cb; }
void keyboard_set_background_poll(bool enable) { (void)enable; }

void keyboard_poll(void)
{
    uint16_t e;

    shim_pump();
    while ((e = shim_key_pop()) != 0) {
        uint8_t state = (uint8_t)(e >> 8);
        uint8_t code  = (uint8_t)e;
        unsigned next = (k_head + 1) % KBD_BUFFER_SIZE;

        if ((state != KEY_STATE_PRESSED && state != KEY_STATE_HOLD) ||
            (code >= KEY_MOD_ALT && code <= KEY_MOD_CTRL) || next == k_tail)
            continue;
        kbuf[k_head] = (char)code;
        k_head = next;
        if (key_callback)
            key_callback();
    }
}

bool keyboard_key_available(void)
{
    keyboard_poll();
    return k_head != k_tail;
}

char keyboard_get_key(void)
{
    char c;

    if (!keyboard_key_available())
        return 0;
    c = kbuf[k_tail];
    k_tail = (k_tail + 1) % KBD_BUFFER_SIZE;
    return c;
}
