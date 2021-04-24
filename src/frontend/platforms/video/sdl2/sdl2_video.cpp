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
    struct ButtonInfo {
        const char* path;
        SDL2::TouchButton::Type type;
        int x, y;
    };

    // todo: set the button x,y to fit inside [160x144] then scale it with the
    // the screen size.
    constexpr std::array<ButtonInfo, 9> paths = {{
        { "res/sprites/controls/transparentDark34.bmp",  SDL2::TouchButton::Type::A, 440, 470 },
        { "res/sprites/controls/transparentDark35.bmp",  SDL2::TouchButton::Type::B, 540, 470 },
        { "res/sprites/controls/transparentDark40.bmp",  SDL2::TouchButton::Type::START, 200, 20 },
        { "res/sprites/controls/transparentDark41.bmp",  SDL2::TouchButton::Type::SELECT, 320, 20 },
        { "res/sprites/controls/transparentDark01.bmp",  SDL2::TouchButton::Type::UP, 80, 410 },
        { "res/sprites/controls/transparentDark08.bmp",  SDL2::TouchButton::Type::DOWN, 80, 490 },
        { "res/sprites/controls/transparentDark03.bmp",  SDL2::TouchButton::Type::LEFT, 25, 460 },
        { "res/sprites/controls/transparentDark04.bmp",  SDL2::TouchButton::Type::RIGHT, 120, 460 },
        { "res/sprites/controls/transparentDark20.bmp",  SDL2::TouchButton::Type::OPTIONS, -100, -100 },
    }};

    for (std::size_t i = 0; i < paths.size(); ++i) {
        const auto [path, type, x, y] = paths[i];

        auto surface = SDL_LoadBMP(path);

        if (surface != NULL) {
            auto texture = SDL_CreateTextureFromSurface(this->renderer, surface);
            const auto w = surface->w;
            const auto h = surface->h;

            // we don't need this anymore
            SDL_FreeSurface(surface);

            if (texture) {
                printf("\tw: %d h: %d path: %s\n", surface->w, surface->h, path);
                this->button_textures[i].texture = texture;
                this->button_textures[i].w = w;
                this->button_textures[i].h = h;

                this->touch_buttons[i].type = type;
                this->touch_buttons[i].x = x;
                this->touch_buttons[i].y = y;
                this->touch_buttons[i].w = w;
                this->touch_buttons[i].h = h;
            }
            else {
                printf("[SDL2] failed to convert surface to texture %s\n", SDL_GetError());
                return false;
            }

        }
        else {
            printf("[SDL2] failed to load button bmp %s\n", SDL_GetError());
            return false;
        }
    }

    return true;
}

auto SDL2::DestroyButtonTextures() -> void {
    for (std::size_t i = 0; i < this->button_textures.size(); ++i) {
        if (this->button_textures[i].texture) {
            SDL_DestroyTexture(this->button_textures[i].texture);

            this->button_textures[i].texture = nullptr;
            this->touch_buttons[i].Reset();
        }
    }
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

    this->core_texture = SDL_CreateTexture(
    	this->renderer,
    	SDL_PIXELFORMAT_BGR555, SDL_TEXTUREACCESS_STREAMING,
    	game_info.w, game_info.h
    );

    if (!this->core_texture) {
        printf("[SDL2] failed to create texture: %s\n", SDL_GetError());
		return false;
	}

    if (!this->LoadButtonTextures()) {
        // fail if on mobile / webasm
        printf("[SDL2] failed to load button textures\n");
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

    SDL_LockTexture(this->core_texture, NULL, &pixles, &pitch);
        std::memcpy(
            pixles,
            this->game_pixels.data(),
            this->game_pixels.size() * sizeof(uint16_t)
        );
    SDL_UnlockTexture(this->core_texture);

    SDL_RenderCopy(this->renderer, this->core_texture, NULL, NULL);
}

auto SDL2::RenderDisplayInternal() -> void {
    SDL_RenderClear(this->renderer);

    this->RenderCore();

    const auto box_col = ttf::Colour{ 0, 0, 0, 150 };

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

    for (std::size_t i = 0; i < this->button_textures.size(); ++i) {
        if (this->button_textures[i].texture) {
            const auto rect = this->touch_buttons[i].ToRect();

            SDL_RenderCopy(
                this->renderer, this->button_textures[i].texture,
                NULL, &rect
            );
        }
    }

    SDL_RenderPresent(this->renderer);
}


} // namespace mgb::platform::video::sdl2


#endif // MGB_SDL2_VIDEO