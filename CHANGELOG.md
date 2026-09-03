# Changelog

All notable changes to PicoCalc Music Maker.
Format loosely follows [Keep a Changelog](https://keepachangelog.com/);
the version here matches the `VERSION` define in `music_maker.c`.

## [Unreleased]

_Nothing yet._

## [0.01A] - 2026-09-02

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
- Release asset: prebuilt `picocalc-music-maker_V001A_RP2040.uf2` (Pico /
  Pico W). No Pico 2 / RP2350 binary yet — build it with `-DPICO_BOARD=pico2`.
