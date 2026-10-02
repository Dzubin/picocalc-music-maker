/*
 * shim_lcd.c - the picocalc-text-starter LCD driver API (lcd.h, by Blair
 * Leduc) re-implemented on a 320x320 RGB565 framebuffer for the Windows
 * (SDL2) build. Same function names and behaviour as drivers/lcd.c, minus the
 * SPI plumbing; the vendored fonts (font-8x10.c / font-5x10.c) are used as-is.
 *
 * The hardware scrolls by rotating a window over display RAM; here we just
 * move the pixels, which looks the same to the program.
 *
 * Author: Thomas Dzubin
 */
#include <string.h>

#include "shim.h"

static uint16_t foreground = 0xFFFF;
static uint16_t background = 0x0000;
static bool     underscore;
static bool     reverse;
static bool     bold;
static bool     cursor_enabled = true;
static uint8_t  cursor_column;
static uint8_t  cursor_row;
static uint16_t scroll_top;
static uint16_t scroll_bottom;
static const font_t *font = &font_8x10;

/* ---- colour / state -------------------------------------------------- */

void lcd_set_reverse(bool reverse_on)
{
    if (reverse != reverse_on) {
        uint16_t t = foreground;
        foreground = background;
        background = t;
    }
    reverse = reverse_on;
}

void lcd_set_underscore(bool on)      { underscore = on; }
void lcd_set_bold(bool on)            { bold = on; }
void lcd_set_font(const font_t *f)    { font = f; }
uint8_t lcd_get_columns(void)         { return (uint8_t)(WIDTH / font->width); }
uint8_t lcd_get_glyph_width(void)     { return font->width; }

void lcd_set_foreground(uint16_t colour)
{
    if (reverse)
        background = colour;
    else
        foreground = colour;
}

void lcd_set_background(uint16_t colour)
{
    if (reverse)
        foreground = colour;
    else
        background = colour;
}

/* ---- low-level SPI calls: nothing to talk to on a PC ------------------ */

void lcd_reset(void)                                        { }
void lcd_display_on(void)                                   { }
void lcd_display_off(void)                                  { }
void lcd_write_cmd(uint8_t cmd)                             { (void)cmd; }
void lcd_write_data(uint8_t len, ...)                       { (void)len; }
void lcd_write16_data(uint8_t len, ...)                     { (void)len; }
void lcd_write16_buf(const uint16_t *buffer, size_t len)    { (void)buffer; (void)len; }

/* ---- drawing --------------------------------------------------------- */

void lcd_blit(const uint16_t *pixels, uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    for (uint16_t row = 0; row < height; row++) {
        if (y + row >= HEIGHT)
            break;
        uint16_t w = width;
        if (x >= WIDTH)
            break;
        if (x + w > WIDTH)
            w = (uint16_t)(WIDTH - x);
        memcpy(&shim_framebuffer[(y + row) * WIDTH + x], pixels + row * width,
               (size_t)w * sizeof(uint16_t));
    }
    shim_mark_dirty();
}

void lcd_solid_rectangle(uint16_t colour, uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    for (uint16_t row = 0; row < height && y + row < HEIGHT; row++)
        for (uint16_t col = 0; col < width && x + col < WIDTH; col++)
            shim_framebuffer[(y + row) * WIDTH + x + col] = colour;
    shim_mark_dirty();
}

/* ---- scrolling ------------------------------------------------------- */

void lcd_define_scrolling(uint16_t top_fixed_area, uint16_t bottom_fixed_area)
{
    if (top_fixed_area + bottom_fixed_area >= HEIGHT) {
        top_fixed_area = 0;
        bottom_fixed_area = 0;
    }
    scroll_top = top_fixed_area;
    scroll_bottom = bottom_fixed_area;
}

void lcd_scroll_reset(void)   { }

void lcd_scroll_clear(void)
{
    lcd_solid_rectangle(background, 0, scroll_top, WIDTH, (uint16_t)(HEIGHT - scroll_top - scroll_bottom));
}

void lcd_scroll_up(void)
{
    uint16_t area = (uint16_t)(HEIGHT - scroll_top - scroll_bottom);

    if (area <= GLYPH_HEIGHT)
        return;
    memmove(&shim_framebuffer[scroll_top * WIDTH],
            &shim_framebuffer[(scroll_top + GLYPH_HEIGHT) * WIDTH],
            (size_t)(area - GLYPH_HEIGHT) * WIDTH * sizeof(uint16_t));
    lcd_solid_rectangle(background, 0, (uint16_t)(scroll_top + area - GLYPH_HEIGHT), WIDTH, GLYPH_HEIGHT);
}

void lcd_scroll_down(void)
{
    uint16_t area = (uint16_t)(HEIGHT - scroll_top - scroll_bottom);

    if (area <= GLYPH_HEIGHT)
        return;
    memmove(&shim_framebuffer[(scroll_top + GLYPH_HEIGHT) * WIDTH],
            &shim_framebuffer[scroll_top * WIDTH],
            (size_t)(area - GLYPH_HEIGHT) * WIDTH * sizeof(uint16_t));
    lcd_solid_rectangle(background, 0, scroll_top, WIDTH, GLYPH_HEIGHT);
}

/* ---- text ------------------------------------------------------------ */

static void draw_glyph(uint16_t px, uint16_t py, uint8_t c)
{
    const uint8_t *glyph = &font->glyphs[c * GLYPH_HEIGHT];
    uint8_t msb = (font->width == 8) ? 0x80 : 0x10;

    for (uint8_t r = 0; r < GLYPH_HEIGHT; r++) {
        bool last_row = (r == GLYPH_HEIGHT - 1);

        for (uint8_t col = 0; col < font->width; col++) {
            bool on = (glyph[r] & (msb >> col)) != 0;

            if (bold && !last_row && col > 0 && (glyph[r] & (msb >> (col - 1))))
                on = true;
            if (last_row && underscore)
                on = true;
            if (py + r < HEIGHT && px + col < WIDTH)
                shim_framebuffer[(py + r) * WIDTH + px + col] = on ? foreground : background;
        }
    }
}

void lcd_putc(uint8_t column, uint8_t row, uint8_t c)
{
    draw_glyph((uint16_t)(column * font->width), (uint16_t)(row * GLYPH_HEIGHT), c);
    shim_mark_dirty();
}

void lcd_putstr(uint8_t column, uint8_t row, const char *str)
{
    uint16_t x = (uint16_t)(column * font->width);

    while (*str && x < WIDTH) {
        draw_glyph(x, (uint16_t)(row * GLYPH_HEIGHT), (uint8_t)*str++);
        x = (uint16_t)(x + font->width);
    }
    shim_mark_dirty();
}

void lcd_clear_screen(void)
{
    lcd_solid_rectangle(background, 0, 0, WIDTH, HEIGHT);
}

void lcd_erase_line(uint8_t row, uint8_t col_start, uint8_t col_end)
{
    lcd_solid_rectangle(background, (uint16_t)(col_start * font->width),
                        (uint16_t)(row * GLYPH_HEIGHT),
                        (uint16_t)((col_end - col_start + 1) * font->width), GLYPH_HEIGHT);
}

/* ---- cursor (drawn as an underline in the bottom pixel row) ---------- */

void lcd_enable_cursor(bool on)   { cursor_enabled = on; }
bool lcd_cursor_enabled(void)     { return cursor_enabled; }

void lcd_move_cursor(uint8_t column, uint8_t row)
{
    uint8_t max_col = (uint8_t)(lcd_get_columns() - 1);

    cursor_column = column > max_col ? max_col : column;
    cursor_row = row > MAX_ROW ? MAX_ROW : row;
}

void lcd_draw_cursor(void)
{
    if (cursor_enabled)
        lcd_solid_rectangle(foreground, (uint16_t)(cursor_column * font->width),
                            (uint16_t)((cursor_row + 1) * GLYPH_HEIGHT - 1), font->width, 1);
}

void lcd_erase_cursor(void)
{
    if (cursor_enabled)
        lcd_solid_rectangle(background, (uint16_t)(cursor_column * font->width),
                            (uint16_t)((cursor_row + 1) * GLYPH_HEIGHT - 1), font->width, 1);
}

void lcd_init(void)
{
    shim_init();
    lcd_clear_screen();
}
