#include "frontend/platforms/video/sdl2/sdl2_video.hpp"

#include <cstring>


namespace mgb::platform::video::sdl2 {


SDL2::~SDL2() {
    if (SDL_WasInit(SDL_INIT_VIDEO)) {
    	if (this->texture) { SDL_DestroyTexture(this->texture); }
		if (this->renderer) { SDL_DestroyRenderer(this->renderer); }
		if (this->window) { SDL_DestroyWindow(this->window); }

    	SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }
    
    if (SDL_WasInit(SDL_INIT_JOYSTICK)) {
    	for (auto &p : this->joysticks) {
	        SDL_JoystickClose(p.ptr);
	        p.ptr = nullptr;
	    }

    	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
    }

    if (SDL_WasInit(SDL_INIT_GAMECONTROLLER)) {
	    for (auto &p : this->controllers) {
	        SDL_GameControllerClose(p.ptr);
	        p.ptr = nullptr;
	    }

    	SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
    }
}

auto SDL2::SetupVideo(VideoInfo vid_info, GameTextureInfo game_info) -> bool {
	if (SDL_InitSubSystem(SDL_INIT_VIDEO)) {
		printf("\n[SDL_VIDEO_ERROR] %s\n\n", SDL_GetError());
		return false;
	}

	if (SDL_InitSubSystem(SDL_INIT_JOYSTICK)) {
		printf("\n[SDL_JOYSTICK_ERROR] %s\n\n", SDL_GetError());
		return false;
	}

	if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER)) {
		printf("\n[SDL_GAMECONTROLLER_ERROR] %s\n\n", SDL_GetError());
		return false;
	}

	// SDL_RendererInfo info;
 //    SDL_GetRendererInfo(this->renderer, &info);

 //    printf("\nRENDERER-INFO\n");
 //    printf("\tname: %s\n", info.name);
 //    printf("\tflags: 0x%X\n", info.flags);
 //    printf("\tnum textures: %u\n", info.num_texture_formats);
 //    for (uint32_t i = 0; i < info.num_texture_formats; ++i) {
 //        printf("\t\ttexture_format %u: 0x%X\n", i, info.texture_formats[i]);
 //    }

	this->window = SDL_CreateWindow(
		vid_info.name.c_str(),
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		vid_info.w, vid_info.h, SDL_WINDOW_SHOWN
	);

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

    this->texture = SDL_CreateTexture(
    	this->renderer,
    	SDL_PIXELFORMAT_BGR555, SDL_TEXTUREACCESS_STREAMING,
    	game_info.w, game_info.h
    );
    
    // set the size of the buffered pixels
    this->game_pixels.resize(game_info.w * game_info.h);

    // SDL_SetWindowMinimumSize(this->window, 160, 144);

    return true;
}

auto SDL2::UpdateGameTexture(GameTextureData data) -> void {
	std::memcpy(
		this->game_pixels.data(),
		data.pixels,
		data.w * data.h * sizeof(uint16_t)
	);
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
