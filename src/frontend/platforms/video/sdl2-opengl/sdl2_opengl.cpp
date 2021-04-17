#include "frontend/platforms/video/sdl2-opengl/sdl2_opengl.hpp"
#include "SDL_video.h"

#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>


namespace mgb::platform::video::sdl2 {


SDL2_GL::~SDL2_GL() {
	if (this->texture != 0) {
		glDeleteTextures(1, &this->texture);
	}

	if (this->gl_ctx) {
		SDL_GL_DeleteContext(this->gl_ctx);
	}

	this->DeinitSDL2();
}

auto SDL2_GL::SetupVideo(VideoInfo vid_info, GameTextureInfo game_info) -> bool {
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    const auto win_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL;
    if (!this->SetupSDL2(vid_info, game_info, win_flags)) {
		return false;
	}

	this->gl_ctx = SDL_GL_CreateContext(this->window);
	if (!this->gl_ctx) {
		printf("[SDL2-GL] failed to create gl ctx %s\n", SDL_GetError());
		return false;
	}

	if (SDL_GL_MakeCurrent(this->window, this->gl_ctx)) {
		printf("[SDL2-GL] failed to make gl current %s\n", SDL_GetError());
		return false;
	}

    if (SDL_GL_SetSwapInterval(1)) {
    	printf("[SDL2-GL] failed to set swap %s\n", SDL_GetError());
		// return false;
    }

    glewExperimental = true;

  	if (auto err = glewInit(); err != GLEW_OK) {
  		printf("[SDL2-GL] failed to init glew %s\n", glewGetErrorString(err));
  		return false;
  	}

  	// game display texture
    glGenTextures(1, &this->texture);
    glBindTexture(GL_TEXTURE_2D, this->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA, 160, 144, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

	return true;
}

auto SDL2_GL::RenderDisplay() -> void {
	glBindTexture(GL_TEXTURE_2D, this->texture);
    glTexSubImage2D(
    	GL_TEXTURE_2D,
    	0, 0, 0,
    	160, 144,
    	GL_BGRA, GL_UNSIGNED_SHORT_5_5_5_1,
    	this->game_pixels.data()
    );

    glBindFramebuffer(0, 0);

    glViewport(0, 0, 160 * 4, 144 * 4);
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, this->texture);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    SDL_GL_SwapWindow(this->window);
}

} // namespace mgb::platform::video::sdl2
