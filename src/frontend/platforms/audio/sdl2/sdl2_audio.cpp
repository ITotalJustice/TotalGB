#include "frontend/platforms/audio/sdl2/sdl2_audio.hpp"
#include "core/types.h"

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

auto SDL2::SetupAudio() -> bool {
	// this is to fix very high cpu usage on my chromebook
	// when using pulseaudio (30%!).
	SDL_setenv("SDL_AUDIODRIVER", "sndio", 1);

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
        /* .freq = */ 48000,
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
	while (SDL_GetQueuedAudioSize(this->device_id) > (sizeof(data->samples) * 4)) {
        SDL_Delay(1);
    }

    SDL_QueueAudio(
    	this->device_id, data->samples, sizeof(data->samples)
    );
}

} // namespace mgb::platform::audio::sdl2
