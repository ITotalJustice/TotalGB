#pragma once

#ifdef MGB_SDL2_VIDEO

#include "frontend/platforms/video/sdl2/sdl2_base.hpp"
#include "frontend/platforms/video/sdl2/sdl2_ttf.hpp"

#ifdef _MSC_VER
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif // _MSC_VER

#include <array>

namespace mgb::platform::video::sdl2 {


class SDL2 final : public base::SDL2 {
public:
	using base::SDL2::SDL2;
	~SDL2();

	auto UpdateGameTexture(GameTextureData data) -> void override;

private:
	auto SetupVideoInternal(VideoInfo vid_info, GameTextureInfo game_info) -> bool override;
	auto RenderDisplayInternal() -> void override;

	auto LoadButtonTextures() -> bool;
	auto DestroyButtonTextures() -> void;

	auto RenderPopUps() -> void;
	auto RenderButtons() -> void;
	auto RenderCore() -> void;

	auto OnWindowResize(int win_w, int win_h, int scale) -> void override;

private:
    SDL_Renderer* renderer{};
    SDL_Texture* core_texture{};
    std::array<SDL_Texture*, 9> button_textures{};

    ttf::Font font;
};

} // namespace mgb::platform::video::sdl2

#endif // MGB_SDL2_VIDEO
