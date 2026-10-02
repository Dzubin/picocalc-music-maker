/*
 * music_maker_const.h - every constant of the PicoCalc Music Maker program:
 * the version, limits and colours, where things sit on the screen, the
 * on-screen help text and the data tables (big-note font, pitch table, pitch
 * names). music_maker.c holds only code. Included by music_maker.c alone, so
 * the tables are plain static data.
 *
 * Author: Thomas Dzubin
 */
#ifndef MUSIC_MAKER_CONST_H
#define MUSIC_MAKER_CONST_H

#include <stdbool.h>
#include <stdint.h>

#include "lcd.h"            /* WIDTH, RGB() */
#include "audio.h"          /* PITCH_* */

/* ===================================================================== */
/*  Version, limits, colours                                              */
/* ===================================================================== */

/* shown on the splash screen */
#define VERSION "V0.01C"

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

/* The big centre-screen note read-out lives in this pixel band, split into
 * a left-channel half [0,HALF_W) and a right-channel half.  The
 * "LEFT CHANNEL" / "RIGHT CHANNEL" headers sit on text row BAND_HDR_ROW,
 * just above the band, and each half's frequency line sits on text row
 * BAND_FROW.  Text can only be placed on 10 pixel rows, so those rows are
 * the nearest ones to the glyph rather than exact pixel offsets.         */
#define BAND_Y      115
#define BAND_H      140
#define HALF_W      (WIDTH / 2)
#define BAND_HDR_ROW 10
#define BAND_FROW    22

/* recorder storage: REC_SONGS song rows (see song_t), each holding a
 * 30-char name and up to REC_LEN entries.  '=' selects the row; record,
 * undo, clear and play all work on the selected row.                     */
#define REC_SONGS 12
#define REC_LEN   200

/* width of one text cell in pixels (the built-in 8x10 font) */
#define FONT_W 8

/* ===================================================================== */
/*  Screen positions (the text grid is 40 columns by 32 rows)             */
/* ===================================================================== */

/* splash screen */
#define SPLASH_TITLE_Y        40    /* pixel y of the double-size title      */
#define SPLASH_AUTHOR_Y      110    /* pixel y of the double-size author     */
#define SPLASH_SCALE           2    /* magnification of those two lines      */
#define SPLASH_ROW_VERSION     7
#define SPLASH_ROW_CREDIT_1   14
#define SPLASH_ROW_CREDIT_2   15
#define SPLASH_ROW_PROMPT     20    /* the reboot confirmation reuses these  */
#define SPLASH_ROW_REBOOT     21    /* two rows                              */
#define ROW_REBOOTING         15    /* "REBOOTING..." over the credits       */

/* music screen: tips, status lines and the note band */
#define ROW_TITLE              2
#define ROW_TIP_NOTES          3
#define ROW_TIP_OCTAVE         4
#define ROW_TIP_SONG           5
#define ROW_RECSTAT            6    /* record status, also the play status   */
#define ROW_SONG               7
#define ROW_OCTAVE             8
#define ROW_NOTEMAP            9
#define CHANNEL_HDR_LEFT_COL   4    /* on row BAND_HDR_ROW                   */
#define CHANNEL_HDR_RIGHT_COL 23
#define ROW_PLAY_LENGTH  (BAND_FROW + 1)    /* playback: length of the note  */
#define ROW_PLAY_COUNT   (BAND_FROW + 2)    /* playback: "idx / total"       */
#define ROW_CONFIRM      (BAND_FROW - 4)    /* DELETE RECORDING prompt       */

/* music screen: footer */
#define ROW_KEYS_HINT         27
#define ROW_RECKEY_BACK       28
#define ROW_RECKEY_SPACE      29
#define ROW_NOTICE            30    /* "... NOT YET IMPLEMENTED"             */
#define ROW_FKEYS             31
#define FKEYS_LIVE_COL         1
#define FKEYS_STUB_COL        16

/* help screen */
#define HELP_X                 8    /* left edge in pixels (column 1)        */
#define HELP_ROW_TITLE         0
#define HELP_ROW_PROMPT       31

/* ===================================================================== */
/*  Help screen text                                                      */
/* ===================================================================== */

typedef struct {
    uint8_t     row;
    bool        heading;            /* section heading (yellow) or body (white) */
    const char *text;
} help_line_t;

/* PLAY NOTES section, FULL layout */
static const help_line_t help_notes_full[] = {
    {  2, true,  "PLAY NOTES  (FULL MAP)" },
    {  3, false, "A S D F G H J   NATURALS A-G" },
    {  4, false, "K L ENTER       + 1 OCTAVE" },
    {  5, false, "Q W E R T Y U   SHARPS  A#-G#" },
    {  6, false, "I O P           SHARPS  + 1 OCT" },
    {  7, false, "Z X C V B N M   FLATS   Ab-Gb" },
    {  8, false, ", .             FLATS   + 1 OCT" },
    {  9, false, "SHIFT + KEY     SHARP OF THAT KEY" },
    { 10, false, "CTRL  + KEY     FLAT  OF THAT KEY" },
    { 11, false, "\\ (BACKSLASH)   SWITCH NOTE-KEY MAP" },
};

/* PLAY NOTES section, A-G ONLY layout */
static const help_line_t help_notes_letters[] = {
    {  2, true,  "PLAY NOTES  (A-G MAP)" },
    {  3, false, "A B C D E F G   PLAY NOTES A-G" },
    {  4, false, "SHIFT + KEY     SHARP OF THAT KEY" },
    {  5, false, "CTRL  + KEY     FLAT  OF THAT KEY" },
    {  6, false, "\\ (BACKSLASH)   SWITCH NOTE-KEY MAP" },
};

/* the rest of the help screen, the same for both layouts */
static const help_line_t help_common[] = {
    { 13, true,  "PHONE TONES  (FIXED, LEFT / RIGHT)" },
    { 14, false, "0-9 # *         DTMF DIAL TONES" },
    { 15, false, "! @ $           BUSY  RING  DIAL" },

    { 16, true,  "OCTAVE / MISC" },
    { 17, false, "UP ARROW        GO UP AN OCTAVE" },
    { 18, false, "DOWN ARROW      GO DOWN AN OCTAVE" },
    { 19, false, "=               SONG 0-11 (2-11 DEMO)" },
    { 20, false, "ESC             BACK TO SPLASH" },
    { 21, false, "?               THIS HELP" },
    { 22, false, "~               REBOOT TO BOOTSEL" },

    { 24, true,  "RECORDER" },
    { 25, false, "F1 / F2         RECORD / PLAY" },
    { 26, false, "SPACE  BACK     REST / UNDO" },
    { 27, false, "DEL             CLEAR  (ASKS Y/N)" },
};

/* ===================================================================== */
/*  Tuning                                                                */
/* ===================================================================== */

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

/* ===================================================================== */
/*  Data tables                                                           */
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

#endif /* MUSIC_MAKER_CONST_H */
