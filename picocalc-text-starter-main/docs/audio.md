# Audio

This simple audio driver can play stereo notes using the PIO, a maximum of one note per channel. Very little memory is used.


## audio_init

`void audio_init(void)`

Initialises the audio driver.


## audio_play_sound_blocking

`void audio_play_tone_blocking(uint32_t left_frequency, uint32_t right_frequency, uint32_t duration_ms)`

Plays a note on each channel for a duration. A frequency of zero represents silence.

### Parameters

- left_frequency - frequency of the note in Hz
- right_frequency - frequency of the note in Hz
- duration_ms – duration of the tone in milliseconds


## audio_play_sound

`void audio_play_sound(uint32_t left_frequency, uint32_t right_frequency)`

Plays a note on each channel until stopped. A frequency of zero represents silence.

### Parameters

- left_frequency - frequency of the note in Hz
- right_frequency - frequency of the note in Hz


## audio_play_note_blocking

`void audio_play_note_blocking(const audio_note_t *note)`

Plays an note, defined by `audio_note_t`.

### Parameters

- note – note to play


## audio_play_song_blocking

`void audio_play_song_blocking(const audio_song_t *song)`

Plays a song (blocking), defined by 'audio_song_t'.


## audio_stop

`void audio_stop(void)`

Stops a tone that is playing asynchronously.

### Parameters

- song – song to play


## audio_is_playing

`bool audio_is_playing(void)`

Return true if an asynchronous tone is playing.

