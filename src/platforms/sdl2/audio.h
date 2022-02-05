#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <SDL_audio.h>

enum
{
    VOLUME = SDL_MIX_MAXVOLUME-20, // volume is not as loud using u8...
    CHANNELS = 2,
#ifdef __SWITCH__
    SAMPLES = 2048,
#else
    SAMPLES = 512,
#endif
    AUDIO_FORMAT = AUDIO_U8,
    AUDIO_FLAGS = SDL_AUDIO_ALLOW_ANY_CHANGE,

    AUDIO_FREQ_11k = 11025,
    AUDIO_FREQ_22k = 22050,
    AUDIO_FREQ_44k = 44100,
    AUDIO_FREQ_48k = 48000,
    AUDIO_FREQ_96k = 96000,
    AUDIO_FREQ_192k = 192000,
};

bool audio_init(emu_t* emu);
void audio_exit(void);

SDL_AudioSpec audio_get_spec(void);
// returns the number of cycles the apu has ticked the emulator for
// resets the cycle counter on call, so save the result if needed
// more than once!
// you should either lock the audio or lock the core before
// calling this!
uint32_t audio_get_elapsed_cycles(void);

int audio_get_core_sample_rate(const emu_t* emu);
void audio_update_core_sample_rate(emu_t* emu);

void audio_lock(void);
void audio_unlock(void);

#ifdef __cplusplus
}
#endif
