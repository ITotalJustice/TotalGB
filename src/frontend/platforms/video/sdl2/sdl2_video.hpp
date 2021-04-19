#pragma once

#ifdef MGB_SDL2_VIDEO

#include "frontend/platforms/video/sdl2/sdl2_base.hpp"

#ifdef _MSC_VER
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif // _MSC_VER

namespace mgb::platform::video::sdl2 {


class SDL2 final : public base::SDL2 {
public:
	using base::SDL2::SDL2;
	~SDL2();

	auto SetupVideo(VideoInfo vid_info, GameTextureInfo game_info) -> bool override;

	auto RenderDisplay() -> void override;

private:
    SDL_Renderer* renderer{nullptr};
    SDL_Texture* texture{nullptr};
};

} // namespace mgb::platform::video::sdl2


#endif // MGB_SDL2_VIDEO
