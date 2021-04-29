#pragma once


#include "frontend/platforms/audio/interface.hpp"
#include <cstdint>


namespace mgb::platform::audio::sdl2 {

class SDL2 final : public Interface {
public:
	using Interface::Interface;
	~SDL2();

	auto SetupAudio(int freq) -> bool override;

	auto PushSamples(const struct GB_ApuCallbackData* data) -> void override;

protected:


private:
	using SDL_AudioDeviceID = uint32_t;

	SDL_AudioDeviceID device_id = 0;

};

} // namespace mgb::platform::audio::sdl2

