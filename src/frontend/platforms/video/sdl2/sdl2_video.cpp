#include "frontend/platforms/video/sdl2/sdl2_video.hpp"

#ifdef MGB_SDL2_VIDEO

#include <cstring>

namespace mgb::platform::video::sdl2 {


SDL2::~SDL2() {
    this->font.CloseFont();

    if (this->texture){
        SDL_DestroyTexture(this->texture);
    }

    if (this->renderer) {
        SDL_DestroyRenderer(this->renderer);
    }

    this->DeinitSDL2();
}

auto SDL2::SetupVideoInternal(VideoInfo vid_info, GameTextureInfo game_info) -> bool {
	if (!this->SetupSDL2(vid_info, game_info, SDL_WINDOW_SHOWN)) {
		return false;
	}

	const uint32_t renderer_flags = [&vid_info]() -> uint32_t {
		switch (vid_info.render_type) {
			case RenderType::SOFTWARE:
				return SDL_RENDERER_SOFTWARE;
			case RenderType::ACCELERATED:
				return SDL_RENDERER_ACCELERATED;
			case RenderType::ACCELERATED_VSYNC:
				return SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
		}

		return 0;
	}();

    this->renderer = SDL_CreateRenderer(
    	this->window, -1, renderer_flags
    );

    if (!this->renderer) {
		printf("[SDL2] failed to create renderer: %s\n", SDL_GetError());
		return false;
	}

    // try and enable blending
    if (SDL_SetRenderDrawBlendMode(this->renderer, SDL_BLENDMODE_BLEND)) {
        printf("[SDL2] failed to enable blending: %s\n", SDL_GetError());
    }

    this->texture = SDL_CreateTexture(
    	this->renderer,
    	SDL_PIXELFORMAT_BGR555, SDL_TEXTUREACCESS_STREAMING,
    	game_info.w, game_info.h
    );

    if (!this->texture) {
		printf("[SDL2] failed to create texture: %s\n", SDL_GetError());
		return false;
	}

    constexpr auto font_path = "res/fonts/Retro Gaming.ttf";
    constexpr auto font_size = 24.f;

    if (!this->font.OpenFont(this->renderer, font_path, font_size)) {
        // this is not serious if it fails, just warn but don't exit!
        printf("[SDL2] failed to open font\n");
    }
    else {
        const auto colour = ttf::Colour{0xFF, 0xFF, 0xFF, 0xFF};
        this->font.SetColour(colour);
    }

    return true;
}

auto SDL2::RenderCore() -> void {
    void* pixles; int pitch;

    SDL_LockTexture(texture, NULL, &pixles, &pitch);
        std::memcpy(
            pixles,
            this->game_pixels.data(),
            this->game_pixels.size() * sizeof(uint16_t)
        );
    SDL_UnlockTexture(texture);

    SDL_RenderCopy(this->renderer, this->texture, NULL, NULL);
}

auto SDL2::RenderDisplayInternal() -> void {
    SDL_RenderClear(this->renderer);

    this->RenderCore();

    const auto box_col = ttf::Colour{0, 0, 0, 150};

    {
        const auto inc_y = this->font.GetHeight() + (5 * 2);

        // todo: don't draw offscreen...
        float y = 0.f;

        for (auto& popup : this->text_popups) {

            this->font.DrawTextBox(this->renderer,
                0, y,
                5, 5,
                box_col,
                popup.GetText()
            );

            y += inc_y;
        }
    }

    SDL_RenderPresent(this->renderer);
}


} // namespace mgb::platform::video::sdl2


#endif // MGB_SDL2_VIDEO