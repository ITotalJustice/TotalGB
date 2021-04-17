#pragma once


#include "frontend/platforms/audio/interface.hpp"

#ifdef _MSC_VER
#include <SDL.h>
#include <SDL_audio.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#endif // _MSC_VER


namespace mgb::platform::audio::sdl2 {

class SDL2 final : public Interface {
public:
	using Interface::Interface;
	~SDL2();

	auto SetupAudio() -> bool override;

	auto PushSamples(const struct GB_ApuCallbackData* data) -> void override;

protected:


private:
	SDL_AudioDeviceID device_id = 0;

};

} // namespace mgb::platform::audio::sdl2

