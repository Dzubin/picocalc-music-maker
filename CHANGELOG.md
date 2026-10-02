# Changelog

All notable changes to PicoCalc Music Maker.
Format loosely follows [Keep a Changelog](https://keepachangelog.com/);
the version here matches the `VERSION` define in `music_maker_const.h`.

## [0.01C] - Unreleased

`VERSION` in `music_maker_const.h` is now `V0.01C` (0.01B was never tagged, so
its changes are part of this round); the heading gets a date when it's tagged.

### Added
- Windows build: `desktop/` holds a small SDL2 shim that re-implements the
  picocalc-text-starter driver API (LCD, keyboard, audio) so the same
  `music_maker.c` runs on a PC; SDL2 is linked statically, giving one
  self-contained `picocalc-music-maker-Windows.exe`.
- Linux build: the `desktop/` build (formerly `windows/`) now also builds on
  Linux and produces `picocalc-music-maker-Linux`. SDL2 is linked dynamically there and the
  build copies `libSDL2-2.0.so.0` next to the executable (found through an
  `$ORIGIN` run-path); needs the SDL2 development package (e.g. `libsdl2-dev`).
- Desktop build: the numeric keypad now works (digits, `.`, `*`, `/`, `+`,
  `-`, `=`), so DTMF can be played from it.
- The firmware build now names every output with the chip (the `.uf2`, `.elf`, `.bin` and `.hex` all end in `-RP2040` / `-RP2350`) and copies the `.uf2` to the top-level folder as
  `picocalc-music-maker-RP2040.uf2` / `picocalc-music-maker-RP2350.uf2`, so the
  two chips' builds sit side by side.

### Changed
- Code layout: every constant (limits, colours, screen row and column
  positions, the help screen text, and the data tables) moved out of
  `music_maker.c` into `music_maker_const.h`; the help screen is now drawn from
  tables. No behaviour change.
- `platform.h` is now the only interface to the machine (clock, sleep, BOOTSEL
  and reboot), implemented in `platform_pico.c` (firmware) and
  `desktop/platform_desktop.c` (PC); `music_maker.c` no longer includes Pico SDK
  headers directly.
- The README now says that the vendored `drivers/audio.pio` carries one patched
  line (upper tone limit 2000 -> 2115 Hz).

### Fixed
- Firmware build: the post-build copy of the `.uf2` pointed at `/` instead of
  the project folder, so the top-level `.uf2` would not have been refreshed.
- `.gitignore` now also covers the desktop outputs copied to the top-level
  folder (`*.exe`, `picocalc-music-maker-Linux`, `libSDL2-2.0.so.0`).
- README: corrected how `~` and ESC behave in the desktop build.

## [0.01A] - 2026-09-03

First tagged release —
<https://github.com/Dzubin/picocalc-music-maker/releases/tag/v0.01A>.
Everything below is the baseline for future entries.

### Splash screen
- Double-size title, `VERSION` line, "BY THOMAS DZUBIN" and the
  picocalc-text-starter / Blair Leduc credit.
- Any key starts the Music Maker screen.
- `ESC` reboots the PicoCalc after a `REBOOT AND ERASE ALL RECORDINGS?` Y/N
  prompt (recordings are RAM-only); already-empty state skips nothing here.
- `~` (SHIFT + backtick) reboots into BOOTSEL; bare SHIFT/CTRL ignored.

### Music Maker screen
- Split left/right note read-out: each half shows the note name as a large
  glyph with its per-channel frequency in Hz, `LEFT CHANNEL` / `RIGHT CHANNEL`
  headers and a divider. Plain notes mirror both halves; DTMF / phone-tone
  keys show the digit on both with the two tones.
- Row-9 indicator shows the active note-key layout.

### Note keys - two layouts, toggled by `\`
- **FULL** (default): `A S D F G H J` naturals A-G; `K L ENTER` the same one
  octave up; `Q W E R T Y U` sharps; `I O P` sharps one octave up;
  `Z X C V B N M` flats; `, .` flats one octave up. `SHIFT`/`CTRL` + a home
  key gives that key's sharp / flat (table-driven layer).
- **A-G ONLY**: only `A B C D E F G` play (keycap = note); `SHIFT` = its
  sharp, `CTRL` = its flat; every other letter key and `ENTER` are silent.
  `H J K` also play `A B C` one octave up (undocumented DO-RE-MI helper, not
  shown on the help screen).
- Choice persists until reboot (resets to FULL).

### Telephone tones (both layouts)
- `0`-`9` `#` `*` - DTMF dialling tones (low tone left, high tone right).
- `!` `@` `$` - North American BUSY (480/620), RING (440/480), DIAL (350/440).

### Octave shift
- `UP` arrow doubles every following musical note, up to x8; `DOWN` halves it,
  a single step to /2. DTMF / phone tones unaffected.

### Recorder
- 12 song slots (`recbuf[REC_SONGS]`), each `{ name[31], count, entry[200] }`.
  `=` cycles the selected slot 0-11; row-7 status shows index and name.
- `F1` toggles RECORD; `F2` plays the selected slot back with stored
  durations, any key stops it.
- `BACK` deletes the last entry; `SPACE` records a rest (silent gap); their
  help lines brighten to yellow while recording.
- `DEL` clears the selected slot after a `DELETE RECORDING n (Y/N)` prompt
  (skipped silently if the slot is already empty); wipes notes and name.
- At `REC_LEN` = 200 entries the status line turns yellow / reads `(FULL)` and
  further notes are dropped with a 1000 Hz chirp.
- Startup loads slots 2-11 from picocalc-text-starter's `songs.c` (10 tunes);
  slots 0-1 stay empty.

### Not implemented yet
- `F3` / `F4` / `F5` (EDIT / SAVE / LOAD) - flash a red notice + beep.
- Song-name editing; recordings do not persist across reboot/power-off.

### Help / misc
- `?` opens a full-screen key reference; its PLAY NOTES section matches the
  live note-key layout.
- Built on the Raspberry Pi Pico SDK and the vendored `picocalc-text-starter`
  drivers by Blair Leduc.

### Project / packaging
- Published to GitHub (`Dzubin/picocalc-music-maker`) under the MIT License;
  the vendored `picocalc-text-starter-main/` keeps its own MIT license.
- Added `LICENSE`, `CHANGELOG.md`, `CLAUDE.md`, `.gitignore` (excludes
  `build/`, `*.uf2`, `*.zip`) and `.gitattributes` (`eol=lf`).
- README gained **License** and **Known limitations** sections and a pointer
  to the Releases page.
- Release assets: prebuilt `picocalc-music-maker_V001A_RP2040.uf2` (Pico /
  Pico W) and `picocalc-music-maker_V001A_RP2350.uf2` (Pico 2).
