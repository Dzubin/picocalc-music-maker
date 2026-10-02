/*
 * shim_audio.c - the picocalc-text-starter audio driver API (audio.h, by
 * Blair Leduc) re-implemented on SDL2 audio for the desktop build. Like the
 * PicoCalc's PWM output, each channel is a square wave of the requested
 * frequency; 0 Hz is silence.
 *
 * Author: Thomas Dzubin
 */
#include "shim.h"
#include "audio.h"

#define NOTE_GAP_MS 20      /* gap between song notes, as in the vendored driver */

static SDL_AudioDeviceID device;
static bool     initialised;
static volatile uint32_t freq[2];       /* Hz per channel, 0 = silent */
static double   phase[2];

static void SDLCALL fill(void *user, Uint8 *stream, int len)
{
    int16_t *out = (int16_t *)stream;
    int frames = len / (int)(2 * sizeof(int16_t));

    (void)user;
    for (int i = 0; i < frames; i++) {
        for (int ch = 0; ch < 2; ch++) {
            uint32_t f = freq[ch];

            if (f == 0) {
                out[i * 2 + ch] = 0;
                continue;
            }
            out[i * 2 + ch] = phase[ch] < 0.5 ? SHIM_AUDIO_AMPLITUDE : -SHIM_AUDIO_AMPLITUDE;
            phase[ch] += (double)f / SHIM_AUDIO_RATE;
            if (phase[ch] >= 1.0)
                phase[ch] -= 1.0;
        }
    }
}

void audio_init(void)
{
    SDL_AudioSpec want;

    if (initialised)
        return;
    shim_init();
    initialised = true;             /* even without a sound device, carry on silently */
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
        return;
    SDL_zero(want);
    want.freq = SHIM_AUDIO_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = SHIM_AUDIO_SAMPLES;
    want.callback = fill;
    device = SDL_OpenAudioDevice(NULL, 0, &want, NULL, 0);
    if (device)
        SDL_PauseAudioDevice(device, 0);
}

void audio_play_sound(uint32_t left_frequency, uint32_t right_frequency)
{
    if (!initialised)
        return;
    if (device)
        SDL_LockAudioDevice(device);
    freq[LEFT_CHANNEL] = left_frequency;
    freq[RIGHT_CHANNEL] = right_frequency;
    if (device)
        SDL_UnlockAudioDevice(device);
}

void audio_stop(void)
{
    audio_play_sound(0, 0);
}

bool audio_is_playing(void)
{
    return freq[LEFT_CHANNEL] != 0 || freq[RIGHT_CHANNEL] != 0;
}

void audio_play_sound_blocking(uint32_t left_frequency, uint32_t right_frequency, uint32_t duration_ms)
{
    audio_play_sound(left_frequency, right_frequency);
    sleep_ms(duration_ms);
    audio_stop();
}

void audio_play_note_blocking(const audio_note_t *note)
{
    if (note)
        audio_play_sound_blocking(note->left_frequency, note->right_frequency, note->duration_ms);
}

void audio_play_song_blocking(const audio_song_t *song)
{
    const audio_note_t *n;

    if (!initialised || !song)
        return;
    for (n = song->notes; n->duration_ms != 0; n++) {
        audio_play_note_blocking(n);
        if (n->left_frequency != SILENCE || n->right_frequency != SILENCE)
            sleep_ms(NOTE_GAP_MS);
    }
}
