#pragma once


#include "frontend/platforms/video/sdl2/sdl2_video.hpp"

#ifdef _MSC_VER
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif // _MSC_VER

extern "C" {
typedef void* SDL_GLContext;
}

namespace mgb::platform::video::sdl2 {


class SDL2_GL final : public BaseSDL2 {
public:
	using BaseSDL2::BaseSDL2;
	~SDL2_GL();

	auto SetupVideo(VideoInfo vid_info, GameTextureInfo game_info) -> bool override;

	auto RenderDisplay() -> void override;

private:
    SDL_GLContext gl_ctx = nullptr;
    unsigned texture = 0;
};

} // namespace mgb::platform::video::sdl2
