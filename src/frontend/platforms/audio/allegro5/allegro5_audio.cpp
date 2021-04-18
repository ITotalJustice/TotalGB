#include "frontend/platforms/audio/allegro5/allegro5_audio.hpp"
#include "core/types.h"

#include <allegro5/allegro_audio.h>
#include <cstring>


namespace mgb::platform::audio::allegro5 {


Allegro5::~Allegro5() {
	if (al_is_audio_installed()) {
		if (this->stream) {
			al_destroy_audio_stream(this->stream);
		}

	    al_uninstall_audio();
	}
}

auto Allegro5::SetupAudio() -> bool {
	// this seems safe to call as many times as needed
	// as it just sets up an atexit()!
    if (!al_init()) {
    	printf("[AL_AUDIO] failed to init audio\n");
    	return false;
    }

    // al_set_config_value(al_get_system_config(), "audio", "driver", "sndio");

    if (!al_install_audio()) {
    	printf("[AL_AUDIO] failed to install audio\n");
    	return false;
    }

    if (!al_reserve_samples(0)) {
    	printf("[AL_AUDIO] failed to reserve samples\n");
    	return false;
    }

   	this->stream = al_create_audio_stream(
   		this->buffer_count,
   		this->samples,
   		this->freq,
		ALLEGRO_AUDIO_DEPTH_INT8,
		ALLEGRO_CHANNEL_CONF_2
	);

	if (!this->stream) {
		printf("[AL_AUDIO] failed to create audio stream\n");
    	return false;
	}

	this->mixer = al_get_default_mixer();
	if (!this->mixer) {
		printf("[AL_AUDIO] failed to get default mixer\n");
		return false;
	}

	if (!al_attach_audio_stream_to_mixer(this->stream, this->mixer)) {
		printf("[AL_AUDIO] failed to attach audio stream to mixer\n");
	 	return false;
	}

	return true;
}

auto Allegro5::PushSamples(const struct GB_ApuCallbackData* data) -> void {
	auto framement = al_get_audio_stream_fragment(stream);
	
	// delay here until the fragment is ready.
	while (!framement) {
		al_rest(0.001);
		framement = al_get_audio_stream_fragment(stream);
	}

	// only doing memcpy because set fragment is non-const
	// so i am not sure if it modifies the buffer...
	std::memcpy(
		framement, data->samples,
		sizeof(data->samples)
	);

	al_set_audio_stream_fragment(
		this->stream, framement
	);
}

}
