#pragma once


#ifdef MGB_SDL1_AUDIO

#include "frontend/platforms/audio/interface.hpp"

#include <mutex>
#include <array>


namespace mgb::platform::audio::sdl1 {

class SDL1 final : public Interface {
public:
	using Interface::Interface;
	~SDL1();

	auto SetupAudio() -> bool override;

	auto PushSamples(const struct GB_ApuCallbackData* data) -> void override;

	auto AudioCallback(uint8_t* buf, int len) -> void;

protected:


private:
	using SampleFormat = int8_t;
	static constexpr auto sample_size = 512 * 2; // stereo
	static constexpr auto buffer_count = 4;

	bool device_opened = false;

	std::mutex mutex;
	
	std::array<
		std::array<SampleFormat, sample_size>,
		buffer_count
	> buffers;

	std::size_t buffer_index = 0;
	std::size_t buffer_size = 0;
};

} // namespace mgb::platform::audio::sdl2

#endif // MGB_SDL1_AUDIO
