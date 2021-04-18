#include "frontend/platforms/video/sdl2/sdl2_video.hpp"

#ifdef MGB_SDL2_VIDEO

#include <cstring>

namespace mgb::platform::video::sdl2 {


SDL2::~SDL2() {
    if (SDL_WasInit(SDL_INIT_VIDEO)) {
    	if (this->texture){
    		SDL_DestroyTexture(this->texture);
    	}

		if (this->renderer) {
			SDL_DestroyRenderer(this->renderer);
		}
    }

    this->DeinitSDL2();
}

auto SDL2::SetupVideo(VideoInfo vid_info, GameTextureInfo game_info) -> bool {
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
		printf("[SDL2] failed to create renderer %s\n", SDL_GetError());
		return false;
	}

    this->texture = SDL_CreateTexture(
    	this->renderer,
    	SDL_PIXELFORMAT_BGR555, SDL_TEXTUREACCESS_STREAMING,
    	game_info.w, game_info.h
    );

    if (!this->texture) {
		printf("[SDL2] failed to create texture %s\n", SDL_GetError());
		return false;
	}

    SDL_SetWindowMinimumSize(this->window, 160, 144);

    return true;
}

auto SDL2::RenderDisplay() -> void {
	void* pixles; int pitch;
    
    SDL_LockTexture(texture, NULL, &pixles, &pitch);
    	std::memcpy(
    		pixles,
    		this->game_pixels.data(),
    		this->game_pixels.size() * sizeof(uint16_t)
    	);
    SDL_UnlockTexture(texture);

    SDL_RenderClear(this->renderer);
    SDL_RenderCopy(this->renderer, this->texture, NULL, NULL);
    SDL_RenderPresent(this->renderer);
}


} // namespace mgb::platform::video::sdl2


#endif // MGB_SDL2_VIDEO