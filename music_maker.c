/*
 *  PicoCalc Music Maker  (keyboard instrument)
 *  ===========================================
 *
 *  A tiny keyboard instrument for the ClockworkPi PicoCalc.
 *  Author: Thomas Dzubin.
 *
 *  Built on the Raspberry Pi Pico SDK and the drivers from
 *  "picocalc-text-starter" by Blair Leduc (drivers/lcd.c, drivers/audio.c,
 *  drivers/southbridge.c) plus its songs.c.  Those give us:
 *
 *      - lcd_init(), lcd_solid_rectangle(), lcd_putstr()  (ST7365P LCD)
 *      - audio_init(), audio_play_sound(), audio_stop()   (PIO PWM, GP26/GP27)
 *      - sb_init(), sb_read_keyboard()                    (I2C south-bridge kbd)
 *      - songs[] : ten built-in tunes (see load_builtin_songs)
 *
 *  We poll the keyboard FIFO ourselves with sb_read_keyboard() so we can see
 *  raw press / release events (a key sounds while it is held).  We therefore
 *  do NOT call picocalc_init() (which would start a background poll that
 *  drains that same FIFO).
 *
 *  ------------------------------------------------------------------------
 *  Controls
 *  ------------------------------------------------------------------------
 *    Splash screen ...... any key begins.  ESC asks "erase all recordings?"
 *                         and, on Y, reboots (recordings are RAM-only).  '~'
 *                         (SHIFT + backtick) reboots into BOOTSEL mode.
 *
 *    Then, a note sounds for as long as its key is held.  The centre of the
 *    screen is split into a left and a right half, one per speaker (headed
 *    "LEFT CHANNEL" / "RIGHT CHANNEL"); each shows the note name large with
 *    its frequency in Hz underneath and a thin divider between them.  A
 *    plain note is identical on both halves; a DTMF (or phone tone) key
 *    shows the same symbol on each side with its low tone on the left and
 *    its high tone on the right.
 *
 *      A S D F G H J .... natural notes  A B C D E F G
 *      K L ENTER ........ A B C one octave up  (so D F G H J K L ENTER
 *                         plays a full C-D-E-F-G-A-B-C scale)
 *      Q W E R T Y U .... sharp notes    A# B# C# D# E# F# G#
 *      I O P ............ A# B# C# one octave up
 *      Z X C V B N M .... flat notes     Ab Bb Cb Db Eb Fb Gb
 *      , . .............. Ab Bb one octave up  (no key for a high Cb)
 *      SHIFT + A..L ..... the SHARP of that key's natural note
 *                         (A S D F G H J -> A# .. G#; K L -> A# B#
 *                          one octave up)
 *      CTRL  + A..L ..... the FLAT of that key's natural note
 *                         (A S D F G H J -> Ab .. Gb; K L -> Ab Bb
 *                          one octave up)
 *      \  (backslash) ... switch the musical-note key layout between:
 *                         FULL    - everything above, and
 *                         A-G ONLY - only the A B C D E F G keys play
 *                                    (their own note); SHIFT = its sharp,
 *                                    CTRL = its flat; every other letter
 *                                    key and ENTER are silent.
 *                         DTMF and phone tones work in both; starts FULL.
 *      0..9  #  *   ..... DTMF telephone tones (fixed pitch)
 *      !  (SHIFT+1) ..... North American BUSY tone   - 480 Hz L / 620 Hz R
 *      @  (SHIFT+2) ..... North American RING tone   - 440 Hz L / 480 Hz R
 *      $  (SHIFT+4) ..... North American DIAL tone   - 350 Hz L / 440 Hz R
 *      UP arrow ......... shift every future musical note up one octave
 *                         (x2 per press: x2, x4, x8); DTMF is unaffected
 *      DOWN arrow ....... shift every future musical note down one octave
 *                         (one step only, to /2 - /4 and /8 would be
 *                         below the 100 Hz audio floor); DTMF unaffected
 *      ESC ............. leave the Music Maker screen and go back to the
 *                         splash screen (main()'s loop redraws it)
 *      ~  (tilde, ASCII 126) .. reboot into BOOTSEL (USB drive) mode.
 *                         Deliberately undocumented on screen: '~' is
 *                         SHIFT + backtick on the PicoCalc keyboard, so
 *                         it takes a two-key combo and is hard to hit by
 *                         accident.
 *
 *  Tuning.  The 21 musical-note pitches come from one fixed table,
 *  tone_freq[] - 12-tone equal temperament, A4 = 440, taken from the
 *  driver's PITCH_* macros.  The octave choice for A and B and the
 *  enharmonic spellings are explained on map_key().  DTMF tones are
 *  fixed and never use the table.
 *
 *  Note layers.  Every key -> note decision is made in map_key(): the
 *  plain letter rows, the dedicated sharp / flat rows and the
 *  SHIFT / CTRL modifier layer.  That modifier layer is a small table
 *  (see "SHIFT / CTRL note layer" in map_key) - each row is
 *  { key, its natural TONE_*, one-octave-up? } - and the sharp / flat
 *  is reached by a fixed offset into the tone_freq[] rows (naturals,
 *  then sharps, then flats).  Adding keys, or another modifier layer
 *  later, is a table edit rather than new switch cases.  The '\' key
 *  flips note_map between that FULL layout and a compact "A-G keys are
 *  notes A-G" layout (MAP_LETTERS in map_key).
 *
 *  Recorder.  Storage is recbuf[REC_SONGS] - 12 song rows, each a
 *  { char name[31]; int count; rec_entry_t entry[REC_LEN]; }: a 30-char
 *  name, the number of entries actually recorded, and a 200-entry take.
 *  '=' cycles the selected row cur_song 0..11 (its index + name show on
 *  the row-7 status line); record, undo, clear and F2 PLAY all act on
 *  recbuf[cur_song] and its own .count.  (Name editing and save / load
 *  are still to come.)  At startup load_builtin_songs() fills rows 2..11
 *  with the 10 songs from Blair Leduc's songs.c (rows 0 and 1 stay empty
 *  for the user).  Each entry is
 *      { uint16 lo, uint16 hi, uint16 dur_ms, char spn[4] }
 *  where lo/hi are the left/right speaker frequencies in Hz (equal for a
 *  plain note, the two distinct tones for a DTMF or phone-tone key), dur_ms
 *  is the hold time in milliseconds (1..65535 => up to ~65.5 s) and spn is
 *  the scientific pitch notation ("A#4", "C4", "Gb3", ... ; "" for a rest;
 *  "1"/"*"/"#" for DTMF; "!"/"@"/"$" for the phone tones).
 *      F1 .............. start / stop RECORD mode.  Pressing F1 again later
 *                         (without DEL) appends to the existing take.
 *      F2 .............. play the recording back with its stored durations;
 *                         each entry shows its SPN, frequency and length
 *                         (SS.HH seconds).  Any key stops it (~ reboots).
 *      F3 / F4 / F5 .... EDIT / SAVE / LOAD - not implemented yet; each
 *                         just flashes a red notice with a 1000 Hz beep
 *      DEL ............. clear the whole recording, after a
 *                         "DELETE RECORDING (Y/N)" prompt (only y/Y wipes)
 *      = .............. step cur_song to the next of the 12 song rows
 *                         (wraps 0..11); shown on the row-7 status line
 *      Back (backspace)  while recording, erase the previous entry
 *      SPACE ........... while recording, store a rest (lo = hi = 0); its
 *                         duration is how long SPACE is held
 *      ? .............. full-screen key reference; any key returns
 */

#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/bootrom.h"       /* reset_usb_boot() - BOOTSEL reboot ('~')    */
#include "hardware/watchdog.h"  /* watchdog_reboot() - plain reboot (ESC)     */

#include "lcd.h"          /* WIDTH, HEIGHT, GLYPH_HEIGHT, RGB(), lcd_*        */
#include "audio.h"        /* audio_*(), PITCH_* freqs, NOTE_* durations      */
#include "southbridge.h"  /* sb_init(), sb_read_keyboard()                   */
#include "keyboard.h"     /* KEY_* codes, KEY_STATE_* values                 */
#include "songs.h"        /* Blair Leduc's songs[] library (not modified)    */

/* audio.c's audio_play_song_blocking() references this; define it so the
 * link is clean even though we never use that function.                     */
volatile bool user_interrupt = false;

/* ===================================================================== */
/*  Compile-time configuration                                            */
/* ===================================================================== */

/* shown on the splash screen */
#define VERSION "V0.01B"

/* audio.pio only produces a tone for 100..2115 Hz (its upper bound was
 * raised from 2000 so the top C, C7 ~2093 Hz, can sound); outside that
 * range a shifted note is shown on screen but stays silent.                 */

/* net up/down-arrow presses allowed: one octave (factor of 2) per press
 * - see shift_octave.  Down stops at -1 (/2): at /4 and /8 every note is
 * below the audio driver's 100 Hz floor, so there is nothing to hear. */
#define OCT_MIN (-1)
#define OCT_MAX ( 3)

/* modifier-key bitmask for the SHIFT / CTRL note layer (see map_key) */
#define MOD_SHIFT (1u << 0)
#define MOD_CTRL  (1u << 1)

/* element count of a fixed-size array */
#define NELEMS(a) ((int)(sizeof (a) / sizeof (a)[0]))

/* colours (RGB565 via the driver's RGB() macro) */
#define COL_BG      RGB(  0,   0,   0)
#define COL_WHITE   RGB(235, 235, 235)
#define COL_RED     RGB(255,  90,  90)
#define COL_GREEN   RGB( 90, 235, 130)
#define COL_YELLOW  RGB(255, 225,  70)
#define COL_CYAN    RGB(120, 225, 255)
#define COL_GREY    RGB(200, 200, 200)

/* text rows use the built-in 8x10 font from Blair Leduc's
 * picocalc-text-starter: 40 columns, 32 rows */
#define TCOLS 40

/* the big centre-screen note read-out lives in this pixel band (moved
 * down 25px from its original y=90 to make room for the "LEFT CHANNEL" /
 * "RIGHT CHANNEL" headers at BAND_HDR_ROW), split into a left-channel
 * half [0,HALF_W) and a right-channel half; each half's frequency line
 * sits on text row BAND_FROW.  Text is row-quantised (10px), so the
 * header/frequency rows land on the nearest row below the 25px-lower
 * glyph rather than at an exact +25px offset.                            */
#define BAND_Y      115
#define BAND_H      140
#define HALF_W      (WIDTH / 2)
#define BAND_HDR_ROW 10
#define BAND_FROW    22

/* recorder storage: REC_SONGS song rows (see song_t), each holding a
 * 30-char name and up to REC_LEN entries.  '=' selects the row; real
 * per-song record / play is a future addition.                           */
#define REC_SONGS 12
#define REC_LEN   200

/* ===================================================================== */
/*  Shared types                                                          */
/* ===================================================================== */
typedef enum { A_NONE, A_NOTE } action_t;

typedef struct {
    action_t act;
    uint16_t left;      /* left-speaker  frequency                            */
    uint16_t right;     /* right-speaker frequency (== left for plain notes)  */
    bool     up8va;     /* octave-up key (K/L/ENTER, I/O/P, ',' '.') - freq   */
                        /* is already x2; flag lets make_spn bump the octave  */
    char     label[4];  /* what to show on screen                             */
} keymap_t;

/*
 *  The 21 playable pitches (7 naturals, 7 sharps, 7 flats) live in one
 *  fixed table, tone_freq[]; map_key() looks a pitch up by TONE_* index.
 *  Enharmonic keys (e.g. D# and Eb) share one pitch - there are no split
 *  accidentals.  The octave choice for A and B (QUIRK note on map_key) is
 *  baked into the table.  The values are 12-TET, A4 = 440, from the
 *  driver's PITCH_* macros.
 */
enum {
    TONE_A,  TONE_B,  TONE_C,  TONE_D,  TONE_E,  TONE_F,  TONE_G,
    TONE_AS, TONE_BS, TONE_CS, TONE_DS, TONE_ES, TONE_FS, TONE_GS,
    TONE_AB, TONE_BB, TONE_CB, TONE_DB, TONE_EB, TONE_FB, TONE_GB,
    NUM_TONES
};

/* the SHIFT / CTRL note layer reaches a sharp / flat by adding a fixed
 * offset to a natural slot, so the three rows must stay in this order */
_Static_assert(TONE_AS == TONE_A + 7 && TONE_AB == TONE_A + 14,
               "tone_freq[] must be naturals, then sharps, then flats (A..G)");

typedef struct {
    uint16_t lo;      /* left-speaker frequency  in Hz; 0/0 = silence / rest  */
    uint16_t hi;      /* right-speaker frequency in Hz; == lo for plain notes,
                       * the two DTMF tones for a DTMF key                    */
    uint16_t dur_ms;  /* duration in milliseconds, 1..65535 (~65.5 s max)     */
    char     spn[4];  /* scientific pitch notation, e.g. "A#4" (3 chars + NUL);
                       * "" for a rest, "1"/"*"/"#" for DTMF keys             */
} rec_entry_t;

/* ===================================================================== */
/*  Global data                                                           */
/* ===================================================================== */

/*
 *  A small 5x7 font, used only for the big centre-screen note read-out.
 *  One byte per column, bit 0 = top pixel row.
 */
static const uint8_t BIGFONT[96][5] = {
    [' ' - 32] = {0x00, 0x00, 0x00, 0x00, 0x00},
    ['!' - 32] = {0x00, 0x00, 0x5F, 0x00, 0x00},   /* busy-tone key         */
    ['#' - 32] = {0x14, 0x7F, 0x14, 0x7F, 0x14},
    ['$' - 32] = {0x24, 0x2A, 0x7F, 0x2A, 0x12},   /* dial-tone key         */
    ['*' - 32] = {0x2A, 0x1C, 0x7F, 0x1C, 0x2A},
    ['-' - 32] = {0x08, 0x08, 0x08, 0x08, 0x08},
    ['/' - 32] = {0x20, 0x10, 0x08, 0x04, 0x02},
    ['0' - 32] = {0x3E, 0x51, 0x49, 0x45, 0x3E},
    ['1' - 32] = {0x00, 0x42, 0x7F, 0x40, 0x00},
    ['2' - 32] = {0x42, 0x61, 0x51, 0x49, 0x46},
    ['3' - 32] = {0x21, 0x41, 0x45, 0x4B, 0x31},
    ['4' - 32] = {0x18, 0x14, 0x12, 0x7F, 0x10},
    ['5' - 32] = {0x27, 0x45, 0x45, 0x45, 0x39},
    ['6' - 32] = {0x3C, 0x4A, 0x49, 0x49, 0x30},
    ['7' - 32] = {0x01, 0x71, 0x09, 0x05, 0x03},
    ['8' - 32] = {0x36, 0x49, 0x49, 0x49, 0x36},
    ['9' - 32] = {0x06, 0x49, 0x49, 0x29, 0x1E},
    ['?' - 32] = {0x02, 0x01, 0x51, 0x09, 0x06},   /* unknown-pitch fallback */
    ['@' - 32] = {0x3E, 0x41, 0x5D, 0x55, 0x5E},   /* ring-tone key         */
    ['A' - 32] = {0x7E, 0x11, 0x11, 0x11, 0x7E},
    ['B' - 32] = {0x7F, 0x49, 0x49, 0x49, 0x36},
    ['C' - 32] = {0x3E, 0x41, 0x41, 0x41, 0x22},
    ['D' - 32] = {0x7F, 0x41, 0x41, 0x22, 0x1C},
    ['E' - 32] = {0x7F, 0x49, 0x49, 0x49, 0x41},
    ['F' - 32] = {0x7F, 0x09, 0x09, 0x09, 0x01},
    ['G' - 32] = {0x3E, 0x41, 0x49, 0x49, 0x7A},
    ['b' - 32] = {0x7F, 0x48, 0x48, 0x48, 0x30},   /* lower-case b = flat */
};

/*
 *  Pitch (Hz) for each TONE_* slot - 12-tone equal temperament, A4 = 440.
 *  Rows, in order:
 *      A   B   C   D   E   F   G
 *      A#  B#  C#  D#  E#  F#  G#
 *      Ab  Bb  Cb  Db  Eb  Fb  Gb
 */
static const uint16_t tone_freq[NUM_TONES] = {
    PITCH_A3,  PITCH_B3,  PITCH_C4,  PITCH_D4,  PITCH_E4,  PITCH_F4,  PITCH_G4,
    PITCH_AS3, PITCH_C4,  PITCH_CS4, PITCH_DS4, PITCH_F4,  PITCH_FS4, PITCH_GS4,
    PITCH_GS3, PITCH_AS3, PITCH_B3,  PITCH_CS4, PITCH_DS4, PITCH_E4,  PITCH_FS4,
};

/* one recording row = a named song plus its take */
typedef struct {
    char        name[31];         /* song name: 30 chars + NUL ("" = unnamed;
                                   * no name-editing UI yet, so always "")   */
    int         count;            /* entries actually recorded, 0..REC_LEN;
                                   * F2 PLAY plays exactly this many          */
    rec_entry_t entry[REC_LEN];   /* the take - up to REC_LEN notes / rests  */
} song_t;

static song_t recbuf[REC_SONGS];    /* the 12 song rows                     */
static int    cur_song = 0;         /* selected row; '=' cycles it 0..11    */

/*
 *  Which musical-note key layout is live; the '\' key toggles it (see
 *  map_key).  MAP_FULL is the original spread-across-the-keyboard layout;
 *  MAP_LETTERS makes only the A B C D E F G keys play (their own note,
 *  SHIFT = sharp, CTRL = flat) and silences every other letter key.  The
 *  choice persists until a reboot; it always starts on MAP_FULL.
 */
enum { MAP_FULL, MAP_LETTERS };
static int note_map = MAP_FULL;

/* ===================================================================== */
/*  Function prototypes                                                   */
/* ===================================================================== */

/* drawing helpers */
static void fill(uint16_t colour, int x, int y, int w, int h);
static void big_char(int x, int y, char c, int s, uint16_t fg);
static void clear_row(int row);
static void put_row(int row, const char *str, uint16_t fg);
static void put_at(int x_px, int row, const char *str, uint16_t fg);
static void big_text(int x_px, int y_px, const char *s, int scale, uint16_t fg);
static void big_text_centered(int y_px, const char *s, int scale, uint16_t fg);
static void clear_band(void);
static void draw_half(int x0, const char *label, uint32_t freq, int frow);
static void draw_band_split(const char *labelL, uint32_t freqL,
                            const char *labelR, uint32_t freqR, int frow);
static void show_note(const char *label, uint32_t lo, uint32_t hi);

/* keyboard (raw south-bridge FIFO polling) */
static inline uint16_t key_event(void);
static inline uint8_t  ev_state(uint16_t e);
static inline uint8_t  ev_code(uint16_t e);
static void go_bootsel(void);
static void go_reboot(void);
static void drain_keys(void);
static bool confirm_reboot(void);
static void wait_any_key(void);
static uint8_t wait_key(void);

/* key -> note mapping */
static void set_tone(keymap_t *m, int tone, const char *label);
static void set_tone_8va(keymap_t *m, int tone, const char *label);
static bool map_key(uint8_t code, uint8_t mods, keymap_t *m);
static inline bool is_dtmf(const keymap_t *m);
static void make_spn(const char *label, bool up, char out[4]);

/* octave shift + status line */
static uint32_t shift_octave(uint32_t f, int oct);
static void show_octave(int oct);

/* recorder */
static uint32_t nowms(void);
static uint16_t clamp_ms(uint32_t ms);
static void rec_push(uint16_t lo, uint16_t hi, uint16_t dur_ms, const char *spn);
static void rec_pop(void);
static void show_recstat(bool recording);
static void show_reckeys(bool recording);
static void show_song(void);
static void show_play(int idx, int total, const rec_entry_t *e);
static void play_recording(void);

/* built-in songs (from Blair Leduc's songs.c) */
static const char *freq_to_spn(uint16_t hz);
static void load_builtin_songs(void);

/* music maker screen */
static void splash_screen(void);
static void draw_static_text(void);
static void show_notemap(void);
static void draw_screen(int oct, bool recording);
static void not_implemented(const char *what);
static void confirm_and_clear(bool *recording);
static void show_help(void);

/* ===================================================================== */
/*  Splash screen                                                         */
/* ===================================================================== */
static void splash_screen(void)
{
    lcd_clear_screen();
    big_text_centered( 40, "PICOCALC MUSIC MAKER", 2, COL_CYAN);
    put_row( 7, VERSION,                                COL_CYAN);
    big_text_centered(110, "BY THOMAS DZUBIN",          2, COL_GREY);
    put_row(14, "Built heavily on code by Blair Leduc", COL_GREY);
    put_row(15, "(picocalc-text-starter)",              COL_GREY);
    put_row(20, "PRESS ANY KEY TO START",               COL_WHITE);
    put_row(21, "or ESC to reboot (erases recordings)", COL_GREY);
}

/* ===================================================================== */
/*  main                                                                  */
/*                                                                        */
/*  Sets up the hardware once, then loops forever: draw the splash, wait  */
/*  for a key, then run the Music Maker main loop screen inline.  ESC in  */
/*  that inner loop breaks back out here and the splash is redrawn.       */
/* ===================================================================== */
int main(void)
{
    stdio_init_all();

    sb_init();               /* keyboard / south-bridge I2C (no bg poll) */
    lcd_init();               /* ST7365P LCD */
    lcd_enable_cursor(false); /* we don't want the blinking text cursor */
    audio_init();             /* PIO PWM audio on GP26 / GP27 */

    load_builtin_songs();     /* fill recorder rows 2..11 from songs.c */

    for (;;) {
        splash_screen();
        wait_any_key();

        /* ------------------------------------------------------------- */
        /*  Music Maker main loop screen                                 */
        /* ------------------------------------------------------------- */
        int      oct       = 0;   /* net up-presses: + multiplies, - divides pitch */
        uint8_t  sounding   = 0;  /* key code of the note / rest in progress, 0=none */
        uint32_t press_ms   = 0;  /* when the in-progress key went down */
        uint16_t press_lo   = 0;  /* left  freq to record for it (0 for a rest) */
        uint16_t press_hi   = 0;  /* right freq to record for it (0 for a rest) */
        char     press_spn[4] = "";  /* SPN to record for it */
        bool     recording  = false;
        uint8_t  mods       = 0;  /* SHIFT / CTRL held, for the note layer */

        draw_screen(oct, recording);

        for (;;) {
            uint16_t e = key_event();
            if (!e) {
                sleep_ms(2);
                continue;
            }

            uint8_t code = ev_code(e);
            uint8_t st   = ev_state(e);

            /* '~' (ASCII 126 = SHIFT + backtick) reboots to BOOTSEL.  This is
             * intentionally not shown anywhere on screen; the two-key combo
             * keeps it from being pressed by accident.                       */
            if (code == '~' && st == KEY_STATE_PRESSED)
                go_bootsel();

            /* ESC leaves this screen; the outer loop then shows the splash */
            if (code == KEY_ESC && st == KEY_STATE_PRESSED) {
                audio_stop();
                break;
            }

            /* ---- track SHIFT / CTRL for the note layer (see map_key) ---- */
            if (code == KEY_MOD_SHL || code == KEY_MOD_SHR) {
                if (st == KEY_STATE_PRESSED)  mods |=  MOD_SHIFT;
                if (st == KEY_STATE_RELEASED) mods &= ~MOD_SHIFT;
                continue;
            }
            if (code == KEY_MOD_CTRL) {
                if (st == KEY_STATE_PRESSED)  mods |=  MOD_CTRL;
                if (st == KEY_STATE_RELEASED) mods &= ~MOD_CTRL;
                continue;
            }

            /* ---- transport / control keys (act on press) ---- */
            if (st == KEY_STATE_PRESSED) {
                if (code == KEY_F1) {                    /* toggle RECORD */
                    recording = !recording;
                    show_recstat(recording);
                    show_reckeys(recording);   /* light up / dim the edit keys */
                    continue;
                }
                if (code == KEY_F2) {                    /* PLAY back */
                    recording = false;
                    play_recording();
                    mods = 0;               /* playback ate any mod-release event */
                    show_recstat(recording);
                    show_reckeys(recording);
                    continue;
                }
                if (code == KEY_F3) {                    /* EDIT - not done yet */
                    not_implemented("EDIT");
                    mods = 0;               /* the notice's drain ate mod releases */
                    continue;
                }
                if (code == KEY_F4) {                    /* SAVE - not done yet */
                    not_implemented("SAVE");
                    mods = 0;
                    continue;
                }
                if (code == KEY_F5) {                    /* LOAD - not done yet */
                    not_implemented("LOAD");
                    mods = 0;
                    continue;
                }
                if (code == KEY_DEL) {                   /* CLEAR (asks Y/N first) */
                    confirm_and_clear(&recording);
                    mods = 0;
                    continue;
                }
                if (code == '?') {                       /* help screen (SHIFT + /) */
                    show_help();
                    draw_screen(oct, recording);
                    mods = 0;               /* help's key-wait ate mod releases */
                    continue;
                }
                if (code == '=') {                       /* cycle selected song 0..11 */
                    cur_song = (cur_song + 1) % REC_SONGS;
                    show_song();
                    show_recstat(recording);   /* note count is per-song now */
                    continue;
                }
                if (code == '\\') {                      /* toggle note-key layout */
                    note_map = (note_map == MAP_FULL) ? MAP_LETTERS : MAP_FULL;
                    show_notemap();
                    continue;
                }
                if (code == KEY_BACKSPACE) {             /* undo last entry */
                    if (recording) {
                        rec_pop();
                        show_recstat(recording);
                    }
                    continue;
                }
                if (code == KEY_UP || code == KEY_DOWN) {/* octave shift (future notes) */
                    if (code == KEY_UP   && oct < OCT_MAX) oct++;
                    if (code == KEY_DOWN && oct > OCT_MIN) oct--;
                    show_octave(oct);
                    continue;
                }
            }

            /* ---- SPACE = rest, only while recording ---- */
            if (code == KEY_SPACE) {
                if (st == KEY_STATE_PRESSED && sounding == 0 && recording) {
                    sounding   = KEY_SPACE;
                    press_ms   = nowms();
                    press_lo   = 0;
                    press_hi   = 0;
                    press_spn[0] = '\0';
                    show_note("-", 0, 0);
                } else if (st == KEY_STATE_RELEASED && sounding == KEY_SPACE) {
                    if (recording) {               /* skip if REC stopped mid-hold */
                        rec_push(0, 0, clamp_ms(nowms() - press_ms), "");
                        show_recstat(recording);
                    }
                    clear_band();
                    sounding = 0;
                }
                continue;
            }

            /* ---- notes ---- */
            if (st == KEY_STATE_PRESSED) {
                if (sounding != 0)                       /* one key at a time */
                    continue;

                keymap_t m;
                if (!map_key(code, mods, &m) || m.act != A_NOTE)
                    continue;

                uint32_t lo = m.left, hi = m.right;
                if (!is_dtmf(&m)) {                      /* arrows don't touch DTMF */
                    lo = shift_octave(lo, oct);
                    hi = shift_octave(hi, oct);
                }

                audio_play_sound(lo, hi);
                show_note(m.label, lo, hi);
                sounding = code;
                press_ms = nowms();
                press_lo = (uint16_t)lo;                /* both channels: equal for a */
                press_hi = (uint16_t)hi;                /* plain note, the DTMF pair  */
                make_spn(m.label, m.up8va, press_spn);  /* otherwise                  */
            } else if (st == KEY_STATE_RELEASED) {
                if (code == sounding) {
                    audio_stop();
                    if (recording) {
                        rec_push(press_lo, press_hi,
                                 clamp_ms(nowms() - press_ms), press_spn);
                        show_recstat(recording);
                    }
                    clear_band();
                    sounding = 0;
                }
            }
        }
    }
}

/* ===================================================================== */
/*  Drawing helpers                                                       */
/* ===================================================================== */

/* clipped solid rectangle */
static void fill(uint16_t colour, int x, int y, int w, int h)
{
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > WIDTH)  w = WIDTH  - x;
    if (y + h > HEIGHT) h = HEIGHT - y;
    if (w > 0 && h > 0)
        lcd_solid_rectangle(colour, (uint16_t)x, (uint16_t)y, (uint16_t)w, (uint16_t)h);
}

/* one big glyph, top-left at (x,y), each source pixel drawn as an s x s block */
static void big_char(int x, int y, char c, int s, uint16_t fg)
{
    if (c >= 'a' && c <= 'z' && c != 'b')
        c -= 32;

    const uint8_t *g = BIGFONT[0];
    if ((uint8_t)c >= 32 && (uint8_t)c < 128)
        g = BIGFONT[(uint8_t)c - 32];

    for (int col = 0; col < 5; col++)
        for (int row = 0; row < 7; row++)
            if (g[col] & (1u << row))
                fill(fg, x + col * s, y + row * s, s, s);
}

static void clear_row(int row)
{
    lcd_solid_rectangle(COL_BG, 0, (uint16_t)(row * GLYPH_HEIGHT), WIDTH, GLYPH_HEIGHT);
}

/* clear a row and print a string centred on it */
static void put_row(int row, const char *str, uint16_t fg)
{
    clear_row(row);
    int len = (int)strlen(str);
    int col = (TCOLS - len) / 2;
    if (col < 0) col = 0;
    lcd_set_foreground(fg);
    lcd_putstr((uint8_t)col, (uint8_t)row, str);
}

/* print a string at a pixel-x / text-row position (left aligned) */
static void put_at(int x_px, int row, const char *str, uint16_t fg)
{
    lcd_set_foreground(fg);
    lcd_putstr((uint8_t)(x_px / 8), (uint8_t)row, str);
}

/*
 *  Draw a line of text in the built-in 8x10 font scaled up `scale` times
 *  (so scale == 2 is "twice as big"), left edge at x_px, top at y_px.
 *  Each set glyph pixel becomes a scale x scale block.  Used for the
 *  double-size splash-screen lines; regular text still uses put_row.
 */
static void big_text(int x_px, int y_px, const char *s, int scale, uint16_t fg)
{
    for (; *s; s++, x_px += 8 * scale) {
        const uint8_t *g = &font_8x10.glyphs[(uint8_t)*s * GLYPH_HEIGHT];
        for (int row = 0; row < GLYPH_HEIGHT; row++)
            for (int col = 0; col < 8; col++)
                if (g[row] & (0x80u >> col))
                    fill(fg, x_px + col * scale, y_px + row * scale, scale, scale);
    }
}

/* big_text, horizontally centred on the screen */
static void big_text_centered(int y_px, const char *s, int scale, uint16_t fg)
{
    int w = (int)strlen(s) * 8 * scale;
    big_text((WIDTH - w) / 2, y_px, s, scale, fg);
}

/* ===================================================================== */
/*  Big centre-screen note read-out                                       */
/* ===================================================================== */

static void clear_band(void)
{
    fill(COL_BG, 0, BAND_Y, WIDTH, BAND_H);
}

/*
 *  Draw one channel's read-out inside the note band: the note LABEL big,
 *  centred in the half that starts at x0 (width HALF_W), with FREQ in Hz
 *  centred on text row `frow` just below it.  freq == 0 => label only
 *  (a rest).  The caller clears the band first.
 */
static void draw_half(int x0, const char *label, uint32_t freq, int frow)
{
    int len = (int)strlen(label);
    if (len < 1)
        return;
    if (len > 3)
        len = 3;                              /* labels are 1..3 chars       */

    int s  = (len == 3) ? 8 : (len == 2) ? 10 : 12;   /* fit half + Hz line */
    int gw = s * (6 * len - 1);
    int gx = x0 + (HALF_W - gw) / 2;
    int gy = BAND_Y + 6;

    for (int i = 0; i < len; i++)
        big_char(gx + i * 6 * s, gy, label[i], s, COL_YELLOW);

    if (freq == 0)
        return;

    char buf[16];
    int  n   = snprintf(buf, sizeof buf, "%lu Hz", (unsigned long)freq);
    int  c0  = x0 / 8;
    int  col = c0 + (HALF_W / 8 - n) / 2;
    if (col < c0)
        col = c0;
    put_at(col * 8, frow, buf, COL_CYAN);
}

/*
 *  Clear the band and draw both speakers side by side with a thin grey
 *  divider: left channel in [0,HALF_W), right channel in the other half.
 *  labelL / labelR are the big note names (normally the same string);
 *  freqL / freqR the per-speaker Hz (0 = silence).  Both Hz lines land on
 *  text row `frow`; the divider runs from the top of the band to the
 *  bottom of that row.
 */
static void draw_band_split(const char *labelL, uint32_t freqL,
                            const char *labelR, uint32_t freqR, int frow)
{
    clear_band();
    fill(COL_GREY, HALF_W - 1, BAND_Y, 2, (frow + 1) * GLYPH_HEIGHT - BAND_Y);
    draw_half(0,      labelL, freqL, frow);
    draw_half(HALF_W, labelR, freqR, frow);
}

/*
 *  Live read-out while a key is held: the note on each speaker drawn big
 *  with its frequency underneath - left channel on the left half, right
 *  channel on the right.  A plain note is identical on both halves; a
 *  DTMF key shows its digit on both with the two different tones.
 *  lo == hi == 0 is a rest ("-", no Hz).
 */
static void show_note(const char *label, uint32_t lo, uint32_t hi)
{
    draw_band_split(label, lo, label, hi, BAND_FROW);
}

/* ===================================================================== */
/*  Keyboard (raw south-bridge FIFO polling)                              */
/* ===================================================================== */
static inline uint16_t key_event(void)        { return sb_read_keyboard(); }
static inline uint8_t  ev_state(uint16_t e)   { return (e >> 8) & 0xFF; }
static inline uint8_t  ev_code(uint16_t e)    { return e & 0xFF; }

/* true for a bare SHIFT / CTRL / ALT / SYM modifier key code */
static inline bool is_mod_key(uint8_t c)
{
    return c == KEY_MOD_SHL || c == KEY_MOD_SHR || c == KEY_MOD_CTRL ||
           c == KEY_MOD_ALT || c == KEY_MOD_SYM;
}

static void go_bootsel(void)
{
    audio_stop();
    put_row(15, "REBOOTING TO BOOTSEL...", COL_YELLOW);
    sleep_ms(300);
    reset_usb_boot(0, 0);
    while (1)
        tight_loop_contents();
}

/* plain reboot back into this program (ESC on the splash screen) */
static void go_reboot(void)
{
    audio_stop();
    put_row(15, "REBOOTING...", COL_YELLOW);
    sleep_ms(300);
    watchdog_reboot(0, 0, 0);      /* pc = 0 -> normal flash boot */
    while (1)
        tight_loop_contents();
}

static void drain_keys(void)
{
    while (key_event() != 0)
        tight_loop_contents();
}

/*
 *  Splash-screen ESC asks before it reboots: the recordings only live in
 *  RAM, so a reboot wipes every song row.  Returns true only for Y / y.
 *  (wait_key() ignores bare modifiers and still sends '~' to BOOTSEL.)
 */
static bool confirm_reboot(void)
{
    put_row(20, "REBOOT AND ERASE ALL RECORDINGS?", COL_YELLOW);
    put_row(21, "PRESS Y TO REBOOT - ANY KEY STAYS", COL_WHITE);
    uint8_t k = wait_key();
    return k == 'y' || k == 'Y';
}

static void wait_any_key(void)
{
    drain_keys();
    for (;;) {
        uint16_t e = key_event();
        if (e && ev_state(e) == KEY_STATE_PRESSED) {
            uint8_t c = ev_code(e);

            if (is_mod_key(c))           /* bare SHIFT / CTRL / etc: not "a key" */
                continue;

            /* '~' (SHIFT + backtick) reboots into BOOTSEL (USB drive) mode */
            if (c == '~')
                go_bootsel();            /* does not return */

            /* ESC reboots the PicoCalc - but confirm first, it wipes songs */
            if (c == KEY_ESC) {
                if (confirm_reboot())
                    go_reboot();         /* does not return */
                splash_screen();         /* user stayed - repaint the splash */
                drain_keys();
                continue;
            }

            /* any other key starts the Music Maker screen */
            drain_keys();
            return;
        }
        sleep_ms(3);
    }
}

/*
 *  Block until a real (non-modifier) key is pressed and return its code.
 *  Bare SHIFT / CTRL / ALT / SYM presses are ignored so they don't count
 *  as the answer to a prompt; '~' reboots to BOOTSEL instead of returning.
 *  Used by the DEL confirm prompt and the '?' help screen.
 */
static uint8_t wait_key(void)
{
    drain_keys();
    for (;;) {
        uint16_t e = key_event();
        if (e && ev_state(e) == KEY_STATE_PRESSED) {
            uint8_t c = ev_code(e);
            if (c == '~')
                go_bootsel();                   /* does not return */
            if (!is_mod_key(c)) {
                drain_keys();
                return c;
            }
        }
        sleep_ms(3);
    }
}

/* ===================================================================== */
/*  Key -> note mapping                                                   */
/* ===================================================================== */

/* fill m with a musical note: its pitch from the tone_freq[] table */
static void set_tone(keymap_t *m, int tone, const char *label)
{
    m->act   = A_NOTE;
    m->left  = m->right = tone_freq[tone];
    strcpy(m->label, label);
}

/*
 *  Like set_tone, but exactly one octave higher (x2).  Used for the extra
 *  keys that repeat A/B/C, A#/B#/C# and Ab/Bb one octave up:
 *      K L ENTER   ,  I O P  ,  ',' '.'
 *  The up/down-arrow octave shift still applies on top.
 */
static void set_tone_8va(keymap_t *m, int tone, const char *label)
{
    set_tone(m, tone, label);
    m->left  = (uint16_t)(m->left  * 2);
    m->right = (uint16_t)(m->right * 2);
    m->up8va = true;
}

/*
 *  Map a raw key code to a note / action.
 *
 *  QUIRK - octave choice for A and B:
 *  The picocalc-text-starter audio driver (by Blair Leduc) names pitches
 *  with *scientific pitch notation*, in which the octave number rolls over
 *  at C, not at A.
 *  So within one octave number the order is  C < D < E < F < G < A < B,
 *  i.e. PITCH_A4 (440) and PITCH_B4 (494) are actually *higher* than
 *  PITCH_C4 (262).
 *
 *  We want the row  A S D F G H J  to sound the run "A B C D E F G" as a
 *  strictly ascending scale.  To get that, A and B (and their accidentals
 *  A#, Ab, B#, Bb, and Cb, which is enharmonic with B) are taken from
 *  octave *3*, while C..G and their accidentals stay in octave 4:
 *
 *      A3 220  B3 247  C4 262  D4 294  E4 330  F4 349  G4 392   -> always rising
 *
 *  B# is enharmonic with C, so with B in octave 3 it maps to C4 (not C5).
 *  The up/down-arrow octave shift multiplies/divides these values, so the
 *  scale stays in order at every shift.
 *
 *  The actual pitch numbers live in the tone_freq[] table (see the
 *  "Tuning" section above); each key just names a TONE_* slot and its
 *  screen label.  DTMF keys carry their two fixed tones directly and
 *  ignore the table.
 *
 *  `mods` is the SHIFT / CTRL bitmask (MOD_SHIFT / MOD_CTRL) captured by
 *  main()'s key loop.  See the "SHIFT / CTRL note layer" block below.
 *
 *  Two note-key layouts, toggled by the '\' key (see note_map):
 *    MAP_FULL    - the layout described above (letters spread across the
 *                  keyboard, dedicated sharp / flat rows, octave-up keys).
 *    MAP_LETTERS - only A B C D E F G play, each its own note; SHIFT = its
 *                  sharp, CTRL = its flat.  Every other letter key and
 *                  ENTER are silent.  DTMF digits and the ! @ $ phone
 *                  tones still work, the same as in MAP_FULL.
 */
static bool map_key(uint8_t code, uint8_t mods, keymap_t *m)
{
    memset(m, 0, sizeof *m);

    uint8_t c = code;
    if (c >= 'A' && c <= 'Z')
        c += 0x20;                       /* normalise letters to lower case */

    /* ---- MAP_LETTERS: A-G keys = notes A-G --------------------------- */
    if (note_map == MAP_LETTERS) {
        if (c >= 'a' && c <= 'g') {
            /* nat[] is in keycap order A..G; TONE_A..TONE_G are 0..6 and
             * +7 / +14 reach the sharp / flat rows (see _Static_assert).  */
            static const uint8_t nat[7] = {
                TONE_A, TONE_B, TONE_C, TONE_D, TONE_E, TONE_F, TONE_G,
            };
            bool sharp = (mods & MOD_SHIFT) != 0;
            bool flat  = (mods & MOD_CTRL)  != 0;   /* SHIFT wins if both */
            int  tone  = nat[c - 'a'] + (sharp ? 7 : flat ? 14 : 0);
            char lbl[4];
            lbl[0] = (char)('A' + (c - 'a'));
            lbl[1] = sharp ? '#' : flat ? 'b' : '\0';
            lbl[2] = '\0';
            set_tone(m, tone, lbl);
            return true;
        }
        /* h j k -> A B C one octave up.  Undocumented on purpose (not in
         * the help screen): it just lets DO-RE-MI be played straight
         * through without reaching for the octave arrows.               */
        if (c == 'h' || c == 'j' || c == 'k') {
            int  slot = (c == 'h') ? TONE_A : (c == 'j') ? TONE_B : TONE_C;
            set_tone_8va(m, slot, (c == 'h') ? "A" : (c == 'j') ? "B" : "C");
            return true;
        }
        /* in this layout no other letter key, ',' '.' or ENTER plays;
         * digits and phone-tone symbols fall through to the switch      */
        if ((c >= 'a' && c <= 'z') ||
            c == ',' || c == '.' || c == KEY_ENTER || c == KEY_RETURN)
            return false;
    }

    /* ---- SHIFT / CTRL note layer (MAP_FULL only) ---------------------
     * SHIFT + a home key plays the SHARP of that key's natural note,
     * CTRL + it plays the FLAT.  Table-driven: each row is a key, the
     * natural TONE_* it carries and whether it sits one octave up (the
     * K / L keys).  The sharp / flat is a fixed offset into the
     * tone_freq[] rows (naturals, then +7 sharps, then +14 flats), so
     * adding keys here - or another modifier layer later - needs no new
     * switch cases.  SHIFT wins if both modifiers are held.  Keys not in
     * the table ignore the modifiers and fall through to the switch.    */
    if (c >= 'a' && c <= 'z' && (mods & (MOD_SHIFT | MOD_CTRL))) {
        static const struct { uint8_t key; uint8_t nat; bool up8va; } layer[] = {
            { 'a', TONE_A, false }, { 's', TONE_B, false }, { 'd', TONE_C, false },
            { 'f', TONE_D, false }, { 'g', TONE_E, false }, { 'h', TONE_F, false },
            { 'j', TONE_G, false }, { 'k', TONE_A, true  }, { 'l', TONE_B, true  },
        };
        for (int i = 0; i < NELEMS(layer); i++) {
            if (layer[i].key != c)
                continue;
            bool sharp = (mods & MOD_SHIFT) != 0;
            int  tone  = layer[i].nat + (sharp ? 7 : 14);
            char lbl[4];
            lbl[0] = "ABCDEFG"[layer[i].nat - TONE_A];
            lbl[1] = sharp ? '#' : 'b';
            lbl[2] = '\0';
            if (layer[i].up8va) set_tone_8va(m, tone, lbl);
            else                set_tone(m, tone, lbl);
            return true;
        }
    }

    switch (c) {
    /* naturals: A S D F G H J -> A B C D E F G  (kept strictly ascending:
     * A and B sit in octave 3 so the run A B C D E F G always rises)        */
    case 'a': set_tone(m, TONE_A, "A"); return true;
    case 's': set_tone(m, TONE_B, "B"); return true;
    case 'd': set_tone(m, TONE_C, "C"); return true;
    case 'f': set_tone(m, TONE_D, "D"); return true;
    case 'g': set_tone(m, TONE_E, "E"); return true;
    case 'h': set_tone(m, TONE_F, "F"); return true;
    case 'j': set_tone(m, TONE_G, "G"); return true;

    /* K L ENTER -> A B C one octave up, so D F G H J K L ENTER is a full
     * C D E F G A B C scale without touching the octave arrows             */
    case 'k':        set_tone_8va(m, TONE_A, "A"); return true;
    case 'l':        set_tone_8va(m, TONE_B, "B"); return true;
    case KEY_ENTER:
    case KEY_RETURN: set_tone_8va(m, TONE_C, "C"); return true;

    /* sharps: Q W E R T Y U -> A# B# C# D# E# F# G#  (A#, B# in octave 3) */
    case 'q': set_tone(m, TONE_AS, "A#"); return true;
    case 'w': set_tone(m, TONE_BS, "B#"); return true;
    case 'e': set_tone(m, TONE_CS, "C#"); return true;
    case 'r': set_tone(m, TONE_DS, "D#"); return true;
    case 't': set_tone(m, TONE_ES, "E#"); return true;
    case 'y': set_tone(m, TONE_FS, "F#"); return true;
    case 'u': set_tone(m, TONE_GS, "G#"); return true;

    /* I O P -> A# B# C# one octave up */
    case 'i': set_tone_8va(m, TONE_AS, "A#"); return true;
    case 'o': set_tone_8va(m, TONE_BS, "B#"); return true;
    case 'p': set_tone_8va(m, TONE_CS, "C#"); return true;

    /* flats: Z X C V B N M -> Ab Bb Cb Db Eb Fb Gb  (Ab, Bb, Cb in octave 3) */
    case 'z': set_tone(m, TONE_AB, "Ab"); return true;
    case 'x': set_tone(m, TONE_BB, "Bb"); return true;
    case 'c': set_tone(m, TONE_CB, "Cb"); return true;
    case 'v': set_tone(m, TONE_DB, "Db"); return true;
    case 'b': set_tone(m, TONE_EB, "Eb"); return true;
    case 'n': set_tone(m, TONE_FB, "Fb"); return true;
    case 'm': set_tone(m, TONE_GB, "Gb"); return true;

    /* , . -> Ab Bb one octave up  (Gb stays on M; there is no key for a
     * one-octave-up Cb)                                                    */
    case ',': set_tone_8va(m, TONE_AB, "Ab"); return true;
    case '.': set_tone_8va(m, TONE_BB, "Bb"); return true;

    /* DTMF telephone tones (low tone on left speaker, high tone on right) */
    case '1': m->act = A_NOTE; m->left = 697; m->right = 1209; strcpy(m->label, "1"); return true;
    case '2': m->act = A_NOTE; m->left = 697; m->right = 1336; strcpy(m->label, "2"); return true;
    case '3': m->act = A_NOTE; m->left = 697; m->right = 1477; strcpy(m->label, "3"); return true;
    case '4': m->act = A_NOTE; m->left = 770; m->right = 1209; strcpy(m->label, "4"); return true;
    case '5': m->act = A_NOTE; m->left = 770; m->right = 1336; strcpy(m->label, "5"); return true;
    case '6': m->act = A_NOTE; m->left = 770; m->right = 1477; strcpy(m->label, "6"); return true;
    case '7': m->act = A_NOTE; m->left = 852; m->right = 1209; strcpy(m->label, "7"); return true;
    case '8': m->act = A_NOTE; m->left = 852; m->right = 1336; strcpy(m->label, "8"); return true;
    case '9': m->act = A_NOTE; m->left = 852; m->right = 1477; strcpy(m->label, "9"); return true;
    case '0': m->act = A_NOTE; m->left = 941; m->right = 1336; strcpy(m->label, "0"); return true;
    case '*': m->act = A_NOTE; m->left = 941; m->right = 1209; strcpy(m->label, "*"); return true;
    case '#': m->act = A_NOTE; m->left = 941; m->right = 1477; strcpy(m->label, "#"); return true;

    /* North American telephone call-progress tones (fixed, like DTMF) */
    case '!': m->act = A_NOTE; m->left = 480; m->right = 620; strcpy(m->label, "!"); return true; /* busy */
    case '@': m->act = A_NOTE; m->left = 440; m->right = 480; strcpy(m->label, "@"); return true; /* ring */
    case '$': m->act = A_NOTE; m->left = 350; m->right = 440; strcpy(m->label, "$"); return true; /* dial */

    default:
        return false;
    }
}

/* two different speaker frequencies: DTMF or a phone call-progress tone */
static inline bool is_dtmf(const keymap_t *m) { return m->left != m->right; }

/*
 *  Build the scientific-pitch-notation string for a key from its display
 *  label, e.g. "A#" -> "A#3", "Ab" -> "Ab3", "C" -> "C4", "G#" -> "G#4".
 *  Matches the octave choice in map_key's QUIRK note (A/B in octave 3,
 *  C..G in octave 4).  When "up" is set (the K/L/ENTER, I/O/P and ',' '.'
 *  keys) the octave digit is bumped by one: "A" -> "A4", "C" -> "C5".
 *  DTMF keys have no pitch, so their label ("1", "*", "#", ...) is copied
 *  through unchanged.  The octave digit is the nominal one; the up/down-
 *  arrow shift changes the recorded frequency but not this string.
 */
static void make_spn(const char *label, bool up, char out[4])
{
    char L = label[0];
    if (L >= 'A' && L <= 'G') {
        int oct = (L == 'A' || L == 'B') ? 3 : 4;
        if (up) oct++;
        int n = 0;
        out[n++] = L;
        if (label[1] == '#' || label[1] == 'b') out[n++] = label[1];
        out[n++] = (char)('0' + oct);
        out[n]   = '\0';
    } else {
        out[0] = L;
        out[1] = '\0';
    }
}

/* ===================================================================== */
/*  Octave shift (up / down arrows, musical notes only)                   */
/* ===================================================================== */

/*
 *  Apply the current octave shift to a musical-note frequency.  `oct` is
 *  the net number of UP presses (negative for DOWN); each step is a true
 *  octave - one halving or doubling - so the factor is a power of two
 *  (x2 x4 x8 up, /2 down: the OCT_MIN..OCT_MAX range).  There is
 *  deliberately no x3 / /3, and no /4 or /8 (all silent anyway).
 */
static uint32_t shift_octave(uint32_t f, int oct)
{
    while (oct > 0) { f *= 2; oct--; }   /* one UP press  = one octave up  */
    while (oct < 0) { f /= 2; oct++; }   /* one DOWN press = one octave down */
    return f;
}

static void show_octave(int oct)
{
    char b[24];
    unsigned factor = 1u << (oct < 0 ? -oct : oct);   /* 2^|oct| : 1 2 4 8 */
    if (oct > 0)      snprintf(b, sizeof b, "OCTAVE  x%u", factor);
    else if (oct < 0) snprintf(b, sizeof b, "OCTAVE  /%u", factor);
    else              strcpy(b, "OCTAVE  x1");
    put_row(8, b, COL_GREEN);
}

/* ===================================================================== */
/*  Recorder  (F1 = record toggle, F2 = play, DEL = clear, Back = undo)    */
/* ===================================================================== */

static uint32_t nowms(void)
{
    return to_ms_since_boot(get_absolute_time());
}

/* hold time in ms, clamped to the 1..65535 range the recorder stores */
static uint16_t clamp_ms(uint32_t ms)
{
    if (ms < 1)     ms = 1;                 /* even a quick tap counts */
    if (ms > 65535) ms = 65535;             /* ~65.5 s ceiling */
    return (uint16_t)ms;
}

/*
 *  Append one entry to the selected song.  At REC_LEN the row is full: the
 *  entry is dropped (no overrun) and a short 1000 Hz chirp tells the player
 *  their key press did not make it in.  show_recstat() also flags "(FULL)".
 */
static void rec_push(uint16_t lo, uint16_t hi, uint16_t dur_ms, const char *spn)
{
    song_t *s = &recbuf[cur_song];
    if (s->count >= REC_LEN) {
        audio_play_sound(1000, 1000);      /* "recording full" chirp */
        sleep_ms(60);
        audio_stop();
        return;
    }
    rec_entry_t *e = &s->entry[s->count++];
    e->lo     = lo;
    e->hi     = hi;
    e->dur_ms = dur_ms;
    snprintf(e->spn, sizeof e->spn, "%s", spn ? spn : "");
}

static void rec_pop(void)
{
    if (recbuf[cur_song].count > 0)
        recbuf[cur_song].count--;
}

/* recorder status line (row 6): note count of the selected song.  At
 * REC_LEN the tail reads " (FULL)" and the whole line turns yellow, so a
 * player hears the rec_push() chirp and sees why nothing more is landing. */
static void show_recstat(bool recording)
{
    char b[40];
    int  n    = recbuf[cur_song].count;
    bool full = (n >= REC_LEN);
    const char *tail = full ? " (FULL)" : " RECORDED";

    if (recording)
        snprintf(b, sizeof b, "* REC     %d NOTES%s", n, tail);
    else
        snprintf(b, sizeof b, "STOPPED   %d NOTES%s", n, tail);

    put_row(6, b, full ? COL_YELLOW : (recording ? COL_RED : COL_GREY));
}

/*
 *  Rows 28-29: the two edit keys that only do anything while RECORD is on
 *  - the Back key deletes the last entry, Space stores a rest (a silent
 *  gap).  They sit dim grey most of the time and light up yellow the
 *  moment recording starts, so a mistake is easy to undo and rests are
 *  easy to find.  Call this wherever `recording` flips (F1 / F2 / DEL /
 *  redraw).
 */
static void show_reckeys(bool recording)
{
    uint16_t col = recording ? COL_YELLOW : COL_GREY;
    put_row(28, "BACK KEY = DELETE LAST NOTE",     col);
    put_row(29, "SPACE = RECORD A REST (SILENCE)", col);
}

/* selected-song line (row 7): the cur_song index and that row's name in
 * quotes.  "SONG 11 \"<30 chars>\"" is exactly 40 columns - the max.      */
static void show_song(void)
{
    char b[48];
    snprintf(b, sizeof b, "SONG %d \"%s\"", cur_song, recbuf[cur_song].name);
    put_row(7, b, COL_CYAN);
}

/*
 *  Playback read-out for one recorded entry: the note on each speaker
 *  drawn big with its frequency underneath (left / right halves, exactly
 *  like the live display), then the entry's duration as SS.HH seconds
 *  ("....." if somehow over 99.99 s) and the "idx / total" counter, both
 *  centred on the rows just below.  A rest is "-" on both halves, no Hz.
 */
static void show_play(int idx, int total, const rec_entry_t *e)
{
    bool        rest  = (e->lo == 0 && e->hi == 0);
    const char *label = rest ? "-" : (e->spn[0] ? e->spn : "?");

    draw_band_split(label, e->lo, label, e->hi, BAND_FROW);

    /* duration as SS.HH (seconds . hundredths); dur_ms maxes at 65535 so
     * the >99.99 s guard is only there for safety.  buf is sized for the
     * worst case the compiler assumes for "%d / %d" (two full ints).      */
    char buf[27];
    if (e->dur_ms > 99990)
        put_row(BAND_FROW + 1, ".....", COL_WHITE);
    else {
        snprintf(buf, sizeof buf, "%02u.%02u SEC",
                 (unsigned)(e->dur_ms / 1000),
                 (unsigned)((e->dur_ms % 1000) / 10));
        put_row(BAND_FROW + 1, buf, COL_WHITE);
    }

    snprintf(buf, sizeof buf, "%d / %d", idx, total);
    put_row(BAND_FROW + 2, buf, COL_GREY);
}

/* F2: play every recorded entry for its stored duration */
static void play_recording(void)
{
    audio_stop();                          /* silence any note still held down */

    int count = recbuf[cur_song].count;    /* play exactly this song's length */
    if (count == 0) {
        put_row(6, "NOTHING RECORDED YET", COL_YELLOW);
        return;
    }

    put_row(6, "PLAYING BACK...", COL_GREEN);
    drain_keys();

    for (int i = 0; i < count; i++) {
        const rec_entry_t *r = &recbuf[cur_song].entry[i];
        show_play(i + 1, count, r);

        if (r->lo || r->hi)
            audio_play_sound(r->lo, r->hi);

        uint32_t end  = nowms() + r->dur_ms;
        bool     stop = false;
        while ((int32_t)(end - nowms()) > 0) {
            uint16_t e = key_event();
            if (e && ev_state(e) == KEY_STATE_PRESSED) {
                uint8_t k = ev_code(e);
                if (k == '~')                          /* SHIFT + backtick */
                    { audio_stop(); go_bootsel(); }
                if (!is_mod_key(k)) {                  /* any real key stops */
                    stop = true;
                    break;
                }
            }
            sleep_ms(3);
        }
        audio_stop();
        if (stop)
            break;
        sleep_ms(40);                      /* short gap between entries */
    }

    clear_band();
    drain_keys();
}

/* ===================================================================== */
/*  Built-in songs (Blair Leduc's songs.c - see songs.h)                  */
/* ===================================================================== */

/*
 *  Reverse of the audio.h PITCH_* macros: exact-match frequency -> its
 *  scientific-pitch-notation name, used to fill each pre-loaded entry's
 *  `spn` (the big glyph F2 PLAY shows).  Covers octaves 3..5, which is
 *  everything the songs use; anything else (or SILENCE) returns "".
 */
static const struct { uint16_t hz; const char *spn; } spn_map[] = {
    { PITCH_C3,  "C3"  }, { PITCH_CS3, "C#3" }, { PITCH_D3,  "D3"  }, { PITCH_DS3, "D#3" },
    { PITCH_E3,  "E3"  }, { PITCH_F3,  "F3"  }, { PITCH_FS3, "F#3" }, { PITCH_G3,  "G3"  },
    { PITCH_GS3, "G#3" }, { PITCH_A3,  "A3"  }, { PITCH_AS3, "A#3" }, { PITCH_B3,  "B3"  },
    { PITCH_C4,  "C4"  }, { PITCH_CS4, "C#4" }, { PITCH_D4,  "D4"  }, { PITCH_DS4, "D#4" },
    { PITCH_E4,  "E4"  }, { PITCH_F4,  "F4"  }, { PITCH_FS4, "F#4" }, { PITCH_G4,  "G4"  },
    { PITCH_GS4, "G#4" }, { PITCH_A4,  "A4"  }, { PITCH_AS4, "A#4" }, { PITCH_B4,  "B4"  },
    { PITCH_C5,  "C5"  }, { PITCH_CS5, "C#5" }, { PITCH_D5,  "D5"  }, { PITCH_DS5, "D#5" },
    { PITCH_E5,  "E5"  }, { PITCH_F5,  "F5"  }, { PITCH_FS5, "F#5" }, { PITCH_G5,  "G5"  },
    { PITCH_GS5, "G#5" }, { PITCH_A5,  "A5"  }, { PITCH_AS5, "A#5" }, { PITCH_B5,  "B5"  },
};

static const char *freq_to_spn(uint16_t hz)
{
    for (int i = 0; i < NELEMS(spn_map); i++)
        if (spn_map[i].hz == hz)
            return spn_map[i].spn;
    return "";
}

/*
 *  Called once at startup: copy each entry of Blair Leduc's songs[] into
 *  a recorder row, starting at row 2 (rows 0 and 1 stay empty for the
 *  user).  There are 10 songs and rows 2..11, so it fills them exactly.
 *  Each audio_note_t { left, right, duration_ms } becomes a rec_entry_t
 *  { lo, hi, dur_ms, spn }; the per-note {SILENCE,SILENCE,0} terminator
 *  is the loop's stop condition and is not copied.  The song's
 *  description becomes the row's name.
 */
static void load_builtin_songs(void)
{
    int row = 2;
    for (int s = 0; songs[s].name != NULL && row < REC_SONGS; s++, row++) {
        song_t *dst = &recbuf[row];
        snprintf(dst->name, sizeof dst->name, "%s", songs[s].description);

        int n = 0;
        for (const audio_note_t *note = songs[s].notes;
             note->duration_ms != 0 && n < REC_LEN; note++, n++) {
            rec_entry_t *e = &dst->entry[n];
            e->lo     = note->left_frequency;
            e->hi     = note->right_frequency;
            e->dur_ms = clamp_ms(note->duration_ms);
            uint16_t f = note->left_frequency ? note->left_frequency
                                              : note->right_frequency;
            snprintf(e->spn, sizeof e->spn, "%s", freq_to_spn(f));
        }
        dst->count = n;
    }
}

/* ===================================================================== */
/*  F3 / F4 / F5 placeholders, DEL confirm, help screen                   */
/* ===================================================================== */
/*
 *  EDIT / SAVE / LOAD (F3 / F4 / F5) aren't written yet.  Each flashes a
 *  red notice on row 30 for ~0.5 s with a 0.5 s 1000 Hz beep, then clears
 *  the row and drains anything mashed in the meantime.  `what` is one of
 *  "EDIT" / "SAVE" / "LOAD".
 */
static void not_implemented(const char *what)
{
    char b[40];
    snprintf(b, sizeof b, "%s FUNCTION NOT YET IMPLEMENTED", what);
    put_row(30, b, COL_RED);
    audio_play_sound(1000, 1000);
    sleep_ms(500);
    audio_stop();
    clear_row(30);
    drain_keys();
}

/*
 *  DEL: clear the selected song row.  If the row has nothing recorded there
 *  is nothing to lose, so just return.  Otherwise ask "DELETE RECORDING n
 *  (Y/N)" (n = cur_song) in the note band and wait for one keypress: only a
 *  lower- or upper-case Y wipes the take (and its name); anything else
 *  cancels.  '~' still reaches BOOTSEL.  On return the band is cleared and
 *  the recorder status line refreshed.
 */
static void confirm_and_clear(bool *recording)
{
    if (recbuf[cur_song].count == 0)         /* empty row - nothing to delete */
        return;

    audio_stop();
    clear_band();

    char b[40];
    snprintf(b, sizeof b, "DELETE RECORDING %d (Y/N)", cur_song);
    put_row(BAND_FROW - 4, b, COL_YELLOW);

    uint8_t k = wait_key();
    if (k == 'y' || k == 'Y') {
        *recording = false;
        recbuf[cur_song].count   = 0;       /* clears only the selected song   */
        recbuf[cur_song].name[0] = '\0';    /* ...and wipes its name/desc too  */
    }
    clear_band();
    show_recstat(*recording);
    show_reckeys(*recording);
    show_song();
}

/*
 *  '?' : a full-screen key reference.  Any key press returns to the
 *  Music Maker screen (the caller redraws it).  '~' still reboots.
 *  The PLAY NOTES block has two forms - FULL vs A-G - matching whichever
 *  note_map is live when '?' is pressed.
 */
static void show_help(void)
{
    audio_stop();
    lcd_clear_screen();
    put_row(0, "PICOCALC MUSIC MAKER  -  HELP", COL_CYAN);

    /* the PLAY NOTES block depends on the live note_map ('\' toggles it) */
    if (note_map == MAP_LETTERS) {
        put_at(8,  2, "PLAY NOTES  (A-G MAP)",             COL_YELLOW);
        put_at(8,  3, "A B C D E F G   PLAY NOTES A-G",    COL_WHITE);
        put_at(8,  4, "SHIFT + KEY     SHARP OF THAT KEY", COL_WHITE);
        put_at(8,  5, "CTRL  + KEY     FLAT  OF THAT KEY", COL_WHITE);
        put_at(8,  6, "\\ (BACKSLASH)   SWITCH NOTE-KEY MAP", COL_WHITE);
    } else {
        put_at(8,  2, "PLAY NOTES  (FULL MAP)",            COL_YELLOW);
        put_at(8,  3, "A S D F G H J   NATURALS A-G",      COL_WHITE);
        put_at(8,  4, "K L ENTER       + 1 OCTAVE",        COL_WHITE);
        put_at(8,  5, "Q W E R T Y U   SHARPS  A#-G#",     COL_WHITE);
        put_at(8,  6, "I O P           SHARPS  + 1 OCT",   COL_WHITE);
        put_at(8,  7, "Z X C V B N M   FLATS   Ab-Gb",     COL_WHITE);
        put_at(8,  8, ", .             FLATS   + 1 OCT",   COL_WHITE);
        put_at(8,  9, "SHIFT + KEY     SHARP OF THAT KEY", COL_WHITE);
        put_at(8, 10, "CTRL  + KEY     FLAT  OF THAT KEY", COL_WHITE);
        put_at(8, 11, "\\ (BACKSLASH)   SWITCH NOTE-KEY MAP", COL_WHITE);
    }

    put_at(8, 13, "PHONE TONES  (FIXED, LEFT / RIGHT)", COL_YELLOW);
    put_at(8, 14, "0-9 # *         DTMF DIAL TONES",   COL_WHITE);
    put_at(8, 15, "! @ $           BUSY  RING  DIAL",  COL_WHITE);

    put_at(8, 16, "OCTAVE / MISC",                     COL_YELLOW);
    put_at(8, 17, "UP ARROW        GO UP AN OCTAVE",   COL_WHITE);
    put_at(8, 18, "DOWN ARROW      GO DOWN AN OCTAVE", COL_WHITE);
    put_at(8, 19, "=               SONG 0-11 (2-11 DEMO)", COL_WHITE);
    put_at(8, 20, "ESC             BACK TO SPLASH",    COL_WHITE);
    put_at(8, 21, "?               THIS HELP",         COL_WHITE);
    put_at(8, 22, "~               REBOOT TO BOOTSEL", COL_WHITE);

    put_at(8, 24, "RECORDER",                          COL_YELLOW);
    put_at(8, 25, "F1 / F2         RECORD / PLAY",     COL_WHITE);
    put_at(8, 26, "SPACE  BACK     REST / UNDO",       COL_WHITE);
    put_at(8, 27, "DEL             CLEAR  (ASKS Y/N)", COL_WHITE);

    put_row(31, "PRESS ANY KEY TO RETURN", COL_WHITE);

    wait_key();
}

/* note-map indicator (row 9): which musical-note key layout '\' selected */
static void show_notemap(void)
{
    if (note_map == MAP_LETTERS)
        put_row(9, "NOTE KEYS  A-G ONLY   \\ SWITCHES",   COL_CYAN);
    else
        put_row(9, "NOTE KEYS  FULL LAYOUT   \\ SWITCHES", COL_CYAN);
}

/* clear the screen and paint the whole Music Maker view (static text plus
 * the current octave / note-map / recorder status) - used at entry and
 * after the help screen                                                  */
static void draw_screen(int oct, bool recording)
{
    lcd_clear_screen();
    draw_static_text();
    clear_band();
    show_song();
    show_octave(oct);
    show_notemap();
    show_recstat(recording);
    show_reckeys(recording);
}

/* ===================================================================== */
/*  Music Maker screen: static text (title, key help, labels)             */
/* ===================================================================== */
static void draw_static_text(void)
{
    put_row(2,  "PICOCALC MUSIC MAKER (BY THOMAS DZUBIN)", COL_CYAN);
    put_row(3,  "PRESS LETTER KEYS TO PLAY NOTES",     COL_WHITE);
    put_row(4,  "USE UP/DOWN ARROW TO CHANGE OCTAVES", COL_WHITE);
    put_row(5,  "\"=\" KEY SELECTS SONG STORAGE SLOT 0-11", COL_WHITE);
    /* the ~ (SHIFT + backtick) BOOTSEL reboot is deliberately not shown */
    /* dynamic status lines: row 6 recstat, row 7 song, row 8 octave,
     * row 9 note-map (see show_notemap)                                 */
    /* headers over the split note band (see BAND_Y / BAND_HDR_ROW); drawn
     * once here since nothing else ever writes to this row               */
    put_at( 4 * 8, BAND_HDR_ROW, "LEFT CHANNEL",  COL_GREY);
    put_at(23 * 8, BAND_HDR_ROW, "RIGHT CHANNEL", COL_GREY);
    put_row(27, "DEL=CLEAR   ESC=SPLASH   ?=HELP",  COL_GREY);
    /* rows 28-29 are the recording-only edit keys - painted by
     * show_reckeys(), which brightens them while RECORD is on           */
    /* function-key legend on the very last row, right above the physical
     * function keys.  F1 / F2 are live (white); F3 EDIT / F4 SAVE /
     * F5 LOAD are placeholders (grey - see not_implemented()).           */
    clear_row(31);
    put_at( 1 * 8, 31, "F1=REC F2=PLAY",          COL_WHITE);
    put_at(16 * 8, 31, "F3=EDIT F4=SAVE F5=LOAD",  COL_GREY);
}
