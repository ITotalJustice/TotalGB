#include "frontend/platforms/audio/sdl1/sdl1_audio.hpp"
#include "core/types.h"

#ifdef MGB_SDL1_AUDIO

#include <SDL/SDL.h>
#include <SDL/SDL_audio.h>
#include <cstring>


namespace mgb::platform::audio::sdl1 {


SDL1::~SDL1() {
	if (SDL_WasInit(SDL_INIT_AUDIO)) {
	    if (this->device_opened) {
            SDL_CloseAudio();
	    }

    	SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
}

static auto callback(void* user, Uint8* buf, int len) {
    static_cast<SDL1*>(user)->AudioCallback(buf, len);
}

auto SDL1::AudioCallback(Uint8* buf, int len) -> void {
    std::scoped_lock lock{this->mutex};
    
    // we have no buffers...
    if (this->buffer_size == 0) {
        // fill with silence...
        std::memset(buf, 0, len);
        return;
    }

    std::memcpy(
        buf,
        this->buffers[this->buffer_index].data(),
        this->buffers[this->buffer_index].size()
    );

    --this->buffer_size;
}

auto SDL1::SetupAudio() -> bool {
	// this is to fix very high cpu usage on my chromebook
	// when using pulseaudio (30%!).
    setenv("SDL_AUDIODRIVER", "sndio", 1);

	if (SDL_InitSubSystem(SDL_INIT_AUDIO)) {
		printf("\n[SDL_AUDIO_ERROR] %s\n\n", SDL_GetError());
		return false;
	}

    SDL_AudioSpec wanted{
        /* .freq = */ 48000,
        /* .format = */ AUDIO_S8,
        /* .channels = */ 2,
        /* .silence = */ 0, // calculated
        /* .samples = */ 512, // 512 * 2 (because stereo)
        /* .padding = */ 0,
        /* .size = */ 0, // calculated
        /* .callback = */ callback,
        /* .userdata = */ this
    };

    // passing NULL forces the device to be opened with the
    // wanted format.
    this->device_opened = 0 == SDL_OpenAudio(&wanted, NULL);
    
    // check if an audio device was failed to be found...
    if (!this->device_opened) {
        printf("failed to find valid audio device\n");
        return false;
    }
    else {
        SDL_PauseAudio(0);
    }

    return true;
}

auto SDL1::PushSamples(const struct GB_ApuCallbackData* data) -> void {
    for (;;) {
        this->mutex.lock();
        
        if (this->buffer_size < (this->buffers.size() - 1)) {
            // leave it still locked!
            break;
        }

        this->mutex.unlock();
        SDL_Delay(1);
    }

    std::memcpy(
        this->buffers[this->buffer_index].data(),
        data->samples,
        sizeof(data->samples)
    );

    this->buffer_index = (this->buffer_index + 1) % this->buffers.size();
    this->buffer_size++;

    // we kept the mutex locked in the loop, unlock before exit!
    this->mutex.unlock();
}

} // namespace mgb::platform::audio::sdl1

#endif // MGB_SDL1_AUDIO
