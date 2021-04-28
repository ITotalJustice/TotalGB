#include "frontend/platforms/video/sdl2/sdl2_video.hpp"

#ifdef MGB_SDL2_VIDEO

#include <cstring>

namespace mgb::platform::video::sdl2 {


SDL2::~SDL2() {
    this->font.CloseFont();

    this->DestroyButtonTextures();

    if (this->core_texture){
        SDL_DestroyTexture(this->core_texture);
    }

    if (this->renderer) {
        SDL_DestroyRenderer(this->renderer);
    }

    this->DeinitSDL2();
}

auto SDL2::LoadButtonTextures() -> bool {
    for (std::size_t i = 0; i < this->touch_buttons.size(); ++i) {
        const auto surface = this->touch_buttons[i].surface;

        if (surface != NULL) {
            auto texture = SDL_CreateTextureFromSurface(this->renderer, surface);

            if (texture) {
                this->button_textures[i] = texture;
            }
            else {
                std::printf("[SDL2] failed to convert surface to texture %s\n", SDL_GetError());
                return false;
            }

        }
    }

    return true;
}

auto SDL2::DestroyButtonTextures() -> void {
    for (auto texture : this->button_textures) {
        if (texture) {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        } 
    }
}

auto SDL2::SetupVideoInternal(VideoInfo vid_info, GameTextureInfo game_info) -> bool {
	const auto win_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;

    if (!this->SetupSDL2(vid_info, game_info, win_flags)) {
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
		std::printf("[SDL2] failed to create renderer: %s\n", SDL_GetError());
		return false;
	}

    // try and enable blending
    if (SDL_SetRenderDrawBlendMode(this->renderer, SDL_BLENDMODE_BLEND)) {
        std::printf("[SDL2] failed to enable blending: %s\n", SDL_GetError());
    }

    this->core_texture = SDL_CreateTexture(
    	this->renderer,
    	SDL_PIXELFORMAT_BGR555, SDL_TEXTUREACCESS_STREAMING,
    	game_info.w, game_info.h
    );

    if (!this->core_texture) {
        std::printf("[SDL2] failed to create texture: %s\n", SDL_GetError());
		return false;
	}

    #ifdef ON_SCREEN_BUTTONS
        if (!this->LoadButtonTextures()) {
            // fail if on mobile / webasm
            std::printf("[SDL2] failed to load button textures\n");
            return false;
        }
    #endif // ON_SCREEN_BUTTONS

    constexpr auto font_path = "res/fonts/Retro Gaming.ttf";
    constexpr auto font_size = 24.f;

    if (!this->font.OpenFont(this->renderer, font_path, font_size)) {
        // this is not serious if it fails, just warn but don't exit!
        std::printf("[SDL2] failed to open font\n");
    }
    else {
        const auto colour = ttf::Colour{0xFF, 0xFF, 0xFF, 0xFF};
        this->font.SetColour(colour);
    }

    return true;
}

auto SDL2::UpdateGameTexture(GameTextureData data) -> void {
#if 1
    // this seems to be faster
    SDL_UpdateTexture(
        this->core_texture, NULL,
        data.pixels, data.w * sizeof(uint16_t)
    );
#else
    void* pixels; int pitch;

    SDL_LockTexture(this->core_texture, NULL, &pixels, &pitch);

    std::memcpy(
        pixels,
        data.pixels,
        data.w * data.h * sizeof(uint16_t)
    );
    
    SDL_UnlockTexture(this->core_texture);
#endif
}

auto SDL2::RenderCore() -> void {
    this->OnWindowResize();
    SDL_RenderCopy(this->renderer, this->core_texture, NULL, &this->texture_rect);
}

auto SDL2::RenderPopUps() -> void {
    if (this->text_popups.size() == 0) {
        return;
    }

    const auto box_col = ttf::Colour{ 0, 0, 0, 150 };
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

auto SDL2::RenderButtons() -> void {
    for (std::size_t i = 0; i < this->button_textures.size(); ++i) {
        if (this->button_textures[i]) {
            const auto rect = this->touch_buttons[i].rect;

            SDL_RenderCopy(
                this->renderer, this->button_textures[i],
                NULL, &rect
            );
        }
    }
}

auto SDL2::RenderDisplayInternal() -> void {
    SDL_RenderClear(this->renderer);

    this->RenderCore();
    this->RenderPopUps();
    this->RenderButtons();

    SDL_RenderPresent(this->renderer);
}

} // namespace mgb::platform::video::sdl2


#endif // MGB_SDL2_VIDEO