#pragma once


#include "frontend/platforms/audio/interface.hpp"

#ifdef MGB_ALLEGRO5_AUDIO

#include <allegro5/allegro5.h>
#include <allegro5/allegro_audio.h>


namespace mgb::platform::audio::allegro5 {

class Allegro5 final : public Interface {
public:
	using Interface::Interface;
	~Allegro5();

	auto SetupAudio() -> bool override;

	auto PushSamples(const struct GB_ApuCallbackData* data) -> void override;

private:
	ALLEGRO_AUDIO_STREAM* stream = nullptr;
	ALLEGRO_MIXER* mixer = nullptr;

	static constexpr unsigned samples = 512;
	static constexpr unsigned freq = 48000;

	// too small of a number and there will be pops
	// when the core is running too slow.
	// also, if the core is running fast, then it
	// will have to wait for a buffer to be free.
	// this makes buffer of 1 not usuable, 2 or higher is
	// fine.
	// higher buffers will have more latency (i think),
	// and will need to be drained when the core is paused,
	// else, it'll keep playing all buffers.
	static constexpr std::size_t buffer_count = 6;

	static_assert(Allegro5::buffer_count > 1,
		"buffer size is too small!");
};

}

#endif // MGB_ALLEGRO5_AUDIO
