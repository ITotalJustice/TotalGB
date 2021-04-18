#pragma once

#ifdef MGB_ALLEGRO5_VIDEO

#include "frontend/platforms/video/interface.hpp"

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>


namespace mgb::platform::video::allegro5 {

class Allegro5 final : public Interface {
public:
	using Interface::Interface;
	~Allegro5();

	auto SetupVideo(VideoInfo vid_info, GameTextureInfo game_info) -> bool;

	auto UpdateGameTexture(GameTextureData data) -> void;

	auto RenderDisplay() -> void;

	auto PollEvents() -> void;

private:


private:
	ALLEGRO_DISPLAY* display = nullptr;
    ALLEGRO_BITMAP* bitmap = nullptr;
    ALLEGRO_EVENT_QUEUE* queue = nullptr;
};


} // namespace mgb::platform::video::allegro5

#endif // MGB_ALLEGRO5_VIDEO
