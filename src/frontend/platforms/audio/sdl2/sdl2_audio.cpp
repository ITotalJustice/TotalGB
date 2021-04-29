#include "frontend/platforms/audio/sdl2/sdl2_audio.hpp"
#include "core/types.h"

#ifdef MGB_SDL2_AUDIO


#ifdef _MSC_VER
#include <SDL.h>
#include <SDL_audio.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#endif // _MSC_VER

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <cstdio>
#include <cstring>


namespace mgb::platform::audio::sdl2 {


SDL2::~SDL2() {
	if (SDL_WasInit(SDL_INIT_AUDIO)) {
	    if (this->device_id != 0) {
	        SDL_CloseAudioDevice(this->device_id);
	    }

    	SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
}

auto SDL2::SetupAudio(int freq) -> bool {
	// this is to fix very high cpu usage on my chromebook
	// when using pulseaudio (30%!).
#ifndef __EMSCRIPTEN__
	// SDL_setenv("SDL_AUDIODRIVER", "sndio", 1);
#endif // __EMSCRIPTEN__

	if (SDL_InitSubSystem(SDL_INIT_AUDIO)) {
		printf("\n[SDL_AUDIO_ERROR] %s\n\n", SDL_GetError());
		return false;
	}

	printf("\nAUDIO_DEVICE_NAMES\n");
    for (int i = 0; i <  SDL_GetNumAudioDevices(0); i++) {
        printf("\tname: %s\n", SDL_GetAudioDeviceName(i, 0));
    }

    printf("\nAUDIO_DRIVER_NAMES\n");
    for (int i = 0; i < SDL_GetNumAudioDrivers(); ++i) {
        const char* driver_name = SDL_GetAudioDriver(i);
        printf("\tname: %s\n", driver_name);
    }

    const SDL_AudioSpec wanted{
        /* .freq = */ freq,
        /* .format = */ AUDIO_S8,
        /* .channels = */ 2,
        /* .silence = */ 0, // calculated
        /* .samples = */ 512, // 512 * 2 (because stereo)
        /* .padding = */ 0,
        /* .size = */ 0, // calculated
        /* .callback = */ NULL,
        /* .userdata = */ NULL
    };

    SDL_AudioSpec obtained{};

    this->device_id = SDL_OpenAudioDevice(NULL, 0, &wanted, &obtained, 0);
    
    // check if an audio device was failed to be found...
    if (this->device_id == 0) {
        printf("failed to find valid audio device\n");
        return false;
    }
    else {
        printf("\nSDL_AudioSpec:\n");
        printf("\tfreq: %d\n", obtained.freq);
        printf("\tformat: %d\n", obtained.format);
        printf("\tchannels: %u\n", obtained.channels);
        printf("\tsilence: %u\n", obtained.silence);
        printf("\tsamples: %u\n", obtained.samples);
        printf("\tpadding: %u\n", obtained.padding);
        printf("\tsize: %u\n", obtained.size);

        SDL_PauseAudioDevice(this->device_id, 0);
    }

    return true;
}

auto SDL2::PushSamples(const struct GB_ApuCallbackData* data) -> void {
    while (SDL_GetQueuedAudioSize(this->device_id) > (1024*4)) {
        SDL_Delay(1);
    }

    uint8_t buffer[512*2]{};
    SDL_MixAudioFormat(buffer, (const uint8_t*)data->buffers.ch1, AUDIO_S8, sizeof(data->buffers.ch1), SDL_MIX_MAXVOLUME/8);
    SDL_MixAudioFormat(buffer, (const uint8_t*)data->buffers.ch2, AUDIO_S8, sizeof(data->buffers.ch2), SDL_MIX_MAXVOLUME/8);
    SDL_MixAudioFormat(buffer, (const uint8_t*)data->buffers.ch3, AUDIO_S8, sizeof(data->buffers.ch3), SDL_MIX_MAXVOLUME/8);
    SDL_MixAudioFormat(buffer, (const uint8_t*)data->buffers.ch4, AUDIO_S8, sizeof(data->buffers.ch4), SDL_MIX_MAXVOLUME/8);

    switch (SDL_GetAudioDeviceStatus(this->device_id)) {
        case SDL_AUDIO_STOPPED:
            // std::printf("[SDL2-AUDIO] stopped\n");
            break;

        case SDL_AUDIO_PAUSED:
            // std::printf("[SDL2-AUDIO] paused\n");
            break;

        case SDL_AUDIO_PLAYING:
            SDL_QueueAudio(this->device_id, buffer, sizeof(buffer));
            break;
    }
}

} // namespace mgb::platform::audio::sdl2

#endif // MGB_SDL2_AUDIO
