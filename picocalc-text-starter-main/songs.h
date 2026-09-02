#pragma once

#include "drivers/audio.h"

// Song lists
extern const audio_song_t songs[];

// Song function prototypes
const audio_song_t* find_song(const char* song_name);
void show_song_library(void);